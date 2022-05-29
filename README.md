<!-- SPDX-FileCopyrightText:  Copyright 2019 Adriaan de Groot <groot@kde.org>
     SPDX-License-Identifier: BSD-2-Clause
-->

# QuatBot

> QuatBot is a simple meeting-management bot for use with the Matrix.
> Taking turns during an online meeting -- and making sure everyone
> gets to have their say -- takes a bit of organizing, and this bot
> helps you do that. QuatBot runs as a command-line application.

## Usage

Once the bot is built, it supports the meeting workflow that
**I personally** participate in. It's not a general all-purpose
meeting-bot. The meetings are a kind of *stand-up*, that is,
a quick status-dump and question-and-answer session. For end-user
information about using the bot from a Matrix client, see the
[user documentation](USER.md).

The bot is controlled from the Matrix chat by text commands. Each command
is preceded by the **command-character**. By default this is `~` but that
can be changed, see `src/watcher.cpp`, the constant `COMMAND_PREFIX`.
The command-character **must** be the first character of
the message; that is, write `~command` and not `@bot: ~command` or
`please do ~command`.

All of the commands start with the *module name* that is responsible
for the command. Modules are subclasses of the `Watcher` class.
Adding more modules is relatively straightforward: implement
the API, instantiate the modules and add them in `Bot::setupWatchers()`.


## Building

Build-requirements for the bot are:
 - CMake, https://cmake.org/
 - Qt5 (Core, Multimedia, Network), https://www.qt.io/
 - libQuotient, https://github.com/quotient-im/libQuotient
 - make or ninja or similar, and a C++ compiler

QuatBot expects that CMake can find libQuotient though its
`QuotientConfig.cmake` file. If you don't have one (e.g. because
of an uninstalled or bundled libQuotient) you will need to
mess around with `-D` flags to CMake to tell it where the library
lives. I don't know what flags those are, though.

## Running

The bot can be run directly from the build directory. It can also be installed,
and has no other files than the single executable. The command-line
for the bot is:

```
quatbot <options> <room..>
```

Use `--help` to get an overview of command-line options. The most useful
ones will be:

 - `-u <user>` to set the user (Matrix user-id) to connect as.
 - `-o <user>` to add additional operators at startup.

You may be prompted for a Matrix password. You can set it on the command-line
with the `-p` option if you like.

**NOTE** that if the password is entered on the command-line, it remains
visible in command-history and in system tools like ps(1). Only run the
bot that way on a trusted host.

Example for a full command-line (note the `'` around each argument
to avoid shell processing of special characters):

```
quatbot -u '@adridg:matrix.org' '#quatbot-myownroom:matrix.org'
```

Logs are written to `/tmp`, in files named `quatbot-<something>.log`.
Meeting logs end up in nicely-named year-and-week logs, others will
get a timestamp or message-id as `<something>`. Note that people
abusing `~log` may create a lot of log files locally.

## Long-term Usage

QuatBot has not been audited for resource usage. It logs regularly to
standard out, which might be redirected to `/dev/null`, or saved somewhere.
Logs from meetings are stored in `/tmp`. It is probably possible to overwrite
logs, or otherwise mess around, if the people in the channel are malicious.

Use the `--operator` command-line argument to set more operators (admins)
for the bot at startup, e.g. by running it as follows:

```
USER="@quatbot:matrix.org"
OPS0="@adridg:matrix.org"
OPS1="@adridg-alt:matrix.org"
CHANNEL="#quatbot-myownroom:matrix.org"
quatbot --user $USER --operator $OPS0 --operator $OPS1 $CHANNEL
```

The bot also keeps a "database" in a writable location for coffee and tea usage,
called "cookiejar". This is persistent across starts of the bot, but is
of no importance whatsoever, since it's about the "amusement" module *coffee*.

## Dumper

There is an additional executable, qb-dumper, which connects to a room and
then logs it. It doesn't do anything else. It happens to get the last
100 lines of history as well (under normal circumstances) which makes
it "get" a bit of the past. This was added to be able to salvage the
log of a meeting from the past, and is of limited functionality.

The dumper has the same options as quatbot, and a `--since` argument
to indicate what chunk of history to retrieve.
