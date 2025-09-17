// Minimal Windows-friendly implementation of Konsole::Pty base class
// Provides definitions for constructor/destructor and default no-op virtuals
// so that Windows ConPTY backend (PtyConPty) can link without POSIX deps.

#ifdef _WIN32

#include "Pty.h"
#include <QtCore/QDebug>

namespace Konsole {

Pty::Pty(QObject* parent)
    : KPtyProcess(parent)
    , _windowColumns(80)
    , _windowLines(24)
    , _eraseChar('\b')
    , _xonXoff(false)
    , _utf8(true)
{
    // No POSIX device wiring on Windows; derived class handles I/O
}

Pty::Pty(int /*ptyMasterFd*/, QObject* parent)
    : KPtyProcess(parent)
    , _windowColumns(80)
    , _windowLines(24)
    , _eraseChar('\b')
    , _xonXoff(false)
    , _utf8(true)
{
}

Pty::~Pty() = default;

int Pty::start(const QString&, const QStringList&, const QStringList&, ulong, bool)
{
    // Should be implemented by derived backend (PtyConPty)
    return -1;
}

void Pty::setEmptyPTYProperties() {}
void Pty::setWriteable(bool) {}
void Pty::setFlowControlEnabled(bool on) { _xonXoff = on; }
bool Pty::flowControlEnabled() const { return _xonXoff; }
void Pty::setWindowSize(int lines, int cols) { _windowLines = lines; _windowColumns = cols; }
QSize Pty::windowSize() const { return QSize(_windowColumns, _windowLines); }
void Pty::setErase(char ch) { _eraseChar = ch; }
char Pty::erase() const { return _eraseChar; }
int Pty::foregroundProcessGroup() const { return 0; }
void Pty::closePty() {}

void Pty::sendData(const char* /*buffer*/, int /*length*/) { /* Derived class should override */ }
void Pty::lockPty(bool /*lock*/) {}
void Pty::setUtf8Mode(bool on) { _utf8 = on; }

// Private slot expected by moc; on Windows base class doesn't read from a QIODevice,
// so this is a safe no-op. ConPTY backend emits receivedData directly.
void Pty::dataReceived()
{
    // no-op on Windows stub
}

} // namespace Konsole

#endif // _WIN32
