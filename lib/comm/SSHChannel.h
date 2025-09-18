/*
 * SSHChannel - SSH connection wrapper using libssh
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QSocketNotifier>

struct ssh_session_struct;
struct ssh_channel_struct;
typedef struct ssh_session_struct* ssh_session;
typedef struct ssh_channel_struct* ssh_channel;

namespace Konsole {

class SSHChannel : public QObject {
    Q_OBJECT
public:
    explicit SSHChannel(QObject* parent=nullptr);
    ~SSHChannel() override;

    // Connect and start an interactive shell with a PTY
    bool connectAndStart(const QString& host,
                         int port,
                         const QString& user,
                         const QString& password,
                         const QString& termName,
                         int cols,
                         int rows);

    bool isRunning() const;
    void resize(int rows, int cols);

public slots:
    void sendData(const QByteArray& data);

signals:
    void receivedData(const QByteArray& data);
    void error(const QString& message);
    void closed();

private slots:
    void onSocketReadable();

private:
    bool authenticate(const QString& user, const QString& pass);
    void teardown();

    ssh_session session_ {nullptr};
    ssh_channel channel_ {nullptr};
    int socketFd_ {-1};
    QSocketNotifier* notifier_ {nullptr};
    bool running_ {false};
};

} // namespace Konsole
