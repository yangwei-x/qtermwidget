#ifdef _WIN32
#include "kptydevice.h"

// Minimal Windows stub; no real PTY functionality.
KPtyDevice::KPtyDevice(QObject *parent)
    : QIODevice(parent)
    , KPty(new KPtyDevicePrivate(this))
{}
KPtyDevice::~KPtyDevice() { close(); }

bool KPtyDevice::open(OpenMode mode)
{
    Q_UNUSED(mode);
    return false; // no PTY
}

bool KPtyDevice::open(int fd, OpenMode mode)
{
    Q_UNUSED(fd); Q_UNUSED(mode); return false;
}

void KPtyDevice::close() { }

bool KPtyDevice::isSequential() const { return true; }
bool KPtyDevice::canReadLine() const { return false; }
bool KPtyDevice::atEnd() const { return true; }
qint64 KPtyDevice::bytesAvailable() const { return 0; }
qint64 KPtyDevice::bytesToWrite() const { return 0; }
bool KPtyDevice::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs);
    return true;
}
bool KPtyDevice::waitForReadyRead(int msecs)
{
    Q_UNUSED(msecs);
    return false;
}
void KPtyDevice::setSuspended(bool suspended) { Q_UNUSED(suspended); }
bool KPtyDevice::isSuspended() const { return true; }

qint64 KPtyDevice::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data); Q_UNUSED(maxlen); return -1;
}

qint64 KPtyDevice::readLineData(char *data, qint64 maxlen)
{
    Q_UNUSED(data); Q_UNUSED(maxlen); return -1;
}

qint64 KPtyDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data); return len; // pretend success
}

// Stub implementations to satisfy Q_PRIVATE_SLOT(moc) references
bool KPtyDevicePrivate::_k_canRead() { return false; }
bool KPtyDevicePrivate::_k_canWrite() { return false; }

#endif // _WIN32
