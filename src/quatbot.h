/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_QUATBOT_H
#define QUATBOT_QUATBOT_H

#include <QObject>
#include <QString>
#include <QVector>

namespace QMatrixClient
{
    class Connection;
    class Room;
}

namespace QuatBot
{
class Watcher;

class Bot : public QObject
{
public:
    explicit Bot(QMatrixClient::Connection& conn, const QString& roomName);
    
    virtual ~Bot() override;
    
protected:
    void baseStateLoaded();
    
    void addedMessages(int from, int to);
    
private:
    QMatrixClient::Room* m_room = nullptr;
    QMatrixClient::Connection& m_conn;
    
    QVector<Watcher*> m_watchers;
    
    QString m_roomName;
    bool m_newlyConnected = true;
};
}  // namespace

#endif
