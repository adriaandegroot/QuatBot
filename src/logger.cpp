/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "logger.h"

#include <room.h>


namespace QuatBot
{
class Logger::Private
{
public:
    Private();
    virtual ~Private();
    
    void log(const QString& message);
    
private:
    int m_lines;
};

Logger::Private::Private() :
    m_lines(0)
{
}

Logger::Private::~Private()
{
}

void Logger::Private::log(const QString& message)
{
    ++m_lines;
}

Logger::Logger(Bot* parent) :
    Watcher(parent),
    d(new Private)
{
}

Logger::~Logger()
{
	delete d;
}

void Logger::handleMessage(QMatrixClient::Room* room, const QMatrixClient::RoomMessageEvent* event)
{
    QString message = event->plainBody();
    d->log(message);
}

}
