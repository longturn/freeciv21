.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Longturn Game Admin Guide
*************************

Any competent player can act as a Game Admin. There are a few tools and some knowledge you will need to be successful.

Tools
=====

Games are established by an admin by editing ruleset, server settings, and player settings files. All of these files
are in plain text. You will need at least two tools:

#. ``git``
#. Plain Text Editor: Kate/Gedit/Nano/Vi on Linux. Notepad++ on Windows are quick suggestions.

GitHub
======

All of the games are on the `Longturn Games Repository <https://github.com/longturn/games>`_. Follow the instructions
in the GitHub section of the :ref:`Dev Environment page <dev-env-github>` to setup a GitHub account and establish a
local working area. Skip the ``clone`` steps for Freeciv21.

Now clone the games repository:

.. code-block:: sh

  ~/$ cd GitHub
  ~/GitHub$ git clone git@github.com:longturn/games.git


You will be prompted for your SSH key to complete the ``clone``. You should have a local :file:`games` directory when
complete.

Games Repo Structure
====================

The Games repository has a logical structure::

  - [Game Number] Directory (e.g. LT86)
    - [Game Number].serv
    - [Game Number].sh
    - db.conf
    - env.sh
    - players.serv

    - data/[Game Number] Directory (e.g. data/LT86)
      - buildings.ruleset
      - cities.ruleset
      - effects.ruleset
      - game.ruleset
      - government.ruleset
      - nations.ruleset
      - parser.lua
      - script.lua
      - styles.ruleset
      - techs.ruleset
      - terrain.ruleset
      - units.ruleset


Here are some notes/edits needed for each file.

#. ``[Game Number].serv`` -- This is the primary server configuration file. You can edit any and all parameters to setup
   the game the way you want. Refer to the :doc:`/Manuals/Server/index`, and for help with the map generator, refer to
   :doc:`/Manuals/Advanced/map-generator`.
#. ``[Game Number].sh`` -- Edit the top three values for the ``Game Number`` and set the TCP Port for the game.
#. ``db.conf`` -- Edit for the ``Game Number``
#. ``env.sh`` -- Edit the ``FREECIV_DATA_PATH`` value for the ``Game Number``
#. ``players.serv`` -- This is the list of players for the game. You can get the list from the game page on the main
   longturn.net website. Refer to :doc:`/Manuals/Advanced/players` for more information. You can also refer to other
   recent games for help with the format.
#. The files in ``data/[Game Number]`` are standard ruleset files. You can copy the ruleset files from our common
   `ruleset repository <https://github.com/longturn/LTT-LTX>`_, or create your own custom ruleset. We have Sim and
   Aviation as current third-part rulesets we play games with. For more information on ruleset files, refer to
   :doc:`/Modding/index`.


Setup a New Game
================

Now that you know the basics of how the game files on the repository work, let us move to the steps to admin a new game.

You must first have been granted Game Admin rights on the main longturn website. The website uses Django as the
back-end, so managing game pages on the site is very simple. If you do not have access, please ask on the main Longturn
Discord server.

#. Login at https://longturn.net
#. On the lower left, click "Admin Site".
#. On the site administration panel on the left, click ``+ Add`` on the row for *Games*.
#. Give it a name, such as LT86. The name must be unique and can not have been used before.
#. Fill in the game description field. See below on some more notes related to the game description.
#. Select the game mode: Team Game, Teamless, Experimental.
#. Select the version of Freeciv21 the game will run on.
#. Select your handle from the Admin drop down box to set yourself as game admin.
#. Leave all the rest of the fields as is. Do not set the port or a start date at this time. Those will get set later.
#. Click ``Save`` in the lower right.


.. note:: The Longturn server is configured to accept incoming connections on ports ``5050`` to ``5099``. Note
  that port ``5060`` can be problematic for some corporate and educational facilites and should be avoided.


Notes on the :strong:`game description`:

* Give details as to the type of game you want to play. This is especially important for Team and Experimental games.
  This is also important if the game is going to be special, such as a Scenario or use a third-party ruleset. Recall
  that Experimental games often have rules change mid game to fix bugs or make changes. They are Experimental by nature,
  so its good to add these kinds of notes to the description.
* Mention what ruleset will be used or any other special rules for the game.
* Mention winning alliance size and any game ending / announcement rules.
* For team games, explain how the teams will be selected / defined up front. For teamless games, potentially mention how
  many players are needed for the game to start.
* Players are very interested in what kind of map topology will be used. Some only play squares, some only hexes. Define
  the map topology in the description.
* Establish the length of turns. We have often run turns at 23 hours. However, lately we have been going with 25 hours.
  Either way, make sure to mention it in the game description. If you intend to change the length of the turn at a
  certain point in the game (e.g. for late game make the turns two days -- ``2*23`` or ``2*25``) you will definitely
  want to state this up front.
* Define up front how you want to handle RTS. RTS (Real Time Strategy) is a term we use to define how actions between
  players can occur when both are playing at the same time. When a player RTS's, they will be acting against you at the
  same time you are trying to do some moves, such as establishing a new city or attacking. A non-RTS game would not
  allow this synchronous action. A RTS game would allow it. Teamless games are often non-RTS, same with Team games.
  However, Experimental or other types are games could allow it.


The following code-block can be used to copy and paste into the Django admin site for new games.

.. code-block:: html

  <b>Description: </b>This is the game description. This game will be a [team | teamless | scenario] game.<br><br>
  <b>Rules: </b>This game will use [LTT | LTX] Rules.<br><br>
  <b>Winning Alliance: </b>This game will use the following math formula for the winning alliance:
    <kbd>round(N/4)</kbd>, where <kbd>N</kbd> is the number of confirmed players rounded down to the nearest
    whole number.<br><br>
  <b>Map Topology: </b>This game will use [hex | square | iso | overhead] map tiles.<br><br>
  <b>Turn Length: </b>This game will have 25 hour turns.<br><br>
  <b>RTS: </b>This game [will | will not] allow Real Time Strategy (RTS) player engagement.


At this point you can announce the game on the ``#new-games`` channel on the Longturn Discord server to let people know
about it. Ask a Discord Admin to create a channel for the game as well.

While players are talking about the game on Discord and signing up on the website, you can get the game files ready. The
easiest way to start is by copying a previous game and then editing the files. When finished you will combine the
changes into a ``git commit`` and push them up to the games repository.

.. code-block:: sh

  ~/GitHub/games$ git status
  ~/GitHub/games$ git add --all
  ~/GitHub/games$ git commit
  ~/GitHub/games$ git push origin


The ``git status`` command gives you information on what changed on your local workspace. It gives you a chance to fix
any files that may have changed that you did not intend to.  The ``git add`` command will add all changed files to the
commit package.  The ``git commit`` command will bring up a text editor (often Nano) allowing you to enter in a commit
message. Preface commit messages with the ``[Game Number]:``. Finally you push the changes up to the repo with the
``git push`` command. You will be prompted for your SSH passkey to complete the step.


Start the New Game
==================

When enough players have signed up, you will want to announce it on the game channel and set a potential start date.

These are the steps to finalize a game:

#. Go into the admin site on the Longturn server and set a start date at least 7 days into the future and save.
#. Ask players to confirm participation on the website. The typical confirmation period is 7 days.
#. Start editing the ``players.serv`` file and push changes up to the repo.
#. Ask a Server Admin to setup a test game with the game server and map generator settings to see how things look.
   :strong:`Note`: Game Admins should test locally to check things out before asking a server admin to do it on the main
   game server.
#. Based on confirmations, you will finalize the ``players.serv`` file and push any other last minute changes to the
   game server settings based on test results. The server can not handle a nation name of "random", you will need to be
   the random nation namer for those players that ask for a random nation.
#. Go to the admin site and set the port for the game.
#. Ask a Server Admin to start the game for you.
#. Announce game start. Ensure first turn is extended.

..note:: Server admins can perform additional actions when starting the game, such as looking at the map or performing
  edits. For example, it is common to post the size of the largest islands. Since this requires extra work, make sure
  you talk to the Server Admin beforehand.


During the Game
===============

Once the game has started, the Game Admin is expected to keep an eye on the game channel on the Discord server. It is
the Game Admin's responsibility to arbitrate complaints or issues between players. Timely response is important as you
are able given the time zone you live in. Other Game and Server Admins often watch the active game channels to provide
guidance and assistance as well.

Ending the Game
===============

Depending on how you setup the game, players can form alliances or other winning conditions will come true. Players
typically announce the win on the game channel on Discord. Admins typically offer a 5 day (``5*24``) cooling period to
allow other players to either reject the win -- keep playing -- or accepting the win -- stop playing. The Server Admin
can end the game when asked. A player from the winning alliance can grab a screenshot of the final game report and post.
Server Admins can also generate an animated ``gif`` file of the map to show the rise and fall of nations as the game
progressed.
