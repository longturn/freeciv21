.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Menu Bar
********

The Menu Bar consists of the following items:

* `Game Menu`_
* `View Menu`_
* `Select Menu`_
* `Unit Menu`_
* `Combat Menu`_
* `Work Menu`_
* `Multiplayer Menu`_
* `Civilization Menu`_
* `Help Menu`_


.. _game-manual-game-menu:

Game Menu
=========

The :guilabel:`Game` menu is used to conduct global level options on the client or the game being played. It
has the following options:

Save Game
    Saves the game as it is :strong:`right now` to the default save location.

Save Game As ...
    Saves the game as it is :strong:`right now` to a location of your choosing.

Save Map to Image
    Save a ``.png`` image file of the map to the user's pictures directory.

Interface Options
    Opens the :guilabel:`Interface Options` dialog box as described in the
    :ref:`Options <game-manual-options>` section above.

Game Options
    Opens the :guilabel:`Game Options` dialog as described in the
    :ref:`More Game Options <game-manual-more-game-options>` section above.


.. _game-manual-message-options:

Messages
    Opens the :guilabel:`Message Options` dialog as shown in :numref:`Message Options Dialog` below. The
    screenshot only shows a few rows of available options, many more will be found in the client. Any item
    with a check mark in the :guilabel:`Out` column will be shown in the :guilabel:`Server Chat/Command Line`
    widget. Any item with a check mark in the :guilabel:`Mes` column will be shown in the :guilabel:`Messages`
    widget (see :ref:`Messages <game-manual-messages>`). Lastly, any item with a check mark in the
    :guilabel:`Pop` column will be shown in a pop up message box window.

    You can go to the :doc:`message-options` page for a complete list of all available options.

.. _Message Options Dialog:
.. figure:: /_static/images/gui-elements/message-options.png
  :scale: 65%
  :align: center
  :alt: Freeciv21 Message Options dialog
  :figclass: align-center

  Message Options Dialog


.. _game-manual-shortcuts:

Shortcuts
    Opens the :guilabel:`Shortcuts` dialog as shown in :numref:`Shortcut Options Dialog` below. The dialog is
    used to set the preferred keyboard shortcuts to be used in the game. The screenshot only shows a few rows
    of available options, many more will be found in the client. To override the default, click your mouse
    cursor on the box on the right and then type the shortcut you want to set the setting to. For example: You
    want to change the :guilabel:`Scroll Map` option to the :guilabel:`PgUp` key, you would click in the box
    and then press the :guilabel:`PgUp` key on your keyboard. To reset to defaults, you can click on the
    :guilabel:`Reset` button. Click :guilabel:`Save` and then :guilabel:`Close` when finished.

    You can go to the :doc:`shortcut-options` page for a complete list of all available options.

.. _Shortcut Options Dialog:
.. figure:: /_static/images/gui-elements/shortcut-options.png
  :scale: 65%
  :align: center
  :alt: Freeciv21 Shortcut Options dialog
  :figclass: align-center

  Shortcut Options Dialog


Load Another Tileset
    Opens the :guilabel:`Available Tilesets` dialog as shown in :numref:`Available Tilesets Dialog` below. You
    can select any tileset installed by clicking on the name. You may get an error message if the tileset is
    not compatible with the current ruleset (for example: if it lacks a unit).

.. _Available Tilesets Dialog:
.. figure:: /_static/images/gui-elements/tileset.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 Available Tilesets dialog
  :figclass: align-center

  Available Tilesets Dialog


Tileset Debugger
    Opens the :guilabel:`Tileset Debugger` dialog. This option is documented in
    :doc:`/Modding/Tilesets/debugger`

Save Options Now
    Save the settings set in :ref:`Options <game-manual-options>` immediately.

Save Options on Exit
    Saves the settings set in :ref:`Options <game-manual-options>` when the client is exited.

Leave Game
    Allows you to leave the game and return to the start screen.

Quit
    Quits the client after a confirmation dialog box.


.. _game-manual-view-menu:

View Menu
=========

The :guilabel:`View` enables a user to manipulate what is shown on the
:ref:`Map View <game-manual-map-view>` as well as varying degrees of zooming in and out.

The :guilabel:`View` menu has the following options:

Center View
    With a unit selected, this menu option will place the unit in the center of the screen.

Fullscreen
    Sets Freeciv21 to use the whole screen, removing the title bar and operating system Task bar.

Minimap
    Shows or hides the :doc:`mini-map` in the lower right corner.

Show New Turn Information
    Enables or Disables populating new turn information in a widget on the
    :ref:`Map View <game-manual-map-view>`.

Show Detailed Combat Information
    Enables or Disables populating the :guilabel:`Battle Log` widget. When enabled you will see a widget
    appear on the screen (typically in the upper left corner) after combat occurs in your nation's vision.
    Your nation's vision is all map tiles that are visible to your nation, either natively or via shared
    vision treaty with an ally or team mate. The figure below gives an example of 3 combat events.

    .. _Battle Log:
    .. figure:: /_static/images/gui-elements/battle-log.png
      :align: center
      :alt: Battle Log
      :figclass: align-center

      Battle Log


    You can move the widget by click+dragging with your mouse on the plus symbol in the upper left corner.
    You can also scale the widget larger or smaller with the plus and minus icon buttons near the upper right
    corner. You can close the widget by clicking on the ``x`` symbol in the upper right corner. Lastly, if you
    click on the winning unit icon in a row, the client will move the map to where the combat occurred.

    If you do not do anything with the :guilabel:`Battle Log` widget after combat occurs, it will fade from
    the map automatically after 20 seconds.

Lock Interface
    Locks the user interface, preventing the move of objects around such as the server log/chat widget.

Zoom In
    Each selection of this menu option (or corresponding keyboard shortcut or user interface button next to
    the :doc:`mini-map`) will zoom in on the :ref:`Map View <game-manual-map-view>` one level.

Zoom Default
    Resets the zoom level to the default position at Zoom Level 0.

Zoom Out
    Each selection of this menu option (or corresponding keyboard shortcut or user interface button next to
    the :doc:`mini-map`) will zoom out on the :ref:`Map View <game-manual-map-view>` one level.

Scale Fonts
    Enables fonts to scale along with the zoom level.

Citybar Style
    This menu has a sub-menu of three options: :guilabel:`Simple`, :guilabel:`Traditional`, and
    :guilabel:`Polished` as shown in :numref:`Citybar Style Simple`, :numref:`Citybar Sytle Traditional`,
    and, :numref:`Citybar Style Polished`, respectively.

.. _Citybar Style Simple:
.. figure:: /_static/images/gui-elements/citybar-simple.png
  :align: center
  :alt: Citybar style - Simple
  :figclass: align-center

  Citybar Style - Simple


.. _Citybar Sytle Traditional:
.. figure:: /_static/images/gui-elements/citybar-traditional.png
  :align: center
  :alt: Citybar style - Traditional
  :figclass: align-center

  Citybar Style - Traditional


.. _Citybar Style Polished:
.. figure:: /_static/images/gui-elements/citybar-polished.png
  :align: center
  :alt: Citybar style - Polished
  :figclass: align-center

  Citybar Style - Polished


City Outlines
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` of the city's vision radius
    or outline.

City Output
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the city's food, production,
    and trade as shown in the :doc:`city-dialog`.

Map Grid
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the tile grid. This can be
    useful to help differentiate individual tiles from others.

National Borders
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the national borders of your
    neighbors. Each nation is given a color at game start (as seen on the
    :ref:`Nations and Diplomacy View <game-manual-nations-and-diplomacy-view>`).

Native Tiles
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` tiles that are native to the
    unit selected. Non-native tiles are marked with a red hash. Non-Native means that the unit cannot move
    there.

City Names
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the names of cities in the
    city bar.

City Growth
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the growth of cities in the
    city bar.

City Production Levels
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the production of cities in
    the city bar.

City Buy Cost
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` the cost to buy the
    currently constructed item in the city bar.

City Traderoutes
    Enables or Disables the display on the :ref:`Map View <game-manual-map-view>` trade routes between cities.


Select Menu
===========

The :guilabel:`Select` is used to select units on the game map in varying degrees. It has the following
options:

Single Unit (Unselect Others)
    Assuming you have selected multiple units (such as the next menu for :guilabel:`All on Tile`) and you want
    to quickly undo that. This menu supports that function.

All on Tile
    Quickly select all of the units on the same tile. This assumes that multiple units of different type are
    stacked on the same tile.

Same Type on Tile
    Quickly select all of the units of the same type on the tile. This assumes that multiple units of
    different types are stacked on the same tile.

Same Type on Continent
    Works the exact same way as the menu option above (:guilabel:`Same Type on Tile`) but expands the
    selection to the same island or continent.

Same Type Everywhere
    Even larger window of unit selection to pick all of the same type, but in all locations across the
    :ref:`Map View <game-manual-map-view>`. This is useful to help you find units placed in allied cities or
    to conduct a mass airlift.

Wait
    If you are not ready to move the currently selected unit, you can tell it to :strong:`wait` with this menu
    option. The rotation of unit selection will bypass this unit and will come back until you either
    move the unit or tell it you are done.

Done
    If you have moved the unit, but still have move points left or are simply done working with the unit for
    you can tell the client you are finished (done) with it with this menu item.

Advanced Unit Selection
    Opens the :guilabel:`Advanced Unit Selection` dialog box as shown in
    :numref:`Advanced Unit Selection Dialog` below. This dialog box gives you options to select a group of
    units using a more nuanced grouping method than the other :guilabel:`Select` menu options.

.. _Advanced Unit Selection Dialog:
.. figure:: /_static/images/gui-elements/advanced-unit-selection.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 Advanced Unit Selection dialog
  :figclass: align-center

  Advanced Unit Selection Dialog


.. _game-manual-unit-menu:

Unit Menu
=========

The :guilabel:`Unit` menu is used to give units orders. It has the following options:

Go to Tile
    With a unit selected, give the unit orders to go to a selected tile on the map.

Go to and ...
    Similar to :guilabel:`Go to Tile` above, however when the unit reaches the given tile you can give the
    unit a specific command to do as selected from the sub-menu. This is useful, for example, for
    :unit:`Settler` units to have them go to a spot and build a city as soon as possible.

Go to Nearest City
    Instruct the unit to go to the nearest city. Nearest in this context is the one that can be reached in
    the fewest move points (MPs).

Go to/Airlift to City...
    If Airlifting is enabled in the game (and assuming you have any required city improvement(s) that are
    required for Airlifting), you use this menu to tell the unit to transport to a city with the Airlift
    capability. A dialog box will pop up asking what city you want to Airlift to.

.. tip::
  Depending on the game rules, a player could use the :guilabel:`Select` menu to select many units of a
  similar type and then use this menu item to Airlift a great number of units all in one move very quickly.

Autoexplore
    Ask the unit to automatically open up the unknown (the area of the map that is black and has not been
    visited by any of your units).

Patrol
    Instruct a unit to make a collection of moves in a pattern as part of a patrol route.

Sentry
    Ask a unit to :strong:`Sentry`. Sentry is not the same as :strong:`Fortify` as found in the
    :guilabel:`Combat` menu. A sentried unit is on lookout and will give notice if another unit from an
    opponent comes into its field of vision.

.. note::
  A sentried unit does not gain a fortification bonus when outside of a city. However, sentried units inside
  of cities gain a default fortification bonus while in the city.

Unsentry All On Tile
    Instruct a stacked set of units on a single tile to stop that activity and ask for new orders.

Load
    Load a unit into a transporter, such as a :unit:`Caravel`, :unit:`Galleon`, or :unit:`Transport` ship.

Unload
    Unload a unit from a transporter.

Unload All From Transporter
    If you have many units inside of a transporter and you want all of them to disembark at the same time,
    then you can use this menu to make that nice and easy.

Set Home City
    Transfers ownership and management (e.g. support) of a unit to the city that it is currently present in.
    This menu allows you to shift support of units around to help with the cost of supporting units. Refer to
    :ref:`Economics View <game-manual-economics-view>` for more information on unit support costs.

Upgrade
    Upgrade a unit from one level to another. For example, :unit:`Phalanx` units are often upgradeable to an
    improved :unit:`Pikemen` with the discovery of :strong:`Feudalism`. The upgrade will cost gold and the
    client will tell you what that cost is before you agree to the spend.

Convert
    Similar to :guilabel:`Upgrade`. The convert option allows you to change a unit from one type to another.
    This is ruleset dependent and is not available in all rulesets.

Disband
    Use this menu option to eliminate (kill, destroy, fire) a unit. If performed inside of a city, then 50% of
    the shields used in the production of the unit is given to the city and helps build whatever is currently
    under construction. If a unit is disanded outside of a city, there is no benefit other than the
    elimination of shield or gold upkeep depending on your form of government.

Rename
    Give the unit a unit name. Similar to many Naval vessels in real life, you can name your units with a
    special name.


.. _game-manual-combat-menu:

Combat Menu
===========

The :guilabel:`Combat` menu is used to give combat units orders. It has the following options:

Fortify Unit
    Instruct the unit to :strong:`Fortify` on the tile. An icon will show on the unit signifying
    fortification. By fortifying, the unit is given a defensive bonus depending on the terrain it is on. See
    in game help for more specifics of what defense bonuses are given by terrain type. When a unit is
    fortifying, it is not in :strong:`Sentry` mode and will not inform you of enemy unit movement inside of
    its vision radius.

Build Fortress/Buoy
    Some units have the ability to build forts, fortresses, and buoys. They are typically :unit:`Workers` or
    :unit:`Engineers`. In some rulesets, :unit:`Transports` can build Buoys. Forts, Pre-Forts, and Fortresses
    give a unit increased defensive bonuses in addition to what is provided by the base terrain. See in
    game help for the specifics. Buoys are used to act as sentries in the oceans around your cities and can
    give you opponent unit movement information.

Build Airbase
    Instructs a unit to build an Airbase. This is often a requirement for Airlifting in many rulesets. They
    also often give the ability to heal an aircraft type unit faster while on the tile.

Build Base
    This generic menu will include a sub-menu of all of the base type tile improvements that can be built
    as defined by the current ruleset.

Pillage
    Tells a unit to remove (pillage) tile infrastructure improvements such as roads, railroad, and bases.

Do ...
    A dialog box will pop up here and give you all of the actions that the unit selected can perform.


.. _game-manual-work-menu:

Work Menu
=========

The :guilabel:`Work` menu is used to give units infrastructure work orders such as building roads, irrigation,
or mines. Tile infrastructure improvements are mostly done with :unit:`Workers` and :unit:`Engineers`, however
some rulesets allow other units to perform this type of work. See in game help on units for details. It has
the following options:

Build City
    Certain units such as :unit:`Settlers` can create cities. If the unit has sufficient move points
    available, then giving this command will build a new city where the unit is currently placed on the
    :ref:`Map View <game-manual-map-view>`. The unit will be consumed by the action.

Auto Worker
    Tell a :unit:`Worker` to use an in game algorithm to improve tiles. The game engine will give the
    :unit:`Worker` instructions so you do not have to.

Build Road/Railroad/Maglev
    Tell a :unit:`Worker` to build a road. If sufficient technological knowledge is available, then a railroad
    and eventually a maglev may be able to be constructed at a later time during the game. The menu will
    change with the best available option depending on what has been done to improve the tile in the past.

Build Path
    Provides a sub-menu of all of the pathing options available for the tile. This is mostly road, railroad,
    and maglev. Other rulesets may have different path types. See in game help for more details.

Build Irrigation/Farmland
    Tell a :unit:`Worker` to irrigate the tile in order to improve the food output from the tile. If
    sufficient technological knowledge is available, then farmland may be added to the tile at a later
    time during the game. The menu will change with the best available option depending on what has been
    done to improve the tile in the past.

Cultivate to Plains
    Cultivation is a multi-step process where a tile is converted from one type to another. Such as converting
    a swamp to plains. Not all tile terrain types can be cultivated to other types. See in game help for
    details.

Build Mine
    Tells a :unit:`Worker` to build a mine on a tile to improve the shield output.

Plant Forest/Swamp
    If the unit is on a grassland tile, then you can tell the :unit:`Worker` to plant a forest on the tile. If
    the unit is on a forest tile, then you can tell the :unit:`Worker` to convert the forest to swamp.

Connect with Road
    Tell a :unit:`Worker` to build a road many times along a given path.

Connect with Railroad/Maglev
    Tell a :unit:`Worker` to build a railroad or maglev many times along a given path.

Connect with Irrigation
    Tell a :unit:`Worker` to connect many tiles together with irrigation. This is often done to get
    irrigation from a source of fresh water over to a city.

Transform to Hills/Swamp/Ocean
    Tell an :unit:`Engineer` to conduct a major terraforming operation on the tile. Mountains can be cut down
    to hills, forests, and grassland can be converted to swamp and then the swamp can be converted to ocean.
    In some circumstances, an ocean tile can be converted to swamp and then the swamp can be converted to
    grassland. In most rulesets, only the :unit:`Engineer` unit can do these major operations.

Clean Pollution
    Tell a :unit:`Worker` to clean pollution from the tile. Pollution on a tile will eliminate or severely
    cripple the output of a tile and contributes to global warming.

Clean Nuclear Fallout
    If a :unit:`Nuclear` unit has been detonated nearby (e.g. attacked a city), then there will be nuclear
    fallout all over the place. Similar to pollution, nuclear fallout severely cripples the output of a tile
    and contributes to nuclear winter.

Help Build Wonder
    Certain units, such as :unit:`Caravan` can be used to move production from one city to another and help
    build small and great wonders. This menu aids that function. Alternately you can simply "walk" the
    :unit:`Caravan` into a city and a pop up dialog will ask what you want to do.

Establish Trade Route
    Certain units, such as :unit:`Caravan` can be used to establish a trade route between two cities. This
    menu aids that function.

As you can see, there are a number of ways that a tile can be altered with infrastructure improvements. Be
sure to have a close look at the in game help on Terrain for more information.


Multiplayer Menu
================

The :guilabel:`Multiplayer` menu has a collection of functions to aid certain multiplayer games. Many of
the options are specifically tailored to the MP2c and WarCiv rulesets. It has the following options:

Delayed GoTo
    Give a unit orders to move at a specific time in the turn. This assumes that the turn is time based.

Delayed Orders Execute
    Execute an action by a unit at a specific time in the turn. This assumes that the turn is time based.

Clear Orders
    Clear any delayed orders from the above two menu items.

Add All Cities to Trade Planning
    All all current cities in your into an advanced trade planning array. This is used by the WarCiv
    ruleset.

Calculate Trade Planning
    Run a trade effectiveness algorithm across all of the cities in the trade plan to determine the best
    routes. This is used by the WarCiv ruleset.

Add/Remove City
    Add or remove a city from the trade planning array.

Clear Trade Planning
    Clear all trade planning to start over.

Automatic Caravan
    Any :unit:`Caravan` units built by a city will follow the trade planning output for that city. This is
    used by the WarCiv ruleset.

Set/Unset Rally Point
    Set or remove a rally point to easily send units, once produced, to a specific spot on the game map.

Quick Airlift
    Depending on what is selected in the menu below, you can quickly airlift a unit to a destination city.

Unit Type for Quickairlifting
    Select the type of unit that will be quickly airlifted by the menu above.

Default Action vs Unit
    A sub-menu will show some optional actions that a unit should do by default against another unit.
    The default is :strong:`Ask`.

Default Action vs City
    A sub-menu will show some optional actions that a unit should do by default against a city.
    The default is :strong:`Ask`.


.. _game-manual-civilization-menu:

Civilization Menu
=================

The :guilabel:`Civilization` menu is used to gain access to many functions of your empire. You can load up
pages for units, cities, nations, etc; change the form of government and see how you are doing compared to
your opponents with the demographics report. It has the following options:

National Budget
    Selecting this menu item will bring up a dialog box allowing you to set the rate in percentage points for
    gold (taxes), science (bulbs), and luxury (goods). This is the same as clicking on the
    :ref:`National Budget View <game-manual-national-budget-view>` button on the :doc:`top-bar`.
    :numref:`National Budget Dialog` dialog below, shows a sample screenshot. In this example, the player's
    nation is in Democracy, has set gold to 30%, science to 40%, and luxury to 30%.

.. _National Budget Dialog:
.. figure:: /_static/images/gui-elements/tax-rates.png
  :scale: 65%
  :align: center
  :alt: Freeciv21 National Budget dialog
  :figclass: align-center

  National Budget Dialog


.. note::
  In Freeciv21 the National Budget is simplified into three segments: gold collection, scientific output, and
  luxury goods.

Government
    Depending on your technological progress through the game, you may be able to select a different form
    of government. The change is achieved from this menu item. This is the same as clicking on the
    :ref:`National Status View <game-manual-national-status-view>` option on the :doc:`top-bar`.

View
    Clicking this menu item will show you the main game map. This is the same as clicking on the
    :ref:`Map View <game-manual-map-view>` option on the :doc:`top-bar`.

Units
    Clicking this menu item will show you the units report widget. This is the same as clicking on the
    :ref:`Units View <game-manual-units-view>` option on the :doc:`top-bar`.

Players
    Clicking this menu item will show you the nations and diplomacy view. This is the same as clicking on the
    :ref:`Nations and Diplomacy View <game-manual-nations-and-diplomacy-view>` option on the :doc:`top-bar`.

Cities
    Clicking this menu item will show you the cities view. This is the same as clicking on the
    :ref:`Cities View <game-manual-cities-view>` option on the :doc:`top-bar`.

Economy
    Clicking this menu item will show you the economics view. This is the same as clicking on the
    :ref:`Economics View <game-manual-economics-view>` option on the :doc:`top-bar`.

Research
    Clicking this menu item will show you the research tree view. This is the same as clicking on the
    :ref:`Research View <game-manual-research-view>` option on the :doc:`top-bar`.

Wonders of the World
    Clicking this menu item will show you a traveler's report widget on the
    :ref:`Map View <game-manual-map-view>`. The widget will give information on any cities that have
    constructed any of the great wonders.

.. _Wonders of the World:
.. figure:: /_static/images/gui-elements/wonders.png
  :scale: 60%
  :align: center
  :alt: Freeciv21 Wonders of the World
  :figclass: align-center

  Wonders of the World


Top Five Cities
    Clicking this menu item will show you a traveler's report widget on the
    :ref:`Map View <game-manual-map-view>`. The widget will give information on the top five largest cities.

.. _Top Five Cities:
.. figure:: /_static/images/gui-elements/top-five-cities.png
  :align: center
  :scale: 60%
  :alt: Freeciv21 Top Five Cities
  :figclass: align-center

  Top Five Cities


Demographics
    Clicking this menu item will show you a demographics report widget on the
    :ref:`Map View <game-manual-map-view>`. The widget will give information about how your nation stacks up
    against your opponents. If you have an embassy with your opponents in the game, the demographics report
    will tell you who is #1, if you are not #1.

.. _Demographics:
.. figure:: /_static/images/gui-elements/demographics.png
  :align: center
  :scale: 65%
  :alt: Freeciv21 Demographics
  :figclass: align-center

  Demographics

Spaceship
    Clicking this menu item will show you the spaceship view. The space race is a ruleset defined option and
    is not enabled in all rulesets. Your nation must also be very technologically advanced to build the
    components needs for a spaceship. See in game help for more details.

    The client will automatically place the components for you as you construct them.

Achievements
    Clicking this menu item will show you an achievements report widget on the main map. Achievements are
    a ruleset defined option and not enabled by default in many rulesets.

.. _Achievements:
.. figure:: /_static/images/gui-elements/achievements.png
  :align: center
  :alt: Freeciv21 Achievements
  :figclass: align-center

  Achievements


Help Menu
=========

The :guilabel:`Help` menu gives you access to the in-game help. It has the following chapters:

  * Overview
  * Strategy and Tactics
  * Terrain
  * Economy
  * Cities
  * City Improvements
  * Wonders of the World
  * Units
  * Combat
  * Zones of Control
  * Government
  * Effects
  * Diplomacy
  * Technology
  * Space Race
  * About Current Tileset
  * About Current Ruleset
  * About Nations
  * Connecting
  * Controls
  * Citizen Governor
  * Chatline
  * Worklist Editor
  * Languages
  * Copying
  * About Freeciv21

Each of these options is simply a quick link to the same named section in the game help menu.
