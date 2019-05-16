/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "watcher.h"

#include <room.h>

namespace QuatBot
{
static constexpr const QChar COMMAND_PREFIX('~'); // 0x1575); // ᕵ Nunavik Hi

static QString munge(const QString& s)
{
    return s.trimmed();
}

CommandArgs::CommandArgs(QString s)
{
    if (isCommand(s))
    {
        QStringList parts = s.remove(0,1).split(' ');
        QStringList r;
        for (int i=1; i<parts.count(); ++i)
            r << munge(parts[i]);

        command = munge(parts[0]);
        args = r;
    }
}

CommandArgs::CommandArgs(const QMatrixClient::RoomMessageEvent* e) :
    CommandArgs(e->plainBody())
{
    id = e->id();
    user = e->senderId();
}


bool CommandArgs::isCommand(const QString& s)
{
    return s.startsWith(COMMAND_PREFIX);
}

bool CommandArgs::isCommand(const QMatrixClient::RoomMessageEvent* e)
{
    // There must be a faster way, by looking at raw data
    return isCommand(e->plainBody());
}

bool CommandArgs::pop()
{
    if (args.count() < 1)
    {
        command.clear();
    }
    else
    {
        command = args.first();
        args.pop_front();
    }
    return isValid();
}


Watcher::Watcher(QuatBot::Bot* parent) :
    m_bot(parent)
{
}

Watcher::~Watcher()
{
}

QString Watcher::displayCommand(const QString& s)
{
    return QString("%1%2").arg(COMMAND_PREFIX).arg(s);
}


}  // namespace
