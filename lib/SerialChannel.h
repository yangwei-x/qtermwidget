/*
 * SerialChannel - abstraction for a serial (e.g. /dev/ttyACM0) connection
 * Provides a similar interface as Pty to integrate with Session/Emulation.
 */

#pragma once

#include <QObject>
#include <QByteArray>

#ifdef QTERMWIDGET_HAVE_QSERIALPORT
#include <QSerialPort>
#endif

namespace Konsole {

class SerialChannel : public QObject
{
    Q_OBJECT
public:
    explicit SerialChannel(QObject *parent = nullptr);
    ~SerialChannel() override;

    bool start(const QString &devicePath,
               int baudRate = 115200,
               int dataBits = 8,
               int stopBits = 1,
               int parity = 0,          // 0=None,1=Odd,2=Even,3=Mark,4=Space (maps to QSerialPort::Parity)
               bool flowControl = false);

    bool isRunning() const;

public slots:
    void sendData(const QByteArray &data);

signals:
    void receivedData(const QByteArray &data);
    void error(const QString &message);

private:
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    QSerialPort *m_port { nullptr };
#endif
    bool m_running { false };
};

} // namespace Konsole
