#ifndef PTY_H
#define PTY_H

#include "qtermwidget_export.h"
#include <QStringList>
#include <QSize>

#include <QObject>
#include <QProcess>
#include "kptyprocess.h"

namespace Konsole {

class QTERMWIDGET_EXPORT Pty : public KPtyProcess {
    Q_OBJECT

public:
    explicit Pty(QObject* parent = nullptr);
    explicit Pty(int ptyMasterFd, QObject* parent = nullptr);
    virtual ~Pty() override;

    // Virtual interface for ConPTY implementation
    virtual int start(const QString&, const QStringList&, const QStringList&, ulong, bool);
    virtual void setEmptyPTYProperties();
    virtual void setWriteable(bool);
    virtual void setFlowControlEnabled(bool);
    virtual bool flowControlEnabled() const;
    virtual void setWindowSize(int lines, int cols);
    virtual QSize windowSize() const;
    virtual void setErase(char);
    virtual char erase() const;
    virtual int foregroundProcessGroup() const;
    virtual void closePty();
    virtual QProcess::ProcessState state() const { return KPtyProcess::state(); }
    virtual qint64 processId() const { return KPtyProcess::processId(); }
    virtual bool waitForFinished(int msecs = 30000) { return KPtyProcess::waitForFinished(msecs); }
    virtual QProcess::ExitStatus exitStatus() const { return KPtyProcess::exitStatus(); }
    virtual void setWorkingDirectory(const QString& dir) { KPtyProcess::setWorkingDirectory(dir); }

    KPtyDevice* pty() const { return KPtyProcess::pty(); }

public slots:
    virtual void sendData(const char* buffer, int length);
    virtual void lockPty(bool lock);
    virtual void setUtf8Mode(bool on);

signals:
    void receivedData(const char* buffer, int length);

private slots:
    void dataReceived();

private:
    void init();
    void addEnvironmentVariables(const QStringList& environment);

    int  _windowColumns;
    int  _windowLines;
    char _eraseChar;
    bool _xonXoff;
    bool _utf8;
};

} // namespace Konsole

#endif // PTY_H