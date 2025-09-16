#ifndef PTY_POSIX_H
#define PTY_POSIX_H

#include "Pty.h"

namespace Konsole {

// Thin wrapper for POSIX-specific Pty behavior. Currently delegates
// to the platform-independent Pty implementation which itself wraps
// KPtyProcess. This file centralizes POSIX-specific additions so the
// factory can return a dedicated backend later.
class PosixPty : public Pty
{
    Q_OBJECT
public:
    explicit PosixPty(QObject* parent = nullptr);
    ~PosixPty() override;
};

}

#endif // PTY_POSIX_H
