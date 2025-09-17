#ifdef _WIN32
#include "kptyprocess.h"
#include <QtCore/QDebug>

// Minimal Windows stub of KPtyProcess providing just enough to satisfy linkage.
// All PTY-related functionality is effectively no-op until a real ConPTY layer
// is implemented.

KPtyProcess::KPtyProcess(QObject *parent) : KProcess(parent), d_ptr(new KPtyProcessPrivate)
{
    // allocate dummy device pointer to avoid null deref if accessed; remains null here
}

KPtyProcess::KPtyProcess(int ptyMasterFd, QObject *parent) : KProcess(parent), d_ptr(new KPtyProcessPrivate)
{
    Q_UNUSED(ptyMasterFd);
}

KPtyProcess::~KPtyProcess() = default;

void KPtyProcess::setPtyChannels(PtyChannels channels)
{
    Q_D(KPtyProcess);
    d->ptyChannels = channels;
}

KPtyProcess::PtyChannels KPtyProcess::ptyChannels() const
{
    Q_D(const KPtyProcess);
    return d->ptyChannels;
}

void KPtyProcess::setUseUtmp(bool value)
{
    Q_D(KPtyProcess);
    d->addUtmp = value;
}

bool KPtyProcess::isUseUtmp() const
{
    Q_D(const KPtyProcess);
    return d->addUtmp;
}

KPtyDevice *KPtyProcess::pty() const
{
    return nullptr; // no PTY device on Windows stub
}

#endif // _WIN32
