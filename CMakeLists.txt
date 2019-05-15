# A Matrix chatbot based on libqmatrixclient and Qt5
#
#   SPDX-License-Identifier: BSD-2-Clause
#   SPDX-License-File: LICENSE
#
# This bot is carefully tuned to my personal needs in a chatbot,
# so it does meeting support, mostly.

cmake_minimum_required(VERSION 3.10)

project(quatbot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.10 REQUIRED COMPONENTS Core Gui Multimedia Network)
find_package(QMatrixClient REQUIRED)

add_executable(quatbot
    src/main.cpp
    src/command.cpp
)
target_link_libraries(quatbot
    PUBLIC 
        QMatrixClient  # It pulls in Qt5 bits
)
