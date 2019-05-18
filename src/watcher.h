/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_WATCHER_H
#define QUATBOT_WATCHER_H

#include "quatbot.h"

#include <QString>
#include <QStringList>

namespace QMatrixClient
{
    class Room;
    class RoomMessageEvent;
}

namespace QuatBot
{
/** @brief A command, with 0 or more arguments.
 * 
 * Commands have a **primary** command, and zero or more arguments.
 * Support for sub-commands comes through the pop() method, which shifts
 * an argument into the primary-command position.
 * 
 * Commands may carry an id and a user, if those were available at
 * creation. Commands are only created from strings (and messages)
 * that start with the special COMMAND_PREFIX character (defaults ~).
 */
struct CommandArgs
{
    /** @brief Build a command list from a string.
     * 
     * This kind of command does not carry id or user information.
     * If the string does not start with COMMAND_PREFIX, creates
     * an invalid command.
     */
    explicit CommandArgs(QString);   // Copied because it's modified in the method
    /** @brief Build a command list from an event.
     * 
     * This kind of command carries the id and user information from
     * the Matrix event. If the text of the message does not start with
     * COMMAND_PREFIX, an invalid command is created.
     */
    explicit CommandArgs(const QMatrixClient::RoomMessageEvent*);
    
    /// @brief Checks @p s for COMMAND_PREFIX
    static bool isCommand(const QString& s);
    /// @brief Checks @p e for COMMAND_PREFIX at the start of the message text
    static bool isCommand(const QMatrixClient::RoomMessageEvent* e);
    
    /// @brief Is this a valid command list?
    bool isValid() const { return !command.isEmpty(); }

    /** @brief "pops" a subcommand
     * 
     * CommandArgs are all formed as <command> <argument..>, but where
     * a command has subcommands, it can be useful to discard the
     * original <command> and move the first argument -- which should be
     * the subcommand -- into that place while removing it from the
     * arguments list.
     * 
     * Returns false (and results in an invalid CommandArgs) if there
     * is no subcommand (e.g. args is empty.
     */
    bool pop();
    
    QString id;  ///< event Id, if available.
    QString user;  ///< user Id, if available. Used for access-control (ops)
    QString command;
    QStringList args;
};
    
/** @brief Base class for handlers of a certain class of commands
 * 
 * A *Watcher* watches incoming messages and processes each text
 * message as it arrives (see handleMessage()). Some messages are
 * also commands, and handleCommand() is called with the parsed-out
 * command **after** the message is handled normally.
 * 
 * Watchers should not handle commands embedded in messages from
 * the body of handleMessage() .. the Bot framework does the work
 * there. Within the bot framework some "virtual" messages are
 * generated (e.g. bot responses as they are sent); these have their 
 * own handleMessage() method, which is usually a do-nothing method.
 * 
 * A Watcher has a name (an id, really) which identifies which class
 * of commands it handles. The Watcher with name "log" responds to
 * commands that start "~log" and processes those. One special
 * subclass of Watcher handles "the rest".
 */
class Watcher
{
public:
    explicit Watcher(Bot* parent);
    virtual ~Watcher();
    
    /** @brief identifier of this module.
     * 
     * This is used to identify which commands go where; except for
     * the one special subclass BasicCommands, this must not be empty.
     */
    virtual const QString& moduleName() const = 0;
    
    /** @brief the (sub) commands that this module understands
     *
     * Returns a list of commands that this module uses.
     * Do not include the module name unless that is a valid subcommand.
     * Do not include "help".
     */
    virtual const QStringList& moduleCommands() const = 0;
    
    /** @brief Handle "virtual" message sent by the bot
     * 
     * The default implementation does nothing.
     */
    virtual void handleMessage(const QString&);
    /// @brief Handle message from the Matrix server
    virtual void handleMessage(const QMatrixClient::RoomMessageEvent*) = 0;
    /** @brief Called **after** handleMessage() for those containing a command.
     * 
     * If the watcher has a name (e.g. "log") then it responds to commands
     * that start with ~log (again, assuming the default command-character).
     * The command that arrives here is has already had that "prefix-command"
     * removed, so that the watcher only needs to handle the **sub**-commands
     * that it supports (e.g. "~log on", "~log off" ..).
     */
    virtual void handleCommand(const CommandArgs&) = 0;

protected:
    /// @brief human-readable version of the-command-for @p s with command-prefix
    QString displayCommand(const QString& s);
    /// @brief human-readable version of the-command-for-this-watcher
    QString displayCommand() { return displayCommand(this->moduleName()); }

    /// @brief Convenience for sending a message to the room
    void message(const QString& s) const { m_bot->message(s); }
    /// @brief Convenience for sending a message to the room
    void message(const QStringList& l) const { m_bot->message(l); }
    
    Bot* m_bot;
};
    
}
#endif
