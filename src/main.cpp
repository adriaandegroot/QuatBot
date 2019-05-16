/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */   

#include "quatbot.h"

// For password-prompt
#include <pwd.h>
#include <unistd.h>

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

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 4)
    {
        qWarning() << "Usage: quatbot <user> <pass> <room..>\n"
            "  Give at least one room name.\n"
            "  Use - as a password to prompt for it.";
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
    conn.connectToServer(argv[1], 
        (argv[2][0]=='-' && !argv[2][1]) ? getpass("Matrix password") : argv[2], 
        "quatbot");  // user pass device
    
    QObject::connect(&conn,
        &QMatrixClient::Connection::connected,
        [&]()
        {
            qDebug() << "Connected to" << conn.homeserver() << "as" << conn.userId();
            conn.setLazyLoading(true);
            conn.syncLoop();
            for (int r=3; r < argc; ++r)
            {
                // Unused, gets cleaned up by itself
                QuatBot::Bot* bot = new QuatBot::Bot(conn, argv[r]);
            }
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
