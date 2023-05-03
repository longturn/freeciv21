..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Glossary of Terms
*****************

This page contains a collection of terms and acronyms commonly used in the documentation as well as in varying
community discussions.

.. glossary::

  AI
    Artificial Intelligence

    Freeciv21 comes with a pre-programmed artificial intelligence that you can play against in single-player
    games.

  FC
    Freeciv

    Classic legacy `Freeciv <http://freeciv.org>`_.

  FC21
    Freeciv21

    A new fork of Freeciv v3.0 concentrating on multiplayer games and a single game interface.

  FCW
    Freeciv Web.

    A web client and highly customized version of the legacy Freeciv server. Not affiliated
    directly with Freeciv, Freeciv21, or Longturn.

  FEP
    For Each Player

    During :doc:`/Playing/turn-change`, activities occur for each player in a random sequence per turn.

  FFA
    Free For All

    These are Longturn games where all the players start the game non-allied to any other player. During the
    game, players may form alliances. However, these alliances are often not formally reported in-game via the
    :ref:`Nations and Diplomacy View <game-manual-nations-and-diplomacy-view>` until necessity requires it.
    This is typically when conflict occurs and allied units need to enter allied cities.

  FOW
    Fog of War

    If you do not have a unit giving vision on a piece of the game map, your nation will not see border
    changes until you move a unit giving vision.

  FP
    Fire Power

    The amount of damage an attacking unit can inflict on a defending unit within a single round of combat.

  Goto
    Units that are out of :term:`MP` can be given advanced orders to take an action at the start of the next
    turn. There are many possibilities. See :menuselection:`Unit --> Goto and ...` for some ideas.

  HP
    Hit Point(s)

    The amount of health a unit has avalable. When this goes to zero the unit is killed.

  LT
    Longturn

  LTT
    Longturn Traditional

    Longturn's standard :ref:`ruleset <modding-rulesets>`.

  LTX
    Longturn Experimental

    Lonturn's experimental :ref:`ruleset <modding-rulesets>`.

  MP2
    Multiplayer 2 Ruleset

    There are many MP2 :ref:`rulesets <modding-rulesets>` and they are numbered: MP2a, MP2b, MP2c, etc.

  MP
    Move Point(s)

    The number of moves a unit has available.

  RTS
    Real Time Strategy

    This is a type of game where gameplay occurs all the time. In the Longturn community, this term is also
    used when players are online at the same time and potentially competing against each other.

  TC
    Turn Change

    A period of time when the server processes end of turn events in a specific
    :doc:`order </Playing/turn-change>`

  UWT
    Unit Wait Time

    A period of time that must be exhausted before a unit can move between turns. For example, if the UWT for
    a game is set to 10 hours and a unit moves 1 hour prior to TC. Then the unit cannot move for another 9
    hours until the UWT counter is completed for that unit.
