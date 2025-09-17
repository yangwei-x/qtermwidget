#ifndef PTYCONPTY_WIN_H
#define PTYCONPTY_WIN_H


#include <windows.h>
#include <QProcess>
#include <QStringList>
#include <QSize>
#include "qtermwidget_export.h"
#include "Pty.h"

namespace Konsole {

// Windows 10+ ConPTY implementation
class QTERMWIDGET_EXPORT PtyConPty : public Pty {
public:
    explicit PtyConPty(QObject* parent = nullptr);
    ~PtyConPty() override;

    int start(const QString& program,
              const QStringList& arguments,
              const QStringList& environment,
              ulong winid,
              bool addToUtmp);
    void setWindowSize(int lines, int cols) override;
    QSize windowSize() const override;
    void sendData(const char* buffer, int length) override;
    void closePty() override;
    QProcess::ProcessState state() const override;
    qint64 processId() const override;
    bool waitForFinished(int msecs = 30000) override;
    QProcess::ExitStatus exitStatus() const override;
    void setWorkingDirectory(const QString& dir) override;

private:
    // ConPTY handles and process info
    void* m_conPty = nullptr; // HPCON
    HANDLE m_inWrite = nullptr;
    HANDLE m_outRead = nullptr;
    PROCESS_INFORMATION m_pi{};
    int m_cols = 80;
    int m_rows = 24;
    QProcess::ProcessState m_state = QProcess::NotRunning;
    QProcess::ExitStatus m_exitStatus = QProcess::NormalExit;
    QString m_workingDir;
    bool m_initialized = false;
};

} // namespace Konsole

#endif // PTYCONPTY_WIN_H
