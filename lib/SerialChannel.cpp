#include "SerialChannel.h"

#include <QDebug>

#ifdef QTERMWIDGET_HAVE_QSERIALPORT
#include <QSerialPortInfo>
#endif

namespace Konsole {

SerialChannel::SerialChannel(QObject *parent)
    : QObject(parent)
{
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    m_port = new QSerialPort(this);
    connect(m_port, &QSerialPort::readyRead, this, [this]() {
        const QByteArray data = m_port->readAll();
        if(!data.isEmpty()) {
            emit receivedData(data);
        }
    });
    connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError err) {
        if(err == QSerialPort::NoError)
            return;
        emit error(QStringLiteral("Serial error: %1").arg(err));
    });
#endif
}

SerialChannel::~SerialChannel() = default;

bool SerialChannel::start(const QString &devicePath,
                          int baudRate,
                          int dataBits,
                          int stopBits,
                          int parity,
                          bool flowControl)
{
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    if(m_running)
        return true;

    m_port->setPortName(devicePath);
    if(!m_port->open(QIODevice::ReadWrite)) {
        emit error(QStringLiteral("Failed to open %1: %2").arg(devicePath, m_port->errorString()));
        return false;
    }

    // Baud
    m_port->setBaudRate(baudRate);

    // Data bits
    switch(dataBits) {
    case 5: m_port->setDataBits(QSerialPort::Data5); break;
    case 6: m_port->setDataBits(QSerialPort::Data6); break;
    case 7: m_port->setDataBits(QSerialPort::Data7); break;
    default: m_port->setDataBits(QSerialPort::Data8); break;
    }

    // Stop bits
    if(stopBits == 2)
        m_port->setStopBits(QSerialPort::TwoStop);
    else
        m_port->setStopBits(QSerialPort::OneStop);

    // Parity
    QSerialPort::Parity qparity = QSerialPort::NoParity;
    switch(parity) {
    case 1: qparity = QSerialPort::OddParity; break;
    case 2: qparity = QSerialPort::EvenParity; break;
    case 3: qparity = QSerialPort::MarkParity; break;
    case 4: qparity = QSerialPort::SpaceParity; break;
    default: break;
    }
    m_port->setParity(qparity);

    // Flow control
    m_port->setFlowControl(flowControl ? QSerialPort::HardwareControl : QSerialPort::NoFlowControl);

    m_running = true;
    return true;
#else
    Q_UNUSED(devicePath)
    Q_UNUSED(baudRate)
    Q_UNUSED(dataBits)
    Q_UNUSED(stopBits)
    Q_UNUSED(parity)
    Q_UNUSED(flowControl)
    return false;
#endif
}

bool SerialChannel::isRunning() const
{
    return m_running;
}

void SerialChannel::sendData(const QByteArray &data)
{
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    if(m_port && m_running) {
        m_port->write(data);
    }
#else
    Q_UNUSED(data)
#endif
}

} // namespace Konsole
