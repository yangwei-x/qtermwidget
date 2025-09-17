#include "PtyConPty_win.h"
#include "Pty.h"

namespace Konsole {

// Factory function for Windows: returns real ConPTY implementation
Pty* createPty(QObject* parent)
{
    return new PtyConPty(parent);
}

} // namespace Konsole
