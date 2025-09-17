// Non-GUI integration test for PTY lifecycle with multiple checks:
// 1) Spawn cmd.exe and verify echo of simple text
// 2) Resize the PTY and echo a message containing UTF-8 and control characters
// 3) Spawn cmd.exe to run a small script and verify output

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QByteArray>
#include <functional>

#include "Pty.h"

#ifdef _WIN32
#include "PtyConPty_win.h"
#endif

using namespace Konsole;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // Test phases: 0 = simple echo, 1 = resize + special chars, 2 = shell script
    int phase = 0;
    const int phaseTimeoutMs = 7000;

    // Minimal environment for subprocesses
    QStringList env;
    env << QStringLiteral("TERM=xterm");

    std::function<void()> runPhase;

    // Helper to create a Pty and connect to its receivedData signal
    auto makePtyAndConnect = [&](Pty*& outPty, QByteArray &accum, std::function<void(const QByteArray&)> onData){
        outPty = new Pty(&app);
        accum.clear();
        // Capture onData by value so the slot doesn't hold a dangling reference after
        // makePtyAndConnect returns.
        QObject::connect(outPty, &Pty::receivedData, &app, [onData, &accum, &phase](const char* buffer, int length){
            accum.append(buffer, length);
            qDebug().noquote() << "[phase" << phase << "recv]" << QString::fromUtf8(accum);
            onData(accum);
        });
    };

    // Runner for each phase
    runPhase = [&]{
        qDebug() << "Starting phase" << phase;

        static Pty* pty = nullptr;
        static QByteArray received;

        // Ensure any previous PTY is cleaned
        if (pty) {
            pty->closePty();
            pty->deleteLater();
            pty = nullptr;
        }

        QTimer::singleShot(phaseTimeoutMs, &app, [&]{
            qWarning() << "Phase" << phase << "timed out";
            if (pty) {
                pty->closePty();
            }
            QCoreApplication::exit(2 + phase);
        });

        if (phase == 0) {
            makePtyAndConnect(pty, received, [&](const QByteArray &acc){
                // Require a line containing exactly "hello" to avoid false positives
                const QString s = QString::fromUtf8(acc);
                const auto lines = s.split('\n');
                if (std::any_of(lines.begin(), lines.end(), [](const QString &l){ return l.trimmed() == QLatin1String("hello"); })) {
                    qDebug() << "Phase 0: simple echo OK";
                    // advance
                    pty->closePty();
                    phase = 1;
                    QTimer::singleShot(200, &app, runPhase);
                }
            });

            QStringList progArgs0;
            int rc = 0;
#ifdef _WIN32
            progArgs0 << QStringLiteral("cmd") << QStringLiteral("/c") << QStringLiteral("echo hello");
            rc = pty->start(QStringLiteral("cmd"), progArgs0, env, 0, false);
#else
            progArgs0 << QStringLiteral("/bin/sh") << QStringLiteral("-c") << QStringLiteral("echo hello");
            rc = pty->start(QStringLiteral("/bin/sh"), progArgs0, env, 0, false);
#endif
            if (rc != 0) {
                qCritical() << "Phase 0: Failed to start cmd, rc=" << rc;
                return QCoreApplication::exit(1);
            }

            return;
        }

        if (phase == 1) {
            makePtyAndConnect(pty, received, [&](const QByteArray &acc){
                // Wait for the UTF-8 snippet to appear
                const QString s = QString::fromUtf8(acc);
                if (s.contains(QString::fromUtf8("héll"))) {
                    qDebug() << "Phase 1: special chars echoed OK";
                    pty->closePty();
                    phase = 2;
                    QTimer::singleShot(200, &app, runPhase);
                }
            });

            QStringList progArgs1;
            int rc = 0;
#ifdef _WIN32
            progArgs1 << QStringLiteral("cmd") << QStringLiteral("/c") << QStringLiteral("echo héllö €");
            rc = pty->start(QStringLiteral("cmd"), progArgs1, env, 0, false);
#else
            // Use a single quoted string so the shell doesn't interpret '&' or split the Chinese text as a command
            progArgs1 << QStringLiteral("/bin/sh") << QStringLiteral("-c") << QStringLiteral("echo 'héllö € 你好'");
            rc = pty->start(QStringLiteral("/bin/sh"), progArgs1, env, 0, false);
#endif
            if (rc != 0) {
                qCritical() << "Phase 1: Failed to start cmd, rc=" << rc;
                return QCoreApplication::exit(1);
            }

            // Resize and verify windowSize roundtrip
            pty->setWindowSize(30, 100);
            QSize ws = pty->windowSize();
            qDebug() << "Phase 1: windowSize after set:" << ws;

            return;
        }

        if (phase == 2) {
            makePtyAndConnect(pty, received, [&](const QByteArray &acc){
                const QString s = QString::fromUtf8(acc);
                if (s.contains(QLatin1String("script-okay"))) {
                    qDebug() << "Phase 2: shell script output OK";
                    // close PTY and wait briefly for the child to exit cleanly
                    pty->closePty();
                    QTimer::singleShot(200, &app, [&](){ QCoreApplication::exit(0); });
                }
            });

            // Spawn a shell that executes a small script and exits
            QStringList progArgs2;
            int rc = 0;
#ifdef _WIN32
            progArgs2 << QStringLiteral("cmd") << QStringLiteral("/c") << QStringLiteral("echo script-okay");
            rc = pty->start(QStringLiteral("cmd"), progArgs2, env, 0, false);
#else
            progArgs2 << QStringLiteral("/bin/sh") << QStringLiteral("-c") << QStringLiteral("echo script-okay");
            rc = pty->start(QStringLiteral("/bin/sh"), progArgs2, env, 0, false);
#endif
            if (rc != 0) {
                qCritical() << "Phase 2: Failed to start cmd, rc=" << rc;
                return QCoreApplication::exit(1);
            }

            return;
        }
    };

    // Kick off first phase
    QTimer::singleShot(0, &app, runPhase);

    return app.exec();
}
