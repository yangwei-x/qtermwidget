#include "pty_posix.h"

using namespace Konsole;

PosixPty::PosixPty(QObject* parent)
    : Pty(parent)
{
    // POSIX-specific initialization could go here in future (termios tweaks, etc.)
}

PosixPty::~PosixPty()
{
}
