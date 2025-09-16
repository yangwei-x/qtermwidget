// Small factory header for creating platform-specific Pty implementations.
#ifndef PTYFACTORY_H
#define PTYFACTORY_H

#include "Pty.h"

namespace Konsole {

// Create a platform-appropriate Pty instance. Caller owns the returned pointer.
Pty* createPty(QObject* parent = nullptr);

}

#endif // PTYFACTORY_H
