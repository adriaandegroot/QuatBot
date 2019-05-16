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
static constexpr const QChar COMMAND_PREFIX('!'); // 0x1575); // ᕵ Nunavik Hi

Watcher::Watcher(QuatBot::Bot* parent) :
    m_bot(parent)
{
}

Watcher::~Watcher()
{
}


static QString munge(const QString& s)
{
    return s.trimmed();
}

CommandArgs Watcher::extractCommand(QString s)
{
    if (!isCommand(s))
        return CommandArgs{};
    
    QStringList parts = s.remove(0,1).split(' ');
    QStringList r;
    for (int i=1; i<parts.count(); ++i)
        r << munge(parts[i]);
                   
    return {munge(parts[0]), r};
}

bool Watcher::isCommand(const QString& s)
{
    return s.startsWith(COMMAND_PREFIX);
}

bool Watcher::isCommand(const QMatrixClient::RoomMessageEvent* e)
{
    // There must be a faster way, by looking at raw data
    return isCommand(e->plainBody());
}

CommandArgs Watcher::extractCommand(const QMatrixClient::RoomMessageEvent* e)
{
    return extractCommand(e->plainBody());
}


}  // namespace
