#pragma once

// Minimal scaffold for a Windows ConPTY backend.
// This file provides a header-only stub so Windows-specific
// code can be compiled conditionally on non-Windows platforms
// without pulling in Win32 headers.

#include "Pty.h"

namespace Konsole {

class ConPty : public Pty
{
    Q_OBJECT
public:
    explicit ConPty(QObject* parent = nullptr) : Pty(parent) {}
    ~ConPty() override {}

    // Minimal stub implementations mirroring the Pty API can be
    // expanded when a real ConPTY implementation is added.
};

} // namespace Konsole
