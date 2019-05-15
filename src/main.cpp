/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include <QCoreApplication>
#include <QDebug>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

#include <connection.h>
#include <networkaccessmanager.h>
#include <room.h>

#include <csapi/joining.h>
#include <events/roommessageevent.h>

#include "command.h"

static constexpr const QChar COMMAND_PREFIX(0x1575); // ᕵ Nunavik Hi

class QuatBot : public QObject
{
public:
    explicit QuatBot(QMatrixClient::Connection& conn, const QString& roomName) :
        QObject(),
        m_conn(conn),
        m_roomName(roomName)
    {
        if (conn.homeserver().isEmpty() || !conn.homeserver().isValid())
        {
            qWarning() << "Connection is invalid.";
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
            return;
        }
            
        qDebug() << "Using home=" << conn.homeserver() << "user=" << conn.userId();
        auto* joinRoom = conn.joinRoom(roomName);
        
        connect(joinRoom, &QMatrixClient::BaseJob::failure,
            [this]() 
            {
                qWarning() << "Joining room" << this->m_roomName << "failed.";
                QTimer::singleShot(0, qApp, &QCoreApplication::quit);
            }
        );
        connect(joinRoom, &QMatrixClient::BaseJob::success,
            [this, joinRoom]()
            {
                m_commands = new CommandHandler( COMMAND_PREFIX );
                
                qDebug() << "Joined room" << this->m_roomName << "successfully.";
                m_room = m_conn.room(joinRoom->roomId(), QMatrixClient::JoinState::Join);
                connect(m_room, &QMatrixClient::Room::baseStateLoaded, this, &QuatBot::baseStateLoaded);
                connect(m_room, &QMatrixClient::Room::addedMessages, this, &QuatBot::addedMessages);
            }
        );
    }
    
    virtual ~QuatBot() override
    {
        if(m_room)
            m_room->leaveRoom();
        delete m_commands;
    }
    
protected:
    void baseStateLoaded()
    {
        qDebug() << "Room base state loaded"
            << "id=" << m_room->id()
            << "name=" << m_room->displayName()
            << "topic=" << m_room->topic();
    }
    
    void addedMessages(int from, int to)
    {
        qDebug() << "Room messages" << from << '-' << to;
        if (m_newlyConnected)
        {
            qDebug() << ".. Ignoring them";
            m_room->markMessagesAsRead(m_room->readMarker()->event()->id());
            m_newlyConnected = false;
            return;
        }
        
        const auto& timeline = m_room->messageEvents();
        for (int it=from; it<=to; ++it)
        {
            const QMatrixClient::RoomMessageEvent* event = timeline[it].viewAs<QMatrixClient::RoomMessageEvent>();
            if (event)
            {
                QString message = event->plainBody();
                
                if(m_commands->isCommand(message))
                {
                    m_commands->handleCommand(m_room, m_commands->extractCommand(message));
                }
            }
        }
        m_room->markMessagesAsRead(timeline[to]->id());
    }
    
private:
    QMatrixClient::Room* m_room = nullptr;
    CommandHandler* m_commands = nullptr;
    
    QMatrixClient::Connection& m_conn;
    QString m_roomName;
    bool m_newlyConnected = true;
};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 4)
    {
        qWarning() << "Usage: quatbot <user> <pass> <room>";
        return 1;
    }

    QObject::connect(QMatrixClient::NetworkAccessManager::instance(),
            &QNetworkAccessManager::sslErrors,
            [](QNetworkReply* reply, const QList<QSslError>& errors)
            {
                reply->ignoreSslErrors(errors);
            }
    );

    QMatrixClient::Connection conn;
    conn.connectToServer(argv[1], argv[2], "quatbot");  // user pass device
    
    QObject::connect(&conn,
        &QMatrixClient::Connection::connected,
        [&]()
        {
            qDebug() << "Connected to" << conn.homeserver();
            conn.setLazyLoading(false);
            conn.syncLoop();
            QuatBot* bot = new QuatBot(conn, argv[3]);
        }
    );
    QObject::connect(&conn,
        &QMatrixClient::Connection::loginError,
        []()
        {
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        }
    );
   
    return app.exec();
}
