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

void Meeting::handleMessage(const QMatrixClient::RoomMessageEvent* e)
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

void Meeting::handleCommand(const CommandArgs& cmd)
{
    if (cmd.command == QStringLiteral("status"))
    {
        status();
    }
    else if (cmd.command == QStringLiteral("rollcall"))
    {
        if (m_state == State::None)
        {
            enableLogging(cmd, true);
            m_state = State::RollCall;
            m_breakouts.clear();
            m_participantsDone.clear();
            m_participants.clear();
            m_participants.append(cmd.user);
            m_chair = cmd.user;
            shortStatus();
            message(QStringList{"Hello @room, this is the roll-call!"} << m_bot->userIds());
        }
        else
        {
            shortStatus();
        }
    }
    else if (cmd.command == QStringLiteral("next"))
    {
        if (!((m_state == State::RollCall) || (m_state == State::InProgress)))
        {
            shortStatus();
        }
        else if ((cmd.user == m_chair) || m_bot->checkOps(cmd))
        {
            if (m_state == State::RollCall)
            {
                m_state = State::InProgress;
                status();
                m_participantsDone.clear();
            }
            doNext();
            if (m_state == State::None)
            {
                enableLogging(cmd, false);
            }
        }
    }
    else if (cmd.command == QStringLiteral("skip"))
    {
        if (!((m_state == State::RollCall) || (m_state == State::InProgress)))
        {
            shortStatus();
        }
        else if ((cmd.user == m_chair) || m_bot->checkOps(cmd))
        {
            for (const auto& u : cmd.args)
            {
                QString user = m_bot->userLookup(u);
                if (!user.isEmpty())
                {
                    m_participants.removeAll(user);
                    m_participantsDone.insert(user);
                    message(QString("User %1 will be skipped this meeting.").arg(user));
                }
            }
        }
    }
    else if (cmd.command == QStringLiteral("bump"))
    {
        if (!((m_state == State::RollCall) || (m_state == State::InProgress)))
        {
            shortStatus();
        }
        else if ((cmd.user == m_chair) || m_bot->checkOps(cmd))
        {
            for (const auto& u : cmd.args)
            {
                QString user = m_bot->userLookup(u);
                if (!user.isEmpty())
                {
                    m_participants.removeAll(user);
                    m_participantsDone.remove(user);
                    m_participants.insert(0, user);
                    message(QString("User %1 is up next.").arg(user));
                }
            }
        }
    }
    else if (cmd.command == QStringLiteral("breakout"))
    {
        if (!((m_state == State::RollCall) || (m_state == State::InProgress)))
        {
            shortStatus();
        }
        else
        {
            m_breakouts.append(cmd.args.join(' '));
            message(QString("Registered breakout '%1'.").arg(m_breakouts.last()));
        }
    }
    else
    {
        message(QString("Usage: %1 <status|rollcall|next|skip|bump>").arg(displayCommand()));
    }
}

void Meeting::doNext()
{
    if (m_state != State::InProgress)
    {
        shortStatus();
        return;
    }
    if (m_participants.count() < 1)
    {
        m_state = State::None;
        shortStatus();
        if (m_breakouts.count() > 0)
        {
            for(const auto& b : m_breakouts)
            {
                message(QString("Breakout: %1").arg(b));
            }
        }
        return;
    }
    
    QString onDeck = m_participants.takeFirst();
    m_participantsDone.insert(onDeck);
    
    if (m_participants.count() > 0)
    {
        message(QString("%1, you're up (after that, %2).").arg(onDeck, m_participants.first()));
    }
    else
    {
        message(QString("%1, you're up (after that, we're done!).").arg(onDeck));
    }
}

void Meeting::shortStatus() const
{
    switch (m_state)
    {
        case State::None:
            message(QString("No meeting in progress."));
            return;
        case State::RollCall:
            message(QString("Doing the rollcall."));
            return;
        case State::InProgress:
            message(QString("Meeting in progress."));
            return;
    }
    message(QString("The meeting is in disarray."));
}

void Meeting::status() const
{
    shortStatus();
    if (m_state != State::None)
    {
        message(QString("There are %1 participants.").arg(m_participants.count()));
    }
}

void Meeting::enableLogging(const CommandArgs& cmd, bool b)
{
    if (m_bot->checkOps(cmd, Bot::Silent{}))
    {
        Watcher* w = m_bot->getWatcher("log");
        if (w)
        {
            // Pass a command to the log watcher, with the same (ops!)
            // user id, but a fake id so that the log file gets a
            // sensible name. Remember that the named watchers expect
            // a subcommand, not their main command.
            CommandArgs logCommand(cmd);
            int year = 0;
            int week = QDate::currentDate().weekNumber(&year);
            logCommand.id = QString("notes_%1_%2").arg(year).arg(week);
            logCommand.command = b ? QStringLiteral("on") : QStringLiteral("off");
            logCommand.args = QStringList{};
            w->handleCommand(logCommand);
        }
    }
}

}  // namespace
