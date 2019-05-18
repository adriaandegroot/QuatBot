/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#include "coffee.h"

#include <QMap>

namespace QuatBot
{

struct CoffeeStats
{
    int m_coffee;
    int m_cookie;
    int m_cookieEated;
};

class Coffee::Private
{
public:
    Private()
    {
    }

    void stats(Bot* bot)
    {
        for (const auto& u : m_stats)
        {
            bot->message(QString("%1 has had %2 cups of coffee and has eaten %3 cookies.").arg(u.first).arg(u.second.m_coffee).arg(u.second.m_cookie));
        }
    }

    int cookies() const { return m_cookiejar; }

    int coffee(const QString& user)
    {
        if (m_stats.contains(user))
        {
            CoffeeStats& c = m_stats[user];
            c.m_coffee++;
            return c.m_coffee;
        }
        else
        {
            m_stats.insert(user, CoffeeStats{1,0,0});
            return 1;
        }
    }


private:
    int m_cookiejar = 12;  // a dozen cookies by default
    QMap<QString, CoffeeStats> m_stats;
};


Coffee::Coffee(Bot* parent):
    Watcher(parent),
    d(new Private)
{
}

Coffee::~Coffee()
{
    delete d;
}

const QString& Coffee::moduleName() const
{
    static const QString name(QStringLiteral("coffee"));
    return name;
}

const QStringList& Coffee::moduleCommands() const
{
    static const QStringList commands{"coffee", "cookie", "status"};
    return commands;
}

void Coffee::handleMessage(const QMatrixClient::RoomMessageEvent*)
{
}

void Coffee::handleCommand(const CommandArgs& cmd)
{
    if (cmd.command == QStringLiteral("status"))
    {
        message(QString("(coffee) There are %1 cookies in the jar.").arg(d->cookies()));
        d->stats(m_bot);
    }
    else if (cmd.command == QStringLiteral("cookie"))
    {
        CommandArgs sub(cmd);
        sub.pop();
        handleSubCommand(sub);
    }
    else if (cmd.command == QStringLiteral("coffee"))
    {
        if (d->coffee(cmd.user) <= 1)
        {
            message(QStringList{cmd.user, "is now a coffee drinker."});
        }
        else
        {
            message(QStringList{cmd.user, "has a nice cup of coffee."});
        }
    }
}
