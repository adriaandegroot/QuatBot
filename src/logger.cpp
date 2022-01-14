/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#include "logger.h"

#include "log_impl.h"
#include "quatbot.h"

#include <room.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace QuatBot
{
Logger::Logger(Bot* parent) :
    Watcher(parent),
    d(new LoggerFile)
{
}

Logger::~Logger()
{
	delete d;
}

const QString& Logger::moduleName() const
{
    static const QString name(QStringLiteral("log"));
    return name;
}

const QStringList& Logger::moduleCommands() const
{
    static const QStringList commands{"on","off","status"};
    return commands;
}

void Logger::handleMessage(const Quotient::RoomMessageEvent* event)
{
    d->log(event);
}

void Logger::handleMessage(const QString& s)
{
    d->log(s);
    d->flush();
}

static void report(Bot* bot, LoggerFile* file)
{
    if (!file->isOpen())
    {
        bot->message("(log) Logging is off.");
    }
    else if (file->lineCount() > 0)
    {
        bot->message(QString("(log) Logging to %1, %2 lines.").arg(file->fileName(), file->lineCount()));
    }
    else
    {
        bot->message(QString("(log) Logging to %1").arg(file->fileName()));
    }
}

void Logger::handleCommand(const CommandArgs& cmd)
{
    if (cmd.command == "on")
    {
        if (m_bot->checkOps(cmd))
        {
            bool quiet = false;
            int argIndex = 0;
            if ((cmd.args.count() > 0) && cmd.args.constFirst() == "?quiet")
            {
                quiet = true;
                argIndex = 1;
            }
            d->open(cmd.args.count() > argIndex ? cmd.args[argIndex] : cmd.id);
            d->log(QString("Log started %1.").arg(QDateTime::currentDateTime().toString()));
            d->flush();
            if (!quiet)
            {
                report(m_bot, d);
            }
        }
    }
    else if (cmd.command == "off")
    {
        if (m_bot->checkOps(cmd))
        {
            bool quiet = false;
            if ((cmd.args.count() > 0) && cmd.args.constFirst() == "?quiet")
            {
                quiet = true;
            }
            d->close();
            if (!quiet)
            {
                report(m_bot, d);
            }
        }
    }
    else if (cmd.command == "status")
    {
        report(m_bot, d);
    }
    else
    {
        message(Usage{});
    }
}

}
