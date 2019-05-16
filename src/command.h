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

    virtual QString moduleName() const override;
    
    virtual void handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent*) override;
    virtual void handleCommand(QMatrixClient::Room* room, const CommandArgs&) override;
};
    
}  // namespace
#endif
