#ifndef PTY_H
#define PTY_H

#include "qtermwidget_export.h"
#include <QStringList>
#include <QSize>

#include <QObject>
#include <QProcess>

namespace Konsole {

class QTERMWIDGET_EXPORT Pty : public QObject {
    Q_OBJECT

public:
    explicit Pty(QObject* parent = nullptr) : QObject(parent) {}
    explicit Pty(int /*ptyMasterFd*/, QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Pty() override {}

    // Virtual interface for ConPTY implementation
    virtual int start(const QString&, const QStringList&, const QStringList&, ulong, bool) { return 0; }
    virtual void setEmptyPTYProperties() {}
    virtual void setWriteable(bool) {}
    virtual void setFlowControlEnabled(bool) {}
    virtual bool flowControlEnabled() const { return true; }
    virtual void setWindowSize(int lines, int cols) { m_rows = lines; m_cols = cols; }
    virtual QSize windowSize() const { return QSize(m_cols, m_rows); }
    virtual void setErase(char) {}
    virtual char erase() const { return '\b'; }
    virtual int foregroundProcessGroup() const { return 0; }
    virtual void closePty() {}
    virtual void setUtf8Mode(bool) {}
    virtual void lockPty(bool) {}
    virtual void sendData(const char*, int) {}
    virtual QProcess::ProcessState state() const { return QProcess::NotRunning; }
    virtual qint64 processId() const { return 0; }
    virtual bool waitForFinished(int = 30000) { return true; }
    virtual QProcess::ExitStatus exitStatus() const { return QProcess::NormalExit; }
    virtual void setWorkingDirectory(const QString&) {}

signals:
    void receivedData(const char* buffer, int length);

private:
    int m_rows = 24;
    int m_cols = 80;
};

} // namespace Konsole

#endif // PTY_H