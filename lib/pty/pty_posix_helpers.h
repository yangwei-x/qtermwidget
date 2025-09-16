#ifndef PTY_POSIX_HELPERS_H
#define PTY_POSIX_HELPERS_H

#include <termios.h>
#include <QByteArray>
#include <limits.h>

bool pty_tcGetAttr(int fd, struct ::termios * ttmode);
bool pty_tcSetAttr(int fd, struct ::termios * ttmode);
bool pty_setWinSize(int fd, int lines, int columns);
bool pty_setEcho(int fd, bool echo);

// Master/slave open/close helpers
bool pty_open_master(int &masterFd, int &slaveFd, QByteArray &ttyName);
bool pty_open_master_from_fd(int fd, int &masterFd, int &slaveFd, QByteArray &ttyName);
bool pty_open_slave(int masterFd, int &slaveFd, const QByteArray &ttyName);
void pty_close_slave(int &slaveFd);
void pty_close_master(int &masterFd, int &slaveFd, QByteArray &ttyName, bool &ownMaster);

// controlling terminal, login/logout
void pty_setCTty(int slaveFd, const QByteArray &ttyName);
void pty_login(int masterFd, const char * user, const char * remotehost, const QByteArray &ttyName);
void pty_logout(int masterFd, const QByteArray &ttyName);

#endif // PTY_POSIX_HELPERS_H
