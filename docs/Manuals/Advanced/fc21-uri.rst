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
