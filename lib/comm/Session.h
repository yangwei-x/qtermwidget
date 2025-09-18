/*
    This file is part of Konsole, an X terminal.

    Copyright (C) 2007 by Robert Knight <robertknight@gmail.com>
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

#ifndef SESSION_H
#define SESSION_H

#include <QProcess>
#include <QStringList>
#include <QWidget>

#include "Emulation.h"
#include "History.h"

class KProcess;

namespace Konsole {

class Emulation;
class Pty;
class TerminalDisplay;
class SerialChannel; // forward (Konsole namespace)
class SSHChannel; // forward

class Session : public QObject {
    Q_OBJECT

public:
    Q_PROPERTY(QString name READ nameTitle)
    Q_PROPERTY(int processId READ processId)
    Q_PROPERTY(QString keyBindings READ keyBindings WRITE setKeyBindings)
    Q_PROPERTY(QSize size READ size WRITE setSize)

    Session(QObject* parent = nullptr);
    ~Session() override;

    bool isRunning() const;

#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    bool runSerial(const QString &devicePath,
                   int baudRate = 115200,
                   int dataBits = 8,
                   int stopBits = 1,
                   int parity = 0,
                   bool flowControl = false);
#endif

#ifdef QTERMWIDGET_HAVE_LIBSSH
    bool runSSH(const QString& host,
                int port,
                const QString& user,
                const QString& password,
                const QString& termName = QStringLiteral("xterm-256color"),
                int cols = 80,
                int rows = 24);
#endif

    void setProfileKey(const QString & profileKey);
    QString profileKey() const;
    void addView(TerminalDisplay * widget);
    void removeView(TerminalDisplay * widget);
    QList<TerminalDisplay *> views() const;
    Emulation * emulation() const;
    QStringList environment() const;
    void setEnvironment(const QStringList & environment);
    int sessionId() const;
    QString userTitle() const;
    enum TabTitleContext { LocalTabTitle, RemoteTabTitle };
    void setTabTitleFormat(TabTitleContext context , const QString & format);
    QString tabTitleFormat(TabTitleContext context) const;
    QStringList arguments() const;
    QString program() const;
    void setArguments(const QStringList & arguments);
    void setProgram(const QString & program);
    QString initialWorkingDirectory() { return _initialWorkingDir; }
    void setInitialWorkingDirectory( const QString & dir );
    void setHistoryType(const HistoryType & type);
    const HistoryType & historyType() const;
    void clearHistory();
    void setMonitorActivity(bool);
    bool isMonitorActivity() const;
    void setMonitorSilence(bool);
    bool isMonitorSilence()  const;
    void setMonitorSilenceSeconds(int seconds);
    void setKeyBindings(const QString & id);
    QString keyBindings() const;
    enum TitleRole { NameRole, DisplayedTitleRole };
    void setTitle(TitleRole role , const QString & title);
    QString title(TitleRole role) const;
    QString nameTitle() const { return title(Session::NameRole); }
    void setIconName(const QString & iconName);
    QString iconName() const;
    void setIconText(const QString & iconText);
    QString iconText() const;
    bool isTitleChanged() const;
    void setAddToUtmp(bool);
    bool sendSignal(int signal);
    void setAutoClose(bool b) { _autoClose = b; }
    void setFlowControlEnabled(bool enabled);
    bool flowControlEnabled() const;
    void sendText(const QString & text) const;
    void sendKeyEvent(QKeyEvent* e) const;
    int processId() const;
    int foregroundProcessId() const;
    QSize size();
    void setSize(const QSize & size);
    void setDarkBackground(bool darkBackground);
    bool hasDarkBackground() const;
    void refresh();
    int getPtySlaveFd() const;

public slots:
    void run();
    void runEmptyPTY();
    void close();
    void setUserTitle( int, const QString & caption );

signals:
    void started();
    void finished();
    void receivedData( const QString & text );
    void titleChanged();
    void profileChanged(const QString & profile);
    void stateChanged(int state);
    void bellRequest( const QString & message );
    void changeTabTextColorRequest(int);
    void changeBackgroundColorRequest(const QColor &);
    void openUrlRequest(const QString & url);
    void resizeRequest(const QSize & size);
    void profileChangeCommandReceived(const QString & text);
    void flowControlEnabledChanged(bool enabled);
    void cursorChanged(Emulation::KeyboardCursorShape cursorShape, bool blinkingCursorEnabled);
    void silence();
    void activity();

private slots:
    void done(int, QProcess::ExitStatus );
    void onReceiveBlock( const char * buffer, int len );
    void monitorTimerDone();
    void onViewSizeChange(int height, int width);
    void onEmulationSizeChange(QSize);
    void activityStateSet(int);
    void viewDestroyed(QObject * view);

private:
    void updateTerminalSize();
    WId windowId() const;

    int            _uniqueIdentifier;
    Pty     *_shellProcess;
    Emulation  *  _emulation;
    QList<TerminalDisplay *> _views;
    bool           _monitorActivity;
    bool           _monitorSilence;
    bool           _notifiedActivity;
    bool           _masterMode;
    bool           _autoClose;
    bool           _wantedClose;
    QTimer    *    _monitorTimer;
    int            _silenceSeconds;
    QString        _nameTitle;
    QString        _displayTitle;
    QString        _userTitle;
    QString        _localTabTitleFormat;
    QString        _remoteTabTitleFormat;
    QString        _iconName;
    QString        _iconText;
    bool           _isTitleChanged;
    bool           _addToUtmp;
    bool           _flowControl;
    bool           _fullScripting;
    QString        _program;
    QStringList    _arguments;
    QStringList    _environment;
    int            _sessionId;
    QString        _initialWorkingDir;
    QColor         _modifiedBackground;
    QString        _profileKey;
    bool _hasDarkBackground;
    static int lastSessionId;
    int ptySlaveFd;
#ifdef QTERMWIDGET_HAVE_QSERIALPORT
    SerialChannel* _serialChannel { nullptr };
    bool _serialActive { false };
#endif
#ifdef QTERMWIDGET_HAVE_LIBSSH
    SSHChannel* _sshChannel { nullptr };
    bool _sshActive { false };
#endif
};

class SessionGroup : public QObject {
    Q_OBJECT

public:
    SessionGroup();
    ~SessionGroup() override;
    void addSession( Session * session );
    void removeSession( Session * session );
    QList<Session *> sessions() const;
    void setMasterStatus( Session * session , bool master );
    bool masterStatus( Session * session ) const;
    enum MasterMode { CopyInputToAll = 1 };
    void setMasterMode( int mode );
    int masterMode() const;

private:
    void connectPair(Session * master , Session * other) const;
    void disconnectPair(Session * master , Session * other) const;
    void connectAll(bool connect);
    QList<Session *> masters() const;
    QHash<Session *,bool> _sessions;
    int _masterMode;
};

}

#endif
