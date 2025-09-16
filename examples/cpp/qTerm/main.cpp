// Minimal serial console example for qtermwidget.
// Demonstrates opening a serial device and interacting via terminal emulation.

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "qtermwidget.h"
#include "Session.h" // for Konsole::Session forward usage inside openSerial impl

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qTerm"));

#ifndef QTERMWIDGET_HAVE_QSERIALPORT
    QMessageBox::critical(nullptr, QObject::tr("Serial Support Missing"),
                          QObject::tr("This build of qtermwidget was compiled without QSerialPort support."));
    return 1;
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Minimal serial console using qtermwidget"));
    parser.addHelpOption();
    QCommandLineOption deviceOpt(QStringList{QStringLiteral("d"),QStringLiteral("device")}, QStringLiteral("Serial device path (e.g. /dev/ttyACM0)"), QStringLiteral("device"));
    QCommandLineOption baudOpt(QStringList{QStringLiteral("b"),QStringLiteral("baud")}, QStringLiteral("Baud rate"), QStringLiteral("baud"), QStringLiteral("115200"));
    parser.addOption(deviceOpt);
    parser.addOption(baudOpt);
    parser.process(app);

    QString device = parser.value(deviceOpt);
    int baud = parser.value(baudOpt).toInt();

    QWidget window;
    window.setWindowTitle(QStringLiteral("qTerm"));

    auto *layout = new QVBoxLayout(&window);
    auto *term = new QTermWidget(&window);
    layout->addWidget(term, 1);

    // Control row
    auto *controls = new QWidget(&window);
    auto *ctrlLayout = new QHBoxLayout(controls);
    auto *deviceEdit = new QLineEdit(device, controls);
    deviceEdit->setPlaceholderText(QStringLiteral("/dev/ttyACM0"));
    auto *baudEdit = new QLineEdit(QString::number(baud), controls);
    auto *openBtn = new QPushButton(QObject::tr("Open"), controls);
    auto *statusLabel = new QLabel(QStringLiteral("Idle"), controls);
    ctrlLayout->addWidget(new QLabel(QStringLiteral("Device:")));
    ctrlLayout->addWidget(deviceEdit);
    ctrlLayout->addWidget(new QLabel(QStringLiteral("Baud:")));
    ctrlLayout->addWidget(baudEdit);
    ctrlLayout->addWidget(openBtn);
    ctrlLayout->addWidget(statusLabel, 1);
    layout->addWidget(controls);

    // Not needed: QTermWidget now exposes openSerial()

    QObject::connect(openBtn, &QPushButton::clicked, [&](){
        QString dev = deviceEdit->text().trimmed();
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
    });

    window.resize(900,600);
    window.show();

    return app.exec();
}
