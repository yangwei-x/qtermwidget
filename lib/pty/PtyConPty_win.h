#ifndef PTYCONPTY_WIN_H
#define PTYCONPTY_WIN_H


#include <windows.h>
#include <QProcess>
#include <QStringList>
#include <QSize>
#include <thread>
#include <atomic>
#include "qtermwidget_export.h"
#include "Pty.h"

// Define HPCON if not available (for older SDKs)
#ifndef HPCON
typedef void* HPCON;
#endif

// Define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE if not available
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

namespace Konsole {

// Function pointer types for ConPTY APIs
typedef HRESULT(WINAPI* CreatePseudoConsole_t)(COORD size, HANDLE hInput, HANDLE hOutput, DWORD dwFlags, HPCON* phPC);
typedef HRESULT(WINAPI* ResizePseudoConsole_t)(HPCON hPC, COORD size);
typedef void(WINAPI* ClosePseudoConsole_t)(HPCON hPC);

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
    // ConPTY API function pointers
    static CreatePseudoConsole_t CreatePseudoConsole;
    static ResizePseudoConsole_t ResizePseudoConsole;
    static ClosePseudoConsole_t ClosePseudoConsole;
    static bool s_apisLoaded;

    // ConPTY handles and process info
    HPCON m_conPty = nullptr;
    HANDLE m_inWrite = nullptr;
    HANDLE m_outRead = nullptr;
    PROCESS_INFORMATION m_pi{};
    int m_cols = 80;
    int m_rows = 24;
    QProcess::ProcessState m_state = QProcess::NotRunning;
    QProcess::ExitStatus m_exitStatus = QProcess::NormalExit;
    QString m_workingDir;
    bool m_initialized = false;

    // Output reader thread
    std::thread* m_outputThread = nullptr;
    std::atomic<bool> m_outputThreadRunning{false};

    // Helper methods
    static bool loadConPtyApis();
    bool createConPty();
    bool startProcess(const QString& program, const QStringList& arguments, const QStringList& environment);
    void startOutputReader();
    void stopOutputReader();
};

} // namespace Konsole

#endif // PTYCONPTY_WIN_H
