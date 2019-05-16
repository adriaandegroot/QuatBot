/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "meeting.h"

#include "quatbot.h"

#include <room.h>

namespace QuatBot
{
    
Meeting::Meeting(Bot* bot) :
    Watcher(bot),
    m_state(State::None)
{
}
    
Meeting::~Meeting()
{
}

QString Meeting::moduleName() const
{
    return QStringLiteral("meeting");
}

void Meeting::handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent* e)
{
    // New speaker?
    if ((m_state != State::None) && !m_participantsDone.contains(e->senderId()) && !m_participants.contains(e->senderId()))
    {
        m_participants.append(e->senderId());
        
        // Keep the chair at the end
        m_participants.removeAll(m_chair);
        m_participants.append(m_chair);
    }
}

void Meeting::handleCommand(QMatrixClient::Room* room, const CommandArgs& cmd)
{
    if (cmd.command == QStringLiteral("status"))
    {
        status(room);
    }
    else if (cmd.command == QStringLiteral("rollcall"))
    {
        if (m_state == State::None)
        {
            m_state = State::RollCall;
            m_participantsDone.clear();
            m_participants.clear();
            m_participants.append(cmd.user);
            m_chair = cmd.user;
            shortStatus(room);
        }
        else
        {
            shortStatus(room);
        }
    }
    else if (cmd.command == QStringLiteral("next"))
    {
        if (!((m_state == State::RollCall) || (m_state == State::InProgress)))
        {
            shortStatus(room);
        }
        else if ((cmd.user == m_chair) || m_bot->checkOps(cmd, room))
        {
            if (m_state == State::RollCall)
            {
                m_state = State::InProgress;
                status(room);
                m_participantsDone.clear();
            }
            doNext(room);
        }
    }
    else
    {
        message(room, QString("Usage: %1 <status|rollcall|next>").arg(displayCommand()));
    }
}

void Meeting::doNext(QMatrixClient::Room* room)
{
    if (m_state != State::InProgress)
    {
        shortStatus(room);
        return;
    }
    if (m_participants.count() < 1)
    {
        m_state = State::None;
        shortStatus(room);
        return;
    }
    
    QString onDeck = m_participants.takeFirst();
    m_participantsDone.insert(onDeck);
    
    if (m_participants.count() > 0)
    {
        message(room, QString("%1, you're up (after that, %2).").arg(onDeck, m_participants.first()));
    }
    else
    {
        message(room, QString("%1, you're up (after that, we're done!).").arg(onDeck));
    }
}

void Meeting::shortStatus(QMatrixClient::Room* room) const
{
    switch (m_state)
    {
        case State::None:
            message(room, QString("No meeting in progress."));
            return;
        case State::RollCall:
            message(room, QString("Doing the rollcall."));
            return;
        case State::InProgress:
            message(room, QString("Meeting in progress."));
            return;
    }
    message(room, QString("The meeting is in disarray."));
}

void Meeting::status(QMatrixClient::Room* room) const
{
    shortStatus(room);
    if (m_state != State::None)
    {
        message(room, QString("There are %1 participants.").arg(m_participants.count()));
    }
}

}  // namespace
