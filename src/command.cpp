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
#include <QTimer>

namespace QuatBot
{
static QString runProcess(const QString& executable, const QStringList& args, const QString& failure)
{
    QProcess f;
    f.start(executable, args);
    f.waitForFinished();
    if (f.exitCode()==0)
    {
        return QString::fromLatin1(f.readAllStandardOutput());
    }
    else
        return failure;
}

static QString fortune()
{
    return runProcess(QStringLiteral("/usr/bin/fortune"), {"freebsd-tips"}, QStringLiteral("No fortune for you!"));
}

#ifdef ENABLE_COWSAY
// Copy because we modify the string
static QString cowsay(QString message)
{
    static int instance = 0;
    static const char* const specials[16]={
        nullptr, nullptr, "-d", nullptr, 
        nullptr, nullptr, "-s", "-p", 
        nullptr, "-y", nullptr, "-g",
        "-w", "-t", "-b", nullptr};
    
    QStringList arg;
    // Go around and around mod 16
    if (specials[instance])
        arg << specials[instance];
    instance = (instance+1) & 0xf;
    // The message
    message.truncate(40);
    arg << message;
    
    return runProcess(QStringLiteral("/usr/local/bin/cowsay"), arg, "Moo!");
}
#endif

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
#ifdef ENABLE_COWSAY
    else if (l.command == QStringLiteral("cowsay"))
    {
        message(cowsay(l.args.join(' ')));
    }
#endif
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
            QTimer::singleShot(1000, m_bot, &QObject::deleteLater);
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
