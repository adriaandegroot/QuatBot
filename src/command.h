/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_COMMAND_H
#define QUATBOT_COMMAND_H

#include "watcher.h"

#include <QTime>

namespace QuatBot
{
class BasicCommands : public Watcher
{
public:
    BasicCommands(Bot* parent);
    virtual ~BasicCommands() override;

    virtual QString moduleName() const override;
    
    virtual void handleMessage(const QMatrixClient::RoomMessageEvent*) override;
    virtual void handleCommand(const CommandArgs&) override;

private:
    QTime m_lastMessageTime;
    int m_messageCount = 0;
    int m_commandCount = 0;
};
    
}  // namespace
#endif
