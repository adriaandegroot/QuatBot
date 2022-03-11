/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019, 2021 Adriaan de Groot <groot@kde.org>
 */

#ifndef QUATBOT_DUMPBOT_H
#define QUATBOT_DUMPBOT_H

#include <QDateTime>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

namespace Quotient
{
    class Connection;
    class Room;
    class RoomMessageEvent;
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

    /** @brief Sets the show-users-only property
     *
     * When set to @c true, the bot will display only users
     * in the room, then exit; it will not dump messages.
     * Call this early on in the connection setup, or messages
     * may have been dumped already.
     */
    void setShowUsersOnly(bool u);

    /** @brief Sets the time-range (logs all messages since the given stamp)
     *
     * This unsets the log-a-number-of-messages value. If @p since is not
     * a valid timestamp, then instead this calls `setLogCriterion(100)`;
     */
    void setLogCriterion(const QDateTime& since);

    /** @brief Sets the number of messages to log
     *
     * This unsets the log-messages-since value. If @p count is 0, uses
     * 100 instead.
     */
    void setLogCriterion(unsigned int count);

protected:
    /// @brief Called once the room is loaded for the first time.
    void baseStateLoaded();
    /// @brief Messages delivered by quotient
    void addedMessages(int from, int to);

    /// @brief Prints a list of users in the room (exit if m_showUsersOnly is set)
    void showUsers();

    /// @brief Tries to get some more history
    void getMoreHistory();

private:
    Quotient::Room* m_room = nullptr;
    Quotient::Connection& m_conn;
    LoggerFile *m_logger = nullptr;

    QString m_roomName;
    bool m_newlyConnected = true;
    bool m_showUsersOnly = false;

    QDateTime m_since;
    unsigned int m_amount = 100;
    QList<const Quotient::RoomMessageEvent*> m_messages;
    QString m_previousChunkToken;
};
}  // namespace

#endif
