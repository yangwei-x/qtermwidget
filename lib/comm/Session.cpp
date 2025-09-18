/*
    This file is part of Konsole

    Copyright (C) 2006-2007 by Robert Knight <robertknight@gmail.com>
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    Rewritten for QT4 by e_k <e_k at users.sourceforge.net>, Copyright (C)2008

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

#include "Session.h"
#ifdef QTERMWIDGET_HAVE_PTY
#include "PtyFactory.h"
#include "Pty.h"
#endif

#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QFile>
#include <QtDebug>
#include <QRegularExpression>

#include "TerminalDisplay.h"
#include "comm/ShellCommand.h"
#include "Vt102Emulation.h"
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
#include "comm/SerialChannel.h"
#endif
#ifdef QTERMWIDGET_HAVE_LIBSSH
#include "comm/SSHChannel.h"
#endif

using namespace Konsole;

int Session::lastSessionId = 0;

Session::Session(QObject* parent) :
    QObject(parent),
        _shellProcess(nullptr)
        , _emulation(nullptr)
        , _monitorActivity(false)
        , _monitorSilence(false)
        , _notifiedActivity(false)
        , _autoClose(true)
        , _wantedClose(false)
        , _silenceSeconds(10)
        , _isTitleChanged(false)
        , _addToUtmp(false)
        , _flowControl(true)
        , _fullScripting(false)
        , _sessionId(0)
        , _hasDarkBackground(false)
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    , _serialChannel(nullptr)
    , _serialActive(false)
#endif
#ifdef QTERMWIDGET_HAVE_LIBSSH
    , _sshChannel(nullptr)
    , _sshActive(false)
#endif
{
#ifdef QTERMWIDGET_HAVE_PTY
    _shellProcess = createPty(this);
#if !defined(_WIN32)
    ptySlaveFd = _shellProcess->pty()->slaveFd();
#else
    ptySlaveFd = -1;
#endif
#else
    _shellProcess = nullptr;
    ptySlaveFd = -1;
#endif

    _emulation = new Vt102Emulation();

    connect( _emulation, SIGNAL( titleChanged( int, const QString & ) ),
             this, SLOT( setUserTitle( int, const QString & ) ) );
    connect( _emulation, SIGNAL( stateSet(int) ),
             this, SLOT( activityStateSet(int) ) );
    connect( _emulation, SIGNAL( changeTabTextColorRequest( int ) ),
             this, SIGNAL( changeTabTextColorRequest( int ) ) );
    connect( _emulation, SIGNAL(profileChangeCommandReceived(const QString &)),
             this, SIGNAL( profileChangeCommandReceived(const QString &)) );

    connect(_emulation, SIGNAL(imageResizeRequest(QSize)),
            this, SLOT(onEmulationSizeChange(QSize)));
    connect(_emulation, SIGNAL(imageSizeChanged(int, int)),
            this, SLOT(onViewSizeChange(int, int)));
    connect(_emulation, &Vt102Emulation::cursorChanged,
            this, &Session::cursorChanged);

    if(_shellProcess) {
        _shellProcess->setUtf8Mode(true);
    }

    if(_shellProcess) {
        connect( _shellProcess,SIGNAL(receivedData(const char *,int)),this,
                 SLOT(onReceiveBlock(const char *,int)) );
        connect( _emulation,SIGNAL(sendData(const char *,int)),_shellProcess,
                 SLOT(sendData(const char *,int)) );
        connect( _emulation,SIGNAL(lockPtyRequest(bool)),_shellProcess,SLOT(lockPty(bool)) );
        connect( _emulation,SIGNAL(useUtf8Request(bool)),_shellProcess,SLOT(setUtf8Mode(bool)) );
        connect( _shellProcess,SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(done(int,QProcess::ExitStatus)) );
    }

    _monitorTimer = new QTimer(this);
    _monitorTimer->setSingleShot(true);
    connect(_monitorTimer, SIGNAL(timeout()), this, SLOT(monitorTimerDone()));
}

WId Session::windowId() const
{
    return 0;
}

void Session::setDarkBackground(bool darkBackground)
{
    _hasDarkBackground = darkBackground;
}
bool Session::hasDarkBackground() const
{
    return _hasDarkBackground;
}
bool Session::isRunning() const
{
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    if(_serialActive && _serialChannel) {
        return _serialChannel->isRunning();
    }
#endif
#ifdef QTERMWIDGET_HAVE_LIBSSH
    if(_sshActive && _sshChannel) {
        return _sshChannel->isRunning();
    }
#endif
    return (_shellProcess != nullptr && _shellProcess->state() == QProcess::Running);
}

void Session::setProgram(const QString & program)
{
    _program = ShellCommand::expand(program);
}
void Session::setInitialWorkingDirectory(const QString & dir)
{
    _initialWorkingDir = ShellCommand::expand(dir);
}
void Session::setArguments(const QStringList & arguments)
{
    _arguments = ShellCommand::expand(arguments);
}

QList<TerminalDisplay *> Session::views() const
{
    return _views;
}

void Session::addView(TerminalDisplay * widget)
{
    Q_ASSERT( !_views.contains(widget) );

    _views.append(widget);

    if ( _emulation != nullptr ) {
        connect( widget , &TerminalDisplay::keyPressedSignal, _emulation ,
                 &Emulation::sendKeyEvent);
        connect( widget , SIGNAL(mouseSignal(int,int,int,int)) , _emulation ,
                 SLOT(sendMouseEvent(int,int,int,int)) );
        connect( widget , SIGNAL(sendStringToEmu(const char *)) , _emulation ,
                 SLOT(sendString(const char *)) );

        connect( _emulation , SIGNAL(programUsesMouseChanged(bool)) , widget ,
                 SLOT(setUsesMouse(bool)) );

        widget->setUsesMouse( _emulation->programUsesMouse() );

        connect( _emulation , SIGNAL(programBracketedPasteModeChanged(bool)) ,
                 widget , SLOT(setBracketedPasteMode(bool)) );

        widget->setBracketedPasteMode(_emulation->programBracketedPasteMode());

        widget->setScreenWindow(_emulation->createWindow());
    }

    QObject::connect( widget ,SIGNAL(changedContentSizeSignal(int,int)),this,
                      SLOT(onViewSizeChange(int,int)));

    QObject::connect( widget ,SIGNAL(destroyed(QObject *)) , this ,
                      SLOT(viewDestroyed(QObject *)) );
    QObject::connect(this, SIGNAL(finished()), widget, SLOT(close()));

}

void Session::viewDestroyed(QObject * view)
{
    TerminalDisplay * display = (TerminalDisplay *)view;

    Q_ASSERT( _views.contains(display) );

    removeView(display);
}

void Session::removeView(TerminalDisplay * widget)
{
    _views.removeAll(widget);

    disconnect(widget,nullptr,this,nullptr);

    if ( _emulation != nullptr ) {
        disconnect( widget, nullptr, _emulation, nullptr);
        disconnect( _emulation , nullptr , widget , nullptr);
    }

    if ( _views.count() == 0 ) {
        close();
    }
}

void Session::run()
{
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    if(_serialActive) {
        return;
    }
#endif
#ifdef QTERMWIDGET_HAVE_LIBSSH
    if(_sshActive) {
        return;
    }
#endif
    QString exec = QString::fromLocal8Bit(QFile::encodeName(_program));
#ifdef _WIN32
    if (exec.isEmpty() || exec.startsWith(QLatin1Char('/'))) {
        QString comspec = QString::fromLocal8Bit(qgetenv("ComSpec"));
        if (comspec.isEmpty()) {
            comspec = QStringLiteral("C:/Windows/System32/cmd.exe");
        }
        exec = comspec;
    }
#else
    if (exec.startsWith(QLatin1Char('/')) || exec.isEmpty())
    {
        const QString defaultShell{QLatin1String("/bin/sh")};

        QFile excheck(exec);
        if ( exec.isEmpty() || !excheck.exists() ) {
            exec = QString::fromLocal8Bit(qgetenv("SHELL"));
        }
        excheck.setFileName(exec);

        if ( exec.isEmpty() || !excheck.exists() ) {
            qWarning() << "Neither default shell nor $SHELL is set to a correct path. Fallback to" << defaultShell;
            exec = defaultShell;
        }
    }
#endif

    QString argsTmp(_arguments.join(QLatin1Char(' ')).trimmed());
    QStringList arguments;
    if (argsTmp.length())
        arguments = _arguments;

    QString cwd = QDir::currentPath();
    if (!_initialWorkingDir.isEmpty()) {
        _shellProcess->setWorkingDirectory(_initialWorkingDir);
    } else {
        _shellProcess->setWorkingDirectory(cwd);
    }

    _shellProcess->setFlowControlEnabled(_flowControl);
    _shellProcess->setErase(_emulation->eraseChar());

    QString backgroundColorHint = _hasDarkBackground ? QLatin1String("COLORFGBG=15;0") : QLatin1String("COLORFGBG=0;15");

    int result = _shellProcess->start(exec,
                                      arguments,
                                      _environment << backgroundColorHint,
                                      windowId(),
                                      _addToUtmp);

    if (result < 0) {
        qDebug() << "CRASHED! result: " << result;
        return;
    }

    _shellProcess->setWriteable(false);
    emit started();
}

void Session::runEmptyPTY()
{
    _shellProcess->setFlowControlEnabled(_flowControl);
    _shellProcess->setErase(_emulation->eraseChar());
    _shellProcess->setWriteable(false);

    disconnect( _emulation,SIGNAL(sendData(const char *,int)),
                _shellProcess, SLOT(sendData(const char *,int)) );

    _shellProcess->setEmptyPTYProperties();
    emit started();
}

void Session::setUserTitle( int what, const QString & caption )
{
    bool modified = false;

    if ((what == 0) || (what == 2)) {
        _isTitleChanged = true;
        if ( _userTitle != caption ) {
            _userTitle = caption;
            modified = true;
        }
    }

    if ((what == 0) || (what == 1)) {
        _isTitleChanged = true;
        if ( _iconText != caption ) {
            _iconText = caption;
            modified = true;
        }
    }

    if (what == 11) {
        QString colorString = caption.section(QLatin1Char(';'),0,0);
        QColor backColor = QColor(colorString);
        if (backColor.isValid()) {
            if (backColor != _modifiedBackground) {
                _modifiedBackground = backColor;
                Q_ASSERT( 0 );
                emit changeBackgroundColorRequest(backColor);
            }
        }
    }

    if (what == 30) {
        _isTitleChanged = true;
        if ( _nameTitle != caption ) {
            setTitle(Session::NameRole,caption);
            return;
        }
    }

    if (what == 31) {
        QString cwd=caption;
        cwd=cwd.replace( QRegularExpression(QLatin1String("^~")), QDir::homePath() );
        emit openUrlRequest(cwd);
    }

    if (what == 32) {
        _isTitleChanged = true;
        if ( _iconName != caption ) {
            _iconName = caption;
            modified = true;
        }
    }

    if (what == 50) {
        emit profileChangeCommandReceived(caption);
        return;
    }

    if ( modified ) {
        emit titleChanged();
    }
}

QString Session::userTitle() const
{
    return _userTitle;
}
void Session::setTabTitleFormat(TabTitleContext context , const QString & format)
{
    if ( context == LocalTabTitle ) {
        _localTabTitleFormat = format;
    } else if ( context == RemoteTabTitle ) {
        _remoteTabTitleFormat = format;
    }
}
QString Session::tabTitleFormat(TabTitleContext context) const
{
    if ( context == LocalTabTitle ) {
        return _localTabTitleFormat;
    } else if ( context == RemoteTabTitle ) {
        return _remoteTabTitleFormat;
    }

    return QString();
}

void Session::monitorTimerDone()
{
    if (_monitorSilence) {
        emit silence();
        emit stateChanged(NOTIFYSILENCE);
    } else {
        emit stateChanged(NOTIFYNORMAL);
    }

    _notifiedActivity=false;
}

void Session::activityStateSet(int state)
{
    if (state==NOTIFYBELL) {
        emit bellRequest(tr("Bell in session '%1'").arg(_nameTitle));
    } else if (state==NOTIFYACTIVITY) {
        if (_monitorSilence) {
            _monitorTimer->start(_silenceSeconds*1000);
        }

        if ( _monitorActivity ) {
            if (!_notifiedActivity) {
                _notifiedActivity=true;
                emit activity();
            }
        }
    }

    if ( state==NOTIFYACTIVITY && !_monitorActivity ) {
        state = NOTIFYNORMAL;
    }
    if ( state==NOTIFYSILENCE && !_monitorSilence ) {
        state = NOTIFYNORMAL;
    }

    emit stateChanged(state);
}

void Session::onViewSizeChange(int /*height*/, int /*width*/)
{
    updateTerminalSize();
}
void Session::onEmulationSizeChange(QSize size)
{
    setSize(size);
}

void Session::updateTerminalSize()
{
    QListIterator<TerminalDisplay *> viewIter(_views);

    int minLines = -1;
    int minColumns = -1;

    const int VIEW_LINES_THRESHOLD = 2;
    const int VIEW_COLUMNS_THRESHOLD = 2;

    while ( viewIter.hasNext() ) {
        TerminalDisplay * view = viewIter.next();
        if ( view->isHidden() == false &&
                view->lines() >= VIEW_LINES_THRESHOLD &&
                view->columns() >= VIEW_COLUMNS_THRESHOLD ) {
            minLines = (minLines == -1) ? view->lines() : qMin( minLines , view->lines() );
            minColumns = (minColumns == -1) ? view->columns() : qMin( minColumns , view->columns() );
        }
    }

    if ( minLines > 0 && minColumns > 0 ) {
        _emulation->setImageSize( minLines , minColumns );
        _shellProcess->setWindowSize( minLines , minColumns );
    }
}

void Session::refresh()
{
    const QSize existingSize = _shellProcess->windowSize();
    _shellProcess->setWindowSize(existingSize.height(),existingSize.width()+1);
    _shellProcess->setWindowSize(existingSize.height(),existingSize.width());
}

bool Session::sendSignal(int signal)
{
    if (processId() <= 0)
        return false;
#ifdef _WIN32
    if (signal == 9 || signal == 15) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)_shellProcess->processId());
        if (hProcess) {
            BOOL res = TerminateProcess(hProcess, 1);
            CloseHandle(hProcess);
            return res ? _shellProcess->waitForFinished(1000) : false;
        }
    }
    return false;
#else
    int result = ::kill(static_cast<pid_t>(_shellProcess->processId()), signal);
    if ( result == 0 )
        return _shellProcess->waitForFinished(1000);
    else
        return false;
#endif
}

void Session::close()
{
    _autoClose = true;
    _wantedClose = true;

    if (isRunning())
    {
#ifdef _WIN32
        sendSignal(9);
        QTimer::singleShot(1, this, SIGNAL(finished()));
#else
        if (sendSignal(SIGHUP))
        {
            return;
        }
        qWarning() << "Process " << processId() << " did not die with SIGHUP";
        _shellProcess->closePty();
        if (!_shellProcess->waitForFinished(1000))
        {
            if (!sendSignal(SIGKILL))
            {
                qWarning() << "Process " << processId() << " did not die with SIGKILL";
                QTimer::singleShot(1, this, SIGNAL(finished()));
            }
        }
#endif
    }
    else
    {
        QTimer::singleShot(1, this, SIGNAL(finished()));
    }
}

void Session::sendText(const QString & text) const
{
    _emulation->sendText(text);
}

void Session::sendKeyEvent(QKeyEvent* e) const
{
    _emulation->sendKeyEvent(e, false);
}

Session::~Session()
{
    close();
    delete _emulation;
    delete _shellProcess;
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    delete _serialChannel;
#endif
}

void Session::setProfileKey(const QString & key)
{
    _profileKey = key;
    emit profileChanged(key);
}
QString Session::profileKey() const
{
    return _profileKey;
}

void Session::done(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!_autoClose) {
        _userTitle = QString::fromLatin1("This session is done. Finished");
        emit titleChanged();
        return;
    }

    QString message;
    if (!_wantedClose || exitCode != 0) {

        if (_shellProcess->exitStatus() == QProcess::NormalExit) {
            message = tr("Session '%1' exited with code %2.").arg(_nameTitle).arg(exitCode);
        } else {
            message = tr("Session '%1' crashed.").arg(_nameTitle);
        }
    }

    if ( !_wantedClose && exitStatus != QProcess::NormalExit )
        message = tr("Session '%1' exited unexpectedly.").arg(_nameTitle);
    else
        emit finished();

}

Emulation * Session::emulation() const
{
    return _emulation;
}

QString Session::keyBindings() const
{
    return _emulation->keyBindings();
}

QStringList Session::environment() const
{
    return _environment;
}

void Session::setEnvironment(const QStringList & environment)
{
    _environment = environment;
}

int Session::sessionId() const
{
    return _sessionId;
}

void Session::setKeyBindings(const QString & id)
{
    _emulation->setKeyBindings(id);
}

void Session::setTitle(TitleRole role , const QString & newTitle)
{
    if ( title(role) != newTitle ) {
        if ( role == NameRole ) {
            _nameTitle = newTitle;
        } else if ( role == DisplayedTitleRole ) {
            _displayTitle = newTitle;
        }

        emit titleChanged();
    }
}

QString Session::title(TitleRole role) const
{
    if ( role == NameRole ) {
        return _nameTitle;
    } else if ( role == DisplayedTitleRole ) {
        return _displayTitle;
    } else {
        return QString();
    }
}

void Session::setIconName(const QString & iconName)
{
    if ( iconName != _iconName ) {
        _iconName = iconName;
        emit titleChanged();
    }
}

void Session::setIconText(const QString & iconText)
{
    _iconText = iconText;
}

QString Session::iconName() const
{
    return _iconName;
}

QString Session::iconText() const
{
    return _iconText;
}

bool Session::isTitleChanged() const
{
    return _isTitleChanged;
}

void Session::setHistoryType(const HistoryType & hType)
{
    _emulation->setHistory(hType);
}

const HistoryType & Session::historyType() const
{
    return _emulation->history();
}

void Session::clearHistory()
{
    _emulation->clearHistory();
}

QStringList Session::arguments() const
{
    return _arguments;
}

QString Session::program() const
{
    return _program;
}

bool Session::isMonitorActivity() const
{
    return _monitorActivity;
}

bool Session::isMonitorSilence()  const
{
    return _monitorSilence;
}

void Session::setMonitorActivity(bool _monitor)
{
    _monitorActivity=_monitor;
    _notifiedActivity=false;

    activityStateSet(NOTIFYNORMAL);
}

void Session::setMonitorSilence(bool _monitor)
{
    if (_monitorSilence==_monitor) {
        return;
    }

    _monitorSilence=_monitor;
    if (_monitorSilence) {
        _monitorTimer->start(_silenceSeconds*1000);
    } else {
        _monitorTimer->stop();
    }

    activityStateSet(NOTIFYNORMAL);
}

void Session::setMonitorSilenceSeconds(int seconds)
{
    _silenceSeconds=seconds;
    if (_monitorSilence) {
        _monitorTimer->start(_silenceSeconds*1000);
    }
}

void Session::setAddToUtmp(bool set)
{
    _addToUtmp = set;
}

void Session::setFlowControlEnabled(bool enabled)
{
    if (_flowControl == enabled) {
        return;
    }

    _flowControl = enabled;

    if (_shellProcess) {
        _shellProcess->setFlowControlEnabled(_flowControl);
    }

    emit flowControlEnabledChanged(enabled);
}
bool Session::flowControlEnabled() const
{
    return _flowControl;
}

void Session::onReceiveBlock( const char * buf, int len )
{
    _emulation->receiveData( buf, len );
    emit receivedData( QString::fromLatin1( buf, len ) );
}

QSize Session::size()
{
    return _emulation->imageSize();
}

void Session::setSize(const QSize & size)
{
    if ((size.width() <= 1) || (size.height() <= 1)) {
        return;
    }

    emit resizeRequest(size);
}
int Session::foregroundProcessId() const
{
    return _shellProcess->foregroundProcessGroup();
}
int Session::processId() const
{
    return static_cast<int>(_shellProcess->processId());
}
int Session::getPtySlaveFd() const
{
    return ptySlaveFd;
}

#ifdef QTERMWIDGET_HAVE_QSERIALPORT
bool Session::runSerial(const QString &devicePath,
                        int baudRate,
                        int dataBits,
                        int stopBits,
                        int parity,
                        bool flowControl)
{
    if(_serialActive) {
        return true;
    }

    if(!_serialChannel) {
        _serialChannel = new SerialChannel(this);
        connect(_serialChannel, &SerialChannel::receivedData, this, [this](const QByteArray &bytes){
            onReceiveBlock(bytes.constData(), bytes.size());
        });
        connect(_serialChannel, &SerialChannel::error, this, [this](const QString &msg){
            qWarning() << "SerialChannel error:" << msg;
        });
    }

    if(!_serialChannel->start(devicePath, baudRate, dataBits, stopBits, parity, flowControl)) {
        return false;
    }

    _serialActive = true;

    disconnect( _shellProcess,SIGNAL(receivedData(const char *,int)),this,
                SLOT(onReceiveBlock(const char *,int)) );
    disconnect( _emulation,SIGNAL(sendData(const char *,int)),_shellProcess,
                SLOT(sendData(const char *,int)) );

    connect(_emulation, &Emulation::sendData, this, [this](const char *data, int len){
        if(_serialChannel) {
            _serialChannel->sendData(QByteArray(data, len));
        }
    });

    emit started();
    return true;
}
#endif

#ifdef QTERMWIDGET_HAVE_LIBSSH
bool Session::runSSH(const QString& host,
                     int port,
                     const QString& user,
                     const QString& password,
                     const QString& termName,
                     int cols,
                     int rows)
{
    if(_sshActive) {
        return true;
    }
    if(!_sshChannel) {
        _sshChannel = new SSHChannel(this);
        connect(_sshChannel, &SSHChannel::receivedData, this, [this](const QByteArray &bytes){
            onReceiveBlock(bytes.constData(), bytes.size());
        });
        connect(_sshChannel, &SSHChannel::error, this, [this](const QString &msg){
            qWarning() << "SSHChannel error:" << msg;
        });
        connect(_sshChannel, &SSHChannel::closed, this, [this](){
            _sshActive = false;
            emit finished();
        });
    }

    if(!_sshChannel->connectAndStart(host, port, user, password, termName, cols, rows)) {
        return false;
    }

    _sshActive = true;

    // Detach from PTY process I/O and hook emulation to SSH
    if(_shellProcess) {
        disconnect( _emulation,SIGNAL(sendData(const char *,int)),_shellProcess,
                    SLOT(sendData(const char *,int)) );
        disconnect( _shellProcess,SIGNAL(receivedData(const char *,int)),this,
                    SLOT(onReceiveBlock(const char *,int)) );
    }
    connect(_emulation, &Emulation::sendData, this, [this](const char *data, int len){
        if(_sshChannel) _sshChannel->sendData(QByteArray(data, len));
    });

    emit started();
    return true;
}
#endif

SessionGroup::SessionGroup()
        : _masterMode(0)
{
}
SessionGroup::~SessionGroup()
{
    connectAll(false);
}
int SessionGroup::masterMode() const
{
    return _masterMode;
}
QList<Session *> SessionGroup::sessions() const
{
    return _sessions.keys();
}
bool SessionGroup::masterStatus(Session * session) const
{
    return _sessions[session];
}

void SessionGroup::addSession(Session * session)
{
    _sessions.insert(session,false);

    QListIterator<Session *> masterIter(masters());

    while ( masterIter.hasNext() ) {
        connectPair(masterIter.next(),session);
    }
}
void SessionGroup::removeSession(Session * session)
{
    setMasterStatus(session,false);

    QListIterator<Session *> masterIter(masters());

    while ( masterIter.hasNext() ) {
        disconnectPair(masterIter.next(),session);
    }

    _sessions.remove(session);
}
void SessionGroup::setMasterMode(int mode)
{
    _masterMode = mode;

    connectAll(false);
    connectAll(true);
}
QList<Session *> SessionGroup::masters() const
{
    return _sessions.keys(true);
}
void SessionGroup::connectAll(bool connect)
{
    QListIterator<Session *> masterIter(masters());

    while ( masterIter.hasNext() ) {
        Session * master = masterIter.next();

        QListIterator<Session *> otherIter(_sessions.keys());
        while ( otherIter.hasNext() ) {
            Session * other = otherIter.next();

            if ( other != master ) {
                if ( connect ) {
                    connectPair(master,other);
                } else {
                    disconnectPair(master,other);
                }
            }
        }
    }
}
void SessionGroup::setMasterStatus(Session * session, bool master)
{
    bool wasMaster = _sessions[session];
    _sessions[session] = master;

    if (wasMaster == master) {
        return;
    }

    QListIterator<Session *> iter(_sessions.keys());
    while (iter.hasNext()) {
        Session * other = iter.next();

        if (other != session) {
            if (master) {
                connectPair(session, other);
            } else {
                disconnectPair(session, other);
            }
        }
    }
}

void SessionGroup::connectPair(Session * master , Session * other) const
{
    if ( _masterMode & CopyInputToAll ) {
        qDebug() << "Connection session " << master->nameTitle() << "to" << other->nameTitle();

        connect( master->emulation() , SIGNAL(sendData(const char *,int)) , other->emulation() ,
                 SLOT(sendString(const char *,int)) );
    }
}
void SessionGroup::disconnectPair(Session * master , Session * other) const
{
    if ( _masterMode & CopyInputToAll ) {
        qDebug() << "Disconnecting session " << master->nameTitle() << "from" << other->nameTitle();

        disconnect( master->emulation() , SIGNAL(sendData(const char *,int)) , other->emulation() ,
                    SLOT(sendString(const char *,int)) );
    }
}
