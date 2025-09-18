// Minimal serial console example for qtermwidget.
// Demonstrates opening a serial device and interacting via terminal emulation.

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QInputDialog>

#include "qtermwidget.h"
#include "Session.h" // for Konsole::Session forward usage inside openSerial impl

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qTerm"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Minimal serial/ssh console using qtermwidget"));
    parser.addHelpOption();
    QCommandLineOption modeOpt(QStringList{QStringLiteral("m"),QStringLiteral("mode")}, QStringLiteral("Mode: serial or ssh"), QStringLiteral("mode"), QStringLiteral("serial"));
    QCommandLineOption deviceOpt(QStringList{QStringLiteral("d"),QStringLiteral("device")}, QStringLiteral("Serial device path (e.g. /dev/ttyACM0) or SSH host (user@host[:port])"), QStringLiteral("device"));
    QCommandLineOption baudOpt(QStringList{QStringLiteral("b"),QStringLiteral("baud")}, QStringLiteral("Baud rate (serial only)"), QStringLiteral("baud"), QStringLiteral("115200"));
    parser.addOption(modeOpt);
    parser.addOption(deviceOpt);
    parser.addOption(baudOpt);
    parser.process(app);

    QString mode = parser.value(modeOpt).toLower();
    QString device = parser.value(deviceOpt);
    int baud = parser.value(baudOpt).toInt();

    QWidget window;
    window.setWindowTitle(QStringLiteral("qTerm"));

    auto *layout = new QVBoxLayout(&window);
    // startnow=0 so we don't auto-start a shell; we will launch serial or ssh explicitly
    auto *term = new QTermWidget(0, &window);
    layout->addWidget(term, 1);

    // Control row
    auto *controls = new QWidget(&window);
    auto *ctrlLayout = new QHBoxLayout(controls);
    auto *modeCombo = new QComboBox(controls);
    modeCombo->addItem(QStringLiteral("Serial"));
    modeCombo->addItem(QStringLiteral("SSH"));
    modeCombo->setCurrentIndex(mode == QLatin1String("ssh") ? 1 : 0);
    auto *deviceEdit = new QLineEdit(device, controls);
    deviceEdit->setPlaceholderText(QStringLiteral("/dev/ttyACM0 or user@host"));
    auto *baudEdit = new QLineEdit(QString::number(baud), controls);
    auto *openBtn = new QPushButton(QObject::tr("Open"), controls);
    auto *statusLabel = new QLabel(QStringLiteral("Idle"), controls);
    ctrlLayout->addWidget(new QLabel(QStringLiteral("Mode:")));
    ctrlLayout->addWidget(modeCombo);
    ctrlLayout->addWidget(new QLabel(QStringLiteral("Device/Host:")));
    ctrlLayout->addWidget(deviceEdit);
    ctrlLayout->addWidget(new QLabel(QStringLiteral("Baud:")));
    ctrlLayout->addWidget(baudEdit);
    ctrlLayout->addWidget(openBtn);
    ctrlLayout->addWidget(statusLabel, 1);
    layout->addWidget(controls);

    // Hide baud rate for SSH mode
    QObject::connect(modeCombo, &QComboBox::currentIndexChanged, [&](int idx){
        baudEdit->setVisible(idx == 0);
    });

    QObject::connect(openBtn, &QPushButton::clicked, [&](){
        int idx = modeCombo->currentIndex();
        QString dev = deviceEdit->text().trimmed();
        if(idx == 0) { // Serial
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
            int b = baudEdit->text().toInt();
            if(dev.isEmpty()) {
                QMessageBox::warning(&window, QObject::tr("Input"), QObject::tr("Please specify a serial device."));
                return;
            }
            if(term->openSerial(dev, b)) {
                statusLabel->setText(QObject::tr("Open: %1 @ %2").arg(dev, QString::number(b)));
            } else {
                statusLabel->setText(QObject::tr("Failed"));
                QMessageBox::critical(&window, QObject::tr("Open Failed"), QObject::tr("Could not open %1").arg(dev));
            }
#else
            QMessageBox::critical(&window, QObject::tr("Serial Support Missing"),
                                  QObject::tr("This build of qtermwidget was compiled without QSerialPort support."));
#endif
        } else { // SSH
            if(dev.isEmpty()) {
                QMessageBox::warning(&window, QObject::tr("Input"), QObject::tr("Please specify an SSH host (user@host[:port])."));
                return;
            }

#ifdef QTERMWIDGET_HAVE_LIBSSH
            // Parse user@host[:port]
            QString user;
            QString hostPort = dev;
            int atIdx = dev.indexOf('@');
            if(atIdx != -1) {
                user = dev.left(atIdx);
                hostPort = dev.mid(atIdx+1);
            }
            QString host = hostPort;
            int port = 22;
            int colonIdx = hostPort.lastIndexOf(':');
            if(colonIdx != -1) {
                host = hostPort.left(colonIdx);
                bool ok=false; int p = hostPort.mid(colonIdx+1).toInt(&ok);
                if(ok && p>0 && p<65536) port = p;
            }

            // Ask for password (optional)
            bool okPwd=false;
            QString pwd = QInputDialog::getText(&window,
                                                QObject::tr("SSH Password"),
                                                QObject::tr("Password for %1@%2:").arg(user.isEmpty()? qgetenv("USER"):user, host),
                                                QLineEdit::Password,
                                                QString(),
                                                &okPwd);
            if(!okPwd) return; // cancelled

            if(term->openSSH(host, port, user, pwd)) {
                statusLabel->setText(QObject::tr("SSH: %1@%2:%3").arg(user.isEmpty()? QString::fromLocal8Bit(qgetenv("USER")) : user,
                                                                        host,
                                                                        QString::number(port)));
            } else {
                statusLabel->setText(QObject::tr("Failed"));
                QMessageBox::critical(&window, QObject::tr("SSH Failed"), QObject::tr("Could not open SSH to %1").arg(dev));
            }
#else
            QMessageBox::critical(&window, QObject::tr("SSH Support Missing"),
                                  QObject::tr("This build of qtermwidget was compiled without libssh support."));
#endif
        }
    });

    window.resize(900,600);
    window.show();

    return app.exec();
}
