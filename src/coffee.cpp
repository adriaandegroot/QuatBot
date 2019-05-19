/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#include "coffee.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMap>
#include <QStandardPaths>
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
    /// @brief Remember to save state on return from a function
    struct AutoSave
    {
        AutoSave(Private* p) : m_p(p) {}
        ~AutoSave() { m_p->save(); }
        Private* m_p;
    } ;
    
public:
    Private()
    {
        QObject::connect(&m_refill, &QTimer::timeout, [this](){ this->addCookie(); });
        m_refill.start(3579100);  // every hour, -ish
        load();
    }

    void stats(Bot* bot)
    {
        for (const auto& u : m_stats)
        {
            QStringList info{QString("%1 has had %2 cups of coffee").arg(u.m_user).arg(u.m_coffee)};
            if (u.m_cookie > 0)
            {
                info << QString("and has %1 cookies").arg(u.m_cookie);
            }
            if (u.m_cookieEated > 0)
            {
                info << QString("and has eaten %1 cookies").arg(u.m_cookieEated);
            }
            info << "so far.";
            bot->message(info);
        }
    }

    int cookies() const { return m_cookiejar; }
    
    /// @brief Give @p user a coffee; returns their coffee count
    int coffee(const QString& user)
    {
        AutoSave a(this);
        auto& c = find(user);
        return ++c.m_coffee;
    }

    /// @brief Give @p user a cookie from the jar; returns true on success
    bool giveCookie(const QString& user)
    {
        if (m_cookiejar > 0)
        {
            AutoSave a(this);
            auto& c = find(user);
            m_cookiejar--;
            c.m_cookie++;
            return true;
        }
        return false;
    }
    
    /// @brief Give @p other one of @p user 's cookies; returns true on success
    bool transferCookie(const QString& user, const QString& other)
    {
        auto& u = find(user);
        auto& o = find(other);
        
        if (u.m_user == o.m_user)
        {
            return true;  // zero-sum
        }
        else
        {
            if (u.m_cookie > 0)
            {
                AutoSave a(this);
                u.m_cookie--;
                o.m_cookie++;
                return true;
            }
            return false;
        }
    }
    
    /// @brief @p user eats a cookie; returns true on success
    bool eatCookie(const QString& user)
    {
        auto& u = find(user);
        if (u.m_cookie > 0)
        {
            AutoSave a(this);
            u.m_cookie--;
            u.m_cookieEated++;
            return true;
        }
        return false;
    }

    const QPair<QString,QString> dataLocation() const
    {
        return qMakePair(QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation), QStringLiteral("cookiejar"));
    }
    
    static constexpr const qint32 MAGIC = 0xcafe;
    
    void save() const
    {
        const auto [dataDirName, saveFileName] = dataLocation();
        
        if (dataDirName.isEmpty())
        {
            static bool warned = false;
            if (!warned)
            {
                qWarning() << "Could no find an AppData location.";
                warned = true;
            }
            return;
        }
        
        QDir dataDir(dataDirName);
        if (!dataDir.exists())
        {
            dataDir.mkdir(dataDirName);
        }
        if (!dataDir.exists())
        {
            static bool warned = false;
            if (!warned)
            {
                qWarning() << "Could not create AppData location" << dataDirName;
                warned = true;
            }
            return;
        }
        
        if (dataDir.exists(saveFileName))
        {
            // The cookie-jar isn't *SO* important that I'm going to do
            // a lot of error-handling here.
            dataDir.rename(saveFileName, saveFileName + QStringLiteral(".old"));
        }
        
        QFile saveFile(dataDir.absolutePath() + "/" + saveFileName);
        if (!saveFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "Could not create save-file" << saveFile.fileName();
        }
        else
        {
            saveV1(QDataStream(&saveFile));
            saveFile.close();
        }
    }
    
    void load()
    {
        const auto [dataDirName, saveFileName] = dataLocation();
        QFile saveFile(dataDirName + "/" + saveFileName);
        if (saveFile.exists() && saveFile.open(QIODevice::ReadOnly))
        {
            QDataStream d(&saveFile);
            qint32 magic;
            QDateTime when;
            d >> magic;
            if (magic != MAGIC)
            {
                qWarning() << "Save file" << saveFile.fileName() << "corrupt.";
                return;
            }
            d >> magic >> when;
            qDebug() << "Loading save file v" << magic <<"from" << when.toString();
            if (magic == 1)
            {
                loadV1(d);
            }
            else
            {
                qWarning() << "Save file has unknown version" << magic;
            }
        }
    }

private:
    CoffeeStats& find(const QString& user)
    {
        if (!m_stats.contains(user))
        {
            m_stats.insert(user, CoffeeStats(user));
        }
        return m_stats[user];
    }
    
    /// @brief Replenish the cookiejar
    void addCookie()
    {
        if (m_cookiejar < 12)
        {
            m_cookiejar++;
        }
    }

    void saveV1(QDataStream d) const
    {
        d << qint32(MAGIC);   // Coffee!
        d << qint32(1);       // Version 1
        d << QDateTime::currentDateTime();  // When?
        
        d << qint32(m_stats.count());   // Number of elements
        for (const auto& u : m_stats)
        {
            d << u.m_user << qint32(u.m_coffee) << qint32(u.m_cookie) << qint32(u.m_cookieEated);
        }
        
        d << qint32(0) << qint32(MAGIC);
        d << QString("Koffiepot");
    }
    
    void loadV1(QDataStream& d)
    {
        qint32 count;
        
        QString user;
        qint32 coffee, cookie, eated;
        
        d >> count;
        if ((count < 1) || (count > 1000))
        {
            qWarning() << "Unreasonable coffee-count" << count;
            return;
        }
        
        while(count>0)
        {
            d >> user >> coffee >> cookie >> eated;
            auto& u = find(user);
            u.m_coffee = coffee;
            u.m_cookie = cookie;
            u.m_cookieEated = eated;
            
            count--;
        }
        
        // Check trailer?
        d >> count;
        if (count != 0)
        {
            qWarning() << "Trailer 1 corrupt.";
            return;
        }
        d >> count;
        if (count != MAGIC)
        {
            qWarning() << "Trailer 2 corrupt.";
            return;
        }
        d >> user;
        if (user != "Koffiepot")
        {
            qWarning() << "Trailer 3 corrupt.";
        }
    }
    
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
    static const QStringList commands{"coffee", "cookie", "lart",
        "stats",  // long status
        "status"  // brief status
    };
    return commands;
}

void Coffee::handleMessage(const QMatrixClient::RoomMessageEvent*)
{
}

void Coffee::handleCookieCommand(const CommandArgs& cmd)
{
    if ((cmd.command == QStringLiteral("eat")) || cmd.command.isEmpty())
    {
        // Empty is when you just go ~cookie
        if (d->eatCookie(cmd.user))
        {
            message(QString("**%1** nom nom nom").arg(cmd.user));
        }
        else
        {
            message("You haz no cookiez :(");
        }
    }
    else if (cmd.command == QStringLiteral("give"))
    {
        const auto& realUsers = m_bot->userIds();
        
        for (const auto& other : cmd.args)
        {
            if (!realUsers.contains(other))
            {
                message(QString("%1's not here, Dave.").arg(other));
                continue;
            }
            if (other == cmd.user)
            {
                message("It's a circular economy.");
            }
            else if (d->transferCookie(cmd.user, other))
            {
                message(QString("**%1** gives %2 a cookie.").arg(cmd.user, other));
            }
            else
            {
                if (d->giveCookie(other))
                {
                    message(QString("%2 gets a cookie from the jar.").arg(other));
                }
                else
                {
                    message(QString("Hey! Who took all the cookies from the jar?"));
                }
            }
        }
    }
    else
    {
        message(QString("Cookies don't work that way."));
    }
}


void Coffee::handleCommand(const CommandArgs& cmd)
{
    if ((cmd.command == QStringLiteral("status")) || (cmd.command == QStringLiteral("stats")))
    {
        message(QString("(coffee) There are %1 cookies in the jar.").arg(d->cookies()));
        if (cmd.command == QStringLiteral("stats"))
        {
            d->stats(m_bot);
        }
    }
    else if (cmd.command == QStringLiteral("cookie"))
    {
        CommandArgs sub(cmd);
        sub.pop();
        handleCookieCommand(sub);
    }
    else if ((cmd.command == QStringLiteral("coffee")) || (cmd.command.isEmpty()))
    {
        // THe empty case is when you enter just ~coffee. That's interpreted
        // as a module name, and the command goes away.
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
    else
    {
        message(Usage{});
    }
}

}
