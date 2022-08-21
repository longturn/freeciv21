..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 louis94 <m_louis30@yahoo.com>

Freeciv21 on the Server
***********************

In order to play Freeciv21 on the network, one machine needs to act as the *server*: the state of
the game lives on the server and players can connect to play. There are many ways to operate such a
server; this page gives an introduction to the topic.

In order to run a server, you need a computer that other players can reach over the network. If
you're planning to play at home, this can usually be any machine connected to your WiFi or local
cabled network. Otherwise, the easiest is to rent a small server from a hosting provider. We
strongly recommend that you choose a Linux-based server, as that is what we have experience with.
You will need the ability to run your own programs on the server, so SSH access is a must. Apart
from that, Freeciv21 is quite light on resources so you will hardly hit the limits of even the
cheapest options.

Whether you choose to use your own machine or to rent one, the basic principle of operating a
server is the same: you need to run a program called ``freeciv21-server`` for as long as the game
will last. This program will wait for players to connect and handle their moves in the exact same
way as in a single player game. In fact, Freeciv21 always uses a server, even when there is only
one player!

Installation
============

Installing Freeciv21 on a server is done the normal way, as documented in :doc:`/General/install`.
Because the official packages come with the complete game, they will pull a number of dependencies
that are normally not used on servers (for instance, a display server). This software is installed
for packaging reasons, but will not be used.

Starting the Server
===================

Once Freeciv21 is installed, it's immediately ready to run a simple server, which one can do by
running the following command:

.. code-block:: sh

    freeciv21-server

This starts a server listening on the port traditionally used for Freeciv21, 5556. You can provide
a custom port number by passing it to the ``-p`` argument:

.. code-block:: sh

    freeciv21-server -p 5000

A server started with this command can be reached by pointing the game to port 5000 of your domain
name. We suggest to start the server from within a terminal multiplexer such as ``tmux`` or
``screen``, which will let it run independently.

User Permissions
================

Freeciv21 supports several access levels for players connected to a server, restricting which
commands they are allowed to run:

``none``
    The user may not issue any command.

``info``
    The user may use informational commands only.

``basic``
    The user may use informational commands as well as commands affecting the game. Commands
    affecting the game start a vote if more than one user is connected.

``ctrl``
    Same as ``basic``, but the vote is bypassed for commands affecting the game.

``admin``
    May use any command, except for ``quit``, ``rfcstyle``, and ``write``. This includes
    potentially destructive commands such as ``save`` and ``fcdb`` --- use with care.

``hack``
    May use all commands without restriction.

By default, users connected to your server have access level ``basic``. This can be changed using
the ``cmdlevel`` command::

    cmdlevel info new

This command would grant access level ``info`` to any newly connecting player. A few more options
are available; please refer to the documentation of ``cmdlevel`` for more information.

.. note::
    The ``take`` and ``observe`` commands require access level ``info`` only. Their use can be
    restricted using the ``allowtake`` server option or, in more advanced setups, using the
    ``user_take`` :doc:`fcdb </Coding/fcdb>` hook.
