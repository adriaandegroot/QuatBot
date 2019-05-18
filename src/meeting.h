/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_MEETING_H
#define QUATBOT_MEETING_H

#include "watcher.h"

#include <QList>
#include <QSet>
#include <QTimer>

namespace QuatBot
{
/// @brief Run a meeting with ~meeting commands.
class Meeting : public Watcher
{
public:
    Meeting(Bot* parent);
    virtual ~Meeting() override;
    
    const QString& moduleName() const override;
    const QStringList& moduleCommands() const override;
    
    virtual void handleMessage(const QMatrixClient::RoomMessageEvent*) override;
    virtual void handleCommand(const CommandArgs&) override;
   
    enum class State
    {
        None,
        RollCall,
        InProgress
    } ;
    
protected:
    void shortStatus() const;
    void status() const;
    
    void enableLogging(const CommandArgs&, bool);
    void doNext();
    
    /// @brief Called by timer, when someone is not responding.
    void timeout();
    
private:
    State m_state;
    QList<QString> m_participants;
    QSet<QString> m_participantsDone;
    QList<QString> m_breakouts;
    QString m_chair;
    QString m_current;
    QTimer m_waiting;
    bool m_currentSeen;
} ;

}  // namespace

#endif
