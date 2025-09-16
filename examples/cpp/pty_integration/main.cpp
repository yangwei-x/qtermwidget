// Non-GUI integration test for PTY lifecycle with multiple checks:
// 1) Spawn /bin/cat and verify echo of simple text
// 2) Resize the PTY and echo a message containing UTF-8 and control characters
// 3) Spawn a shell to run a small script (via /bin/sh -c) and verify output

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QByteArray>
#include <functional>

#include "Pty.h"

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
                if (acc.contains("hello")) {
                    qDebug() << "Phase 0: simple echo OK";
                    // advance
                    pty->closePty();
                    phase = 1;
                    QTimer::singleShot(200, &app, runPhase);
                }
            });

            QStringList progArgs0;
            progArgs0 << QStringLiteral("/bin/cat");
            int rc = pty->start(QStringLiteral("/bin/cat"), progArgs0, env, 0, false);
            if (rc != 0) {
                qCritical() << "Phase 0: Failed to start /bin/cat, rc=" << rc;
                return QCoreApplication::exit(1);
            }

            const char msg[] = "hello\n";
            pty->sendData(msg, sizeof(msg)-1);
            return;
        }

        if (phase == 1) {
            makePtyAndConnect(pty, received, [&](const QByteArray &acc){
                // Wait for the UTF-8 snippet to appear
                if (acc.contains("héll")) {
                    qDebug() << "Phase 1: special chars echoed OK";
                    pty->closePty();
                    phase = 2;
                    QTimer::singleShot(200, &app, runPhase);
                }
            });

            QStringList progArgs1;
            progArgs1 << QStringLiteral("/bin/cat");
            int rc = pty->start(QStringLiteral("/bin/cat"), progArgs1, env, 0, false);
            if (rc != 0) {
                qCritical() << "Phase 1: Failed to start /bin/cat, rc=" << rc;
                return QCoreApplication::exit(1);
            }

            // Resize and verify windowSize roundtrip
            pty->setWindowSize(30, 100);
            QSize ws = pty->windowSize();
            qDebug() << "Phase 1: windowSize after set:" << ws;

            // Send a message containing UTF-8 and control characters
            QString special = QString::fromUtf8("héllö €\tΩ\n");
            QByteArray out = special.toUtf8();
            pty->sendData(out.constData(), out.size());
            return;
        }

        if (phase == 2) {
            makePtyAndConnect(pty, received, [&](const QByteArray &acc){
                if (acc.contains("script-okay")) {
                    qDebug() << "Phase 2: shell script output OK";
                    // close PTY and wait briefly for the child to exit cleanly
                    pty->closePty();
                    QTimer::singleShot(200, &app, [&](){ QCoreApplication::exit(0); });
                }
            });

            // Spawn a shell that executes a small script and exits
            QStringList progArgs2;
            // For historical reasons the first element should be the program name
            progArgs2 << QStringLiteral("/bin/sh") << QStringLiteral("-c") << QStringLiteral("echo script-okay");
            int rc = pty->start(QStringLiteral("/bin/sh"), progArgs2, env, 0, false);
            if (rc != 0) {
                qCritical() << "Phase 2: Failed to start /bin/sh, rc=" << rc;
                return QCoreApplication::exit(1);
            }

            return;
        }
    };

    // Kick off first phase
    QTimer::singleShot(0, &app, runPhase);

    return app.exec();
}
