/*
 * This file is a part of QTerminal - http://gitorious.org/qterminal
 *
 * This file was un-linked from KDE and modified
 * by Maxim Bourmistrov <maxim@unixconn.com>
 *
 */

/*
    This file is part of the KDE libraries

    Copyright (C) 2007 Oswald Buddenhagen <ossi@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KPROCESS_H
#define KPROCESS_H

//#include <kdecore_export.h>

#include <QProcess>

#include <memory>

class KProcessPrivate;

class KProcess : public QProcess
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(KProcess)

public:
    enum OutputChannelMode {
        SeparateChannels = QProcess::SeparateChannels,
        MergedChannels = QProcess::MergedChannels,
        ForwardedChannels = QProcess::ForwardedChannels,
        OnlyStdoutChannel = QProcess::ForwardedErrorChannel,
        OnlyStderrChannel = QProcess::ForwardedOutputChannel
    };

    explicit KProcess(QObject *parent = nullptr);
    ~KProcess() override;

    void setOutputChannelMode(OutputChannelMode mode);
    OutputChannelMode outputChannelMode() const;

    void setNextOpenMode(QIODevice::OpenMode mode);
    void setEnv(const QString &name, const QString &value, bool overwrite = true);
    void unsetEnv(const QString &name);
    void clearEnvironment();

    void setProgram(const QString &exe, const QStringList &args = QStringList());
    void setProgram(const QStringList &argv);

    KProcess &operator<<(const QString& arg);
    KProcess &operator<<(const QStringList& args);
    void clearProgram();

    QStringList program() const;
    void start();
    int execute(int msecs = -1);
    static int execute(const QString &exe, const QStringList &args = QStringList(), int msecs = -1);
    static int execute(const QStringList &argv, int msecs = -1);
    int startDetached();
    static int startDetached(const QString &exe, const QStringList &args = QStringList());
    static int startDetached(const QStringList &argv);

protected:
    KProcess(KProcessPrivate *d, QObject *parent);
    std::unique_ptr<KProcessPrivate> const d_ptr;

private:
    using QProcess::setProcessChannelMode;
    using QProcess::processChannelMode;
};

class KProcessPrivate {

    Q_DECLARE_PUBLIC(KProcess)

protected:
    KProcessPrivate(KProcess *qq) :
        openMode(QIODevice::ReadWrite),
        q_ptr(qq)
    {
    }

    QString prog;
    QStringList args;
    QIODevice::OpenMode openMode;

    KProcess *q_ptr = nullptr;
};

#endif
