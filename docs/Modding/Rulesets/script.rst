.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: Freeciv Wiki contributors <https://freeciv.fandom.com/wiki/Lua_reference_manual?action=history>
.. SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

.. Usage references:
.. https://longturn.readthedocs.io/en/latest/Contributing/style-guide.html
.. https://luals.github.io/wiki/definition-files
.. https://luals.github.io/wiki/annotations/#documenting-types
.. https://taminomara.github.io/sphinx-lua-ls/index.html#autodoc-directives
.. https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-primer

.. include:: /global-include.rst

Lua Scripting
*************

All Lua code for a ruleset currently goes in the :file:`game/script.lua`
file.

.. code-block:: lua

   -- Place Ruins at the location of the destroyed city.
   function city_destroyed_callback(city, loser, destroyer)
     city.tile:create_extra("Ruins", nil)
     -- continue processing
     return false
   end

   signal.connect("city_destroyed", "city_destroyed_callback")

In this example, we have a callback handler for the 
:lua:func:`Events.city_destroyed` signal that creates a Ruins extra on the
site of the destroyed city.

Defaults
========

To avoid having to copy large amounts of standard functionality across to
every ruleset, there is a :file:`data/default/default.lua` file containing
signal handlers that are included automatically.

Signal Handlers
---------------

.. lua:autoobject::  _deflua_hut_get_gold
.. lua:autoobject::  _deflua_hut_consolation_prize
.. lua:autoobject::  _deflua_hut_get_tech
.. lua:autoobject::  _deflua_hut_get_mercenaries
.. lua:autoobject::  _deflua_hut_get_city
.. lua:autoobject::  _deflua_hut_get_barbarians
.. lua:autoobject::  _deflua_hut_enter_callback
.. lua:autoobject::  _deflua_hut_frighten_callback
.. lua:autoobject::  _deflua_make_partisans_callback
.. lua:autoobject::  _deflua_harmless_disaster_message
.. lua:autoobject::  _deflua_city_conquer_gold_loot

Disable a Default Signal Handler
--------------------------------

Disabling a default signal handler is as simple as calling the
:lua:func:`signal.remove` function with the same signal and handler function
that created it. E.g. 

.. code-block:: lua

   signal.remove("city_loot", "_deflua_city_conquer_gold_loot")

This disables gold loot on city capture completely.

Override a Default Signal Handler
---------------------------------

You can also change the default behaviour for a specific handler by overriding
the function.

.. code-block:: lua

   function _deflua_city_conquer_gold_loot(city, looterunit)
     local loot = 25
     looterunit.owner:change_gold(loot)
     notify.event(looterunit.owner, city.tile, E.UNIT_WIN_ATT, 
       string.format(
         PL_("Your lootings from %s accumulate to %d gold!",
             "Your lootings from %s accumulate to %d gold!",
             loot),
         city:link_text(), loot
       )
     )
   end

   signal.replace("city_loot", "_deflua_city_conquer_gold_loot")

This just grants a flat 25 gold to the attacker and doesn't take anything from
the defender.

Alternatively, you can just :lua:func:`signal.remove` the default handler as
above, and implement your own signal handler using :lua:func:`signal.connect`.

API Reference
=============

.. _script-api-modules:

Modules
-------

.. lua:autoobject:: const

.. lua:autoobject:: log
   :members:
   :recursive:

.. lua:autoobject:: game
   :members:
   :recursive:

.. lua:autoobject:: find
   :members:
   :recursive:

.. lua:autoobject:: effects
   :members:
   :recursive:

.. lua:autoobject:: direction
   :members:
   :recursive:

.. lua:autoobject:: E
   :members:
   :recursive:

.. lua:autoobject:: notify
   :members:
   :recursive:

.. lua:autoobject:: server
   :members:
   :recursive:

.. lua:autoobject:: edit
   :members:
   :recursive:

.. lua:autoobject:: Events
   :members:
   :recursive:

.. lua:autoobject:: signal
   :members:
   :recursive:


Types
-----

.. lua:autoobject:: Player
   :members:
   :recursive:

.. lua:autoobject:: City
   :members:
   :recursive:

.. lua:autoobject:: Unit
   :members:
   :recursive:

.. lua:autoobject:: Tile
   :members:
   :recursive:

.. lua:autoobject:: Government
   :members:
   :recursive:

.. lua:autoobject:: Nation_Type
   :members:
   :recursive:

.. lua:autoobject:: Building_Type
   :members:
   :recursive:

.. lua:autoobject:: Unit_Type
   :members:
   :recursive:

.. lua:autoobject:: Tech_Type
   :members:
   :recursive:

.. lua:autoobject:: Terrain
   :members:
   :recursive:

.. lua:autoobject:: Disaster
   :members:
   :recursive:

.. lua:autoobject:: Achievement
   :members:
   :recursive:

.. lua:autoobject:: Connection
   :members:
   :recursive:

.. lua:autoobject:: Action
   :members:
   :recursive:

.. lua:autoobject:: Nonexistent
   :members:
   :recursive:
   
.. _script-api-functions:

Functions
---------

Internationalization
^^^^^^^^^^^^^^^^^^^^

String translation functions are used for localizable event messages included
with the game. See :ref:`Internationalization <coding-i18n>`

.. lua:autoobject:: _
.. lua:autoobject:: N_
.. lua:autoobject:: Q_
.. lua:autoobject:: PL_

Utilities
^^^^^^^^^

.. lua:autoobject:: random
.. lua:autoobject:: fc_version
.. lua:autoobject:: players_iterate
.. lua:autoobject:: whole_map_iterate

Debugging
^^^^^^^^^

.. lua:autoobject:: listenv
.. lua:autoobject:: _freeciv_state_dump
.. lua:autoobject:: signal.list
.. lua:autoobject:: fc_version
.. lua:autoobject:: _VERSION
.. lua:autoobject:: assert

.. _script-unit-loss-reasons:

Unit Loss Reasons
-----------------

Loss reasons are supplied to show in what case an event of destroying a unit
happens; also, specifying different reasons in :lua:func:`Unit:kill` has
different side effects, predominantly on the loser's "units lost" score and the
killer's "units killed" score (if the killer is specified).

.. list-table:: Unit Loss Reasons
   :header-rows: 1

   * - Reason
     - Game Event
     - Loser Score
     - Killer Score
   * - "killed"
     - a) Unit loses a battle
       b) Paradropped on enemy unit tile (killer player not specified)
     - Yes
     - Yes
   * - "executed"
     - Since 2.6: never; before: when establishing embassy to a "No_Diplomacy" affected nation
     - Yes
     - Yes
   * - "retired"
     - Happens to barbarians with no targets around
     - No
     - No
   * - "disbanded"
     - a) Killed by shields upkeep
       b) Disbanded by user request
     - No
     - No
   * - "barb_unleash"
     - Killed in a barbarian uprising on its tile from a hut or by script
     - Yes
     - No
   * - "city_lost"
     - a) An unique unit is transferred with a city to a player having such one
       b) A unit from a lost city can't be rehomed to another city
       c) Destruction of a city left a unit on its tile on unsuitable terrain, or deprived an adjacent city or a cascade of cities of sea connection.
     - Yes
     - No
   * - "starved"
     - Killed for food upkeep
     - Yes
     - No
   * - "sold"
     - Killed by gold upkeep
     - Yes
     - No
   * - "used"
     - Spent in a successful action (except disbanding, nuking and suicide attack, or diplomatic actions)
     - No
     - No
   * - "eliminated"
     - Lost in a diplomatic battle
     - Yes
     - Yes
   * - "editor"
     - Edited out
       Gameloss unit removed with this cause won't make its owner losing the game
     - No
     - No
   * - "nonnative_terr"
     - a) Got to a nonnative terrain by edit.unit_teleport or paradropping
       b) Could not be bounced from a terrain change
     - Yes
     - No
   * - "player_died"
     - All what has remained of a nation of a lost player is wiped completely
     - No
     - No
   * - "armistice"
     - A military unit stays in a peaceful territory at turn end
     - Yes
     - No
   * - "sdi"
     - A nuke was unsuccessful against SDI defense
     - Yes
     - Yes
   * - "detonated"
     - A nuke was successfully exploded
     - No
     - No
   * - "missile"
     - A suicide attack or wiping units is performed
     - No
     - No
   * - "nuke"
     - Killed by a nuclear blast
     - Yes
     - Yes
   * - "hp_loss"
     - Lost all hitpoints in the open
     - Yes
     - No
   * - "fuel"
     - Lost all fuel in the open
     - Yes
     - No
   * - "stack_conflict"
     - a) Could not bounce out of a tile with non-allied units or city
       b) Moved to non-allied city or unit tile by edit.unit_teleport()
     - Yes
     - No
   * - "bribed"
     - Bribed by enemy diplomat.***
     - Yes
     - Yes*
   * - "captured"
     - Captured by enemy unit.***
     - Yes
     - Yes*
   * - "caught"
     - a) A diplomatic action failed from the beginning
       b) A diplomat could not escape after performing an action
     - Yes
     - Yes**
   * - "transport_lost"
     - Transport of a unit is destroyed and it could not be rescued
     - Yes
     - Yes

.. note::
   :literal:`*` The killer can be specified only by a script.

   :literal:`**` Killer submitted only if the action was failed.

   :literal:`***` A new unit is created under the new ownership to replace the
                  old unit after the signal is processed.


Lua Built-ins
-------------

Some Lua builtin functions and modules are also available in Freeciv21 (some
functionality is intentionally left out by policy). It is not our intention to
document Lua builtins here, but just to mention a selection of the useful parts. 

.. _script-lua-builtin-functions:

Lua Functions
^^^^^^^^^^^^^

.. lua:autoobject:: pcall
.. lua:autoobject:: pairs
.. lua:autoobject:: ipairs

Lua Globals
^^^^^^^^^^^

.. lua:autoobject:: _G

.. _script-lua-builtin-modules:

Lua Modules
^^^^^^^^^^^

.. lua:autoobject:: math
.. lua:autoobject:: string
.. lua:autoobject:: table

