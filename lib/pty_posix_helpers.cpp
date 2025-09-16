#include "pty_posix_helpers.h"
#include <sys/ioctl.h>
#include <cstring>
#include <unistd.h>

bool pty_tcGetAttr(int fd, struct ::termios * ttmode)
{
    if (fd < 0 || !ttmode) return false;
    return tcgetattr(fd, ttmode) == 0;
}

bool pty_tcSetAttr(int fd, struct ::termios * ttmode)
{
    if (fd < 0 || !ttmode) return false;
    return tcsetattr(fd, TCSANOW, ttmode) == 0;
}

bool pty_setWinSize(int fd, int lines, int columns)
{
    if (fd < 0) return false;
    struct winsize winSize;
    memset(&winSize, 0, sizeof(winSize));
    winSize.ws_row = (unsigned short)lines;
    winSize.ws_col = (unsigned short)columns;
    return ioctl(fd, TIOCSWINSZ, &winSize) != -1;
}

bool pty_setEcho(int fd, bool echo)
{
    if (fd < 0) return false;
    struct ::termios ttmode;
    if (!pty_tcGetAttr(fd, &ttmode)) return false;
    if (!echo)
        ttmode.c_lflag &= ~ECHO;
    else
        ttmode.c_lflag |= ECHO;
    return pty_tcSetAttr(fd, &ttmode);
}
