/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_COMMAND_H
#define QUATBOT_COMMAND_H

#include "watcher.h"

#include <QString>
#include <QStringList>

namespace QuatBot
{
struct CommandArgs
{
    QString command;
    QStringList args;
};
    
class BasicCommands : public Watcher
{
public:
    BasicCommands(QChar prefixChar, Bot* parent);
    virtual ~BasicCommands() override;

    virtual void handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent*) override;
    
protected:    
    bool isCommand(const QString& s)
    {
        return s.startsWith(m_prefix);
    }
    
    CommandArgs extractCommand(QString);
    void handleCommand(QMatrixClient::Room* room, const CommandArgs&);

private:
    const QChar m_prefix;
};
    
}  // namespace
#endif
