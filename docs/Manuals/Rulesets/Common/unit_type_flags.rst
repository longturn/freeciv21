..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

Unit Type Flags
***************

Every unit type (e.g. a Unit) in a ruleset can be given zero (``0``) or more flags by ruleset editors. Each
flag gives additional features to the unit type that has been assigned the flag. The following flags are used
in all of the rulesets shipped by the Freeciv21 developers: Alien, Civ1, Civ2, Civ2Civ3, Classic,
Experimental, Granularity, Multiplayer, Royale, and Sandbox.

* :strong:`AirAttacker`: Very bad at attacking AEGIS.
* :strong:`Airbase`: This flag does not have a pre-set use. It's available for ruleset modders to add as a
  condition to an :doc:`action enabler requirements vector </Modding/Rulesets/Effects/actions>`.
* :strong:`AddToCity`: Citizen containing units (e.g. Settlers) can add population to a city. Must travel to
  the target city.
* :strong:`BadCityDefender`: If attacked while in a city, firepower is set to ``1`` and firepower of attacker
  is doubled.
* :strong:`BadWallAttacker`: The firepower of this unit is set to ``1`` if attacking a city defended by a
  :improvement:`City Walls`, or other city building defense.
* :strong:`BarbarianOnly`: Only barbarians can build this unit.
* :strong:`Bombarder`: Can bombard cities.
* :strong:`CanEscape`: This unit has, given that certain conditions are fulfilled, a 50% chance to escape
  rather than being killed when ``killstack`` is enabled and the defender of its tile is defeated. The
  conditions are that it has more move points than required to move to an adjacent tile plus the attacker's
  move points and that the attacker does not have the ``CanKillEscaping`` unit type flag.
* :strong:`CanKillEscaping`: An attack from this unit ignores the ``CanEscape`` unit type flag.
* :strong:`Cant_Fortify`: Unit cannot fortify inside or outside of cities.
* :strong:`Capturable`: Can be captured by some enemy units.
* :strong:`Capturer`: Can capture some enemy units.
* :strong:`Cities`: Can found new cities.
* :strong:`CityBuster`: This unit has double firepower against cities.
* :strong:`Coast`: Sea Class only. Can refuel on coast. Set fuel to force unit to regularly end turn on coast.
* :strong:`CoastStrict`: Sea Class only. Cannot leave coast.
* :strong:`Consensus`: Undisbandable when your bureaucracy has a veto.
* :strong:`Diplomat`: Can defend against diplomat actions.
* :strong:`EvacuateFirst`: The game will try to rescue units with this flag before it tries to rescue units
  without it when their transport is destroyed. Think of the Birkenhead drill ("women and children first").
  Replace "women and children" with "units with the ``EvacuateFirst`` unit type flag".
* :strong:`Fanatic` Can only be built by governments that allow them (see civ2/governments.ruleset,
  Fundamentalism government).

.. todo:: Link to the civ2 rules page when we get sphinx_fc21_manuals integrated.

* :strong:`FieldUnit`: Cause unhappiness even when not being aggressive.
* :strong:`GameLoss`: Losing one of these units means you lose the game, but it is produced without homecity
  and upkeep
* :strong:`HasNoZOC`: Unit has no Zone of Control (ZOC), thus any unit can move around it freely.
* :strong:`Helicopter`: Defends very badly against :unit:`Fighters`.
* :strong:`HelpWonder`: Can help to build a wonder. Must travel to target city.
* :strong:`Horse`: Attack value reduced when attacking :unit:`Pikemen`.
* :strong:`IgTer`: Use constant move cost defined in the ``igter_cost`` variable in :file:`terrain.ruleset`,
  rather than terrain/road etc cost, unless terrain cost is less.
* :strong:`IgZOC`: Ignores unit zone of control (ZOC), even if the unit class has the ``ZOC`` flag.
* :strong:`Infra`: Can build infrastructure.
* :strong:`Marines`: Can launch attack from non-native tiles.
* :strong:`NeverProtects`: Does not protect reachable units on its tile from enemy attackers, even if the
  ``unreachableprotects`` server setting is enabled and the unit class is unreachable.
* :strong:`NewCityGamesOnly`: Unit cannot be built on scenarios where founding new cities is not allowed.
  Give this flag to units that would make no sense to have in a game with such a scenario.
* :strong:`NoBuild`: This unit cannot be built.
* :strong:`NoHome`: This unit has no homecity and will be free of all upkeep, and therefore will not revolt
  along with its city of origin should it be incited.
* :strong:`NonMil`: A non-military unit: no attacks, no martial law, and can enter peaceful borders.
  See ``DoesntOccupyTile``
* :strong:`NoVeteran`: This unit cannot gain veteran levels through experience, as if both ``base_raise_chance``
  and ``work_raise_chance`` were zero.
* :strong:`Nuclear`: Is nuclear capable.
* :strong:`NuclearOP`: Over Powered Nuclear. See :file:`sandbox/units.ruleset`.
* :strong:`OneAttack`: Only attacks once and is destroyed/consumed as part of the attacking action.
* :strong:`Only_Native_Attack`: Cannot attack targets on non-native tiles even if the unit class can.
* :strong:`Paratroopers`: Can be paradropped from a friendly city or suitable base.
* :strong:`Provoking`: A unit considering to auto attack this unit will choose to do so even if has better
  odds when defending against it then when attacking it. Applies when the ``autoattack`` server setting is
  enabled.
* :strong:`RealDiplomat`: Can do real diplomat actions, unlike tech transfer units.
* :strong:`RealSpy`: Can do real spy actions, unlike tech transfer units.
* :strong:`Settlers`: Can irrigate and build roads.
* :strong:`Shield2Gold`: Switch from shield upkeep to gold upkeep.
* :strong:`Spy`: Strong in diplomatic battles. `Must` be ``Diplomat`` also.
* :strong:`Submarine`: Attack value reduced when attacking :unit:`Destroyer`.
* :strong:`SuperSpy`: This unit always wins diplomatic contests, that is, unless it encounters another
  ``SuperSpy``, in which case the defender wins. Can also be used on non-diplomat units, in which case it can
  protect cities from diplomats. Also 100% spy survival chance.
* :strong:`TradeRoute`: Can establish trade routes. Must travel to target city.
* :strong:`Transform`:
* :strong:`Unbribable`: Unit cannot be bribed.
* :strong:`Unique`: A player can only have one of these units in the game at the same time. Barbarians cannot
  use this at present.
