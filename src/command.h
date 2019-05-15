/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_COMMAND_H
#define QUATBOT_COMMAND_H

#include <QString>
#include <QStringList>

namespace QMatrixClient
{
    class Room;
}

struct CommandArgs
{
    QString command;
    QStringList args;
};

class CommandHandler
{
public:
    CommandHandler(QChar prefixChar);
    
    bool isCommand(const QString& s)
    {
        return s.startsWith(m_prefix);
    }
    
    CommandArgs extractCommand(QString);
    void handleCommand(QMatrixClient::Room* room, const CommandArgs&);

private:
    const QChar m_prefix;
};
    
#endif
