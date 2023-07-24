..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

freeciv21-client
****************

SYNOPSIS
========

``freeciv21-client`` [ -f|--file `<FILE>` ] [ -l|--log `<FILE>` ] [ -n|--name `<NAME>` ]
[ -s|--server `<HOST>` ] [ -p|--port `<PORT>` ] [ -t|--tiles `<FILE>` ] [ -h|--help ] [ -v|--version ]


DESCRIPTION
===========

.. include:: freeciv21-server.rst
  :start-line: 17
  :end-line: 30

This is the client interface `program` used to connect to a Freeciv21 server. For more information on the
server, refer to freeciv21-server(6). The program can be launched without any command line parameters. A
player can use the main menu to connect to a running game, open an existing savegame, start a new game, or
start a new scenario game. However, if you wish to automate the connection to a running server or start a new
game with a server settings file, command line options are available.

OPTIONS
=======

The following options are accepted on the command line of the client. They may not be combined as with other
tools. For example: ``freeciv21-client -sp localhost 5556`` will not work. Instead you will need to enter
each option separately, such as, ``freeciv21-client -s localhost -p 5556``.

``-A, --Announce <PROTO>``
    Announce game in LAN using protocol PROTO (IPv4/IPv6/none), Default is ``IPv4``. Options include:
    ``IPv4``, ``IPv6``, or ``none``.

``-a, --autoconnect``
    Skip connection dialog (usually with `url` or ``-n``, ``-s``, and ``-p``).

``-d, --debug <LEVEL>``
    Set debug log level (fatal/critical/warning/info/debug). Default log level is ``info``.

``-F, --Fatal``
    Raise a signal on failed assertion. An assertion is a code calculation error. With this set, the client
    process will SEGFAULT instead of issuing a warning message to the terminal console.

``-f, --file <FILE>``
    Load saved game FILE. Useful when wanting to restart a game.

``-H, --Hackless``
    Do not request hack access to local, but not spawned, server.

``-l, --log <FILE>``
    Use FILE as logfile.

``-M, --Meta <HOST>``
    Connect to the metaserver at HOST.

.. note::
  With regard to the command line arguments concerning the metaserver, Freeciv21 does not have its own
  metaserver at this time and the legacy Freeciv metaserver does not support Freeciv21. This means, right now,
  that all commands related to the metaserver are held over from the fork from legacy Freeciv until the
  Longturn community creates a custom metaserver.


``-n, --name <NAME>``
    Use NAME as username on server.

``-p, --port <PORT>``
    Connect to server port PORT (usually with ``-a``).

``-P, --Plugin <PLUGIN>``
    Use PLUGIN for sound output []. The default is `SDL`.

``-r, --read <FILE>``
    Read startup script FILE. Options are passed to the spawned server.

``-s, --server <HOST>``
    Connect to the server at HOST (usually with ``-a``).

``-S, --Sound <FILE>``
    Read sound tags from FILE. Default is `stdsounds.soundspec`.

``-m, --music <FILE>``
    Read music tags from FILE. Default is `stdmusic.musicspec`.

``-t, --tiles <FILE>``
    Use data file `FILE.tilespec` for tiles.

``-w, --warnings``
    Warn about deprecated modpack constructs.

``-h, --help``
    Display help on command line options.

``--help-all``
    Display help including Qt specific options.

``-v, --version``
    Display version information.

``url``
    Server information in URL format. See examples below.

EXAMPLES
========

Here are some examples:

freeciv21-client -n mycoolusernname -s localhost -p 2004
  Connect to an already running server with a username of `mycoolusername`, server host name of `localhost`,
  and port of `2004`.

freeciv21-client """fc21://[username]:[password]@[server]:[port]""" -a -t amplio2
  Connect to an already running server using `URL` format. The parameters `[username]`, `[password]`,
  `[server]`, and `[port]` are replaced with valid values. This command also passes the autoconnect option
  (``-a``) as well as loads the Amplio2 Tileset.

.. note::
  The communication between the client and the server is plain text, so make sure the password you use is
  unique. This is especially true for Longturn games on the open Internet.

ENVIRONMENT
===========

The Freeciv21 client accepts these environment variables:

FREECIV_DATA_PATH
  A colon separated list of directories pointing to the Freeciv21 data directories. By default Freeciv21
  looks in the following directories, in order, for any data files: the current directory; the "data"
  subdirectory of the current directory; the subdirectory ".local/share/freeciv21" in the user's home
  directory; and the directory where the files are placed by running "cmake --target install".

HOME
  Specifies the user's home directory.

http_proxy
  Set this variable accordingly when using a proxy. For example, """http://my_proxy_address/""".

LANG or LANGUAGE
  Sets the language and locale on some platforms.

LC_ALL or LC_CTYPE
  Similar to LANG (see documentation for your system).

USER
  Specifies the username of the current user.

.. include:: freeciv21-server.rst
  :start-line: 148
