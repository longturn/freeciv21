..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

Unit Class Flags
****************

Unit Types (e.g. a Unit) are associated or grouped together in a class. Every unit is associated with a single
class. Each class has a collection of flags that can be associated. Each class can have zero (``0``) or more
flags associated with it. The flags give additional features to the class that has been assigned the flag. The
following flags are used in all of the rulesets shipped by the Freeciv21 developers: Alien, Civ1, Civ2,
Civ2Civ3, Classic, Experimental, Granularity, Multiplayer, Royale, and Sandbox.

* :strong:`Airliftable`: Can be airlifted from a suitable city.
* :strong:`AttackNonNative`: Unit can attack units on non-native tiles (e.g. :unit:`Marines` in a
  :unit:`Transport` attacking a city). The Unit Flag ``Only_Native_Attack`` can override this.
* :strong:`AttFromNonNative`: Unit can attack from non-native tile. See ``AttackNonNative``.
* :strong:`BuildAnywhere`: Unit can be built even in the middle of non-native terrain.
* :strong:`CanFortify`: Unit can fortify inside and outside of cities gaining a defense bonus.
* :strong:`CanOccupyCity`: Can enter a city and take onwership of it.
* :strong:`CanPillage`: Can pillage a tile's infrastructure improvements.
* :strong:`CollectRansom`: Can collect a ransom from killing a :unit:`Barbarian Leader`.
* :strong:`DamageSlows`: Movement is reduced when damaged.
* :strong:`DoesntOccupyTile`: Even if this kind of enemy unit is on a tile, cities can still work that tile.
* :strong:`KillCitizen`: When attacking a city will kill citizens.
* :strong:`Missile`: Acts as a missile.
* :strong:`TerrainDefense`: Units gain defense bonus from terrain.
* :strong:`TerrainSpeed`: Units use terrain specific speed.
* :strong:`Unreachable`: Unit can be attacked only by units explicitly listing this class in its `targets`,
  unless on a city or native base. For class members which are transports, cargo cannot load/unload except in
  a city or native base, unless that unit explicitly lists this class in its `embarks`/`disembarks` actions.
* :strong:`ZOC`: Unit is subject to zone of control rules.
