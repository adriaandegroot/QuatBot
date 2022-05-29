<!-- SPDX-FileCopyrightText:  Copyright 2019 Adriaan de Groot <groot@kde.org>
     SPDX-License-Identifier: BSD-2-Clause
-->

# QuatBot User Guide

> QuatBot is a simple meeting-management bot for use with the Matrix.
> Taking turns during an online meeting -- and making sure everyone
> gets to have their say -- takes a bit of organizing, and this bot
> helps you do that.

## Usage

QuatBot supports the meeting workflow that **I personally** participate in.
It's not a general all-purpose meeting-bot, although it has facilities
for many things and is easily extended. The meetings are a kind of *stand-up*,
that is, a quick status-dump and question-and-answer session. The meeting
runs as follows:

 - we agree on a meeting time and Matrix room
 - when the meeting starts, check who's really online
 - ask each person participating what their status is; they info-dump
 - possibly questions or comments; sometimes we decide we need a breakout
 - move on to the next person
 - when we're done, remind us of what breakouts we need.

The bot also has some extra possibilities (logging, and entertainment
for the easily-amused) but its primary focus is handling the meeting.

The bot is controlled from the Matrix chat by text commands. Each command
is preceded by the **command-character**. By default this is `~` but that
can be changed. The command-character **must** be the first character of
the message; that is, write `~command` and not `@bot: ~command` or
`please do ~command`.

All of the commands start with the *module name* that is responsible
for the command. By default, there are five modules, all described below.
Each module (e.g. the *log* module) understands a collection of **sub**commands
(e.g. the *quatbot* module understands subcommand *echo*). If a subcommand
is not ambiguous, you can leave out the module name and write the subcommand
as a command. That is, `~echo` is the same as `~quatbot echo` as long
as no other module defines an *echo* subcommand.

### Commands - General

Commands that are general, not tied to the meeting:

 - `~quatbot echo` The bot will reply with whatever follows on the same line.
 - `~quatbot fortune` The bot will reply with a random fortune.
 - `~quatbot cowsay` The bot will reply with wisdom from cows.
   (*Optional*, may be disabled at build-time).
 - `~quatbot status` The bot will reply with some internal counters **and**
   the status message from each other module.
 - `~quatbot help` The bot will reply with a list of modules, or use
   `~quatbot help <name..>` for a list of commands for the named modules.

Commands that are general, but only available to the bot's **operator**:

 - `~quatbot quit` Leave the room.


### Commands - Meeting Administrators

The bot connects to Matrix under a given user-name. At the outset, only that
user is a operator, so only that user can perform administrative actions.
Other users can be added to allow better control (but really, there's not
much call for that outside of `~quit`).

Commands related to operator status, available to all:

 - `~quatbot ops status` Lists current operators.

Commands related to operator status, only available to the **operator**:

 - `~quatbot ops add <name..>` Add people to the list of operators.
   You can use `+` or `op` as synonyms for `add`.
 - `~quatbot ops remove <name..>` Remove people from the list of operators.
   You can use `-` or `deop` as synonyms for `remove`.


### Commands - Logging

The bot can log messages to a file (in `/tmp` on the machine where it is
running). Each message is logged with a timestamp and username in a fairly
flat file format.

Commands related to logging, available to all:

 - `~log status` The bot will reply with some internal counters.

Commands related to logging, only available to the **operator**:

 - `~log on` Switches on logging. Starts a log with a filename based on
   the message ID. Tells the room that logging is on and where the log
   is stored (although this is a path on the local machine, so useless
   to most people). You can add `?quiet` to suppress the message
   to the room. You can also add a filename, which is massaged and used
   on the host where the bot runs.
 - `~log off` Stops logging and saves the file.


### Commands - Meeting

The bot's main purpose is to run a meeting. The meeting has a *chair*
who is the "boss" of the meeting. The chair helps the meeting proceed.
Anyone can start a meeting (if there is not one in progress) and by
starting the meeting, that person becomes the chair of that meeting.
Operators can influence the meeting as well.

Commands related to meetings, available to all:

 - `~meeting status` The bot will reply with some internal counters.
 - `~meeting rollcall` This is available only if no meeting is currently
   in progress. The person who issues the command becomes the *chair*
   of this meeting. The bot replies with a roll-call announcement,
   naming every person in the room. Persons that reply to the roll-call
   (e.g. "here") are marked as present. After one minute, a repeat roll-call
   for those that have not yet responded is sent.
 - `~meeting breakout <id> [description]` Registers a breakout with the
   given description. This is available during the meeting (not during
   roll-call). At the end of the meeting, any breakouts that have been
   registered are listed with their description. It's ok for *description*
   to be blank. The first person to register a breakout with a given
   *id* is the chair of that breakout.
 - `~meeting breakout <id>` Anyone else can **also** register for a
   breakout. The Matrix Ids that register for a breakout are mentioned
   at the end.
 - `~meeting queue [n]` shows the next *n* participants (defaults to all)
   in the meeting queue.

Commands related to meetings, available to the *chair* and **operator**:

 - `~meeting next` Calls upon the next person in the meeting to provide
   their info-dump. The *chair* is always last. If the person named does
   not respond within 30 seconds, a repeat call is made.
 - `~meeting skip <name..>` Removes a person from the roll. This can be
   done to a participant so they will not be called upon. It can also
   be done during the roll-call to exclude, e.g., other bots from the
   roll-call.
 - `~meeting bump <name..>` Moves a person to the front of the queue
   of participants. In principle participants are called upon in the
   order in which they respond during roll-call. This command allows
   some changes in the order (e.g. if someone has to leave soon).
   Use numbers to bump participants to a specific spot, e.g.
   `~meeting bump 3 fred charlie 2 kate` to get Kate, Fred, and then
   Charlie into order (but after the current speaker and after whoever's
   next).

Commands related to meetings, available to the **operator**:

 - `~meeting done` Ends the meeting, no questions asked.

### Commands - Coffee

As an amusement, the bot also contains a *coffee* module which manages
a virtual coffee machine and a cookie jar. This has no real functionality.

Commands related to coffee, available to all:

 - `~coffee status` Look in the cookie jar.
 - `~coffee coffee` Drink a cup of coffee. You can also just write `~coffee`.
 - `~coffee tea` Because there is more than one beverage. You may write `~tea`.
 - `~coffee cookie eat` Eat a cookie (if you have one). You can also just write
   `~coffee cookie` or `~cookie`.
 - `~coffee cookie give <name..>` Give cookies to other people.
 - `~coffee stats` Give statistics on coffee and cookie usage.
 - `~coffee lart` Suggest behavioral improvements.


