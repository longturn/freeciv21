.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Launching the Game with fc21 Links
**********************************

The Freeciv21 Server accepts connection requests with a ``fc21://`` protocol handler on all platforms. This
makes it possible to include login links on the Longturn.net webserver, or for games self-hosted by players.

Here is a sample from a command line:

.. code-block:: sh

    $ ./freeciv21-client "fc21://[username]:[password]@[server]:[port]" -a -t amplio2


In this example, a player is launching the client from a command line and including the URI as the primary
input. By passing ``[username]``, ``[password]``, ``[server]``, and ``[port]`` this command provides all the
details needed to connect to a game. The ``-a`` enables auto-connect and ``-t amplio2`` tells the client to
load the Amplio2 tileset.

Alternately, HTML can be used in a webpage. For example:

.. code-block:: html

    <a href="fc21://[username]:[password]@[server]:[port]">Game Name</a>


Notice the same 4 parameters. The ``[password]`` parameter can be obmitted. If so, the server will prompt the
user for credentials before connecting.

.. note::
  The square brackets ``[`` and ``]`` are meant to be replaced. For example: ``[username]`` should be replaced
  with ``myusername``. Same for the other three parameters.


Automating Login on Linux
=========================

The simplest way to automate the login process on Linux is to create a shell script with the parameters. For
example:

.. code-block:: sh

  $ nano lt-game.sh

    #!/bin/bash

    /fully/qualified/path/to/freeciv21-client "fc21://[username]:[password]@[server]:[port]" -a

  $ chmod +x lt-game.sh
  $ ./lt-game.sh


Alternatively, you can create a custom :file:`.desktop` file for the game.

Automating Login on Windows
===========================

With Windows as your operating system, you can automate the login process with a shortcut. Start by finding the
location where you installed Freeciv21. By default this is in :file:`C:\\Program Files\\Freeciv21`. Right-click
on the :file:`freeciv21-client.exe` and select :guilabel:`Create Shortcut`. Right-click on the new shortcut and
select :guilabel:`Properties`. At the end of the path to the :file:`freeciv21-client.exe` (which maybe inside
quotes), add ``fc21://[username]:[password]@[server]:[port] -a`` and click :guilabel:`Apply`
