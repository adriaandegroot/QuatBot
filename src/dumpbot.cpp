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

void log_messages(const MessageList& messages, int from, LoggerFile& logger)
{
    bool first = true;
    for (int it=from; it<messages.count(); ++it)
    {
        if (first)
        {
            qDebug() << "Room messages" << from << '-' << (messages.count() - 1) << messages[it].originTimestamp().toString() << "arrived" << QDateTime::currentDateTimeUtc().toString();
            first = false;
        }
        logger.log(messages[it]);
    }
}

void DumpBot::finished()
{
    if (!isSatisfied())
    {
        qWarning() << "finished() called too soon.";
    }

    if (m_amount > 0)
    {
        const int from = m_amount <= m_messages.count() ? m_messages.count() - m_amount  : 0;
        log_messages(m_messages, from, *m_logger);
    }
    else
    {
        auto first = std::find_if(m_messages.begin(), m_messages.end(), [since=m_since](const MessageData& e){ return since < e.originTimestamp(); });
        if (first != m_messages.end())
        {
            log_messages(m_messages,
                         std::distance(m_messages.begin(), first), *m_logger);
        }
        else
        {
            qWarning() << "No message after" << m_since;
        }
    }
}

MessageData::MessageData(const Quotient::RoomMessageEvent* p)
    : m_dt(p->originTimestamp())
    , m_id(p->id())
    , m_sender(p->senderId())
    , m_plainBody(p->plainBody())
{
}


static void organize_messages(MessageList& messages)
{
    std::sort(messages.begin(), messages.end(), []( const MessageData& a, const MessageData& b) { return a.originTimestamp() < b.originTimestamp(); } );
    auto duplicates = std::unique(messages.begin(), messages.end(), []( const MessageData& a, const MessageData& b) { return a.id() == b.id(); });
    messages.erase(duplicates, messages.end());

    if (messages.isEmpty())
    {
        qDebug() << "No messages!";
    }
    else
    {
        qDebug() << "There are now" << messages.count() << "messages from" << messages.first().originTimestamp() << "to" << messages.last().originTimestamp();
    }
}

static void add_messages(const Quotient::Room::Timeline& timeline, MessageList& messages)
{
    std::for_each(timeline.cbegin(), timeline.cend(),
                    [&messages](const Quotient::TimelineItem& i){ const QMatrixClient::RoomMessageEvent* event = i.viewAs<QMatrixClient::RoomMessageEvent>(); if(event) { messages.append(MessageData(event)); } });
    organize_messages(messages);
}

static void add_messages(const Quotient::RoomEvents& timeline, MessageList& messages)
{
    std::for_each(timeline.cbegin(), timeline.cend(),
                  [&messages](const std::unique_ptr<Quotient::RoomEvent>& e) { Quotient::visit(*e, [&messages](const Quotient::RoomMessageEvent& i) { messages.append(MessageData(&i)); }); } );
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
        qWarning() << "No history job! Starting new one from" << m_previousChunkToken;
        p = new GetRoomEventsJob(m_room->id(), m_previousChunkToken, QStringLiteral("b"), QString(), 100);
        connect(p, &GetRoomEventsJob::finished, [p](){ qDebug() << "Extra history job finished"; p->deleteLater(); });
        connect(p, &GetRoomEventsJob::success, [this, p](){ add_messages( p->chunk(), m_messages); if (!isSatisfied()) { qDebug() << "Need more"; m_previousChunkToken = p->end(); QTimer::singleShot(0, this, &DumpBot::getMoreHistory); } else { qDebug() << "All done." ; QTimer::singleShot(0, this, &DumpBot::finished);} });
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
    if (!isSatisfied())
    {
        getMoreHistory();
    }
    else
    {
        finished();
    }
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

bool DumpBot::isSatisfied() const
{
    if (m_amount && m_messages.count() >= m_amount)
    {
        return true;
    }
    if (m_since.isValid() && !m_messages.isEmpty())
    {
        // We don't add messages **after** the end, so if the start timestamp
        // is later than the most-recent message, we are not going to find any;
        // pretend it's satisfied.
        if (m_messages.last().originTimestamp() < m_since)
        {
            return true;
        }
        // This is the proper sense of "since": we have messages starting
        // before the given time, assume we have all of them from "since"
        // to most-recent.
        if (m_since < m_messages.first().originTimestamp())
        {
            return true;
        }
    }
    return false;
}

}  // namespace
