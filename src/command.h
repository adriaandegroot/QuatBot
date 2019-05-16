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
class BasicCommands : public Watcher
{
public:
    BasicCommands(Bot* parent);
    virtual ~BasicCommands() override;

    virtual void handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent*) override;
    
protected:    
    void handleCommand(QMatrixClient::Room* room, const CommandArgs&);
};
    
}  // namespace
#endif