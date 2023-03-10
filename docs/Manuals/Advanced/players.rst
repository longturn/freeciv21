.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>


Advanced Player Setup
*********************

In Freeciv21, there are many ways to setup a game with varying types of players. Games can be single-player in
that there is only one human against a collection of :term:`AI`. Games can be setup to be all human, such as
Longturn games. Lastly, you can do a combination of the two: multiple humans versus multiple :term:`AI`.
Players can play :term:`FFA` or in pre-allied teams as well.

More details are available in :doc:`/Playing/how-to-play`, and in the in-game help. The
:doc:`/Manuals/Game/index` has instructions on how you can manipulate players in the
:ref:`pre-game interface <game-manual-start-new-game-players>`.

The sections below use the server command line features, which is considered an advanced operation.

Single Player
=============

In most cases when you start a single-player game you can change the number of players and their difficulty,
directly through the :ref:`pre-game interface <game-manual-start-new-game-players>`. However, you may want to
do something more complex.

When playing on a remote server, you typically do all the player manipulation by hand. Start by selecting the
ruleset you which to play and then change the ``aifill`` setting to the number of :term:`AI` players you wish
to play against.

.. code-block:: sh

    $ freeciv21-server --ruleset royale

    ...

    > /set aifill 30
    > /list


.. note::
  The default number of players is often ruleset defined, so selecting a different ruleset can change the
  pre-configured set of ``aifill`` players. Have a look at the server output at startup to see what the value
  of ``aifill`` was set to.

Difficulty levels are set with the ``/cheating``, ``/hard``, ``/normal``, ``/easy``, ``/novice``, and
``/handicapped`` commands.

You may also create :term:`AI` players individually. For instance, to create one hard and one easy :term:`AI`
player, enter:

.. code-block:: rst

    > /create ai1
    > /hard ai1
    > /create ai2
    > /easy ai2
    > /list


Human vs Human Players
======================

The steps to create a human only game is similar to the previous section. Start the server with the ruleset
you wish to play and then remove all of the :term:`AI` players. Here is a quick recipe:

.. code-block:: sh

    $ freeciv21-server --ruleset royale

    ...

    > /set aifill 0
    > /set nationset all
    > /create [player username]
    > /aitoggle [player username]
    > /playernation [player username] [Nation] [0/1]
    > /list


.. note::
   The value of ``player username`` can be anything, but is often the logon ID of the player that will play,
   is case sensitive, and it sets the leader name. The value of ``Nation`` is one of the available nations,
   also case sensitive. Notice the command to enable all ``nationset``. The bit value of ``0/1`` is ``1``
   for Male and ``0`` for Female.

The human player will right-click their logon ID in the
:ref:`pre-game interface <game-manual-start-new-game>` and select :guilabel:`Take this player` at
game start. This is a one-time procedure.

.. tip::
   You can also create a plain text file named :file:`players.serv`. Inside the file you add all of the user
   creation commands into one file. Then include a ``read players.serv`` command line at the bottom of the
   :ref:`server settings file <server-settings-file>`.


Teams of AI or Human Players
============================

Following from the section above, you can easily set the team for a player with the ``/team`` command, like
this:

.. code-block:: sh

    > /set aifill 2
    > /team AI*2 1
    > /team AI*3 1
    > /team human1 2
    > /team human2 2
    > /list


You will now have a two team game of two :term:`AI` players against two human players.
