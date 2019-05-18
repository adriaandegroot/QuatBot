/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#include "coffee.h"

#include <QMap>
#include <QTimer>

namespace QuatBot
{

class CoffeeStats
{
public:
    CoffeeStats()
    {
    }
    CoffeeStats(const QString& u) :
        m_user(u)
    {
    }
    
    QString m_user;
    int m_coffee = 0;
    int m_cookie = 0;
    int m_cookieEated = 0;
};

class Coffee::Private
{
public:
    Private()
    {
        QObject::connect(&m_refill, &QTimer::timeout, [this](){ this->addCookie(); });
        m_refill.start(3579100);  // every hour, -ish
    }

    void stats(Bot* bot)
    {
        for (const auto& u : m_stats)
        {
            bot->message(QString("%1 has had %2 cups of coffee and has eaten %3 cookies.").arg(u.m_user).arg(u.m_coffee).arg(u.m_cookie));
        }
    }

    int cookies() const { return m_cookiejar; }
    
    CoffeeStats& find(const QString& user)
    {
        if (!m_stats.contains(user))
        {
            m_stats.insert(user, CoffeeStats(user));
        }
        return m_stats[user];
    }
    
    int coffee(const QString& user)
    {
        auto& c = find(user);
        return ++c.m_coffee;
    }

    bool giveCookie(CoffeeStats& user)
    {
        if (m_cookiejar > 0)
        {
            m_cookiejar--;
            user.m_cookie++;
            return true;
        }
        return false;
    }
    
    void addCookie()
    {
        if (m_cookiejar < 12)
        {
            m_cookiejar++;
        }
    }
    
private:
    int m_cookiejar = 12;  // a dozen cookies by default
    QMap<QString, CoffeeStats> m_stats;
    QTimer m_refill;
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
    static const QStringList commands{"coffee", "cookie", "status", "lart"};
    return commands;
}

void Coffee::handleMessage(const QMatrixClient::RoomMessageEvent*)
{
}

void Coffee::handleSubCommand(const CommandArgs& cmd)
{
    if (cmd.command == QStringLiteral("eat"))
    {
        auto& c = d->find(cmd.user);
        if (c.m_cookie > 0)
        {
            c.m_cookie--;
            c.m_cookieEated++;
            message(QString("**%1** nom nom nom").arg(cmd.user));
        }
        else
        {
            message("You haz no cookiez :(");
        }
    }
    if (cmd.command == QStringLiteral("give"))
    {
        const auto& realUsers = m_bot->userIds();
        
        auto& user = d->find(cmd.user);
        for (const auto& other : cmd.args)
        {
            if (!realUsers.contains(other))
            {
                message(QString("%1's not here, Dave.").arg(other));
                continue;
            }
            auto& otheruser = d->find(other);
            if (otheruser.m_user == user.m_user)
            {
                message("It's a circular economy.");
            }
            else if (user.m_cookie > 0)
            {
                user.m_cookie--;
                otheruser.m_cookie++;
                message(QString("**%1** gives %2 a cookie.").arg(user.m_user, otheruser.m_user));
            }
            else
            {
                if (d->giveCookie(otheruser))
                {
                    message(QString("%2 gets a cookie from the jar.").arg(otheruser.m_user));
                }
                else
                {
                    message(QString("Hey! Who took all the cookies from the jar?"));
                }
            }
        }
    }
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
    else if (cmd.command == QStringLiteral("lart"))
    {
        message(QString("%1 is eaten by a large trout.").arg(cmd.user));
    }
}

}
