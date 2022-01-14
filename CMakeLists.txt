# A Matrix chatbot based on libqmatrixclient and Qt5
#
#   SPDX-License-Identifier: BSD-2-Clause
#   SPDX-License-File: LICENSE
#
# This bot is carefully tuned to my personal needs in a chatbot,
# so it does meeting support, mostly.

cmake_minimum_required(VERSION 3.18)

project(quatbot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(
    COWSAY
    "Enables the ~cowsay command (requires /usr/local/bin/cowsay)"
    OFF
)
option(COFFEE "Enables the ~coffee module" ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Multimedia Network)
find_package(Quotient 0.6.5 REQUIRED)

### TARGETS
#
#
add_executable(
    quatbot
    src/main.cpp
    src/log_impl.cpp
    src/command.cpp
    src/logger.cpp
    src/meeting.cpp
    src/quatbot.cpp
    src/watcher.cpp
)
target_link_libraries(quatbot PUBLIC Quotient Qt5::Core Qt5::Network)

add_executable(qb-dumper src/dumper.cpp src/dumpbot.cpp src/log_impl.cpp)
target_link_libraries(qb-dumper PUBLIC Quotient Qt5::Core Qt5::Network)

### OPTIONS HANDLING
#
#
if(COFFEE)
    target_sources(quatbot PUBLIC src/coffee.cpp)
    target_compile_definitions(quatbot PUBLIC ENABLE_COFFEE)
endif()
if(COWSAY)
    target_compile_definitions(quatbot PUBLIC DENABLE_COWSAY)
endif()
