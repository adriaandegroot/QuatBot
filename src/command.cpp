/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "command.h"

#include <room.h>

#include <QProcess>

void echo(QMatrixClient::Room* room, const QStringList& l)
{
    room->postPlainText(l.join(' '));
}

void fortune(QMatrixClient::Room* room)
{
    QProcess f;
    f.start("/usr/bin/fortune", {"freebsd-tips"});
    f.waitForFinished();
    if (f.exitCode()==0)
    {
        QString text = QString::fromLatin1(f.readAllStandardOutput());
        echo(room, QStringList{text});
    }
    else
        echo(room, QStringList{"No fortune for you!"});
}

CommandHandler::CommandHandler(QChar prefixChar) :
    m_prefix(prefixChar)
{
}

QString munge(const QString& s)
{
    return s.trimmed();
}

CommandArgs CommandHandler::extractCommand(QString s)
{
    if (!isCommand(s))
        return CommandArgs{};
    
    QStringList parts = s.remove(0,1).split(' ');
    QStringList r;
    for (int i=1; i<parts.count(); ++i)
        r << munge(parts[i]);
                   
    return {munge(parts[0]), r};
}

void CommandHandler::handleCommand(QMatrixClient::Room* room, const CommandArgs& l)
{
    if (l.command == QStringLiteral("echo"))
        echo(room, l.args);
    else if (l.command == QStringLiteral("fortune"))
        fortune(room);
    else
        echo(room, QStringList{QString("I don't understand '%1'.").arg(l.command)});
}
