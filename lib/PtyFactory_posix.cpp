#include "PtyFactory.h"
#include "Pty.h"
#include "pty_posix.h"

#if defined(QTERMWIDGET_ENABLE_CONPTY) && (WIN32 OR MINGW)
#include "pty_windows.h"
#endif

namespace Konsole {

Pty* createPty(QObject* parent)
{
#if defined(QTERMWIDGET_ENABLE_CONPTY) && (WIN32 OR MINGW)
    // If ConPTY support is requested at configure time and we're on Windows,
    // prefer the ConPty backend. The `pty_windows.h` file is a scaffold for
    // a future ConPTY implementation.
    return new ConPty(parent);
#else
    // Default: return the POSIX wrapper
    return new PosixPty(parent);
#endif
}

}
