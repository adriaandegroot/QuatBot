/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019, 2021 Adriaan de Groot <groot@kde.org>
 */

#include "dumpbot.h"

#include "log_impl.h"

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

namespace QuatBot
{

QStringList DumpBot::userIds()
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

QString DumpBot::botUser() const
{
    return m_conn.userId();
}



static int instance_count = 0;

static void bailOut(int timeout=0)
{
    QTimer::singleShot(timeout, qApp, &QCoreApplication::quit);
}


DumpBot::DumpBot(QMatrixClient::Connection& conn, const QString& roomName, const QStringList& ops) :
        QObject(),
        m_conn(conn),
        m_roomName(roomName)
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

    m_logger = new LoggerFile;
    m_logger->open(QString());  // Default name

    connect(joinRoom, &QMatrixClient::BaseJob::failure,
        [this]()
        {
            qWarning() << "Joining room" << this->m_roomName << "failed.";
            bailOut();
        }
    );
    connect(joinRoom, &QMatrixClient::BaseJob::success,
        [this, joinRoom]()
        {
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
                m_room->setDisplayed(true);
                // Some rooms never generate a baseStateLoaded signal, so just wait 10sec
                QTimer::singleShot(10000, this, &DumpBot::baseStateLoaded);
                connect(m_room, &QMatrixClient::Room::baseStateLoaded, this, &DumpBot::baseStateLoaded);
                connect(m_room, &QMatrixClient::Room::addedMessages, this, &DumpBot::addedMessages);
            }
        }
    );
}

DumpBot::~DumpBot()
{
    m_logger->close();
    if (m_room)
    {
        m_room->leaveRoom();
    }

    instance_count--;
    if (instance_count<1)
    {
        bailOut(3000);  // give some time for messages to be delivered
    }

    delete m_logger;
    m_logger = nullptr;
}

void DumpBot::baseStateLoaded()
{
    if (m_newlyConnected)
    {
        m_newlyConnected = false;
        qDebug() << "Room base state loaded"
            << "id=" << m_room->id()
            << "name=" << m_room->displayName()
            << "topic=" << m_room->topic();
    }
}

void DumpBot::addedMessages(int from, int to)
{
    bool first = true;
    const auto& timeline = m_room->messageEvents();
    for (int it=from; it<=to; ++it)
    {
        const QMatrixClient::RoomMessageEvent* event = timeline[it].viewAs<QMatrixClient::RoomMessageEvent>();
        if (event)
        {
            if (first)
            {
                qDebug() << "Room messages" << from << '-' << to << event->originTimestamp().toString() << "arrived" << QDateTime::currentDateTimeUtc().toString();
                first = false;
            }
            m_logger->log(event);
        }
    }
    m_room->markMessagesAsRead(timeline[to]->id());
    m_logger->flush();
}


}  // namespace
