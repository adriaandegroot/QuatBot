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

void DumpBot::showUsers()
{
    QStringList l = userIds();
    l.sort();
    for (const auto& u : l)
    {
        m_logger->log(u);
    }
    if (m_showUsersOnly)
    {
        QTimer::singleShot(100, qApp, &QCoreApplication::quit);
    }
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
        if (m_showUsersOnly)
        {
            showUsers();
        }
    }
}

void log_messages(const Quotient::Room::Timeline& timeline, int from, int to, LoggerFile& logger)
{
    bool first = true;
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
            logger.log(event);
        }
    }
}

static void organize_messages(QList<const Quotient::RoomMessageEvent*>& messages)
{
    std::sort(messages.begin(), messages.end(), []( const Quotient::RoomMessageEvent* a, const Quotient::RoomMessageEvent* b) { return a->originTimestamp() < b->originTimestamp(); } );
    auto duplicates = std::unique(messages.begin(), messages.end(), []( const Quotient::RoomMessageEvent* a, const Quotient::RoomMessageEvent* b) { return a->id() == b->id(); });
    messages.erase(duplicates, messages.end());

    if (messages.isEmpty())
    {
        qDebug() << "No messages!";
    }
    else
    {
        qDebug() << "There are now" << messages.count() << "messages from" << messages.first()->originTimestamp() << "to" << messages.last()->originTimestamp();
    }
}

static void add_messages(const Quotient::Room::Timeline& timeline, QList<const Quotient::RoomMessageEvent*>& messages)
{
    std::for_each(timeline.cbegin(), timeline.cend(),
                    [&messages](const Quotient::TimelineItem& i){ const QMatrixClient::RoomMessageEvent* event = i.viewAs<QMatrixClient::RoomMessageEvent>(); if(event) { messages.append(event); } });
    organize_messages(messages);
}

static void add_messages(const Quotient::RoomEvents& timeline, QList<const Quotient::RoomMessageEvent*>& messages)
{
    std::for_each(timeline.cbegin(), timeline.cend(),
                  [&messages](const std::unique_ptr<Quotient::RoomEvent>& e) { Quotient::visit(*e, [&messages](const Quotient::RoomMessageEvent& i) { messages.append(&i); }); } );
    organize_messages(messages);
}


void DumpBot::getMoreHistory()
{
    auto* p = m_room->eventsHistoryJob();
    if (p)
    {
        qDebug() << "History" << p->begin() << p->end();
        m_conn.run(p);
    }
    else
    {
        // const QString startId = m_messages.isEmpty() ? m_room->firstDisplayedEventId() : m_messages[0]->id();
        using GetRoomEventsJob = Quotient::GetRoomEventsJob;
        qWarning() << "No history job!";
        p = new GetRoomEventsJob(m_room->id(), m_previousChunkToken, QStringLiteral("b"), QString(), 100);
        connect(p, &GetRoomEventsJob::finished, [p](){ qDebug() << "Extra history job finished"; p->deleteLater(); });
        connect(p, &GetRoomEventsJob::success, [this, p](){ add_messages( p->chunk(), m_messages); });
        m_conn.run(p);
    }
}

void DumpBot::addedMessages(int from, int to)
{
    const auto& timeline = m_room->messageEvents();
    if (!m_showUsersOnly)
    {
        add_messages(timeline, m_messages);
    }
    m_room->markMessagesAsRead(timeline[to]->id());
    m_logger->flush();
    getMoreHistory();
}

void DumpBot::setShowUsersOnly(bool u)
{
    m_showUsersOnly = u;
    if (!m_newlyConnected && u)
    {
        showUsers();
    }
}

void DumpBot::setLogCriterion(unsigned int count)
{
    if (count < 1)
    {
        count = 100;
    }
    m_amount = count;
    m_since = QDateTime();
}

void DumpBot::setLogCriterion(const QDateTime& since)
{
    if (!since.isValid())
    {
        setLogCriterion(100);
    }
    else
    {
        m_amount = 0;
        m_since = since;
    }
}

}  // namespace
