.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Server Command Line Options
***************************

The ``freeciv21-server`` program has a collection of command-line options that can be used to control the
behavior of the server when run. To get a listing of all the options, you can use the ``--help`` option,
such as:

.. code-block:: sh

    $ ./freeciv21-server --help


.. tip::
  It is generally considered a best practice to write a ``.sh`` script to run your server. This way you do not
  have to remember all the command line options to use every time you run your server.

``-A, --Announce <PROTO>``
    Announce game in LAN using protocol PROTO (IPv4/IPv6/none). Default is ``IPv4``. Options include:
    ``IPv4``, ``IPv6``, or ``none``.

``-B, --Bind-meta <ADDR>``
    Connect to metaserver from this address. Default will be your public IP address assigned from your
    internet service provider.

.. note::
  With regard to the command line arguments concerning the metaserver, Freeciv21 does not have its own
  metaserver at this time and the legacy Freeciv metaserver does not support Freeciv21. This means, right now,
  that all commands related to the metaserver are held over from the fork from legacy Freeciv until the
  Longturn community creates a custom metaserver.


``-b, --bind <ADDR>``
    Listen for clients on ADDR. Default is the IP address of the server computer.

``-d, --debug <LEVEL>``
    Set debug log level (fatal/critical/warning/info/debug). Default log level is ``info``.

``-e, --exit-on-end``
    When a game ends, exit instead of restarting.

``-F, --Fatal``
    Raise a signal on failed assertion. An assertion is a code calculation error. With this set, the server
    process will SEGFAULT instead of issuing a warning message to the terminal console.

``-f, --file <FILE>``
    Load saved game FILE. Useful when wanting to restart a game.

``-i, --identity <ADDR>``
    Be known as ADDR at metaserver or LAN client.

``-k, --keep``
    Keep updating game information on metaserver even after failure. Default is not to keep updating.

``-l, --log <FILE>``
    Use FILE as logfile. Generally a very good idea when running a server.

``-M, --Metaserver <ADDR>``
    Set ADDR as metaserver address. Allows you to point to a specific metaserver instead of the default:
    https://meta.freeciv.org/

``-m, --meta``
    Notify metaserver and send server's info.

``-p, --port <PORT>``
    Listen for clients on port PORT. Default is 5556 as assigned by
    `IANA <https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=5556>`_

``-q, --quitidle <TIME>``
    Quit if no players for TIME seconds.

``-R, --Ranklog <FILE>``
    Use FILE as ranking logfile. Generally a very good idea when running a server.

``-r, --read <FILE>``
    Read startup file FILE.

``-S, --Serverid <ID>``
    Sets the server id to ID.

``-s, --saves <DIR>``
    Save games to directory DIR. Generally a very good idea to save games at least every turn and depending on
    how long the turns are set to, to save within a turn. In the case of a server crash, restarting from a
    save comes in very handy.

``-t, --timetrack``
    Prints stats about elapsed time on misc tasks. Typically used to test code performance.

``-w, --warnings``
    Warn about deprecated modpack constructs.

``--ruleset <RULESET>``
    Load ruleset RULESET. Default is the Civ2Civ3 ruleset.

``--scenarios <DIR>``
    Save scenarios to directory DIR.

``-a, --auth``
    Enable database authentication (requires --Database).

``-D, --Database <FILE>``
    Enable database connection with configuration from FILE.

``-G, --Guests``
    Allow guests to login if auth is enabled. See

``-N, --Newusers``
    Allow new users to login if auth is enabled.

``-h, --help``
    Display help on command line options and quits.

``--help-all``
    Display help including Qt specific options and quits.

``-v, --version``
    Display version information and quits.
