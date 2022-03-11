/*
 *  SPDX-License-Identifier: BSD-2-Clause
 *  SPDX-License-File: LICENSE
 *
 * Copyright 2019 Adriaan de Groot <groot@kde.org>
 */

/* This is the main entry for QuatBot, the meeting-management bot
 * for Matrix channels. It sets up command-line arguments, then
 * creates bot instances for each channel named, and then leaves
 * the rest to the Quotient library.
 */
#include "quatbot.h"

// For password-prompt
#include <pwd.h>
#include <unistd.h>

#include <QCommandLineParser>
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
    app.setApplicationName("QuatBot");
    app.setApplicationVersion("0.8");

    QCommandLineOption userOption(QStringList { "u", "user" }, "User to use to connect.", "user");
    QCommandLineOption passOption(
        QStringList { "p", "password" }, "Password to use to connect (will prompt if unset).", "password");
    QCommandLineOption operatorOption(
        QStringList { "o", "operator" }, "Additional user-id to consider as operator.", "userid");
    QCommandLineParser parser;
    parser.setApplicationDescription("Chatbot for meeting-management on Matrix");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(userOption);
    parser.addOption(passOption);
    parser.addOption(operatorOption);
    parser.addPositionalArgument("rooms", "Room names to join", "[rooms..]");
    parser.process(app);

    if (parser.positionalArguments().count() < 1)
    {
        qWarning() << "Usage: quatbot <options> <room..>\n"
                      "  Give at least one room name.\n";
        return 1;
    }

    QObject::connect(QMatrixClient::NetworkAccessManager::instance(),
                     &QNetworkAccessManager::sslErrors,
                     [](QNetworkReply* reply, const QList<QSslError>& errors) { reply->ignoreSslErrors(errors); });

    QMatrixClient::Connection conn;
    conn.connectToServer(parser.value(userOption),
                         parser.isSet(passOption) ? parser.value(passOption) : QString(getpass("Matrix password: ")),
                         "quatbot");  // user pass device

    QObject::connect(&conn,
                     &QMatrixClient::Connection::connected,
                     [&]()
                     {
                         qDebug() << "Connected to" << conn.homeserver() << "as" << conn.userId();
                         conn.setLazyLoading(false);
                         conn.syncLoop();
                         for (const auto& r : parser.positionalArguments())
                         {
                             // Unused, gets cleaned up by itself
                             QuatBot::Bot* bot = new QuatBot::Bot(conn, r, parser.values(operatorOption));
                         }
                     });
    QObject::connect(
        &conn, &QMatrixClient::Connection::loginError, []() { QTimer::singleShot(0, qApp, &QCoreApplication::quit); });

    return app.exec();
}
