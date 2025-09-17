#include "PtyConPty_win.h"
#include <windows.h>
#include <vector>
#include <thread>
#include <atomic>
#include <QtDebug>
#include <QCoreApplication>

namespace Konsole {

// Static member initialization
CreatePseudoConsole_t PtyConPty::CreatePseudoConsole = nullptr;
ResizePseudoConsole_t PtyConPty::ResizePseudoConsole = nullptr;
ClosePseudoConsole_t PtyConPty::ClosePseudoConsole = nullptr;
bool PtyConPty::s_apisLoaded = false;

bool PtyConPty::loadConPtyApis()
{
    if (s_apisLoaded) {
        return CreatePseudoConsole != nullptr;
    }

    s_apisLoaded = true;

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        qWarning() << "Failed to get kernel32.dll handle";
        return false;
    }

    CreatePseudoConsole = reinterpret_cast<CreatePseudoConsole_t>(
        GetProcAddress(hKernel32, "CreatePseudoConsole"));
    ResizePseudoConsole = reinterpret_cast<ResizePseudoConsole_t>(
        GetProcAddress(hKernel32, "ResizePseudoConsole"));
    ClosePseudoConsole = reinterpret_cast<ClosePseudoConsole_t>(
        GetProcAddress(hKernel32, "ClosePseudoConsole"));

    if (!CreatePseudoConsole || !ResizePseudoConsole || !ClosePseudoConsole) {
        qWarning() << "ConPTY APIs not available on this Windows version";
        CreatePseudoConsole = nullptr;
        ResizePseudoConsole = nullptr;
        ClosePseudoConsole = nullptr;
        return false;
    }

    return true;
}

bool PtyConPty::createConPty()
{
    if (!CreatePseudoConsole) {
        return false;
    }

    // Create pipes for ConPTY
    HANDLE inRead = nullptr;
    HANDLE inWrite = nullptr;
    HANDLE outRead = nullptr;
    HANDLE outWrite = nullptr;

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

    if (!CreatePipe(&inRead, &inWrite, &sa, 0)) {
        qWarning() << "Failed to create input pipe";
        return false;
    }

    if (!CreatePipe(&outRead, &outWrite, &sa, 0)) {
        qWarning() << "Failed to create output pipe";
        CloseHandle(inRead);
        CloseHandle(inWrite);
        return false;
    }

    // Create the ConPTY
    COORD size = { (SHORT)m_cols, (SHORT)m_rows };
    HRESULT hr = CreatePseudoConsole(size, inRead, outWrite, 0, &m_conPty);

    if (FAILED(hr)) {
        qWarning() << "Failed to create ConPTY:" << hr;
        CloseHandle(inRead);
        CloseHandle(inWrite);
        CloseHandle(outRead);
        CloseHandle(outWrite);
        return false;
    }

    // Close the handles that were passed to CreatePseudoConsole
    CloseHandle(inRead);
    CloseHandle(outWrite);

    // Keep the handles we need
    m_inWrite = inWrite;
    m_outRead = outRead;

    return true;
}

bool PtyConPty::startProcess(const QString& program, const QStringList& arguments, const QStringList& environment)
{
    if (!m_conPty) {
        return false;
    }

    // Prepare command line - ensure .exe extension on Windows
    QString actualProgram = program;
    if (!program.contains('.') && !program.contains('\\') && !program.contains('/')) {
        if (program == "cmd") {
            actualProgram = "C:\\Windows\\System32\\cmd.exe";
        } else {
            actualProgram += ".exe";
        }
    }

    QString cmdLine = actualProgram;
    for (const QString& arg : arguments) {
        cmdLine += " " + arg;
    }

    // Prepare startup info
    STARTUPINFOEXW si = { sizeof(STARTUPINFOEXW) };

    // Set up the process attributes for ConPTY
    SIZE_T attrSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);

    si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, attrSize));
    if (!si.lpAttributeList) {
        qWarning() << "Failed to allocate attribute list";
        return false;
    }

    if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrSize)) {
        qWarning() << "Failed to initialize attribute list";
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
        return false;
    }

    if (!UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   m_conPty, sizeof(HPCON), nullptr, nullptr)) {
        qWarning() << "Failed to update proc thread attribute";
        DeleteProcThreadAttributeList(si.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
        return false;
    }

    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.StartupInfo.dwFlags = EXTENDED_STARTUPINFO_PRESENT;

    // Create the process
    std::wstring cmdLineW = (actualProgram + " " + arguments.join(' ')).toStdWString();
    std::wstring workingDirW = m_workingDir.isEmpty() ? std::wstring() : m_workingDir.toStdWString();

    BOOL success = CreateProcessW(
        nullptr,                           // lpApplicationName
        const_cast<LPWSTR>(cmdLineW.data()), // lpCommandLine
        nullptr,                           // lpProcessAttributes
        nullptr,                           // lpThreadAttributes
        TRUE,                              // bInheritHandles
        EXTENDED_STARTUPINFO_PRESENT,      // dwCreationFlags
        nullptr,                           // lpEnvironment
        nullptr,                           // lpCurrentDirectory
        &si.StartupInfo,                   // lpStartupInfo
        &m_pi                              // lpProcessInformation
    );

    // Clean up
    DeleteProcThreadAttributeList(si.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, si.lpAttributeList);

    if (!success) {
        DWORD error = GetLastError();
        qWarning() << "Failed to create process:" << error << "for command:" << (actualProgram + " " + arguments.join(' '));
        return false;
    }

    qDebug() << "Successfully created process with PID:" << m_pi.dwProcessId;

    m_state = QProcess::Running;
    return true;
}

void PtyConPty::startOutputReader()
{
    if (m_outputThread || !m_outRead) {
        return;
    }

    m_outputThreadRunning = true;
    m_outputThread = new std::thread([this]() {
        const int bufferSize = 4096;
        char buffer[bufferSize];

        while (m_outputThreadRunning) {
            DWORD bytesRead = 0;
            if (ReadFile(m_outRead, buffer, bufferSize, &bytesRead, nullptr)) {
                if (bytesRead > 0) {
                    // Emit the received data signal
                    emit receivedData(buffer, bytesRead);
                }
            } else {
                DWORD error = GetLastError();
                if (error != ERROR_IO_PENDING && error != ERROR_BROKEN_PIPE) {
                    qWarning() << "ReadFile failed:" << error;
                }
                break;
            }
        }
    });
}

void PtyConPty::stopOutputReader()
{
    if (m_outputThread) {
        m_outputThreadRunning = false;
        if (m_outputThread->joinable()) {
            m_outputThread->join();
        }
        delete m_outputThread;
        m_outputThread = nullptr;
    }
}

PtyConPty::PtyConPty(QObject* parent) : Pty(parent) {
    if (!loadConPtyApis()) {
        qWarning() << "Failed to load ConPTY APIs";
    }
}

PtyConPty::~PtyConPty()
{
    closePty();
}

int PtyConPty::start(const QString& program,
                     const QStringList& arguments,
                     const QStringList& environment,
                     ulong /*winid*/,
                     bool /*addToUtmp*/)
{
    if (m_initialized) {
        return -1; // Already started
    }

    if (!CreatePseudoConsole) {
        qWarning() << "ConPTY APIs not available";
        return -1;
    }

    // Create the ConPTY
    if (!createConPty()) {
        qWarning() << "Failed to create ConPTY";
        return -1;
    }

    // Start the process
    if (!startProcess(program, arguments, environment)) {
        qWarning() << "Failed to start process";
        closePty();
        return -1;
    }

    // Start the output reader thread
    startOutputReader();

    m_initialized = true;
    return 0;
}

void PtyConPty::setWindowSize(int lines, int cols)
{
    m_rows = lines;
    m_cols = cols;

    if (m_conPty && ResizePseudoConsole) {
        COORD size = { (SHORT)cols, (SHORT)lines };
        HRESULT hr = ResizePseudoConsole(m_conPty, size);
        if (FAILED(hr)) {
            qWarning() << "Failed to resize ConPTY:" << hr;
        }
    }
}

QSize PtyConPty::windowSize() const
{
    return QSize(m_cols, m_rows);
}

void PtyConPty::sendData(const char* buffer, int length)
{
    if (!m_inWrite || !m_initialized) {
        return;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(m_inWrite, buffer, length, &bytesWritten, nullptr)) {
        qWarning() << "WriteFile failed:" << GetLastError();
    }
}

void PtyConPty::closePty()
{
    stopOutputReader();

    if (m_pi.hProcess) {
        TerminateProcess(m_pi.hProcess, 0);
        CloseHandle(m_pi.hProcess);
        CloseHandle(m_pi.hThread);
        ZeroMemory(&m_pi, sizeof(m_pi));
    }

    if (m_conPty && ClosePseudoConsole) {
        ClosePseudoConsole(m_conPty);
        m_conPty = nullptr;
    }

    if (m_inWrite) {
        CloseHandle(m_inWrite);
        m_inWrite = nullptr;
    }

    if (m_outRead) {
        CloseHandle(m_outRead);
        m_outRead = nullptr;
    }

    m_state = QProcess::NotRunning;
    m_initialized = false;
}

// In a future implementation, a reader thread or overlapped IO will capture output and emit:
// emit receivedData(reinterpret_cast<const char*>(buf), bytesRead);

QProcess::ProcessState PtyConPty::state() const
{
    return m_state;
}

qint64 PtyConPty::processId() const
{
    return m_pi.dwProcessId ? (qint64)m_pi.dwProcessId : 0;
}

bool PtyConPty::waitForFinished(int msecs)
{
    if (!m_pi.hProcess) {
        return true;
    }

    DWORD timeout = (msecs < 0) ? INFINITE : (DWORD)msecs;
    DWORD result = WaitForSingleObject(m_pi.hProcess, timeout);

    if (result == WAIT_OBJECT_0) {
        // Process has finished
        DWORD exitCode = 0;
        if (GetExitCodeProcess(m_pi.hProcess, &exitCode)) {
            m_exitStatus = (exitCode == 0) ? QProcess::NormalExit : QProcess::CrashExit;
            // Emit the finished signal
            emit finished((int)exitCode, m_exitStatus);
        }
        m_state = QProcess::NotRunning;
        return true;
    } else if (result == WAIT_TIMEOUT) {
        return false; // Timeout
    } else {
        qWarning() << "WaitForSingleObject failed:" << GetLastError();
        return false;
    }
}

QProcess::ExitStatus PtyConPty::exitStatus() const
{
    return m_exitStatus;
}

void PtyConPty::setWorkingDirectory(const QString& dir)
{
    m_workingDir = dir;
}

} // namespace Konsole
