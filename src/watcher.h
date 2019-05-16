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
    
    QString id;  // event Id, if available
    QString user;  // user Id, if available
    QString command;
    QStringList args;
};
    
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
    virtual QString moduleName() const = 0;
    
    virtual void handleMessage(const QMatrixClient::RoomMessageEvent*) = 0;
    virtual void handleCommand(const CommandArgs&) = 0;

protected:
    /// @brief human-readable version of the-command-for @p s with command-prefix
    QString displayCommand(const QString& s);
    QString displayCommand() { return displayCommand(this->moduleName()); }

    void message(const QString& s) const { m_bot->message(s); }
    void message(const QStringList& l) const { m_bot->message(l); }
    
    Bot* m_bot;
};
    
}
#endif
