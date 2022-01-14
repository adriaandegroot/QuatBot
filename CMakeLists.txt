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

option(COWSAY "Enables the ~cowsay command (requires /usr/local/bin/cowsay)" OFF)
option(COFFEE "Enables the ~coffee module" ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Multimedia Network)
find_package(Quotient 0.6.5 REQUIRED)

set(quatbot_SRCS
    src/main.cpp
    src/command.cpp
    src/logger.cpp
    src/meeting.cpp
    src/quatbot.cpp
    src/watcher.cpp
)

### OPTIONS HANDLING
#
#
if(COFFEE)
    list(APPEND quatbot_SRCS src/coffee.cpp)
    add_definitions(-DENABLE_COFFEE)
endif()
if(COWSAY)
    add_definitions(-DENABLE_COWSAY)
endif()

### TARGETS
#
#
add_executable(quatbot ${quatbot_SRCS})
target_link_libraries(quatbot
    PUBLIC
        Quotient
        Qt5::Core
        Qt5::Network
)
