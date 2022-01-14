/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019, 2021 Adriaan de Groot <groot@kde.org>
 */

#ifndef QUATBOT_DUMPBOT_H
#define QUATBOT_DUMPBOT_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

namespace Quotient
{
    class Connection;
    class Room;
}

namespace QuatBot
{
class LoggerFile;

/** @brief Top-level class for the DumpBot
 *
 * The bot is basically self-managing. Once you have a connection
 * to the Matrix server that has reached the *connected* state,
 * create one Bot object per room to follow.
 */
class DumpBot : public QObject
{
public:
    /** @brief Create a bot for the named @p roomName
     *
     * Joins the @p roomName. Matrix-ids listed in @p ops are
     * added immediately to the set of operators. The user
     * set in @p conn is also always an operator.
     */
    explicit DumpBot(Quotient::Connection& conn, const QString& roomName, const QStringList& ops=QStringList());
    virtual ~DumpBot() override;

    /// @brief All the user ids from the room
    QStringList userIds();
    /// @brief User id of the bot user itself
    QString botUser() const;
    /// @brief Room name this bot is attached to
    QString botRoom() const { return m_roomName; }

protected:
    /// @brief Called once the room is loaded for the first time.
    void baseStateLoaded();
    /// @brief Messages delivered by quotient
    void addedMessages(int from, int to);


private:
    Quotient::Room* m_room = nullptr;
    Quotient::Connection& m_conn;
    LoggerFile *m_logger = nullptr;

    QString m_roomName;
    bool m_newlyConnected = true;
};
}  // namespace

#endif
