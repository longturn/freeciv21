..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

Unit Type Role Flags
********************

Every unit type (e.g. a unit such as a :unit:`Phalanx`) in a ruleset can be given zero or more roles by
ruleset :doc:`modders </Modding/index>`. Each role gives additional features to the unit type that has been
assigned the flag. A reader can find the roles by looking at the :file:`units.ruleset` file in any ruleset
directory. The following roles are used in all of the rulesets shipped by the Freeciv21 developers.

:strong:`AttackFastStartUnit`
  Can be designated as an attacking unit at game start.

:strong:`AttackStrongStartUnit`
  Can be designated as a strong attacking unit at game start.

:strong:`Barbarian`
  Can be created as land :unit:`Barbarian`.

:strong:`BarbarianTech`
  Can be created as land :unit:`Barbarian`, if someone has researched its technology requirements.

:strong:`BarbarianBoat`
  Can be created as boat for sea :unit:`Barbarian`.

:strong:`BarbarianBuild`
  Can be built by barbarians.

:strong:`BarbarianBuildTech`
  Can be built by barbarians if someone has researched its technology requirements.

:strong:`BarbarianLeader`
  This unit is the :unit:`Barbarian Leader`. Only one can be defined once.

:strong:`BarbarianSea`
  Can be created as a :unit:`Barbarian` that disembarks from a barbarian boat.

:strong:`BarbarianSeaTech`
  Can be created as a :unit:`Barbarian` that disembarks from a barbarian boatif someone has researched its
  technology requirements.

:strong:`BorderPolice`
  Can peacefully expel certain foreign units. See ``Expellable``.

:strong:`CitiesStartUnit`
  Can be designated as city founding unit at game start.

:strong:`DefendGood`
  AI hint: Good for defending with.

:strong:`DefendGoodStartUnit`
  Can be designated as an good defending unit at game start.

:strong:`DefendOk`
  AI hint: Ok for defending with.

:strong:`DefendOkStartUnit`
  Can be designated as an ok defending unit at game start.

:strong:`DiplomatStartUnit`
  Can be designated as :unit:`Diplomat` unit at game start.

:strong:`Expellable`
  Can be peacefully expelled from foreign tiles. See ``BorderPolice``.

:strong:`Explorer`
  This unit performs :unit:`Explorer` functions.

:strong:`ExplorerStartUnit`
  Can be designated as an :unit:`Explorer` unit at game start.

:strong:`Ferryboat`
  AI hint: Useful for ferrying.

:strong:`FerryStartUnit`
  Can be designated as a ferry boat unit at game start.

:strong:`FirstBuild`
  First to be built when a city is founded.

:strong:`HeavyWeight`
  Airliftable from :improvement:`Airport` after you learn `Fusion Power`.

:strong:`Hunter`
  A special type of hunter. See :file:`alien/units.ruleset`.

:strong:`Hut`
  Can be found in a hut.

:strong:`HutTech`
  Can be found in a hut.

:strong:`Partisan`
  Can be created as a :unit:`Partisan`. Only one unit can have this flag.

:strong:`LightWeight`
  Airliftable once you learn `Flight`.

:strong:`MediumWeight`
  Airliftable from :improvement:`Airport` after you learn `Advanced Flight`.

:strong:`Settlers`
  Acts as a :unit:`Settler` type unit.

:strong:`WorkerStartUnit`
  Can be designated as a :unit:`Worker` unit at game start.
