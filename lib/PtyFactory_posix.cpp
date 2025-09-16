#include "PtyFactory.h"
#include "Pty.h"
#include "pty_posix.h"

namespace Konsole {

Pty* createPty(QObject* parent)
{
    // Return a POSIX-specific Pty wrapper. Currently this simply
    // delegates to existing Pty implementation but centralizes the
    // POSIX backend creation point for future extensions.
    return new PosixPty(parent);
}

}
