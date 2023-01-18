..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022-2023 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Unit Class Flags
****************

Unit types (e.g. a unit such as a :unit:`Phalanx`) are associated or grouped together in classes. Every
unit type is associated with a single unit class. Each class can have zero or more :ref:`flags <Effect Flags>`
assigned to it by ruleset :doc:`modders </Modding/index>`. The flags give additional features to all of the
unit types associated with the class. A reader can find the unit type flags by looking at the
:file:`units.ruleset` file in any ruleset directory. The following flags are used in all of the rulesets
shipped by the Freeciv21 developers.

:strong:`Airliftable`
  Allows a unit to be airlifted from a suitable city. This quite often requires an :improvement:`Airport`.

:strong:`AttackNonNative`
  Allows a native tiled unit to attack a non-native tiled unit (e.g. :unit:`Cannon` attacks a :unit:`Galleon`
  from inside a city). The Unit Type Flag ``Only_Native_Attack`` can override this.

:strong:`AttFromNonNative`
  Allows a unit to attack another unit on from non-native tiles (e.g. :unit:`Marines` in a :unit:`Transport`
  attacking a city). The Unit Type Flag ``Only_Native_Attack`` can override this.

:strong:`BuildAnywhere`
  Allows a unit to be built even in the middle of non-native terrain.

:strong:`CanFortify`
  Allows a Unit to fortify inside and outside of cities, gaining a defense bonus.

:strong:`CanOccupyCity`
  Allows a unit to enter a city and take ownership of it.

:strong:`CanPillage`
  Allows a unit to pillage a tile's infrastructure improvements. This can be your own tile or that of another
  nation.

:strong:`CollectRansom`
  Allows a unit to collect a ransom in gold after killing a :unit:`Barbarian Leader`.

:strong:`DamageSlows`
  Unit classes with this flag are slowed down (have less than 100% of expected movement points) when damaged.

:strong:`DoesntOccupyTile`
  These kinds of units do not fully occupy the tile they are on. If the unit is of an enemy nation, your
  city's citizens can still work that tile. Unit classes without this flag fully occupy the tile and prevent
  citizen workers from utilizing the tile's capabilities.

:strong:`KillCitizen`
  A unit class with this flag will kill city citizens during attack. If too many citizens are killed as part
  of taking a city, the player may end up with ruins instead of a city.

:strong:`Missile`
  As the flag name implies, these are missiles.

:strong:`TerrainDefense`
  Unit classes with this flag give unit types a defense bonus from certain terrain.

:strong:`TerrainSpeed`
  Units use terrain specific speed.

:strong:`Unreachable`
  Unit can be attacked only by units explicitly listing this class in its `targets`, unless on a city or
  native base. For class members which are transports, cargo cannot load/unload except in a city or native
  base, unless that unit explicitly lists this class in its `embarks`/`disembarks` actions.

:strong:`ZOC`
  Unit is subject to zone of control rules.
