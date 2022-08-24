..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Format Description of the Scorelog
**********************************

Empty lines and lines starting with ``#`` are comments. Each non-comment line starts with a command. The
parameter are supplied on that line and are seperated by a space. Strings which may contain whitespaces
are always the last parameter and so extend until the end of line.

The following commands exists:

* :code:`id <game-id>` : :code:`<game-id>` is a string without whitespaces which is used to match a scorelog
  against a savegame.

* :code:`tag <tag-id> <descr>` : Add a data-type (tag) the :code:`<tag-id>` is used in the 'data' commands
  :code:`<descr>` is a string without whitespaces which identified this tag.

* :code:`turn <turn> <number> <descr>` : Adds information about the :code:`<turn>` turn :code:`<number>` can
  be for example year :code:`<descr>` may contain whitespaces.

* :code:`addplayer <turn> <player-id> <name>` : Adds a player starting at the given turn (inclusive).
  :code:`<player-id>`  is a number which can be reused :code:`<name>` may contain whitespaces.

* :code:`delplayer <turn> <player-id>` : Removes a player from the game. The player was active till the given
  turn (inclusive) :code:`<player-id>` used by the creation.

* :code:`data <turn> <tag-id> <player-id> <value>` : Gives the value of the given tag for the given player for
  the given turn.
