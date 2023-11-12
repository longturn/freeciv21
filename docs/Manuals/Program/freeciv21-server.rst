.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

freeciv21-server
****************

SYNOPSIS
========

``freeciv21-server`` [ -f|--file `<FILE>` ] [ -l|--log `<FILE>` ] [ -p|--port `<PORT>` ]
[ -r|--read `<FILE>` ] [ -s|--saves `<DIR>` ] [ --ruleset `<RULESET>` ] [ -a|--auth ]
[ -D|--Database `<FILE>` ] [ -G|--Guests ] [ -N|--Newusers ] [ -h|--help ] [ -v|--version ]


DESCRIPTION
===========

.. This description is also in README.md, about.rst, the 4 metainfo files, and snapcraft.yaml

Freeciv21 is a free open source turn-based empire-building 4x strategy game, in which each player becomes the
leader of a civilization. You compete against several opponents to build cities and use them to support a
military and an economy. Players strive to complete an empire that survives all encounters with its neighbors
to emerge victorious. Play begins at the dawn of history in 4,000 BCE.

Freeciv21 takes its roots in the well-known FOSS game Freeciv and extends it for more fun, with a revived
focus on competitive multiplayer environments. Players can choose from over 500 nations and can play against
the computer or other people in an active online community.

The code is maintained by the team over at Longturn.net and is based on the QT framework. The game supports
both hex and square tiles and is easily modified to create custom rules.

An HTML version of this manual page along with much more information is available on our documentation
website at https://longturn.readthedocs.io/.

This is the server `program` used to establish a Freeciv21 server. For more information on the game, refer
to freeciv21-client(6).

This manual page only lists the command line arguments for the server `program`. For details of the
directives a server administrator can use to control aspects of the server, see freeciv21-game-manual(6).


OPTIONS
=======

The following options are accepted on the command line of the server. They may not be combined as with other
tools. For example: ``freeciv21-server -fp savegame.sav 5556`` will not work. Instead you will need to enter
each option separately, such as, ``freevic21-server -s savegame.sav -p 5556``.

.. include:: /Manuals/Server/command-line.rst
    :start-line: 15


EXAMPLES
========

Here are some examples:

freeciv21-server --file oldgame.sav -p 2224
  Starts a server on port `2224`, loading the savegame named `oldgame.sav`.

freeciv21-server -R ranklog -l logfile -r start-script -f oldgame.sav.xz -p 2224
  Starts a server on port `2224`, loading the savegame named `oldgame.sav.xz`. Player ranking events are
  logged to `ranklog`, other logging events are placed in `logfile`. When the server first starts, it
  immediately executes the server settings script named `start-script`. See freeciv21-server-manual(6) for
  more information on the server settings script.

freeciv21-server -a -D users.conf -G -N -q 60 -l logfile -s ./saves -p 2224
  Starts a server on port `2224` with authentication enabled (`-a`, `-D`, `-G`, and `-N`). If no activity
  for 60 seconds the server will quit (`-q`). Logging events are placed in `logfile` and periodic saves are
  placed in the `./saves` directory.


FILES
=====

The freeciv21-server requires the default ruleset files to be readable at start.

* civ2civ3/buildings.ruleset
* civ2civ3/cities.ruleset
* civ2civ3/effects.ruleset
* civ2civ3/game.ruleset
* civ2civ3/governments.ruleset
* civ2civ3/nations.rulest
* civ2civ3/styles.ruleset
* civ2civ3/techs.ruleset
* civ2civ3/terrain.ruleset
* civ2civ3/units.ruleset
* civ2civ3/script.lua
* default/default.lua
* default/nationlist.ruleset

These files are for the default ruleset for the game (civ2civ3), which are loaded if you do not use the
`--ruleset` arguement. Alternate rules can be loaded with the ``rulesetdir`` directive in a start up script.
Type ``help rulesetdir`` at the server command prompt for more information.

If you are going to enable user authentication, you will need the `database.lua` file. The file is
available on the GitHub repository (see below).


ENVIRONMENT
===========

The Freeciv21 server accepts these environment variables:

FREECIV_CAPS
  A string containing a list of "capabilities" provided by the server. The compiled default should be
  correct for most purposes, but if you are familiar with the capability string facility in the source you
  may use it to enforce some constraints between clients and server.

FREECIV_COMPRESSION_LEVEL
  Sets the compression level for network traffic.

FREECIV_MULTICAST_GROUP
  Sets the multicast group (for the LAN tab).

FREECIV_DATA_PATH
  A colon separated list of directories pointing to the Freeciv21 data directories. By default Freeciv21
  looks in the following directories, in order, for any data files: the current directory; the "data"
  subdirectory of the current directory; the subdirectory ".local/share/freeciv21" in the user's home
  directory; and the directory where the files are placed by running "cmake --target install".

FREECIV_SAVE_PATH
  A colon separated list of directories pointing to the Freeciv21 save directories. By default Freeciv21
  looks in the following directories, in order, for save files: the current directory; and the subdirectory
  ".local/share/freeciv21/saves" in the user's home directory.

  (This does not affect where the server creates save game files. See the `--saves` option for that.)

FREECIV_SCENARIO_PATH
  A colon separated list of directories pointing to the Freeciv21 scenario directories. By default Freeciv21
  looks in the following directories, in order, for scenario files: the current directory; the
  "data/scenarios" subdirectory of the current directory; the subdirectory
  ".local/share/freeciv21/scenarios" in the user's home directory; and the directory where the files are
  placed by running "cmake --target install".

  (This does not affect where the server creates scenario files. See the `--scenarios` option for that.)

HOME
  Specifies the user's home directory. For example, """http://my_proxy_address/""".

http_proxy
  Set this variable accordingly when using a proxy.

LANG or LANGUAGE
  Sets the language and locale on some platforms.

LC_ALL or LC_CTYPE
  Similar to LANG (see documentation for your system).

USER
  Specifies the username of the current user.


BUGS
====

Please report bugs to the Freeciv21 bug tracker at https://github.com/longturn/freeciv21/issues/new/choose

MORE INFORMATION
================

See the Longturn home page at https://longturn.net/. You can also find the code repository at
https://github.com/longturn/freeciv21/.

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
