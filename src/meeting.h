/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_COMMAND_H
#define QUATBOT_COMMAND_H

#include "watcher.h"

namespace QuatBot
{
/// @brief Run a meeting with ~meeting commands.
class Meeting : public Watcher
{
public:
    Meeting(Bot* parent);
    virtual ~Meeting() override;
    
    virtual QString moduleName() const override;
    
    virtual void handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent*) override;
    virtual void handleCommand(QMatrixClient::Room* room, const CommandArgs&) override;
    
protected:
    void status(QMatrixClient::Room*);
} ;

}  // namespace

#endif
