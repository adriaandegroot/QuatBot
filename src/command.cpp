/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "command.h"

#include <room.h>

#include <QProcess>

namespace QuatBot
{
static void fortune(QMatrixClient::Room* room)
{
    QProcess f;
    f.start("/usr/bin/fortune", {"freebsd-tips"});
    f.waitForFinished();
    if (f.exitCode()==0)
    {
        QString text = QString::fromLatin1(f.readAllStandardOutput());
        Watcher::message(room, text);
    }
    else
        Watcher::message(room, "No fortune for you!");
}

BasicCommands::BasicCommands(Bot* parent) :
    Watcher(parent)
{
}

BasicCommands::~BasicCommands()
{
}

QString BasicCommands::moduleName() const
{
    return QString();
}

void BasicCommands::handleCommand(QMatrixClient::Room* room, const CommandArgs& l)
{
    if (l.command == QStringLiteral("echo"))
        message(room, l.args);
    else if (l.command == QStringLiteral("fortune"))
        fortune(room);
    else
        message(room, QString("I don't understand '%1'.").arg(l.command));
}

void QuatBot::BasicCommands::handleMessage(QMatrixClient::Room* room, const QMatrixClient::RoomMessageEvent* event)
{
}

}
