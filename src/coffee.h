/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#ifndef QUATBOT_COFFEE_H
#define QUATBOT_COFFEE_H

#include "watcher.h"

#include <QString>
#include <QStringList>

namespace QuatBot
{

/** @brief KoffiePot -- an amusing bot
 *
 * This is a throwback to #koffie on early '90s EFNet, where a bot
 * kept watch over a pot of coffee and a jar of cookies. Users could
 * talk to the bot and give each other cookies.
 */
class Coffee : public Watcher
{
    class Private;

public:
    Coffee(Bot* parent);
    virtual ~Coffee() override;

    const QString& moduleName() const override;
    const QStringList& moduleCommands() const override;

    virtual void handleMessage(const Quotient::RoomMessageEvent*) override;
    virtual void handleCommand(const CommandArgs&) override;

protected:
    void handleCookieCommand(const CommandArgs&);
    bool handleMissingVerb(const CommandArgs&);
    
private:
    Private* d;
};

}  // namespace
#endif
