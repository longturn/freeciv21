.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
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

All Lua code for a ruleset currently goes in the :literal:`game/script.lua`
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
every ruleset, there is a :literal:`data/default/default.lua` file containing
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

.. lua:autoobject:: notify
   :members:
   :recursive:

.. lua:autoobject:: server
   :members:
   :recursive:

.. lua:autoobject:: edit
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

.. _script-api-events:

Events
------

The E module contains a set of event types, for example :literal:`E.SCRIPT`.
These correspond to the categories that the client can filter on.

.. list-table:: Event Notification Categories
   :header-rows: 1

   * - Enum
     - Description
   * - E.TECH_GAIN
     - Acquired a New Tech
   * - E.TECH_LEARNED
     - Learned a New Tech
   * - E.TECH_GOAL
     - Selected a New Goal
   * - E.TECH_LOST
     - Lost a Tech
   * - E.TECH_EMBASSY
     - Other Player Gained / Lost a Tech
   * - E.IMP_BUY
     - Bought
   * - E.IMP_BUILD
     - Built
   * - E.IMP_AUCTIONED
     - Forced to Sell
   * - E.IMP_AUTO
     - New Improvement Selected
   * - E.IMP_SOLD
     - Sold
   * - E.CITY_CANTBUILD
     - Building Unavailable Item
   * - E.CITY_LOST
     - Captured / Destroyed
   * - E.CITY_LOVE
     - Celebrating
   * - E.CITY_DISORDER
     - Civil Disorder
   * - E.CITY_FAMINE
     - Famine
   * - E.CITY_FAMINE_FEARED
     - Famine Feared
   * - E.CITY_GROWTH
     - Growth
   * - E.CITY_MAY_SOON_GROW
     - May Soon Grow
   * - E.CITY_IMPROVEMENT
     - Needs Improvement to Grow
   * - E.CITY_IMPROVEMENT_BLDG
     - Needs Improvement to Grow, Being Built
   * - E.CITY_NORMAL
     - Normal
   * - E.CITY_NUKED
     - Nuked
   * - E.CITY_CMA_RELEASE
     - Released from Citizen Governor
   * - E.CITY_GRAN_THROTTLE
     - Suggest Throttling Growth
   * - E.CITY_TRANSFER
     - Transfer
   * - E.CITY_BUILD
     - Was Built
   * - E.CITY_PLAGUE
     - Has Plague
   * - E.CITY_RADIUS_SQ
     - City Map changed
   * - E.WORKLIST
     - Worklist Events
   * - E.CITY_PRODUCTION_CHANGED
     - Production Changed
   * - E.DISASTER
     - Disaster
   * - E.MY_DIPLOMAT_BRIBE
     - Bribe
   * - E.DIPLOMATIC_INCIDENT
     - Caused Incident
   * - E.MY_DIPLOMAT_ESCAPE
     - Escape
   * - E.MY_DIPLOMAT_EMBASSY
     - Embassy
   * - E.MY_DIPLOMAT_FAILED
     - Failed
   * - E.MY_DIPLOMAT_INCITE
     - Incite
   * - E.MY_DIPLOMAT_POISON
     - Poison
   * - E.MY_DIPLOMAT_SABOTAGE
     - Sabotage
   * - E.MY_DIPLOMAT_THEFT
     - Theft
   * - E.MY_SPY_STEAL_GOLD
     - Gold Theft
   * - E.MY_SPY_STEAL_MAP
     - Map Theft
   * - E.MY_SPY_NUKE
     - Suitcase Nuke
   * - E.ENEMY_DIPLOMAT_BRIBE
     - Bribe
   * - E.ENEMY_DIPLOMAT_EMBASSY
     - Embassy
   * - E.ENEMY_DIPLOMAT_FAILED
     - Failed
   * - E.ENEMY_DIPLOMAT_INCITE
     - Incite
   * - E.ENEMY_DIPLOMAT_POISON
     - Poison
   * - E.ENEMY_DIPLOMAT_SABOTAGE
     - Sabotage
   * - E.ENEMY_DIPLOMAT_THEFT
     - Theft
   * - E.ENEMY_SPY_STEAL_GOLD
     - Gold Theft
   * - E.ENEMY_SPY_STEAL_MAP
     - Map Theft
   * - E.ENEMY_SPY_NUKE
     - Suitcase Nuke
   * - E.GLOBAL_ECO
     - Eco-Disaster
   * - E.NUKE
     - Nuke Detonated
   * - E.HUT_BARB
     - Barbarians in a Hut Roused
   * - E.HUT_CITY
     - City Founded from a Hut
   * - E.HUT_GOLD
     - Gold Found in a Hut
   * - E.HUT_BARB_KILLED
     - Killed by Barbarians in a Hut
   * - E.HUT_MERC
     - Mercenaries Found in a Hut
   * - E.HUT_SETTLER
     - Settlers Found in a Hut
   * - E.HUT_TECH
     - Tech Found in a Hut
   * - E.HUT_BARB_CITY_NEAR
     - Unit Spared by Barbarians
   * - E.ACHIEVEMENT
     - Achievements
   * - E.UPRISING
     - Barbarian Uprising
   * - E.CIVIL_WAR
     - Civil War
   * - E.ANARCHY
     - Collapse to Anarchy
   * - E.FIRST_CONTACT
     - First Contact
   * - E.NEW_GOVERNMENT
     - Learned New Government
   * - E.LOW_ON_FUNDS
     - Low Funds
   * - E.POLLUTION
     - Pollution
   * - E.REVOLT_DONE
     - Revolution Ended
   * - E.REVOLT_START
     - Revolution Started
   * - E.SPACESHIP
     - Spaceship Events
   * - E.TREATY_ALLIANCE
     - Alliance
   * - E.TREATY_BROKEN
     - Broken
   * - E.TREATY_CEASEFIRE
     - Cease-fire
   * - E.TREATY_EMBASSY
     - Embassy
   * - E.TREATY_PEACE
     - Peace
   * - E.TREATY_SHARED_VISION
     - Shared Vision
   * - E.UNIT_LOST_ATT
     - Attack Failed
   * - E.UNIT_TIE_ATT
     - Attack Tied
   * - E.UNIT_WIN_ATT
     - Attack Succeeded
   * - E.UNIT_BOMB_ATT
     - Attacker Bombarding
   * - E.UNIT_BUY
     - Bought
   * - E.UNIT_BUILT
     - Built
   * - E.UNIT_LOST_DEF
     - Defender Destroyed
   * - E.UNIT_TIE_DEF
     - Defender Tied
   * - E.UNIT_WIN_DEF
     - Defender Survived
   * - E.UNIT_BOMB_DEF
     - Defender Bombarded
   * - E.UNIT_BECAME_VET
     - Promoted to Veteran
   * - E.UNIT_LOST_MISC
     - Lost Outside Battle
   * - E.UNIT_UPGRADED
     - Production Upgraded
   * - E.UNIT_RELOCATED
     - Relocated
   * - E.UNIT_ORDERS
     - Orders / Goto Events
   * - E.UNIT_BUILT_POP_COST
     - Built Unit with Population Cost
   * - E.UNIT_WAS_EXPELLED
     - Was Expelled
   * - E.UNIT_DID_EXPEL
     - Did Expel
   * - E.UNIT_ACTION_FAILED
     - Action Failed
   * - E.UNIT_WAKE
     - Sentried Units Awaken
   * - E.VOTE_NEW
     - New Vote
   * - E.VOTE_RESOLVED
     - Vote Resolved
   * - E.VOTE_ABORTED
     - Vote Canceled
   * - E.WONDER_BUILD
     - Finished
   * - E.WONDER_OBSOLETE
     - Made Obsolete
   * - E.WONDER_STARTED
     - Started
   * - E.WONDER_STOPPED
     - Stopped
   * - E.WONDER_WILL_BE_BUILT
     - Will Finish Next Turn
   * - E.AI_DEBUG
     - AI Debug Messages
   * - E.BROADCAST_REPORT
     - Broadcast Report
   * - E.CARAVAN_ACTION
     - Caravan Actions
   * - E.CHAT_ERROR
     - Chat Error Messages
   * - E.CHAT_MSG
     - Chat Messages
   * - E.CONNECTION
     - Connect / Disconnect Messages
   * - E.DIPLOMACY
     - Diplomatic Message
   * - E.BAD_COMMAND
     - Error Message from Bad Command
   * - E.GAME_END
     - Game Ended
   * - E.GAME_START
     - Game Started
   * - E.NATION_SELECTED
     - Nation Selected
   * - E.DESTROYED
     - Player Destroyed
   * - E.REPORT
     - Report
   * - E.LOG_FATAL
     - Server Aborting
   * - E.LOG_ERROR
     - Server Problems
   * - E.MESSAGE_WALL
     - Message from Server Operator
   * - E.SETTING
     - Server Settings Changed
   * - E.TURN_BELL
     - Turn Bell
   * - E.SCRIPT
     - Scenario / Ruleset Script Message
   * - E.NEXT_YEAR
     - Year Advance
   * - E.DEPRECATION_WARNING
     - Deprecated Modpack Syntax Warnings
   * - E.SPONTANEOUS_EXTRA
     - Extra Appears / Disappears
   * - E.UNIT_ILLEGAL_ACTION
     - Unit Illegal Action
   * - E.UNIT_ESCAPED
     - Unit Escaped
   * - E.BEGINNER_HELP
     - Help for Beginners
   * - E.MY_UNIT_DID_HEAL
     - Unit Did Heal
   * - E.MY_UNIT_WAS_HEALED
     - Unit Was Healed
   * - E.MULTIPLIER
     - Multiplier Changed
   * - E.UNIT_ACTION_ACTOR_SUCCESS
     - Your Unit Did
   * - E.UNIT_ACTION_ACTOR_FAILURE
     - Your Unit Failed
   * - E.UNIT_ACTION_TARGET_HOSTILE
     - Unit Did to You
   * - E.UNIT_ACTION_TARGET_OTHER
     - Unit Did


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
     - Bribed by enemy diplomat. At v.3.1 and earlier, when a signal is called thus to an old unit, new one is created.
     - Yes
     - Yes*
   * - "captured"
     - Captured by enemy unit. At v.3.1 and earlier, when a signal is called thus to an old unit, new one is created.
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
   :literal:`*` At v.3.1 and earlier, the killer can be specified only by a script.
   
   :literal:`**` Killer submitted only if the action was failed.


Lua Built-ins
-------------

Some Lua builtin functions and modules are also available in Freeciv (some
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

