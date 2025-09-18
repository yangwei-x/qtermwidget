/*
    Copyright (C) 2007 by Robert Knight <robertknight@gmail.com>

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

#ifndef SHELLCOMMAND_H
#define SHELLCOMMAND_H

// Qt
#include <QStringList>

namespace Konsole {

class ShellCommand {
public:
    ShellCommand(const QString & fullCommand);
    ShellCommand(const QString & command , const QStringList & arguments);

    QString command() const;
    QStringList arguments() const;
    QString fullCommand() const;
    bool isRootCommand() const;
    bool isAvailable() const;
    static QString expand(const QString & text);
    static QStringList expand(const QStringList & items);

private:
    QStringList _arguments;
};

}

#endif // SHELLCOMMAND_H
