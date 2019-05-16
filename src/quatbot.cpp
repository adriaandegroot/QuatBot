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

#include "command.h"
#include "logger.h"
#include "meeting.h"

namespace QuatBot
{
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
    
    for (const auto& u : m_room->users())
    {
        l << u->id();
    }
    return l;
}


Bot::Bot(QMatrixClient::Connection& conn, const QString& roomName) :
        QObject(),
        m_conn(conn),
        m_roomName(roomName)
{
    if (conn.homeserver().isEmpty() || !conn.homeserver().isValid())
    {
        qWarning() << "Connection is invalid.";
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }
        
    auto* joinRoom = conn.joinRoom(roomName);
    
    connect(joinRoom, &QMatrixClient::BaseJob::failure,
        [this]() 
        {
            qWarning() << "Joining room" << this->m_roomName << "failed.";
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        }
    );
    connect(joinRoom, &QMatrixClient::BaseJob::success,
        [this, joinRoom]()
        {
            m_watchers.reserve(3);
            m_watchers.append(new BasicCommands(this));
            m_watchers.append(new Logger(this));
            m_watchers.append(new Meeting(this));
            
            qDebug() << "Joined room" << this->m_roomName << "successfully.";
            m_room = m_conn.room(joinRoom->roomId(), QMatrixClient::JoinState::Join);
            connect(m_room, &QMatrixClient::Room::baseStateLoaded, this, &Bot::baseStateLoaded);
            connect(m_room, &QMatrixClient::Room::addedMessages, this, &Bot::addedMessages);
        }
    );
    
    setOps(conn.userId(), true);
}
    
Bot::~Bot()
{
    if(m_room)
        m_room->leaveRoom();
    qDeleteAll(m_watchers);
}
    
void Bot::baseStateLoaded()
{
    m_newlyConnected = false;
    qDebug() << "Room base state loaded"
        << "id=" << m_room->id()
        << "name=" << m_room->displayName()
        << "topic=" << m_room->topic();
}
    
void Bot::addedMessages(int from, int to)
{
    qDebug() << "Room messages" << from << '-' << to;
    if (m_newlyConnected)
    {
        qDebug() << ".. Ignoring them";
        m_room->markMessagesAsRead(m_room->readMarker()->event()->id());
        return;
    }
    
    const auto& timeline = m_room->messageEvents();
    for (int it=from; it<=to; ++it)
    {
        const QMatrixClient::RoomMessageEvent* event = timeline[it].viewAs<QMatrixClient::RoomMessageEvent>();
        if (event)
        {
            for(const auto& w : m_watchers)
                w->handleMessage(event);
            
            CommandArgs cmd(event);
            if (cmd.isValid())
            {
                bool handled = false;
                for(const auto& w : m_watchers)
                {
                    if (w->moduleName() == cmd.command)
                    {
                        cmd.pop();
                        w->handleCommand(cmd);
                        handled = true;
                        break;
                    }
                }
                // Special case: unhandled ones go to BasicCommands
                // which handles all the "rest" items.
                if (!handled)
                    m_watchers[0]->handleCommand(cmd);
            }
        }
    }
    m_room->markMessagesAsRead(timeline[to]->id());
}

bool Bot::setOps(const QString& user, bool op)
{
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
    else return false;  // Can't remove last op
}

bool Bot::checkOps(const QString& user, Silent s)
{
    return !user.isEmpty() && m_operators.contains(user);
}

bool Bot::checkOps(const QuatBot::CommandArgs& cmd, Silent s)
{
    return checkOps(cmd.user, s);
}

bool Bot::checkOps(const QuatBot::CommandArgs& cmd)
{
    if (checkOps(cmd, Silent{}))
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
    m_room->postPlainText(s);
    for (const auto& p : m_watchers)
        p->handleMessage(s);
    qDebug() << "**BOT**" << s;
}

Watcher* Bot::getWatcher(const QString& name)
{
    for (const auto& w : m_watchers)
        if (w->moduleName() == name)
            return w;
    return nullptr;
}

}  // namespace
