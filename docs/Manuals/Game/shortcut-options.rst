.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: 2023 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Client Shortcut Options
***********************

As discussed in the :ref:`Shortcut Options <client-manual-shortcuts>` section of the game manual, there is a
large collection of keyboard and mouse shortcut options available. This page lists them in detail. Here is a
sample of the top of the shortcut options dialog.

.. _Shortcut Options Dialog2:
.. figure:: /_static/images/gui-elements/shortcut-options.png
  :scale: 65%
  :align: center
  :alt: Freeciv21 Shortcut Options dialog
  :figclass: align-center

  Shortcut Options Dialog


To override the default (e.g. change a shortcut setting), click your mouse cursor on the box on the right and
then type the shortcut you want to set the setting to. For example: You want to change the
:guilabel:`Scroll Map` option to the :guilabel:`PgUp` key, you would click in the box and then press the
:guilabel:`PgUp` key on your keyboard. To reset to defaults, you can click on the :guilabel:`Reset` button.
Click :guilabel:`Save` and then :guilabel:`Close` when finished.

In most cases, the keyboard shortcut will also show on an associated menu entry depending on the type of
shortcut.

Here are the available shortcuts:

Scroll Map
  Scroll the map view. You can also use two-finger swipe on a trackpad if you have one (such as on a laptop)
  in all four directions to scroll the map.

  Default: Right mouse button

Center View
  With a unit selected, pressing this shortcut will center the unit on the screen. Similar to the option on
  the :guilabel:`View` menu.

  Default: ``C``

Fullscreen
  Toggle fullscreen mode of the game interface.

  Default: ``Alt+Return``

Show minimap
  Toggle the display of the minimap.

  Default: ``Ctrl+M``

City Output
  Toggle the display of the city citizen managed tile output on the main map.

  Default: ``Ctrl+W``

Map Grid
  Toggle the display of the map grid on the main map.

  Default: ``Ctrl+G``

National Borders
  Toggle the display of the national borders of players on the main map.

  Default: ``Ctrl+B``

Quick buy from map
  Quick rush buy production without opening up the city dialog. When used, whatever is selected from the
  menu will be immediately purchased from your national treasury.

  Default: ``Ctrl+Shift`` plus Left mouse button

Quick production select from map
  Quick production change for a city without opening up the city dialog. When used, whatever is selected from
  the menu will be immediately added to the top of the work list.

  Default: ``Ctrl`` plus Left mouse button

Select button
  Select an item on the map, such as a unit or a city.

  Default: Left mouse button

Adjust workers
  Auto adjust the citizens managing tiles in a city without opening up the city dialog. This is the same as
  opening a city in the city dialog and clicking on the city center tile.

  Default: ``Meta+Ctrl`` plus Left mouse button (for example, ``Meta`` is the windows key for Windows
  keyboards).

Append focus
   When applied to units, allow you to move a unit with a goto order without having to enter the
   :guilabel:`Unit` menu.

   Default: ``Shift`` plus Left mouse button

Popup tile info
  Pop up a widget of information about the tile, unit(s) on the tile, and other information related to the
  tile.

  Default: Middle mouse button (different OSs handle this differently and is often a press of the scroll
  wheel).

Wakeup sentries
  Quickly wake up all units that are sentried.

  Default: ``Ctrl`` plus Middle mouse button (different OSs handle this differently and is often a press of
  the scroll wheel).

Show link to tile
  Copies a link to a tile to the :guilabel:`Server Chat/Command Line` widget on the main map.

  Default: ``Ctrl+Alt`` plus Right mouse button

Paste Production
  Pastes a previously copied production item to the top of a selected city's worklist. See Copy Production
  below.

  Default: ``Ctrl+Shift`` plus Right mouse button

Copy Production
  Copy the item being produced in the selected city into a memory buffer. See Paste Production above.

  Default: ``Shift`` plus Right mouse button

Show/hide workers
  Toggle the display of :unit:`Workers` or :unit:`Engineers` on the main map.

  Default: ``Alt+Shift`` plus Right mouse button

Units selection (for tile under mouse position)
  Opens a widget to allow you to select a unit from a stack for movement.

  Default: ``Ctrl+Spacebar``

City Traderoutes
  Toggles the display of traderoutes for your cities.

  Default: ``Ctrl+D``

City Production Levels
  Toggles the disply of city production in the city bar.

  Default: ``Ctrl+P``

City Names
  Toggles the display of the city name in the city bar.

  Default: ``Ctrl+N``

Done Moving
  Tell the selected unit that you are finished with it, even if it still has move points available.

  Default: ``Spacebar``

Go to/Airlift to City...
  Instruct a selected unit to go to or airlift to a city. Go to can happen on any tile on the main map.
  Airlift requires the unit to be inside a city that has an :improvement:`Airport`.

  Default: ``T``

Auto Explore
  Instruct a unit to have the computer control where it explores around the map.

  Default: ``X``

Patrol
  Tell a unit to patrol. When used, you will give the unit a patrol route with the mouse.

  Default: ``Q``

Unsentry All on Tile
  Unsentry all sentried units on the tile under the mouse pointer.

  Default: ``Ctrl+Shift+D``

Do...
  Instruct the unit to do some action. Be sure to have the :guilabel:`Messages` widget open when using this
  shortcut as there are often instructions given there.

  Default: ``D``

Upgrade
  Upgrade the selected unit to a new type. The unit must be in one of your cities and you need to have
  sufficient gold in your national treasury.

  Default: ``Ctrl+U``

Set Home City
  Changes a unit's supporting home city to a new home city. The unit must be inside of the city that will
  become its new supporting home city.

  Default: ``H``

Build Mine
  Instruct a :unit:`Workers` or :unit:`Engineers` to build a mine on the tile.

  Default: ``M``

Plant
  Instruct a :unit:`Workers` or :unit:`Engineers` to plant on the tile.

  Default: ``Shift+M``

Build Irrigation
  Instruct a :unit:`Workers` or :unit:`Engineers` to build irrigation on the tile.

  Default: ``I``

Cultivate
  Instruct a :unit:`Workers` or :unit:`Engineers` to cultivate on the tile.

  Default: ``Shift+I``

Build Road
  Instruct a :unit:`Workers` or :unit:`Engineers` to build a road/railroad/maglev on the tile.

  Default: ``R``

Build City
  Instruct a :unit:`Settlers` to build a new city on the tile.

  Default: ``B``

Sentry
  Instruct a unit to sentry on the tile.

  Default: ``S``

Fortify
  Instruct a unit to fortify on the tile.

  Default: ``F``

Go to Tile
  Instruct a unit to go to a selected tile with the Left mouse button.

  Default: ``G``

Wait
  Tell a unit to wait as you are not ready to give it orders yet.

  Default: ``W``

Transform
  Instruct a :unit:`Workers` or :unit:`Engineers` to transform a tile.

  Default: ``O``

Explode Nuclear
  Explode a :unit:`Nuclear` bomb on the tile where the unit is located. Assumes you have moved the unit to the
  tile where you want to explode it.

  Default: ``Shift+N``

Load
  Load a unit on a transporter, such as a :unit:`Transport`.

  Default: ``L``

Unload
  Unload a unit from a transporter, such as a :unit:`Transport`.

  Default: ``U``

Quick buy current production from map
  Quick rush buy the current item being produced without opening up the city dialog. When used, the item will
  be immediately purchased from your national treasury.

  Default: BackButton (Different OSs handle this differently. Often there is a similar named button on your
  mouse.)

Lock/Unlock interface
  Lock the placement of the widgets on the screen so they cannot be accidentally moved.

  Default: ``Ctrl+Shift+L``

Auto Worker
  Instruct a :unit:`Workers` or :unit:`Engineers` to allow the computer to control their work.

  Default: ``A``

Paradrop/clean pollution
  If you have a :unit:`Paratroopers` selected you can send it to a tile of your choice with the Left mouse
  button. Alternately, if you have a :unit:`Workers` or :unit:`Engineers` selected, you can instruct them to
  clean pollution on the tile they are on. In some rulesets, :unit:`Transport` units can also remove pollution
  from ocean tiles.

  Default: ``P``

Popup combat information
  When used just before a combat round, will output the combat results into the
  :guilabel:`Server Chat/Command Line` widget on the main map.

  Default: ``Ctrl+F1``

Reload Theme
  Reloads the current theme.

  Default: ``Ctrl+Shift+F5``

Reload Tileset
  Reloads the current tileset.

  Default: ``Ctrl+Shift+F6``

Toggle city full bar visibility
  Toggles the display of the city bar.

  Default: ``Ctrl+F``

Zoom In
  Zoom in on the map.

  Default: ``+`` (plus sign)

Zoom Out
  Zoom out on the map.

  Default: ``-`` (minus sign)

Load Lua Script
  Load a pre-written Lua script file and runs it.

  Default: ``Ctrl+Shift+J``

Load last loaded Lua Script
  Re-run the previously selected Lua script file.

  Default: ``Ctrl+Shift+K``

Reload tileset with default scale
  Reload the current tileset and go to the default zoom level.

  Default: ``Ctrl+Backspace``

Go And Build City
  Instruct a :unit:`Settlers` to go to a tile and as soon as possible build a city there.

  Default: ``Shift+B``

Go and Join City
  Instruct a :unit:`Settlers` or :unit:`Migrants` to go to a selected city as as soon as possible add to the
  city's population.

  Default: ``Shift+J``

Pillage
  Instruct a unit to pillage infrastructure improvements on a tile.

  Default: ``Shift+P``
