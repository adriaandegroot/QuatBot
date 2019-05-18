/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "logger.h"

#include "quatbot.h"

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
    void log(const QString& s);
    void report(Bot*);

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

// make copies because we'll be modifying them
static void logX(QTextStream& s, QString timestamp, QString sender, QString message)
{
    timestamp.truncate(19);
    if (sender.contains(':'))
        sender.truncate(sender.indexOf(':'));
    sender.truncate(12);
    
    // Just the HH:mm:ss
    s << timestamp.right(8).leftJustified(8)
        << ' ' << qSetFieldWidth(12) << sender << qSetFieldWidth(-1) << '\t';
    if (message.contains('\n'))
    {
        QStringList parts = message.split('\n');
        bool first = true;
        for (const auto& p : parts)
        {
            if (!first)
            {
                // 8, space, 12, tab
                s << "        " << ' ' << "            " << '\t'; 
            }
            first = false;
            s << p << '\n';
        }
    }
    else
    {
        s << message << '\n';
    }
}

void Logger::Private::log(const QString& s)
{
    if (m_stream)
    {
        ++m_lines;
        logX(*m_stream, QString(), QString(), s);
    }
}

void Logger::Private::log(const QMatrixClient::RoomMessageEvent* message)
{
    if (m_stream)
    {
        ++m_lines;
        logX(*m_stream, message->timestamp().toString(Qt::DateFormat::ISODate), message->senderId(), message->plainBody());
    }
}

void Logger::Private::report(Bot* bot)
{
    if (!isOpen())
    {
        bot->message("Logging is off.");
    }
    else if (m_lines > 0)
    {
        bot->message(QString("Logging is on, %1 lines.").arg(m_lines));
    }
    else
        bot->message(QString("Logging to %1").arg(m_file->fileName()));
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

void Logger::handleMessage(const QMatrixClient::RoomMessageEvent* event)
{
    d->log(event);
}

void Logger::handleMessage(const QString& s)
{
    d->log(s);
}

void Logger::handleCommand(const CommandArgs& cmd)
{
    bool statusReport = true;
    if (cmd.args.count() > 0)
    {
        message(QString("Usage: %1 <on|off|status>").arg(displayCommand()));
        statusReport = false;
    }
    else if (cmd.command == "on")
    {
        if (m_bot->checkOps(cmd))
        {
            d->open(cmd.id);
        }
    }
    else if (cmd.command == "off")
    {
        if (m_bot->checkOps(cmd))
        {
            d->close();
        }
    }
    else if (cmd.command == "status")
    {
        ;  // Nothing, the status report is the side-effect
    }
    else
    {
        message(QString("Usage: %1 <on|off|status>").arg(displayCommand()));
        statusReport = false;
    }
    
    if (statusReport)
    {
        d->report(m_bot);
    }
}

}
