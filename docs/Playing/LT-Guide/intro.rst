.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Introduction
************

The Longturn community maintains and plays three specialized rulesets: Longturn Tradidional (LTT), Longturn
Experimental (LTX), and Royale. The LTT and LTX rulesets are losely based on the Civ2Civ3 ruleset as it existed
"way back" in legacy Freeciv v2.0 days. Royale is new as of 2023.

Here are some generalized notes related to the three rulesets:

* :strong:`LTT`: There are actually two versions: LTT and LTT2. LTT is the original standard ruleset played
  across many games, including `The League <https://longturn.net/game/TheLeague/>`_. In 2024, we created an
  updated LTT2 based on input from the community as well as the :doc:`game admin's </Contributing/game-admin>`
  experience. LTT2 should be considered a logical evolution of LTT.

  .. note::
    LTT2 is considered the default stable ruleset. The League still plays LTT and will change to LTT2 when
    appropriate.

* :strong:`LTX`: This is our experimental ruleset. LTX is based on LTT, so many of the play style elements in
  LTT are similar in LTX. We use LTX as a test bed for new ideas. Ideas that gain traction and are well
  received by our player community can be merged into LTT2.

* :strong:`Royale`: This is a newer ruleset that takes many thoughts from LTT and tries to create a simpler
  set of rules for brand new Longturn players. The general idea is to create a set of rules that can be played
  by experieced and new players, with enough slack that new players will not get overwhelmed if they make
  gameplay mistakes.


.. _lt-game-guide-other-tutorials:

Other External Tutorials
========================

Here are links to some other tutorials. Your mileage may vary with some of them.

* `LTT on Freeciv Fandom <https://freeciv.fandom.com/wiki/Longturn_Traditional_Ruleset>`_
* `LTX on Freeciv Fandom <https://freeciv.fandom.com/wiki/LongTurn_Experimental_ruleset>`_
* `Intro to Freeciv on Freeciv Fandom <https://freeciv.fandom.com/wiki/Introduction_to_Freeciv>`_
* `Game Manual on Freeciv Fandom <https://freeciv.fandom.com/wiki/Game_Manual>`_. Overall game mechanics for
  the Classic ruleset. LTT/LTX rulesets are similar and are derived from the Civ2Civ3 ruleset. Topics include:
  Terrain, Cities, Economy, Units, Combat, Diplomacy, Government, Technology, Wonders, and Buildings. Gives a
  good overview of how the varying elements of a game work if you are new to Freeciv or Freeciv21.
* `Multiplayer Game Manual on Freeciv Fandom <https://freeciv.fandom.com/wiki/Multiplayer_Game_Manual>`_.
* `Multiplayer Ruleset (cheatsheet) on Freeciv Fandom <https://freeciv.fandom.com/wiki/Multiplayer_Ruleset>`_.
* `How to Play Freeciv on Freeciv Fandom <https://freeciv.fandom.com/wiki/How_to_Play>`_.
* `The Art of Freeciv on Freeciv Fandom <https://freeciv.fandom.com/wiki/The_Art_of_Freeciv_2.1>`_
* `How to Join a Longturn Game on Longturn Blog <https://longturn21.blogspot.com/p/how-to-join-longturn-game.html>`_.
* `Intro to Longturn on Longturn Blog <https://longturn21.blogspot.com/p/introduction-to-longturn.html>`_.
* `Freeciv for Absolute Beginners (short version) on Longturn Blog <https://longturn21.blogspot.com/p/freeciv-for-absolute-beginners-short.html>`_.


Connecting to Longturn Games
============================

Freeciv21 is a client/server system. The game (client) is where we play and the server runs the rules and
controls all aspects of the game engine. If you have only played single-player games, you may not be aware of
this separation between components.

Longturn games are hosted on a server with access to the Internet. Players of Longturn games will connect to
the server from the :ref:`Connect to Network Game <game-manual-connect-network>` start menu option. Players
will use the manual section in the lower left of the dialog box. The host is always ``longturn.net``, the
game page on the Longturn website will tell you the TCP Port number to connect to for the specific game. You
will use the same username and password as you use on the Longturn website when you registered for a game.


A Note About Tilesets
=====================

Longturn games will generally use one of two map topologies: squares or hexes. Some players love squares, and
hate hexes. Some players are the opposite! Longturn game admins will annouce up front what map topology a game
will use.

For squares: The shipped Amplio2 tileset is the default. It is in an isometric direction. There are two
overhead tilesets available in the modpack installer --- RoundSquare and Sextant --- that you might prefer.

For hexes: The shipped Hexemplio tileset is the default. It is in an isometric direction. There is an overhead
tileset available in the modpack installer --- RoundHex --- that you might prefer.

To use the alternate tilesets: download via the modpack installer, close the game, reopen and select an
alternate tileset from the game menu. You can also make it default in the
:ref:`Game Options <game-manual-more-game-options>` menu.
