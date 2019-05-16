/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_QUATBOT_H
#define QUATBOT_QUATBOT_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

namespace QMatrixClient
{
    class Connection;
    class Room;
}

namespace QuatBot
{
class CommandArgs;
class Watcher;

/// @brief Looks up a Matrix Id from a userName (possibly a nickname)
QString userLookup(QMatrixClient::Room* room, const QString& userName);
/// @brief All the user ids from the room
QStringList userIds(QMatrixClient::Room* room);

class Bot : public QObject
{
friend class BasicCommands;  // Allows to call setOps

public:
    explicit Bot(QMatrixClient::Connection& conn, const QString& roomName);
    
    virtual ~Bot() override;
    
    /// @brief is the @p user an operator of the bot?
    bool checkOps(const QString& user);
    /// @brief is the @p cmd issued by an operator?
    bool checkOps(const CommandArgs& cmd);
    /// @brief is the @p cmd issued by an operator? Warning message if not.
    bool checkOps(const CommandArgs& cmd, QMatrixClient::Room* room);

    /// @brief Sends a message to the given @p room
    void message(const QStringList& l);
    /// @brief Sends a message to the given @p room
    void message(const QString& s);
    
protected:
    void baseStateLoaded();
    
    void addedMessages(int from, int to);

    /** @brief Changes operator status of @p user to @p op
     * 
     * Returns true if the operation was successful (with @p op true,
     * it is always successful). May return false when de-opping a
     * non-existent name, or de-opping the last operator.
     */
    bool setOps(const QString& user, bool op);
    
private:
    QMatrixClient::Room* m_room = nullptr;
    QMatrixClient::Connection& m_conn;
    
    QVector<Watcher*> m_watchers;
    QSet<QString> m_operators;
    
    QString m_roomName;
    bool m_newlyConnected = true;
};
}  // namespace

#endif
