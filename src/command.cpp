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
static void echo(QMatrixClient::Room* room, const QStringList& l)
{
    room->postPlainText(l.join(' '));
}

static void fortune(QMatrixClient::Room* room)
{
    QProcess f;
    f.start("/usr/bin/fortune", {"freebsd-tips"});
    f.waitForFinished();
    if (f.exitCode()==0)
    {
        QString text = QString::fromLatin1(f.readAllStandardOutput());
        echo(room, QStringList{text});
    }
    else
        echo(room, QStringList{"No fortune for you!"});
}

BasicCommands::BasicCommands(Bot* parent) :
    Watcher(parent)
{
}

BasicCommands::~BasicCommands()
{
}

void BasicCommands::handleCommand(QMatrixClient::Room* room, const CommandArgs& l)
{
    if (l.command == QStringLiteral("echo"))
        echo(room, l.args);
    else if (l.command == QStringLiteral("fortune"))
        fortune(room);
    else
        echo(room, QStringList{QString("I don't understand '%1'.").arg(l.command)});
}

void QuatBot::BasicCommands::handleMessage(QMatrixClient::Room* room, const QMatrixClient::RoomMessageEvent* event)
{
    QString message = event->plainBody();
    if (isCommand(message))
        handleCommand(room, extractCommand(message));
}

}
