#include "PtyFactory.h"
#include "Pty.h"

namespace Konsole {

Pty* createPty(QObject* parent)
{
    // POSIX backend: return existing Pty which wraps KPtyProcess
    return new Pty(parent);
}

}
