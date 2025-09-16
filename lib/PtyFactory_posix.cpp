#include "PtyFactory.h"
#include "Pty.h"
#include "pty_posix.h"

// Only include the Windows scaffold when building on Windows and the ConPTY
// configure option was enabled.
#if defined(QTERMWIDGET_ENABLE_CONPTY) && (defined(_WIN32) || defined(__MINGW32__))
#include "pty_windows.h"
#endif

namespace Konsole {

Pty* createPty(QObject* parent)
{
#if defined(QTERMWIDGET_ENABLE_CONPTY) && (defined(_WIN32) || defined(__MINGW32__))
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
