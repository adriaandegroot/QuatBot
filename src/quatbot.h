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

namespace Quotient
{
class Connection;
class Room;
}  // namespace Quotient

namespace QuatBot
{
struct CommandArgs;
class Watcher;

/** @brief Top-level class for the QuatBot
 * 
 * The bot is basically self-managing. Once you have a connection
 * to the Matrix server that has reached the *connected* state,
 * create one Bot object per room to follow.
 * 
 * The bot has a collection of *Watchers* that handle messages in the
 * room. Only regular text messages are tracked, and no redaction or
 * anything is done. The watchers handle each incoming message, **and**
 * if the message starts with the special command-prefix (~, by default)
 * then the message is considered a command and is processed by one
 * Watcher (which one, depends on the command).
 * 
 * The bot keeps track of a list of *operators* for the room. These
 * operators can perform administrative operations on the bot,
 * such as adding more operators, switching on logging, and
 * running meetings (the full scope depends on the modules
 * (Watcher instances) that run in the bot).
 */
class Bot : public QObject
{
    friend class BasicCommands;  // Allows to call setOps

public:
    /** @brief Create a bot for the named @p roomName
     * 
     * Joins the @p roomName. Matrix-ids listed in @p ops are
     * added immediately to the set of operators. The user
     * set in @p conn is also always an operator.
     */
    explicit Bot(Quotient::Connection& conn, const QString& roomName, const QStringList& ops = QStringList());
    virtual ~Bot() override;

    /// @brief Tag-class used in checkOps() overrides.
    struct Silent
    {
    };

    /// @brief is the @p user an operator of the bot? No message.
    bool checkOps(const QString& user, Silent s);
    /// @brief is the @p cmd issued by an operator? No message.
    bool checkOps(const CommandArgs& cmd, Silent s);
    /** @brief is the @p cmd issued by an operator?
     * 
     * The @p cmd holds a user-id. If the user is an operator, returns
     * true. If not, then a warning message is sent to the room and
     * false is returned.
     */
    bool checkOps(const CommandArgs& cmd);

    /** @brief Looks up a Matrix Id from a userName (possibly a nickname)
     * 
     * When using tab-completion from Riot and other clients, a HTML-
     * formatted message is sent containing a link, but the plain-text
     * message just has the nickname itself. This is an issue when
     * a command is issued that names people (e.g. ~op).
     * 
     * This method looks up a Matrix-id for the nicknames @p userName.
     */
    QString userLookup(const QString& userName);
    /** @brief Looks up matrix ids based on expanded nicknames
     * 
     * When nicknames are used, they can contain spaces, and things
     * rapidly become a real mess. Here, given a list of words, try
     * to match those to Matrix Ids that are in the room.
     * 
     * Returns a list of plausible Matrix Ids for the given words.
     * (in particular, the list of four words {adridg the bot [ade]}
     * would return two Ids, with my current nicknames in use)
     * 
     * Names that are not recognized are kept intact; so looking up
     * {derp adridg the bot} would return {derp @adridg:matrix.org}.
     * Calling code is expected to call userLookup() individually to check.
     */
    QStringList userLookup(const QStringList& users);

    /// @brief All the user ids from the room
    QStringList userIds();
    /// @brief User id of the bot user itself
    QString botUser() const;
    /// @brief Room name this bot is attached to
    QString botRoom() const { return m_roomName; }

    /// @brief Sends a message to the room. @p l is joined with spaces.
    void message(const QStringList& l);
    /** @brief Sends a message to the room. @p l is sent as plain text.
     * 
     * Multiple messages may be sent in response to a single command.
     * The bot main loop collects them and calls message(Flush{})
     * to send the collected messages are one Matrix message. If a module
     * needs to **explicitly** make sure that a message is sent as a
     * separate response, call flush explicitly.
     */
    void message(const QString& s);

    struct Flush
    {
    };  ///< Tag class
    /// @brief Flushes the message queue.
    void message(Flush);

    /** @brief Get the watcher with the given @p name
     * 
     * In some cases one Watcher needs to use a service from another,
     * (generally by calling handleCommand() directly) and this allows
     * looking up the others by name.
     */
    Watcher* getWatcher(const QString& name);
    QStringList watcherNames() const;

protected:
    /// @brief Called once the room is loaded for the first time.
    void baseStateLoaded();
    /// @brief Messages delivered by libqmatrixclient
    void addedMessages(int from, int to);

    /** @brief Changes operator status of @p user to @p op
     * 
     * Returns true if the operation was successful (with @p op true,
     * it is always successful). May return false when de-opping a
     * non-existent name, or de-opping the last operator.
     */
    bool setOps(const QString& user, bool op);

    /// @brief Instantiate the watchers for this bot
    void setupWatchers();

private:
    Quotient::Room* m_room = nullptr;
    Quotient::Connection& m_conn;

    QVector<Watcher*> m_watchers;
    QSet<QString> m_operators;
    QSet<QString> m_ambiguousCommands;

    QStringList m_accumulatedMessages;
    QString m_roomName;
    bool m_newlyConnected = true;
};
}  // namespace QuatBot

#endif
