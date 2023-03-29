.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Rulesets Overview
*****************

Quickstart
==========

Rulesets allow modifiable sets of data for units, advances, terrain, improvements, wonders, nations, cities,
governments, and miscellaneous game rules, without requiring recompilation, in a way which is consistent
across a network and through savegames.

* To play Freeciv21 normally: do not do anything special. The new features all have defaults which give the
  standard Freeciv21 behavior.

* To play a game with rules more like Civ1, start the server with:

.. code-block:: sh

    $ freeciv21-server -r data/civ1.serv


and any other command-line arguments you normally use, depending on how you have Freeciv21 installed you may
have to give the installed data directory path instead of :file:`data`.

Start the game normally. The game must be network-compatible to the server (usually meaning the same
or similar version), but otherwise nothing special is needed. However, some third-party rulesets may
potentially require special graphics to work properly, in which case the game should have those graphics
available and be started with an appropriate :code:`--tiles` argument.

Note that the Freeciv21 :term:`AI` might not play as well with rules other than standard Freeciv21 `Classic`
or `Civ2Civ3` rulesets. The :term:`AI` is supposed to understand and utilize all sane rules variations, so
please report any :term:`AI` failures so that they can be fixed.

The rest of this file contains:

* More detailed information on creating and using custom/mixed rulesets.

* Information on implementation, and notes for further development.

Using and Modifying Rulesets
============================

Rulesets are specified using the server command :code:`rulesetdir`. The command line example given above just
reads a file which uses this command (as well as a few of the standard server options). The server command
specifies in which directory the ruleset files are to be found.

The ruleset files in the data directory are user-editable, so you can modify them to create modified or custom
rulesets (without having to recompile Freeciv21). It is suggested that you `do not` edit the existing files in
the "civ2civ3", "classic", "experimental", "multiplayer", "alien", "civ1", or "civ2" directories, but rather
copy them to another directory and edit the copies. This is so that its clear when you are using modified
rules and not the standard ones.

The format used in the ruleset files should be fairly self-explanatory. A few points:

* The files are not all independent, since e.g., units depend on advances specified in the techs file.

* Units have a field, "roles", which is like "flags", but determines which units are used in various
  circumstances of the game (rather than intrinsic properties of the unit). See comments in
  :file:`common/unit.h`

* Rulesets must be in UTF-8. Translatable texts should be American English ASCII.

Restrictions and Limitations
============================

* :file:`units.ruleset` :

:strong:`Restrictions`:

* At least one unit with role "FirstBuild" must be available from the start (i.e., tech_req = "None").

* There must be units for these roles:

    * "Explorer"
    * "FerryBoat"        (Must be able to move at sea)
    * "Hut"              (Must be able to move on land)
    * "Barbarian"        (Must be able to move on land)
    * "BarbarianLeader"  (Must be able to move on land)
    * "BarbarianBuild"   (Must be able to move on land)
    * "BarbarianBoat"    (Must be able to move at sea)
    * "BarbarianSea"

* :file:`nations.ruleset` :

:strong:`Restrictions`:

* Government used during revolution cannot be used as ``default_government`` or ``init_government`` for any
  nation.

Implementation Details
======================

This section and following section will be mainly of interest to developers who are familiar with the
Freeciv21 source code.

Rulesets are mainly implemented in the server. The server reads the files and then sends information to the
game. Rulesets are used to fill in the basic data tables on units etc., but in some cases some extra
information is required.

For units and advances, all information regarding each unit or advance is now captured in the data tables, and
these are now "fully customizable", with the old enumeration types completely removed.

Game Settings Defined In The Ruleset
====================================

Game settings can be defined in the section ``[settings]`` of the file :file:`game.ruleset`. The name key is
equal to the setting name as listed by 'show all'. If the setting should be locked by the ruleset, the last
column should be set to TRUE.

.. code-block:: ini

    set =
      { "name", "value", "lock"
        "bool_set", TRUE, FALSE
        "int_set", 123, FALSE
        "str_set", "test", FALSE
      }


Scenario Capabilities
=====================

Some scenarios can be unlocked from a ruleset, meaning that they are not meant to be used with strictly one
ruleset only. To control that such a scenario file and a ruleset are compatible, capabilities are used.
Scenario file lists capabilities it requires from the ruleset, and ruleset lists capabilities it provides.

Some standard capabilities are:

* std-terrains: Ruleset provides at least terrain types Inaccessible, Lake, Ocean, Deep Ocean, Glacier,
  Desert, Forest, Grassland, Hills, Jungle, Mountains, Plains, Swamp, Tundra. Ruleset provides River extra

