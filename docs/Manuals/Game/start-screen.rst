..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Starting the Game
*****************

Depending on how you installed Freeciv21 will determine how you start it. If you installed with one of the
precompiled binary packages, then you should find Freeciv21 in your OS's launcher, Start Menu, etc. If you
compiled the code yourself, then you will go to the location you asked ``--target install`` to place the
files. Double-clicking ``freeciv21-client`` should start it up.

.. _Start Screen:
.. figure:: /_static/images/gui-elements/start-screen.png
    :scale: 70%
    :align: center
    :alt: Freeciv21 start screen
    :figclass: align-center

    Start Screen with NightStalker Theme


.. note::
  You may have noticed that the version string is a bit different than other games or software. The version
  number for Freeciv21 is broken into a few parts:
  ``[major version].[minor version].[hash version].[version string level]-[version string]``. In
  :numref:`Start Screen` the major version is ``3``, minor version is ``0``, hash version is ``699369``,
  version string level is ``7`` and the version string is ``beta``. The easiest way to "speak" the version in
  this example is version ``3.0-beta.7``. The hash version is an integer derived from the hexadecimal git
  hash and is generally ignored when discussing the version.

The following buttons are available on the :guilabel:`Start Screen`:

* :guilabel:`Tutorial` -- Quickly start the game `Tutorial`_. This is a great place to start for new players.
* :guilabel:`Start New Game` -- Start a new single player game. See `Start New Game`_ below.
* :guilabel:`Connect to Network Game` -- Connect to a Longturn multiplayer game or one you host yourself. See
  `Connect to Network Game`_ below.
* :guilabel:`Load Saved Game` -- Load a previously saved single player game. See `Load Saved Game`_ below.
* :guilabel:`Start Scenario Game` -- Start a single player scenario game. See `Start Scenario Game`_ below.
* :guilabel:`Options` -- Set client options. See `Options`_ below.
* :guilabel:`Quit` -- Quit Freeciv21

.. Note::
  Notice that there is not a :guilabel:`Help` button available. This is by design. The in-game help is
  compiled at run-time based on the ruleset you select and other server settings you may set.


Tutorial
========

After clicking :guilabel:`Tutorial` on the start screen, a pregame menu will show similar to
:numref:`Start New Game Dialog`, below. All the settings are preconfigured. Simply click :guilabel:`Start`
to begin the game tutorial.


Start New Game
==============

Clicking :guilabel:`Start New Game` is used to start a new single player game. When clicked the following
dialog will appear.

.. _Start New Game Dialog:
.. figure:: /_static/images/gui-elements/start-new-game.png
    :scale: 55%
    :align: center
    :alt: Freeciv21 Start New Game dialog
    :figclass: align-center

    Start New Game Dialog


From upper left to lower right, the following user interface elements are available:

* :guilabel:`Players List Table`
* :guilabel:`Nation`
* :guilabel:`Rules`
* :guilabel:`Number of Players`
* :guilabel:`AI Skill Level`
* :guilabel:`More Game Options`
* :guilabel:`Interface Options`
* :guilabel:`Server Output Window`
* :guilabel:`Server Chat/Command Line`
* :guilabel:`Disconnect`
* :guilabel:`Observe`
* :guilabel:`Start`


Players List Table
------------------

The :guilabel:`Players List Table` shows information about the configured players in the game. The information
shown in :numref:`Start New Game dialog` is what a single player game looks like. A Longturn multiplayer game
will look very similar, except that all the player's aliases will be shown as set up by the game
administrator. You can right-click on a player's row to configure details about the specific player:

* :guilabel:`Observe` -- Allows you to connect to a running game and observe that player. This is useful
  during Longturn multiplayer games when you want to connect and see what a player is doing, but you cannot
  make any actual moves for the player. This works for LAN games as well. You can also use the server
  chat line and issue this command: ``/observe <player>``.
* :guilabel:`Remove Player` -- Removes the player from the list.
* :guilabel:`Take This Player` -- Allows you to claim this player as your own and then when you click
  :guilabel:`Start` you will join the game as that player. This is a required step for Longturn multiplayer
  games at the start of a new game. Subsequent logins to a game when you `Connect to Network Game`_ will not
  require another ``take`` action. You can also use the server chat line and issue this command:
  ``/take <player>``
* :guilabel:`Pick Nation` -- Allows you to choose the Nation of the player. See `Nation`_ below.
* :guilabel:`Set Difficulty` -- Set the difficulty of the AI.
* :guilabel:`Put on Team` -- Combine players into teams.
* :guilabel:`AI Toggle Player` -- Toggle if the player is an AI or a human. This is needed before you can use
  the ``take`` option above as players when added are AI by default.


Nation
------

Clicking on the button that says :guilabel:`Random` as shown in :numref:`Start New Game Dialog` above,
Freeciv21 will bring up a dialog box allowing you to pick the nation you want to play as shown in
:numref:`Select Nation Dialog` below. Freeciv21 comes with hundreds of available nations to pick from. Each
nation has a city graphics style that is automatically selected, but you can also change it. You can pick from
European, Classical, Tropical, Asian, Babylonian, and Celtic. You can also change the gender of your empire's
leader between male and female. Lastly you can use the built-in leader names or enter one of your choosing.

.. _Select Nation dialog:
.. figure:: /_static/images/gui-elements/pick-nation.png
    :scale: 65%
    :align: center
    :alt: Freeciv21 Select Nation dialog
    :figclass: align-center

    Select Nation Dialog


Rules
-----

Freeciv21 comes with a collection of rulesets that define the game parameters. Rulesets control all the
aspects of playing a game. For more information on rulesets, you can refer to
:ref:`Ruleset Modding <modding-rulesets>`.

Freeciv21 comes with the following rulesets:

* Alien
* Civ1
* Civ2
* Civ2Civ3 (Default)
* Classic
* Experimental
* Multiplayer
* Royale


Number of Players
-----------------

The spinner can be changed up or down to customize the number of players. The ruleset can also set the number
of players, so be sure to pick the ruleset before you pick the number of players.


AI Skill Level
--------------

This box will do a mass-change for all the AIs to be the same level. If you want to customize this, then
use the `Players list table`_ right-click menu.


.. _game-manual-more-game-options:

More Game Options
-----------------

Clicking on this button will bring up the :guilabel:`Game Options` dialog box as shown in
:numref:`Game Options dialog` below. From here you can customize other settings for the game before you start
it. The ruleset defines many of these options, so be sure to select the ruleset you want to play before
attempting to set other settings. You can hover the mouse over the entries to get an explanation of what the
setting does.

.. _Game Options dialog:
.. figure:: /_static/images/gui-elements/game-options.png
    :scale: 65%
    :align: center
    :alt: Freeciv21 Game Options dialog
    :figclass: align-center

    Game Options dialog


Interface Options
-----------------

Refer to the section on `Options`_ below.


Server Output Window
--------------------

When you pick a ruleset, you will often see a bunch of output inside of this window. Also, if you make changes
to the game in `More Game Options`_, you will see output from those settings in this window as well. This is
actually a good way to learn what the varying game options are that can be issued via the
`Server chat/command line`_. This window is read-only, however you can select text from it and copy it to
paste in a text file if needed.


Server Chat/Command Line
------------------------

The :guilabel:`Server Chat/Command Line` is a text box below the server output window. From here you can
manually enter ``/set`` commands to the server directly if you know what you want to set.


Disconnect
----------

Clicking this button takes you back to the :guilabel:`Start Screen` as shown in :numref:`Start Screen`. The
local ``freeciv21-server`` instance will terminate at this time. A subsequent click of
:guilabel:`Start New Game` will spawn a new instance of the server, however any changes made previously will
be lost and you will have to start over.


Observe
-------

This button allows you to do a :strong:`global` observe of all players. This is a special server setting and
is not enabled by default, however it is available in single player games. If you are globally observing a
game, you can right-click on the :ref:`Nations and Diplomacy View <game-manual-nations-and-diplomacy-view>` button in the :doc:`top-bar`, and select
a particular nation to observe.


Start
-----

When you are finished with all the settings, clicking :guilabel:`Start` will cause the game to start!


Connect to Network Game
=======================

When you click on the :guilabel:`Connect to Network Game` button, a dialog box will appear as in
:numref:`Server Connect dialog` below.

.. _Server Connect Dialog:
.. figure:: /_static/images/gui-elements/connect-to-server.png
    :scale: 65%
    :align: center
    :alt: Freeciv21 Server Connect dialog
    :figclass: align-center

    Server Connect Dialog


There are three ways to connect to a server:

#. :strong:`A Local Server`: If you are hosting a local server on the same IP subnet as the client, then it
   will show in the :guilabel:`Local Server` box at the top.
#. :strong:`An Internet Server`: If the Longturn community lists running games on its internet metaserver,
   games will show here and can be connected to via the :guilabel:`Internet Servers` box in the middle.
#. :strong:`Manually`: If you simply need to connect to a remote server and you know the the
   :guilabel:`Servername` and :guilabel:`Port`, then this is your option.

For the first two options you will select the server in the table and ensure that your :guilabel:`Username` is
correct and then click :guilabel:`Connect` to connect to the server. The :guilabel:`Password` box will
activate when you have connected to the server. Type in your password and then click :guilabel:`Connect` a
second time and you will join the server.

For the last option, enter in the server name or IP address into the :guilabel:`Connect` text box and the
server port in the corresponding :guilabel:`Port` text box. Ensure your username is correct and then
click :guilabel:`Connect` to connect to the server. The :guilabel:`Password` box will activate when you have
connected to the server. Type in your password and then click :guilabel:`Connect` a second time and you will
join the server. Pretty much all Longturn online multiplayer games are connected this way.

.. Note::
  If you are hosting your own server with authentication enabled and a player has never connected before they
  may be prompted to confirm the password a second time in the :guilabel:`Confirm Password` box before being
  allowed to connect.


Load Saved Game
===============

When you click on the :guilabel:`Load Saved Game` button, a dialog box will appear as in :numref:`Load Saved
Game Dialog` below. Find the saved game you want to load and click (select) it in the table.

.. _Load Saved Game Dialog:
.. figure:: /_static/images/gui-elements/saved-game.png
    :scale: 65%
    :align: center
    :alt: Freeciv21 Load Saved Game dialog
    :figclass: align-center

    Load Saved Game Dialog


Freeciv21 will show you a sample of the game map and some information about the game. If this is not what you
were looking for, select another save from the table. When you have found the save you want to load, click on
the :guilabel:`Load` button and you will be placed in the game at the save point.

Alternately, you can click on the :guilabel:`Browse` button to browse your local filesystem to pick a saved
game that is not in your user profile.


Start Scenario Game
===================

When you click on the :guilabel:`Start Scenario Game` button, a dialog box will appear as in
:numref:`Scenarios Dialog` below. You can click on a scenario to select it and see information about the
selected scenario on the panel to the right side. When you have found the scenario you want to run, click on
the :guilabel:`Load Scenario` button. This will bring up the new game dialog as shown in :numref:`Start New
Game Dialog` above. When ready, click :guilabel:`Start` to begin the scenario game.

.. _Scenarios Dialog:
.. figure:: /_static/images/gui-elements/scenarios.png
    :scale: 65%
    :align: center
    :alt: Freeciv21 scenarios dialog
    :figclass: align-center

    Scenarios Dialog


.. tip::
  If you are new to Freeciv21, the `Tutorial`_ scenario will walk you through about 100 turns of tips on how
  to get started playing a single player game.


Clicking on the :guilabel:`Browse` button will bring a filesystem browser that you can use to pick a
scenario file in an alternate location. Clicking on :guilabel:`Cancel` will bring you back to the
`Starting the Game`_ start screen.


.. _game-manual-options:

Options
=======

From the Start Screen, as shown in :numref:`Start Screen`, when you click on the :guilabel:`Options` button, a
dialog box will appear as in :numref:`Interface Options dialog` below. This dialog allows you to set a myriad
of options that affect the look and feel of the user interface when you are playing a game.

The interface options dialog box can also be opened while in a game via the
:menuselection:`Game --> Interface Options` menu as well as from the `Start New Game`_ dialog by clicking on
the :guilabel:`Interface Options` button.

.. _Interface Options dialog:
.. figure:: /_static/images/gui-elements/interface-options.png
    :scale: 65%
    :align: center
    :alt: Freeciv21 interface options dialog
    :figclass: align-center

    Interface Options dialog


The Interface Options dialog is broken down into the following tabs:

* :guilabel:`Network`: On this tab you can save your preferred username, server and port information to be
  used during `Connect to Network Game`_.
* :guilabel:`Sound`: On this tab you can set everything related to in game sound and music.
* :guilabel:`Interface`: On this tab you can set anything related to how you interact with the interface of the
  client while playing a game.
* :guilabel:`Graphics`: On this tab you can change the look and feel with a different theme. NightStalker is
  the default theme out of the box. You can also set the default tileset to use under different map styles as
  well as various things you may or may not want the client to paint (draw) on the screen.
* :guilabel:`Overview`: On this tab turn various features of the minimap on and off.
* :guilabel:`Map Image`: Freeciv21 can save summary images of the map every turn. This tab allows you to
  configure how you want to do that. Refer to :ref:`Game Menu <game-manual-game-menu>` for more information.
* :guilabel:`Font`: There is a collection of font styles used by the client. This tab allows you to tailor
  them to your own favorites. We ship with the Libertinus font set and it is the default.

The buttons along the bottom of the dialog box are:

* :guilabel:`Reset`: Clicking this button will reset all the options to the out of box defaults.
* :guilabel:`Cancel`: Clicking this button will either return you to the `Starting the Game`_ start screen
  or close the dialog and drop you back to the :ref:`Map View <game-manual-map-view>` if requested from the :guilabel:`Game` menu.
* :guilabel:`Refresh`: If you used the modpack installer to add a new tileset, soundset, or musicset with the
  local options dialog open, you can use the :guilabel:`Refresh` button to reload the available choices for
  some of the drop down selection boxes. For more information on the modpack installer, refer to
  :doc:`../modpack-installer`.
* :guilabel:`Apply`: Apply the settings as set to the client for immediate effect. This button will not do
  much if local options was called from the `Start New Game`_ dialog box.
* :guilabel:`Save`: Save the current settings.
* :guilabel:`OK`: Apply the settings and close the dialog box. This button does not do a save operation by
  default.
