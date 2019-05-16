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
static QString fortune()
{
    QProcess f;
    f.start("/usr/bin/fortune", {"freebsd-tips"});
    f.waitForFinished();
    if (f.exitCode()==0)
    {
        return QString::fromLatin1(f.readAllStandardOutput());
    }
    else
        return QStringLiteral("No fortune for you!");
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

void BasicCommands::handleCommand(const CommandArgs& l)
{
    if (l.command == QStringLiteral("echo"))
    {
        message(l.args);
    }
    else if (l.command == QStringLiteral("fortune"))
    {
        message(fortune());
    }
    else if (l.command == QStringLiteral("op"))
    {
        if (m_bot->checkOps(l))
        {
            if (l.args.count() > 0) 
            {
                for (const auto& u : l.args)
                {
                    QString user = m_bot->userLookup(u);
                    if (user.isEmpty())
                    {
                        continue;
                    }
                    if (m_bot->setOps(user, true))
                    {
                        message(QString("%1 is now an operator").arg(user));
                    }
                    else
                    {
                        message(QString("%1 failed.").arg(displayCommand("op")));
                    }
                }
            }
            else
            {
                message(QString("Usage: %1 <userid>").arg(displayCommand("op")));
            }
        }
    }
    else if (l.command == QStringLiteral("ops"))
    {
        message(QStringList{QString("There are %1 operators.").arg(m_bot->m_operators.count())} << m_bot->m_operators.toList());
    }
    else if (l.command == QStringLiteral("deop"))
    {
        if (m_bot->checkOps(l))
        {
            if (l.args.count() > 0) 
            {
                for (const auto& u : l.args)
                {
                    QString user = m_bot->userLookup(u);
                    if (user.isEmpty())
                    {
                        continue;
                    }
                    if (m_bot->setOps(user, false))
                    {
                        message(QString("%1 is no longer an operator").arg(user));
                    }
                    else
                    {
                        message(QString("%1 failed.").arg(displayCommand("deop")));
                    }
                }
            }
            else
            {
                message(QString("Usage: %1 <userid>").arg(displayCommand("deop")));
            }
        }
    }
    else if (l.command == QStringLiteral("quit"))
    {
        if (m_bot->checkOps(l))
        {
            m_bot->deleteLater();
            message(QString("Goodbye (bot operation terminated)!"));
        }
    }
    else
        message(QString("I don't understand '%1'.").arg(l.command));
}

void QuatBot::BasicCommands::handleMessage(const QMatrixClient::RoomMessageEvent* event)
{
}

}
