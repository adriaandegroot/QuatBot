/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#ifndef QUATBOT_WATCHER_H
#define QUATBOT_WATCHER_H

#include <QString>
#include <QStringList>

namespace QMatrixClient
{
    class Room;
    class RoomMessageEvent;
}

namespace QuatBot
{
class Bot;

struct CommandArgs
{
    explicit CommandArgs(QString);   // Copied because it's modified in the method
    explicit CommandArgs(const QMatrixClient::RoomMessageEvent*);
    
    static bool isCommand(const QString& s);
    static bool isCommand(const QMatrixClient::RoomMessageEvent*);
    
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
    
    virtual void handleMessage(QMatrixClient::Room*, const QMatrixClient::RoomMessageEvent*) = 0;
    virtual void handleCommand(QMatrixClient::Room*, const CommandArgs&) = 0;

    // Duplicated for convenience
    static bool isCommand(const QString& s) { return CommandArgs::isCommand(s); }
    static bool isCommand(const QMatrixClient::RoomMessageEvent* e) { return CommandArgs::isCommand(e); }
    
    static void message(QMatrixClient::Room* room, const QStringList& l);
    static void message(QMatrixClient::Room* room, const QString& s);
    
protected:
    /// @brief human-readable version of the-command-for @p s with command-prefix
    QString displayCommand(const QString& s);
    QString displayCommand() { return displayCommand(this->moduleName()); }
    
    Bot* m_bot;
};
    
}
#endif
