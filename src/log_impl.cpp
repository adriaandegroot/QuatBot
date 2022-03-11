/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#include "log_impl.h"

// Slightly weird: non-Quotient type for logging
#include "dumpbot.h"

#include <room.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace QuatBot
{
LoggerFile::LoggerFile() :
    m_lines(0)
{
}

LoggerFile::~LoggerFile()
{
    close();
}

void LoggerFile::close()
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

void LoggerFile::open(const QString& name)
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

    qDebug() << "Logging to" << m_file->fileName();
}

void LoggerFile::flush()
{
    if (m_stream)
    {
        m_stream->flush();
    }
    if (m_file)
    {
        m_file->flush();
    }
}

template<class stream> void eol(stream& s) { s << '\n'; }
template<> void eol<QDebug>(QDebug&) { }

// make copies because we'll be modifying them
template<class stream>
void logX(stream& s, QString timestamp, QString sender, QString message)
{
    timestamp.truncate(19);
    if (sender.contains(':'))
        sender.truncate(sender.indexOf(':'));
    sender.truncate(12);

    // Just the HH:mm:ss
    s << timestamp.right(8).leftJustified(8)
        << ' ' << sender.leftJustified(12) << '\t';
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
        s << message;
        eol(s);
    }
}

void LoggerFile::log(const QString& s)
{
    if (m_stream)
    {
        ++m_lines;
        logX(*m_stream, QString(), QStringLiteral("*BOT*"), s);
    }

    auto d = qDebug().noquote().nospace();
    logX(d, QString(), QStringLiteral("*BOT*"), s);
}

void LoggerFile::log(const QMatrixClient::RoomMessageEvent* message)
{
    if (m_stream)
    {
        ++m_lines;
        logX(*m_stream, message->originTimestamp().toString(Qt::DateFormat::ISODate), message->senderId(), message->plainBody());
    }

    auto d = qDebug().noquote().nospace();
    logX(d, message->originTimestamp().toString(Qt::DateFormat::ISODate), message->senderId(), message->plainBody());
}

void LoggerFile::log(const QuatBot::MessageData& message)
{
    if (m_stream)
    {
        ++m_lines;
        logX(*m_stream, message.originTimestamp().toString(Qt::DateFormat::ISODate), message.senderId(), message.plainBody());
    }

    auto d = qDebug().noquote().nospace();
    logX(d, message.originTimestamp().toString(Qt::DateFormat::ISODate), message.senderId(), message.plainBody());
}


QString LoggerFile::makeName(QString s)
{
    if (s.isEmpty())
    {
        return QString("/tmp/quatbot.log");
    }
    return QString("/tmp/quatbot-%1.log").arg(s.remove(QRegularExpression("[^a-zA-Z0-9_]")));
}

}
