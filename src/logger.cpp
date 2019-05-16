/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "logger.h"

#include <room.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace QuatBot
{
class Logger::Private
{
public:
    Private();
    virtual ~Private();
    
    void log(const QMatrixClient::RoomMessageEvent* message);
    void report(QMatrixClient::Room* r);

    void open(const QString& name);
    void close();
    bool isOpen() const
    {
        return m_stream != nullptr;
    }
    QString fileName() const
    {
        return m_file ? m_file->fileName() : QString();
    }
    
private:
    QFile* m_file = nullptr;
    QTextStream* m_stream = nullptr;
    int m_lines;
    
    QString makeName(QString);  // Copied because it is modified in the method
};

Logger::Private::Private() :
    m_lines(0)
{
}

Logger::Private::~Private()
{
    close();
}

void Logger::Private::close()
{
    if (m_stream)
    {
        delete m_stream;
        m_stream = nullptr;
    }
    if (m_file)
    {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    m_lines = -1;
}

void Logger::Private::open(const QString& name)
{
    close();
    
    QFile* f = new QFile(makeName(name));
    if (!f)
    {
        qCritical() << "Could not allocate QFile" << name;
        return;
    }
    if (!f->open(QFile::WriteOnly))
    {
        qCritical() << "Could not open" << f->fileName();
        delete f;
        return;
    }
    
    m_file = f;
    
    QTextStream* t = new QTextStream(m_file);
    if (!t)
    {
        qCritical() << "Could not allocate QTextStream for" << name;
        close();
        return;
    }
    m_stream = t;
    m_lines = 0;
}

void Logger::Private::log(const QMatrixClient::RoomMessageEvent* message)
{
    if (m_stream)
    {
        ++m_lines;
        (*m_stream) << qSetFieldWidth(6) << m_lines << qSetFieldWidth(-1)
            << '\t' << message->timestamp().toString(Qt::DateFormat::ISODate)
            << '\t' << message->senderId()
            << '\t' << message->plainBody()
            << '\n';
    }
}

void Logger::Private::report(QMatrixClient::Room* room)
{
    if (!isOpen())
    {
        Watcher::message(room, "Logging is off.");
    }
    else if (m_lines > 0)
    {
        Watcher::message(room, QString("Logging is on, %1 lines.").arg(m_lines));
    }
    else
        Watcher::message(room, QString("Logging to %1").arg(m_file->fileName()));
}
        

QString Logger::Private::makeName(QString s)
{
    if (s.isEmpty())
        return QString("/tmp/quatbot.log");
    return QString("/tmp/quatbot-%1.log").arg(s.remove(QRegularExpression("[^a-zA-Z0-9_]")));
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
    d->log(event);
    if (isCommand(event))
        handleCommand(room, CommandArgs(event));
}

void Logger::handleCommand(QMatrixClient::Room* room, const CommandArgs& cmd)
{
    if (cmd.command == "log")
    {
        bool statusReport = true;
        if (cmd.args.count() < 1)
        {
            message(room, QString("Usage: %1 <on|off|status>").arg(displayCommand("log")));
            statusReport = false;
        }
        else if (cmd.args.first() == "on")
            d->open(cmd.id);
        else if (cmd.args.first() == "off")
            d->close();
        else if (cmd.args.first() == "status")
            ;  // Nothing, the status report is the side-effect
        else
        {
            message(room, QString("Unknown %1-subcommand %2").arg(displayCommand("log"), cmd.args.first()));
            statusReport = false;
        }
        
        if (statusReport)
            d->report(room);
    }
}

}
