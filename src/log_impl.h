/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

#ifndef QUATBOT_LOG_IMPL_H
#define QUATBOT_LOG_IMPL_H

#include <room.h>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace QuatBot
{

class MessageData;

class LoggerFile
{
public:
    LoggerFile();
    virtual ~LoggerFile();

    void log(const Quotient::RoomMessageEvent* message);
    void log(const QString& s);
    void log(const MessageData& message);

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
    int lineCount() const
    {
        return m_lines;
    }
    void flush();

private:
    QFile* m_file = nullptr;
    QTextStream* m_stream = nullptr;
    int m_lines = 0;

    QString makeName(QString);  // Copied because it is modified in the method
};

}  // namespace
#endif
