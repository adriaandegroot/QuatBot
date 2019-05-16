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

#include <csapi/joining.h>
#include <events/roommessageevent.h>

#include "command.h"
#include "logger.h"

namespace QuatBot
{
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
        
    qDebug() << "Using home=" << conn.homeserver() << "user=" << conn.userId();
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
            m_watchers.reserve(2);
            m_watchers.append(new BasicCommands(this));
            m_watchers.append(new Logger(this));
            
            qDebug() << "Joined room" << this->m_roomName << "successfully.";
            m_room = m_conn.room(joinRoom->roomId(), QMatrixClient::JoinState::Join);
            connect(m_room, &QMatrixClient::Room::baseStateLoaded, this, &Bot::baseStateLoaded);
            connect(m_room, &QMatrixClient::Room::addedMessages, this, &Bot::addedMessages);
        }
    );
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
                w->handleMessage(m_room, event);
        }
    }
    m_room->markMessagesAsRead(timeline[to]->id());
}

}  // namespace