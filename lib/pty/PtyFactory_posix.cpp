#include "PtyFactory.h"
#include "Pty.h"
#ifndef _WIN32
#include "pty_posix.h"
#endif

namespace Konsole {

Pty* createPty(QObject* parent)
{
#if defined(_WIN32) || defined(__MINGW32__)
    // Windows: Pty methods are implemented in Pty_win.cpp using ConPTY APIs
    return new Pty(parent);
#else
    return new PosixPty(parent);
#endif
}

}
