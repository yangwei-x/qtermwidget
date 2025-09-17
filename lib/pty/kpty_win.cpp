#ifdef _WIN32
#include "kpty_p.h"
#include <QtCore/QDebug>

// Windows stub implementations to satisfy linkage without POSIX.
// Provides minimal, no-op behavior for KPty and KPtyPrivate on Windows.

KPtyPrivate::KPtyPrivate(KPty* parent)
    : masterFd(-1)
    , slaveFd(-1)
    , ownMaster(true)
    , q_ptr(parent)
{
}

KPtyPrivate::~KPtyPrivate() = default;

bool KPtyPrivate::chownpty(bool)
{
    return true;
}

KPty::KPty()
    : d_ptr(new KPtyPrivate(this))
{
}

KPty::KPty(KPtyPrivate* d)
    : d_ptr(d)
{
    d_ptr->q_ptr = this;
}

KPty::~KPty()
{
    close();
}

bool KPty::open()
{
    // No PTY on Windows stub
    return false;
}

bool KPty::open(int /*fd*/)
{
    return false;
}

void KPty::close()
{
    Q_D(KPty);
    d->masterFd = -1;
    d->slaveFd = -1;
}

void KPty::closeSlave()
{
    Q_D(KPty);
    d->slaveFd = -1;
}

bool KPty::openSlave()
{
    return false;
}

void KPty::setCTty() {}
void KPty::login(const char* /*user*/, const char* /*remotehost*/) {}
void KPty::logout() {}

bool KPty::tcGetAttr(struct ::termios* /*ttmode*/) const { return false; }
bool KPty::tcSetAttr(struct ::termios* /*ttmode*/) { return false; }
bool KPty::setWinSize(int /*lines*/, int /*columns*/) { return true; }
bool KPty::setEcho(bool /*echo*/) { return true; }
const char* KPty::ttyName() const { return ""; }
int KPty::masterFd() const { Q_D(const KPty); return d->masterFd; }
int KPty::slaveFd() const { Q_D(const KPty); return d->slaveFd; }

#endif // _WIN32
