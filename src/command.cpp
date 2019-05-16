/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "command.h"

#include "quatbot.h"

#include <room.h>

#include <QProcess>

namespace QuatBot
{
static void fortune(QMatrixClient::Room* room)
{
    QProcess f;
    f.start("/usr/bin/fortune", {"freebsd-tips"});
    f.waitForFinished();
    if (f.exitCode()==0)
    {
        QString text = QString::fromLatin1(f.readAllStandardOutput());
        message(room, text);
    }
    else
        message(room, "No fortune for you!");
}

BasicCommands::BasicCommands(Bot* parent) :
    Watcher(parent)
{
}

BasicCommands::~BasicCommands()
{
}

QString BasicCommands::moduleName() const
{
    return QString();
}

void BasicCommands::handleCommand(QMatrixClient::Room* room, const CommandArgs& l)
{
    if (l.command == QStringLiteral("echo"))
    {
        message(room, l.args);
    }
    else if (l.command == QStringLiteral("fortune"))
    {
        fortune(room);
    }
    else if (l.command == QStringLiteral("op"))
    {
        if (m_bot->checkOps(l, room))
        {
            if (l.args.count() == 1) 
            {
                if (m_bot->setOps(l.args.first(), true))
                {
                    message(room, QString("%1 is now an operator").arg(l.args.first()));
                }
                else
                {
                    message(room, QString("%1 failed.").arg(displayCommand("op")));
                }
            }
            else
            {
                message(room, QString("Usage: %1 <userid>").arg(displayCommand("op")));
            }
        }
    }
    else if (l.command == QStringLiteral("deop"))
    {
        if (m_bot->checkOps(l, room))
        {
            if (l.args.count() == 1) 
            {
                if (m_bot->setOps(l.args.first(), false))
                {
                    message(room, QString("%1 is no longer an operator").arg(l.args.first()));
                }
                else
                {
                    message(room, QString("%1 failed.").arg(displayCommand("deop")));
                }
            }
            else
            {
                message(room, QString("Usage: %1 <userid>").arg(displayCommand("deop")));
            }
        }
    }
    else
        message(room, QString("I don't understand '%1'.").arg(l.command));
}

void QuatBot::BasicCommands::handleMessage(QMatrixClient::Room* room, const QMatrixClient::RoomMessageEvent* event)
{
}

}
