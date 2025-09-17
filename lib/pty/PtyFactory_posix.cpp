#include "PtyFactory.h"
#include "pty_posix.h"

namespace Konsole {

Pty* createPty(QObject* parent)
{
    return new PosixPty(parent);
}

}
