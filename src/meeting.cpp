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
    
struct Meeting::Private
{
    explicit Private(Bot* bot) :
        m_bot(bot),
        m_state(State::None)
    {
        QObject::connect(&m_waiting, &QTimer::timeout, [this](){ this->timeout(); });
        m_waiting.setSingleShot(true);
    }
    
    bool hasStarted() const { return m_state != State::None; }
    bool isNew(const QString& s) { return !m_participantsDone.contains(s) && !m_participants.contains(s); }
    
    void addParticipant(const QString& s)
    {
        m_participants.append(s);
        // Keep the chair at the end
        m_participants.removeAll(m_chair);
        m_participants.append(m_chair);
    }

    /// @brief Start the meeting (roll-call)
    void start(const QString& chair)
    {
        m_state = State::RollCall;
        m_breakouts.clear();
        m_participantsDone.clear();
        m_participants.clear();
        m_participants.append(chair);
        m_chair = chair;
        m_current.clear();

        if (m_bot->botUser() != m_chair)
        {
            // Don't rollcall the bot itself
            m_participantsDone.insert(m_bot->botUser());
        }
        m_waiting.start(60000);  // one minute until reminder
    }

    /// @brief Start the meeting (main part)
    void startProper()
    {
        m_state = State::InProgress;
        m_participantsDone.clear();
        if (m_bot->botUser() != m_chair)
        {
            m_participants.removeAll(m_bot->botUser());
            m_participantsDone.insert(m_bot->botUser());
        }
    }
    
    bool isChair(const CommandArgs& cmd) { return cmd.user == m_chair; }

    void skip(const QString& user)
    {
        m_participants.removeAll(user);
        m_participantsDone.insert(user);
    }
    
    void bump(const QString& user)
    {
        m_participants.removeAll(user);
        m_participantsDone.remove(user);
        m_participants.insert(0, user);
    }
    
    void next()
    {
        if (m_state != State::InProgress)
        {
            return;
        }
        if (m_participants.count() < 1)
        {
            m_state = State::None;
            m_bot->message("That was the last one! We're done.");
            if (m_breakouts.count() > 0)
            {
                m_bot->message(Bot::Flush{});
                for(const auto& b : m_breakouts)
                {
                    m_bot->message(QString("Breakout: %1").arg(b));
                }
            }
            m_waiting.stop();
            return;
        }
        
        m_current = m_participants.takeFirst();
        m_participantsDone.insert(m_current);
        
        if (m_participants.count() > 0)
        {
            m_bot->message(QString("%1, you're up (after that, %2).").arg(m_current, m_participants.first()));
        }
        else
        {
            m_bot->message(QString("%1, you're up (after that, we're done!).").arg(m_current));
        }
        m_waiting.start(30000); // half a minute to reminder
    }
    
    void timeout();
    
    Bot* m_bot;
    State m_state;
    QList<QString> m_participants;
    QSet<QString> m_participantsDone;
    QList<QString> m_breakouts;
    QString m_chair;
    QString m_current;
    QTimer m_waiting;
    bool m_currentSeen;
};

Meeting::Meeting(Bot* bot) :
    Watcher(bot),
    d(new Private(bot))
{
}
    
Meeting::~Meeting()
{
}

const QString& Meeting::moduleName() const
{
    static const QString name(QStringLiteral("meeting"));
    return name;
}

const QStringList& Meeting::moduleCommands() const
{
    static const QStringList commands{"status","rollcall","next","breakout","skip","bump","done"};
    return commands;
}

void Meeting::handleMessage(const QMatrixClient::RoomMessageEvent* e)
{
    // New speaker?
    if (d->hasStarted() && d->isNew(e->senderId()))
    {
        d->addParticipant(e->senderId());
    }
    if ((d->m_state == State::InProgress ) && (e->senderId() == d->m_current))
    {
        d->m_waiting.stop();
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
        if (!d->hasStarted())
        {
            d->start(cmd.user);
            enableLogging(cmd, true);
            message(QStringList{"Hello @room, this is the roll-call!"} << m_bot->userIds());
        }
        else
        {
            shortStatus();
        }
    }
    else if (cmd.command == QStringLiteral("next"))
    {
        if (!d->hasStarted())
        {
            shortStatus();
        }
        else if (d->isChair(cmd) || m_bot->checkOps(cmd))
        {
            if (d->m_state == State::RollCall)
            {
                d->startProper();
                status();
            }
            d->next();
            if (!d->hasStarted())
            {
                enableLogging(cmd, false);
            }
        }
    }
    else if (cmd.command == QStringLiteral("skip"))
    {
        if (!d->hasStarted())
        {
            shortStatus();
        }
        else if (d->isChair(cmd) || m_bot->checkOps(cmd))
        {
            for (const auto& user : m_bot->userLookup(cmd.args))
            {
                if (!user.isEmpty())
                {
                    d->skip(user);
                    message(QString("User %1 will be skipped this meeting.").arg(user));
                }
            }
        }
    }
    else if (cmd.command == QStringLiteral("bump"))
    {
        if (!d->hasStarted())
        {
            shortStatus();
        }
        else if (d->isChair(cmd) || m_bot->checkOps(cmd))
        {
            for (const auto& user : m_bot->userLookup(cmd.args))
            {
                if (!user.isEmpty())
                {
                    d->bump(user);
                    message(QString("User %1 is up next.").arg(user));
                }
            }
        }
    }
    else if (cmd.command == QStringLiteral("breakout"))
    {
        if (!(d->m_state == State::InProgress))
        {
            shortStatus();
        }
        else
        {
            QString name = d->breakout(cmd.args);
            message(QString("Registered breakout '%1'.").arg(name));
        }
    }
    else if  (cmd.command == QStringLiteral("done"))
    {
        if (m_bot->checkOps(cmd))
        {
            d->m_state = State::None;
            d->m_waiting.stop();
            message(QString("The meeting has been forcefully ended."));
        }
    }
    else
    {
        message(Usage{});
    }
}

static QString _shortStatus(Meeting::State s)
{
    switch (s)
    {
        case Meeting::State::None:
            return QString("No meeting in progress.");
        case Meeting::State::RollCall:
            return QString("Doing the rollcall.");
        case Meeting::State::InProgress:
            return QString("Meeting in progress.");
    }
    return QString("The meeting is in disarray.");
}

void Meeting::shortStatus() const
{
    message(_shortStatus(d->m_state));
}

void Meeting::status() const
{
    QStringList l{"(meeting)"};
    l << _shortStatus(d->m_state);
    if (d->m_state != State::None)
    {
        l << QString("There are %1 participants.").arg(d->m_participants.count());
    }
    if ((d->m_state == State::InProgress) && !d->m_current.isEmpty())
    {
        l << QString("It is %1 's turn.").arg(d->m_current);
    }
    message(l);
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
            logCommand.args = QStringList{"?quiet"};
            w->handleCommand(logCommand);
        }
    }
}


void Meeting::Private::timeout()
{
    if (m_state == State::RollCall)
    {
        QStringList noResponse{"Roll-call reminder for"};
        
        for (const auto& u : m_bot->userIds())
        {
            if (!m_participants.contains(u) && !m_participantsDone.contains(u))
            {
                noResponse.append(u);
            }
        }
        
        if (noResponse.count() > 1)
        {
            m_bot->message(noResponse);
        }
    }
    else if (m_state == State::InProgress)
    {
        m_bot->message(QStringList{m_current, "are you with us?"});
    }
    m_bot->message(Bot::Flush{});
}

}  // namespace
