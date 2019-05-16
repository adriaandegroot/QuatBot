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
    Watcher(bot)
{
}
    
Meeting::~Meeting()
{
}

QString Meeting::moduleName() const
{
    return QStringLiteral("meeting");
}

void Meeting::handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent*)
{
    // Ignored
}

void Meeting::handleCommand(QMatrixClient::Room* room, const CommandArgs& cmd)
{
    if (cmd.command == QStringLiteral("status"))
    {
        status(room);
    }
    else
    {
        message(room, QString("Usage: %1 <status>").arg(displayCommand()));
    }
}

void Meeting::status(QMatrixClient::Room* room)
{
    message(room, QString("No meeting in progress."));
}

}  // namespace
