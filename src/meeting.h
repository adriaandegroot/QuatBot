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
    struct Private;
    
public:
    Meeting(Bot* parent);
    virtual ~Meeting() override;
    
    const QString& moduleName() const override;
    const QStringList& moduleCommands() const override;
    
    virtual void handleMessage(const Quotient::RoomMessageEvent*) override;
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
    
private:
    Private *d;
} ;

}  // namespace

#endif
