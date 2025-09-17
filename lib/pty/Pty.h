#ifndef PTY_H
#define PTY_H

#include "qtermwidget_export.h"
#include <QStringList>
#include <QSize>

#ifdef _WIN32

#include <QObject>
#include <QProcess>

namespace Konsole {

class QTERMWIDGET_EXPORT Pty : public QObject {
public:
    explicit Pty(QObject* parent = nullptr) : QObject(parent) {}
    explicit Pty(int /*ptyMasterFd*/, QObject* parent = nullptr) : QObject(parent) {}
    ~Pty() override {}

    int start(const QString&, const QStringList&, const QStringList&, ulong, bool) { return 0; }
    void setEmptyPTYProperties() {}
    void setWriteable(bool) {}
    void setFlowControlEnabled(bool) {}
    bool flowControlEnabled() const { return true; }
    void setWindowSize(int lines, int cols) { m_rows = lines; m_cols = cols; }
    QSize windowSize() const { return QSize(m_cols, m_rows); }
    void setErase(char) {}
    char erase() const { return '\b'; }
    int foregroundProcessGroup() const { return 0; }
    void closePty() {}
    void setUtf8Mode(bool) {}
    void lockPty(bool) {}
    void sendData(const char*, int) {}
    QProcess::ProcessState state() const { return QProcess::NotRunning; }
    qint64 processId() const { return 0; }
    bool waitForFinished(int = 30000) { return true; }
    QProcess::ExitStatus exitStatus() const { return QProcess::NormalExit; }
    void setWorkingDirectory(const QString&) {}
private:
    int m_cols = 80;
    int m_rows = 24;
};

} // namespace Konsole

#else // POSIX

#include <QVector>
#include <QList>
#include "kptyprocess.h"

namespace Konsole {

class QTERMWIDGET_EXPORT Pty : public KPtyProcess {
#ifndef Q_MOC_RUN
    Q_OBJECT
#endif
public:
    explicit Pty(QObject* parent = nullptr);
    explicit Pty(int ptyMasterFd, QObject* parent = nullptr);
    ~Pty() override;
    int start(const QString& program,
              const QStringList& arguments,
              const QStringList& environment,
              ulong winid,
              bool addToUtmp);
    void setEmptyPTYProperties();
    void setWriteable(bool writeable);
    void setFlowControlEnabled(bool on);
    bool flowControlEnabled() const;
    void setWindowSize(int lines, int cols);
    QSize windowSize() const;
    void setErase(char erase);
    char erase() const;
    int foregroundProcessGroup() const;
    void closePty();
public slots:
    void setUtf8Mode(bool on);
    void lockPty(bool lock);
    void sendData(const char* buffer, int length);
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

#endif // _WIN32

#endif // PTY_H