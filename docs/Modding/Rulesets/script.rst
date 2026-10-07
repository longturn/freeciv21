.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: Freeciv Wiki contributors <https://freeciv.fandom.com/wiki/Lua_reference_manual?action=history>
.. SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

.. Usage references:
.. https://longturn.readthedocs.io/en/latest/Contributing/style-guide.html
.. https://luals.github.io/wiki/definition-files
.. https://luals.github.io/wiki/annotations/#documenting-types
.. https://sphinx-lua-ls.readthedocs.io/en/stable/autodoc.html#autodoc-directives
.. https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-primer

.. include:: /global-include.rst

Lua Scripting
*************

:file:`game/script.lua` runs when a ruleset is first loaded and is used to
initialize all of the Lua code for a ruleset. For simpler rulesets, this may
contain all Lua code. E.g.

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

However, as script complexity increases, you may wish to organize your code into
modules using the :lua:obj:`require` function. See
:ref:`Advanced Scripting <script-advanced>`

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

.. lua:autoobject:: Team
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

.. lua:autoobject:: Unit_Class
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

.. lua:autoobject:: log
   :members:
   :recursive:
.. lua:autoobject:: players_iterate
.. lua:autoobject:: whole_map_iterate

Debugging
^^^^^^^^^

.. lua:autoobject:: listenv
.. lua:autoobject:: _freeciv_state_dump
.. lua:autoobject:: signal.list
.. lua:autoobject:: fc_version

:lua:obj:`_VERSION`

A global variable (not a function) that holds a string containing the running
Lua version. The current value of this variable is "Lua 5.4". 

:lua:func:`assert`

Raises an error if the value of its argument v is false (i.e., nil or false);
otherwise, returns all its arguments. In case of error, message is the error
object; when absent, it defaults to "assertion failed!" 

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
   - :literal:`*` The killer can be specified only by a script.
   - :literal:`**` Killer submitted only if the action was failed.
   - :literal:`***` A new unit is created under the new ownership to replace the old unit after the signal is processed.


Lua Built-ins
-------------

Some Lua builtin functions and modules are also available in Freeciv21 (some
functionality is intentionally left out by policy). It is not our intention to
document Lua builtins here, but just to mention a selection of the useful parts. 

.. _script-lua-builtin-functions:

Lua Functions
^^^^^^^^^^^^^

.. lua:function:: require(file_path: string): (module: table)

A restricted version of the standard Lua
`require <https://www.lua.org/manual/5.3/manual.html#pdf-require>`_ function.
This is limited to only :file:`.lua` files under the current
:ref:`rulesetdir <server-command-rulesetdir>` or in a :file:`lua` directory
under the data path.

It can be used for more :ref:`Advanced Scripting <script-advanced>` to
separate out concerns in more complex scripts, making code reuse easier, and
allows importing useful tools from the base game.

The full `require <https://www.lua.org/manual/5.3/manual.html#pdf-require>`_
is available via :ref:`lua unsafe-cmd <server-command-lua>` or in client
scripts.

:lua:func:`pcall`

Calls the function f with the given arguments in protected mode. This means that
any error inside f is not propagated; instead, pcall catches the error and
returns a status code. Its first result is the status code (a boolean), which is
true if the call succeeds without errors. In such case, pcall also returns all
results from the call, after this first result. In case of any error, pcall
returns false plus the error object. Note that errors caught by pcall do not
call a message handler.

:lua:func:`pairs` 

If t has a metamethod __pairs, calls it with t as argument and returns the
first three results from the call.

Otherwise, returns three values: the next function, the table t, and nil, so
that the construction

.. code-block:: lua

   for k,v in pairs(t) do body end

will iterate over all key–value pairs of table t.

See function next for the caveats of modifying the table during its traversal. 

:lua:func:`ipairs`

Returns three values (an iterator function, the table t, and 0) so that the
construction

.. code-block:: lua

   for i,v in ipairs(t) do body end

will iterate over the key–value pairs (1,t[1]), (2,t[2]), ..., up to the first
absent index.

Lua Globals
^^^^^^^^^^^

:lua:obj:`_G` 

A global variable (not a function) that holds the global environment (see 
`§2.2 <https://www.lua.org/manual/5.4/manual.html#2.2>`_). Lua itself does not
use this variable; changing its value does not affect any environment, nor vice
versa.

.. _script-lua-builtin-modules:

Lua Modules
^^^^^^^^^^^

.. lua:autoobject:: os
   :members: time, date, difftime, clock
   :recursive:

.. lua:autoobject:: math
   :members: abs, ceil, floor, max, min
   :recursive:

.. lua:table:: string

   `The String Library <https://www.lua.org/pil/20.html>`_

   This is a subset of useful functions. There are more available in the manual:
   `string <https://www.lua.org/manual/5.4/manual.html#pdf-string>`_.

   .. lua:function:: find(s: string, pattern: string, init: Number, plain: boolean): (start: Number, end: Number, groups: string...)

      Looks for the first match of pattern (see 
      `§6.4.1 <https://www.lua.org/manual/5.4/manual.html#6.4.1>`_) in the 
      string s. If it finds a match, then find returns the indices of s where
      this occurrence starts and ends; otherwise, it returns fail. A third,
      optional numeric argument init specifies where to start the search; its
      default value is 1 and can be negative. A true as a fourth, optional
      argument plain turns off the pattern matching facilities, so the
      function does a plain "find substring" operation, with no characters in
      pattern being considered magic.

      If the pattern has captures, then in a successful match the captured
      values are also returned, after the two indices.

      See `string.find <https://www.lua.org/manual/5.4/manual.html#pdf-string.find>`_

   .. lua:function:: gmatch(s: string, pattern: string, init: Number): (matches: iterator)

      Returns an iterator function that, each time it is called, returns the
      next captures from pattern (see 
      `§6.4.1 <https://www.lua.org/manual/5.4/manual.html#6.4.1>`_) over the
      string s. If pattern specifies no captures, then the whole match is
      produced in each call. A third, optional numeric argument init specifies
      where to start the search; its default value is 1 and can be negative.

      See `string.gmatch <https://www.lua.org/manual/5.4/manual.html#pdf-string.gmatch>`_

   .. lua:function:: match(s: string, pattern: string, init: Number): (match: string)

      Looks for the first match of the pattern (see
      `§6.4.1 <https://www.lua.org/manual/5.4/manual.html#6.4.1>`_) in the
      string s. If it finds one, then match returns the captures from the
      pattern; otherwise it returns fail. If pattern specifies no captures,
      then the whole match is returned. A third, optional numeric argument init
      specifies where to start the search; its default value is 1 and can be
      negative.

      See `string.match <https://www.lua.org/manual/5.4/manual.html#pdf-string.match>`_

   .. lua:function:: format(format: string, args...: any): (formatted: string)

      Returns a formatted version of its variable number of arguments following
      the description given in its first argument, which must be a string. The
      format string follows the same rules as the ISO C function sprintf. The
      only differences are that the conversion specifiers and modifiers F, n,
      \*, h, L, and l are not supported and that there is an extra specifier, q.
      Both width and precision, when present, are limited to two digits.

      See `string.format <https://www.lua.org/manual/5.4/manual.html#pdf-string.format>`_

   .. lua:function:: len(s: string): (length: Number)

      Receives a string and returns its length. The empty string "" has length
      0. Embedded zeros are counted, so "a\000bc\000" has length 5. 

      See `string.len <https://www.lua.org/manual/5.4/manual.html#pdf-string.len>`_

   .. lua:function:: lower(s: string): (converted: string)

      Receives a string and returns a copy of this string with all uppercase
      letters changed to lowercase. All other characters are left unchanged.
      The definition of what an uppercase letter is depends on the current
      locale. 

      See `string.lower <https://www.lua.org/manual/5.4/manual.html#pdf-string.lower>`_

   .. lua:function:: upper(s: string): (converted: string)

      Receives a string and returns a copy of this string with all lowercase
      letters changed to uppercase. All other characters are left unchanged.
      The definition of what a lowercase letter is depends on the current
      locale. 

      See `string.upper <https://www.lua.org/manual/5.4/manual.html#pdf-string.upper>`_

.. lua:table:: table

   `The Table Library <https://www.lua.org/pil/19.html>`_
   `Array Size <https://www.lua.org/pil/19.1.html>`_

   This is a subset of useful functions. There are more available in the manual:
   `table <https://www.lua.org/manual/5.4/manual.html#pdf-table>`_.

   .. lua:function:: concat(list: table, sep: string|Number, i: Number, j: Number): (joined: string)

      Given a list where all elements are strings or numbers, returns the
      string list[i]..sep..list[i+1] ··· sep..list[j]. The default value for
      sep is the empty string, the default for i is 1, and the default for j is
      #list. If i is greater than j, returns the empty string. 

      `table.concat <https://www.lua.org/manual/5.4/manual.html#pdf-table.concat>`_

   .. lua:function:: insert(list: table, [pos: Number,] value: any)

      Inserts element value at position pos in list, shifting up the elements
      list[pos], list[pos+1], ···, list[#list]. The default value for pos is
      #list+1, so that a call table.insert(t,x) inserts x at the end of the
      list t.

      See `Insert and Remove <https://www.lua.org/pil/19.2.html>`_
      `table.insert <https://www.lua.org/manual/5.4/manual.html#pdf-table.insert>`_

   .. lua:function:: remove(list: table [, pos: Number]): (removed: any)

      Removes from list the element at position pos, returning the value of the
      removed element. When pos is an integer between 1 and #list, it shifts
      down the elements list[pos+1], list[pos+2], ···, list[#list] and erases
      element list[#list]; The index pos can also be 0 when #list is 0, or
      #list + 1.

      See `Insert and Remove <https://www.lua.org/pil/19.2.html>`_
      `table.remove <https://www.lua.org/manual/5.4/manual.html#pdf-table.remove>`_

   .. lua:function:: sort(list: table, comp: function)

      Sorts the list elements in a given order, in-place, from list[1] to
      list[#list]. If comp is given, then it must be a function that receives
      two list elements and returns true when the first element must come
      before the second in the final order, so that, after the sort, i <= j
      implies not comp(list[j],list[i]). If comp is not given, then the
      standard Lua operator < is used instead.

      The comp function must define a consistent order; more formally, the
      function must define a strict weak order. (A weak order is similar to a
      total order, but it can equate different elements for comparison
      purposes.)

      The sort algorithm is not stable: Different elements considered equal by
      the given order may have their relative positions changed by the sort.

      See `Sorting <https://www.lua.org/pil/19.3.html>`_
      `table.sort <https://www.lua.org/manual/5.4/manual.html#pdf-table.sort>`_

.. _script-advanced:

Advanced Scripting
==================

As script complexity starts to increase, you may find it useful to structure
your Lua code into modules using the :lua:obj:`require` function. E.g.

.. code-block:: text

   MyRuleset/
   |- MyRuleset/
   |  |- feature1/
   |  |  |- tests.lua
   |  |- feature1.lua
   |  |- feature2/
   |  |  |- tests.lua
   |  |- feature2.lua
   |  |- script.lua
   |  |- settings.lua
   |  |- tests.lua
   |- MyRuleset.serv

Your :file:`script.lua` will then look something like:

.. code-block:: lua

    local settings = require("MyRuleset.settings")
    local feature1 = require("MyRuleset.feature1")
    local feature2 = require("MyRuleset.feature2")

    function on_turn_begin(turn, year)
      feature1.do_the_thing(turn)
      feature2.do_something()
    end

    signal.connect("turn_begin", "on_turn_begin")

    function on_unit_moved(unit, tile_from, tile_to)
      feature2.track_unit(unit, tile_from, tile_to)
    end

    signal.connect("unit_moved", "on_unit_moved")

    feature1.init(settings)
    feature2.init(settings)

Creating Modules
----------------

A Lua module is basically a script that returns an object. It could be a table
of data, a function, or a table of functions.

It might look something like this:

.. code-block:: lua

    local M = {}

    function M.do_the_thing(turn)
      if not M.settings.enabled then return end
      if turn == M.settings.initial_turn then
        something_cool_happened = true
      end
    end

    function M.init(settings)
      M.settings = settings.feature1
    end

    return M

In this case, we have a module that can do something interesting on a
configurable turn number. However, loading the module itself will do nothing,
it just provides you with a table of functions to call in your main
:file:`script.lua` or elsewhere.

The :file:`settings.lua` file in this example acts as a convenient place to modify
configuration settings for all of the Lua features. It might look something
like this:

.. code-block:: lua

    settings = {
      feature1 = {
        enabled = true,
        initial_turn = 10
      },
      feature2 = {
        enabled = true
      }
    }

    return settings

Since the ``settings`` are global in this example, they could then also be
modified in :file:`MyRuleset.serv` like this:

.. code-block:: text

    lua cmd settings.feature1.enabled = true
    lua cmd settings.feature1.initial_turn = 12

    lua cmd settings.feature2.enabled = false

Unit Testing
------------

To provide support for unit testing, we have included a copy of
`luaunit v3.5 <https://luaunit.readthedocs.io/en/luaunit_v3_5>`_, which can be
imported using the :lua:obj:`require` function.

The simplest way is to add test functions to the global scope. Anything that
starts with `test` (case-insensitive) will automatically be picked up as a unit
test by luaunit. E.g.

.. code-block:: lua

    local lu = require("luaunit")

    function testCanMoveUnit()
      local unit = find.unit(nil, 437)
      lu.assertTrue(unit:move(find.tile(42, 42), 0))
      lu.assertEquals(unit.tile.x, 42)
      lu.assertEquals(unit.tile.y, 42)
    end

    function run(...)
      os.exit(lu.LuaUnit.new():run(...))
    end

.. note::

   Note that ``os.exit`` is a shim added for compatibility with external
   modules like luaunit. It does not exit the server as the real 
   `os.exit <https://www.lua.org/manual/5.3/manual.html#pdf-os.exit>`_ would
   do, just generates an error if it is a fail status and reports whether it
   was a success or fail, and the code.

You can then run the tests from the server console, like:

.. code-block:: text

    lua cmd require("MyRuleset.tests").run("-v")

However, if you want to avoid polluting the global namespace, or organize your
tests in a more structured way, you can also run tests explicitly with
`runSuiteByInstances <https://luaunit.readthedocs.io/en/luaunit_v3_5/4_reference_doc.html#LuaUnit.runSuiteByInstances>`_. E.g.

.. code-block:: lua

    local lu = require("luaunit")

    function run(...)
      os.exit(lu.LuaUnit.new():runSuiteByInstances({
        {"TestFeature1", require("MyRuleset.feature1.tests")},
        {"TestFeature2", require("MyRuleset.feature2.tests")}
      }, "--verbose", ...))
    end

In this instance, each feature module gets its own set of tests that are
grouped separately in the readout.

.. code-block:: lua

    local lu = require("luaunit")
    local feature1 = require("feature1")

    local M = {}

    function M.testDoesSomethingCool()
      feature1.init({
        feature1 = {
          enabled = true,
          initial_turn = 5
        }
      })
      feature1.do_the_thing(5)
      lu.assertTrue(something_cool_happened)
    end

    return M

