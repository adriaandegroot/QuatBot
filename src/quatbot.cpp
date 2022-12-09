/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#include "quatbot.h"

#include <QCoreApplication>
#include <QDebug>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

#include <connection.h>
#include <networkaccessmanager.h>
#include <room.h>
#include <user.h>

#include <csapi/joining.h>
#include <events/roommessageevent.h>

#ifdef ENABLE_COFFEE
#include "coffee.h"
#endif
#include "command.h"
#include "logger.h"
#include "meeting.h"

namespace QuatBot
{
QStringList splitUserName(const QString& s)
{
    QStringList l;
    for (QString part : s.split(' ', Qt::SkipEmptyParts))
    {
        QString shortPart = part.trimmed();
        if (!shortPart.isEmpty())
        {
            l << shortPart;
        }
    }
    return l;
}

/** @brief Pair of Matrix-Id and split-up displayname.
 *
 * This is used in matching names to Matrix-Ids: a command can name
 * a user, either by Matrix-Id or by nickname. Since nicknames may
 * be more than one word, we need to be able to match, say
 * `@adridg:matirx.org` with the nickname *adridg the bot*.
 *
 * To do that, we'll sort nicknames in the room by length,
 * longest first, so that *adridg the bot* and *adridg* are
 * treated separately (and using the long nickname won't match
 * with the short one first).
 */
struct DisplayName
{
    QString id;
    QStringList displayName;
};

/** @brief Sort displaynames, longest first, then alphabetical.
 */
bool operator<(const DisplayName& a, const DisplayName& b)
{
    if (a.displayName.count() > b.displayName.count())
    {
        return true;
    }
    if (a.displayName.count() < b.displayName.count())
    {
        return false;
    }

    // Equal length
    for (int i = 0; i < a.displayName.count(); ++i)
    {
        if (a.displayName[i] < b.displayName[i])
        {
            return true;
        }
        if (a.displayName[i] > b.displayName[i])
        {
            return false;
        }
    }
    return false;
}

QStringList Bot::userLookup(const QStringList& users)
{
    QStringList ids;

    if (!m_room)
        return ids;

    QList<DisplayName> idToDisplayName;
    idToDisplayName.reserve(m_room->users().count());
    for (const auto& u : m_room->users())
    {
        idToDisplayName.append({ u->id(), splitUserName(u->displayname(m_room)) });
    }
    std::sort(idToDisplayName.begin(), idToDisplayName.end());

    int i = 0;
    while (i < users.count())
    {
        QString accumulator = users[i];
        if (accumulator.isEmpty())
        {
            // Just skip this one
        }
        else if (accumulator.startsWith('@') && accumulator.contains(':'))
        {
            // Looks like an Id already
            ids << accumulator;
        }
        else
        {
            bool found = false;
            for (const auto& [matrixId, userParts] : idToDisplayName)
            {
                found = userParts.count() > 0;  // initialize to false if the for-loop would be skipped
                for (int j = 0; (j < userParts.count()) && ((i + j) < users.count()); ++j)
                {
                    if (userParts[j] != users[i + j])
                    {
                        found = false;
                        break;
                    }
                }
                if (found)
                {
                    ids << matrixId;
                    i += userParts.count() - 1;
                    break;
                }
            }
            // if there is no id found, leave it for the caller
            if (!found)
            {
                ids << accumulator;
            }
        }
        i++;
    }
    return ids;
}

QString Bot::userLookup(const QString& userName)
{
    if (!m_room)
        return QString();

    QString n = userName.trimmed();
    if (n.isEmpty())
        return QString();

    if (n.startsWith('@') && n.contains(':'))
        return n;

    for (const auto& u : m_room->users())
    {
        if (n == u->displayname(m_room))
            return u->id();
    }

    return QString();
}

QStringList Bot::userIds()
{
    QStringList l;
    if (!m_room)
        return l;

    m_room->setDisplayed(true);
    for (const auto& u : m_room->users())
    {
        l << u->id();
    }
    return l;
}

QString Bot::botUser() const
{
    return m_conn.userId();
}


static int instance_count = 0;

static void bailOut(int timeout = 0)
{
    QTimer::singleShot(timeout, qApp, &QCoreApplication::quit);
}


Bot::Bot(QMatrixClient::Connection& conn, const QString& roomName, const QStringList& ops)
    : QObject()
    , m_conn(conn)
    , m_roomName(roomName)
{
    instance_count++;
    if (conn.homeserver().isEmpty() || !conn.homeserver().isValid())
    {
        qWarning() << "Connection is invalid.";
        bailOut();
        return;
    }

    auto* joinRoom = conn.joinRoom(roomName);
    if (!joinRoom)
    {
        qWarning() << "Can't get a join-room job.";
        bailOut();
        return;
    }

    connect(joinRoom,
            &QMatrixClient::BaseJob::failure,
            [this]()
            {
                qWarning() << "Joining room" << this->m_roomName << "failed.";
                bailOut();
            });
    connect(joinRoom,
            &QMatrixClient::BaseJob::success,
            [this, joinRoom]()
            {
                setupWatchers();

                qDebug() << "Joined room" << this->m_roomName << "successfully.";
                m_room = m_conn.room(joinRoom->roomId(), QMatrixClient::JoinState::Join);
                if (!m_room)
                {
                    qDebug() << ".. pending invite, giving up already.";
                    bailOut();
                }
                else
                {
                    m_room->checkVersion();
                    qDebug() << "Room version" << m_room->version();
                    m_room->setDisplayed(true);  // Force non-lazy load
                    // Some rooms never generate a baseStateLoaded signal, so just wait 10sec
                    QTimer::singleShot(10000, this, &Bot::baseStateLoaded);
                    connect(m_room, &QMatrixClient::Room::baseStateLoaded, this, &Bot::baseStateLoaded);
                    connect(m_room, &QMatrixClient::Room::addedMessages, this, &Bot::addedMessages);
                }
            });

    setOps(conn.userId(), true);
    for (const auto& u : ops)
    {
        setOps(u, true);
    }
}

Bot::~Bot()
{
    if (m_room)
    {
        m_room->leaveRoom();
    }
    qDeleteAll(m_watchers);

    instance_count--;
    if (instance_count < 1)
    {
        bailOut(3000);  // give some time for messages to be delivered
    }
}

void Bot::baseStateLoaded()
{
    if (m_newlyConnected)
    {
        m_newlyConnected = false;
        qDebug() << "Room base state loaded"
                 << "id=" << m_room->id() << "name=" << m_room->displayName() << "topic=" << m_room->topic();
    }
}

/// @brief RAII helper to always flush bot messages
class Flusher
{
public:
    Flusher(Bot* b)
        : m_b(b)
    {
    }
    ~Flusher() { m_b->message(Bot::Flush {}); }

private:
    Bot* m_b;
};

void Bot::addedMessages(int from, int to)
{
    if (m_newlyConnected)
    {
        qDebug() << "Room messages" << from << '-' << to << ".. Ignoring them";
        m_room->markMessagesAsRead(m_room->readMarkerEventId());
        return;
    }

    bool first = true;
    const auto& timeline = m_room->messageEvents();
    for (int it = from; it <= to; ++it)
    {
        const QMatrixClient::RoomMessageEvent* event = timeline[it].viewAs<QMatrixClient::RoomMessageEvent>();
        if (event)
        {
            if (first)
            {
                qDebug() << "Room messages" << from << '-' << to << event->originTimestamp().toString() << "arrived"
                         << QDateTime::currentDateTimeUtc().toString();
                first = false;
            }
            for (const auto& w : m_watchers)
            {
                w->handleMessage(event);
            }

            CommandArgs cmd(event);
            if (cmd.isValid())
            {
                Flusher f(this);
                bool handled = false;
                for (const auto& w : m_watchers)
                {
                    if (w->moduleName() == cmd.command)
                    {
                        cmd.pop();
                        w->handleCommand(cmd);
                        handled = true;
                        break;
                    }
                }

                if (handled)
                {
                    continue;
                }

                if (m_ambiguousCommands.contains(cmd.command))
                {
                    message(QString("'%1' is ambiguous. Please use a module command.").arg(cmd.command));
                }
                else
                {
                    for (const auto& w : m_watchers)
                    {
                        if (w->moduleCommands().contains(cmd.command))
                        {
                            w->handleCommand(cmd);
                            handled = true;
                            break;
                        }
                    }

                    if (!handled)
                    {
                        message(QString("I don't understand '%1'.").arg(cmd.command));
                    }
                }
            }
        }
    }
    m_room->markMessagesAsRead(timeline[to]->id());
}

bool Bot::setOps(const QString& user, bool op)
{
    if (!user.startsWith('@') || !user.contains(':'))
    {
        // Doesn't look like a matrix ID to me
        return false;
    }
    if (op)
    {
        m_operators.insert(user);
        return true;
    }
    else if (m_operators.count() > 1)
    {
        // Can't deop the bot itself
        if (user == m_conn.userId())
            return false;
        if (m_operators.contains(user))
        {
            m_operators.remove(user);
            return true;
        }
        return false;  // Wasn't removed
    }
    else
        return false;  // Can't remove last op
}

bool Bot::checkOps(const QString& user, Silent s)
{
    return !user.isEmpty() && m_operators.contains(user);
}

bool Bot::checkOps(const QuatBot::CommandArgs& cmd, Silent s)
{
    return cmd.internalOperator || checkOps(cmd.user, s);
}

bool Bot::checkOps(const QuatBot::CommandArgs& cmd)
{
    if (checkOps(cmd, Silent {}))
        return true;
    message("Only operators can do that.");
    return false;
}

void Bot::message(const QStringList& l)
{
    if (!m_room)
        return;
    message(l.join(' '));
}

void Bot::message(const QString& s)
{
    if (!m_room)
        return;
    if (s.isEmpty())
        return;
    m_accumulatedMessages.append(s);
    for (const auto& p : m_watchers)
        p->handleMessage(s);
}

void Bot::message(Bot::Flush)
{
    if (!m_accumulatedMessages.isEmpty())
    {
        m_room->postPlainText(m_accumulatedMessages.join('\n'));
        m_accumulatedMessages.clear();
    }
}

Watcher* Bot::getWatcher(const QString& name)
{
    for (const auto& w : m_watchers)
        if (w->moduleName() == name)
            return w;
    return nullptr;
}

QStringList Bot::watcherNames() const
{
    QStringList names;
    for (const auto& w : m_watchers)
    {
        if (!w->moduleName().isEmpty())
        {
            names << w->moduleName();
        }
    }
    return names;
}

QStringList Bot::operatorIds()
{
    return m_operators.values();
}

static QSet<QString> commandSet(const QStringList& commands)
{
    return QSet<QString>(commands.begin(), commands.end());
}

void Bot::setupWatchers()
{
    m_watchers.reserve(6);
    m_watchers.append(new BasicCommands(this));
    m_watchers.append(new Logger(this));
    m_watchers.append(new Meeting(this));
#ifdef ENABLE_COFFEE
    m_watchers.append(new Coffee(this));
#endif

    QSet<QString> commands;
    for (const auto& w : m_watchers)
    {
        for (const auto& cmd : w->moduleCommands())
        {
            if (commands.contains(cmd))
            {
                m_ambiguousCommands.insert(cmd);
            }
            else
            {
                commands.insert(cmd);
            }
        }
    }

    // Except that the "basic commands" are always interpreted by basic (because it's first)
    // so those are not considered ambiguous.
    m_ambiguousCommands.subtract(commandSet(m_watchers[0]->moduleCommands()));
}

}  // namespace QuatBot
