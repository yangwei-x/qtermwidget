#include "SSHChannel.h"

#ifdef QTERMWIDGET_HAVE_LIBSSH
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#endif

#include <QDebug>
#include <QScopeGuard>

namespace Konsole {

SSHChannel::SSHChannel(QObject* parent)
    : QObject(parent)
{
}

SSHChannel::~SSHChannel()
{
    teardown();
}

bool SSHChannel::connectAndStart(const QString& host,
                                 int port,
                                 const QString& user,
                                 const QString& password,
                                 const QString& termName,
                                 int cols,
                                 int rows)
{
#ifndef QTERMWIDGET_HAVE_LIBSSH
    Q_UNUSED(host)
    Q_UNUSED(port)
    Q_UNUSED(user)
    Q_UNUSED(password)
    Q_UNUSED(termName)
    Q_UNUSED(cols)
    Q_UNUSED(rows)
    emit error(QStringLiteral("libssh not available"));
    return false;
#else
    teardown();

    session_ = ssh_new();
    if(!session_) {
        emit error(QStringLiteral("ssh_new failed"));
        return false;
    }

    ssh_options_set(session_, SSH_OPTIONS_HOST, host.toUtf8().constData());
    ssh_options_set(session_, SSH_OPTIONS_PORT, &port);
    if(!user.isEmpty()) {
        ssh_options_set(session_, SSH_OPTIONS_USER, user.toUtf8().constData());
    }

    if(ssh_connect(session_) != SSH_OK) {
        emit error(QString::fromLatin1("ssh_connect: %1").arg(QString::fromUtf8(ssh_get_error(session_))));
        teardown();
        return false;
    }

    if(!authenticate(user, password)) {
        emit error(QStringLiteral("SSH authentication failed"));
        teardown();
        return false;
    }

    channel_ = ssh_channel_new(session_);
    if(!channel_) {
        emit error(QStringLiteral("ssh_channel_new failed"));
        teardown();
        return false;
    }

    if(ssh_channel_open_session(channel_) != SSH_OK) {
        emit error(QStringLiteral("ssh_channel_open_session failed"));
        teardown();
        return false;
    }

    // Request PTY and shell
    if(ssh_channel_request_pty_size(channel_, termName.toUtf8().constData(), cols, rows) != SSH_OK) {
        emit error(QStringLiteral("ssh_channel_request_pty_size failed"));
        teardown();
        return false;
    }

    if(ssh_channel_request_shell(channel_) != SSH_OK) {
        emit error(QStringLiteral("ssh_channel_request_shell failed"));
        teardown();
        return false;
    }

    socketFd_ = ssh_get_fd(session_);
    if(socketFd_ < 0) {
        emit error(QStringLiteral("ssh_get_fd failed"));
        teardown();
        return false;
    }

    notifier_ = new QSocketNotifier(socketFd_, QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated, this, &SSHChannel::onSocketReadable);
    running_ = true;
    return true;
#endif
}

bool SSHChannel::isRunning() const
{
    return running_;
}

void SSHChannel::resize(int rows, int cols)
{
#ifdef QTERMWIDGET_HAVE_LIBSSH
    if(channel_) {
        ssh_channel_change_pty_size(channel_, cols, rows);
    }
#else
    Q_UNUSED(rows)
    Q_UNUSED(cols)
#endif
}

void SSHChannel::sendData(const QByteArray& data)
{
#ifdef QTERMWIDGET_HAVE_LIBSSH
    if(channel_ && !data.isEmpty()) {
        ssh_channel_write(channel_, data.constData(), data.size());
    }
#else
    Q_UNUSED(data)
#endif
}

void SSHChannel::onSocketReadable()
{
#ifdef QTERMWIDGET_HAVE_LIBSSH
    if(!channel_) return;
    char buf[8192];
    int n = ssh_channel_read_nonblocking(channel_, buf, sizeof(buf), 0);
    if(n > 0) {
        emit receivedData(QByteArray(buf, n));
        return;
    }
    if(n == SSH_EOF || ssh_channel_is_eof(channel_) || ssh_channel_is_closed(channel_)) {
        running_ = false;
        emit closed();
        teardown();
    }
#endif
}

bool SSHChannel::authenticate(const QString& user, const QString& pass)
{
#ifdef QTERMWIDGET_HAVE_LIBSSH
    Q_UNUSED(user)
    if(!pass.isEmpty()) {
        int rc = ssh_userauth_password(session_, nullptr, pass.toUtf8().constData());
        if(rc == SSH_AUTH_SUCCESS) return true;
    }
    // Fallback: try none/publickey/agent
    int rc = ssh_userauth_publickey_auto(session_, nullptr, nullptr);
    return rc == SSH_AUTH_SUCCESS;
#else
    Q_UNUSED(user)
    Q_UNUSED(pass)
    return false;
#endif
}

void SSHChannel::teardown()
{
#ifdef QTERMWIDGET_HAVE_LIBSSH
    if(notifier_) { notifier_->deleteLater(); notifier_ = nullptr; }
    if(channel_) { ssh_channel_close(channel_); ssh_channel_free(channel_); channel_ = nullptr; }
    if(session_) { ssh_disconnect(session_); ssh_free(session_); session_ = nullptr; }
    socketFd_ = -1;
    running_ = false;
#endif
}

} // namespace Konsole
