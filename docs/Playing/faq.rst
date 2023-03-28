.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
.. SPDX-FileCopyrightText: louis94 <m_louis30@yahoo.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Frequently Asked Questions (FAQ)
********************************


The following page has a listing of frequenty asked questions with answers about Freeciv21.

Gameplay
========

This section of the FAQ is broken into a collection of sub-sections all surrounding general GamePlay aspects
of Freeciv21:

* `Gameplay General`_
* `Diplomacy`_
* `Game Map and Tilesets`_
* `Cities and Terrain`_
* `Units - General`_
* `Units - Military`_
* `Other`_


Gameplay General
----------------

This subsection of the Gameplay section is a generalized discussion of questions.

OK, I installed Freeciv21. How do I play?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Start the game. Depending on your system, you might choose it from a menu, double-click on the
:file:`freeciv21-client` executable program, or type :file:`freeciv21-client` in a terminal window.

Once the game starts, to begin a single-player game, select :guilabel:`Start new game`. Now edit your
game settings (the defaults should be fine for a beginner-level single-player game) and press the
:guilabel:`Start` button.

Freeciv21 is a client/server system. However, in most cases you do not have to worry about this. The client
starts a server automatically for you when you start a new single-player game.

Once the game is started you can find information in the :guilabel:`Help` menu. If you have never played a
Civilization-style game before you may want to look at help on :title-reference:`Strategy and Tactics`.

You can continue to change the game settings through the :menuselection:`Game --> Game Options` menu.
Type :literal:`/help` in the :guilabel:`server/chatline` widget to get more information about server commands.

A more detailed explanation of how to play Freeciv21 is available in :doc:`how-to-play`, and in the
in-game help.

There is also a detailed :doc:`/Manuals/Game/index` available for your reference.

Where is the chatline you are talking about, how do I chat?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are two widgets on the :ref:`Main Map <game-manual-map-view>` View (F1): :guilabel:`server/chatline` and
:guilabel:`messages`. The :guilabel:`messages` widget can be toggled visible/not-visible via a button on the
:ref:`Top Bar <game-manual-messages>`. The :guilabel:`server/chatline` is a floating widget on the
:ref:`Main Map <game-manual-map-view>` View (F1) and is always visible.

The :guilabel:`server/chatline` widget can be used for normal chatting between players. To issue server
commands, you start by typing a forward-slash :literal:`/` followed by the server command. You will see
resulting server output messages.

During :ref:`pre-game <game-manual-start-new-game>` There is a :guilabel:`server/chatline` feature of the
dialog box that can be used to set server parameters before a game starts. All game parameters are also
available in the :ref:`Game Options <game-manual-more-game-options>` dialog.

See the in-game help on :title-reference:`Chatline` for more detail.

That sounds complicated and tedious. Is there a better way to do this?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No, there is no other better user interface way at this time. This is a big reason why the Longturn Community
prefers using Discord. There are plans to improve this, but it is not implemented yet.

Is there a way to send a message to all your allies?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the game, there is an option to the lower right of the :guilabel:`server/chatline` widget. When selected,
any messages typed will only go to your allies.

.. Note::
  This option only shows up if you are playing an online Longturn Community game with a remote server.
  If you are playing a local single-player game against :term:`AI`, this option does not show up since you
  cannot chat with the :term:`AI`.

How do I find out about the available units, improvements, terrain types, and technologies?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is extensive help on these topics in the :guilabel:`Help` menu, but only once the game has been started.
This is because the in-game help is generated at run-time based on the settings as configured.

The game comes with an interactive tutorial scenario. You can run it by clicking on the :guilabel:`Tutorial`
button on the start menu.

How do I play against computer players?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is commonly called a single-player game and is the default. See
`OK, I installed Freeciv21. How do I play?`_

Can I build up the palace or throne room as in the commercial Civilization games?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No. This feature is not present in Freeciv21, and will not be until someone draws the graphics and writes the
game related code for it. Feel free to :doc:`contribute </Contributing/index>`.

My opponents seem to be able to play two moves at once!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

They are not. It only seems that way. Freeciv21's multi-player facilities are asynchronous: during a turn,
moves from connected game interfaces are processed in the order they are received. Server managed movement is
executed in between turns (e.g. at :term:`TC`). This allows human players to surprise their opponents by
clever use of :term:`Goto` or quick fingers.

In single-player games against the :term:`AI`, the moves from the computer are made at the beginning of the
turn.

A turn in a typical Longturn game lasts 23 hours and it is always possible that they managed to log in twice
between your two consecutive logins. Once a player has made all of their moves, a :term:`TC` event must occur
before they can move again. This does mean that a player can move a unit just before :term:`TC` and just
after and in between your two logins. In short, a player cannot :emphasis:`move twice` until you do.

The primary server setting to mitigate the :term:`TC` problem is called ``unitwaittime``, which imposes a
minimum time between moves of a single unit on successive turns. This setting is used to prevent a varying
collection of what the Longturn community calls "turn change shenanigans". For example, one such issue is
moving a :unit:`Worker` into enemy territory just before :term:`TC` and giving it orders to build a road.
After :term:`TC` you go in and capture a city using the road for move benefit. Without ``unitwaittime`` you
would be able to move the :unit:`Worker` back to safety immediately, thereby preventing it from being captured
or destroyed. With ``unitwaittime`` enabled, you have to wait the requisite amount of time. This makes the
game harder, but also more fair since not everyone can be online at every :term:`TC`.

.. Note::
  The ``unitwaittime`` setting is really only used in Longturn multi-player games and is not enabled/used for
  any of the single-player rulesets shipped with Freeciv21.

Why are the AI players so hard on 'novice' or 'easy'?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Short answer is... You are not expanding fast enough.

You can also turn off Fog of War. That way, you will see the attacks of the :term:`AI`. Just type
:code:`/set fogofwar disabled` on the :guilabel:`server/chatline` before the game starts or by unchecking the
box for fog of war in the :ref:`Game Options <game-manual-more-game-options>` dialog on the Military tab.

Why are the AI players so easy on 'hard'?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Several reasons. For example, the :term:`AI` is heavily play tested under and customized to the default
ruleset and server settings. Although there are several provisions in the code to adapt to changing rules,
playing under different conditions is quite a handicap for it. Though mostly the :term:`AI` simply does not
have a good, all encompassing strategy besides :strong:`"eliminate nation x"`.

To make the game harder, you could try putting some or all of the AI into a team. This will ensure that they
will waste no time and resources negotiating with each other and spend them trying to eliminate you. They
will also help each other by trading techs. Refer to :doc:`/Manuals/Advanced/players` for more information.

You can also form more than one :term:`AI` team by using any of the different predefined teams, or put some
:term:`AI` players teamed with you. Another alternative is to create :term:`AI`'s that are of differing skill
levels. The stronger :term:`AI`'s will then go after the weaker ones.

What distinguishes AI players from humans? What do the skill levels mean?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:term:`AI` players in Freeciv21 operate in the server, partly before all game moves, partly afterwards.
Unlike the game interface, they can in principle observe the full state of the game, including everything
about other players, although most levels deliberately restrict what they look at to some extent.

All :term:`AI` players can change production without penalty. Some levels (generally the harder ones) get
other exceptions from game rules. Conversely, easier levels get some penalties, and deliberately play less
well in some regards.

For more details about how the skill levels differ from each other, see the help for the relevant server
command (for instance :code:`/help hard`).

Other than as noted here, the :term:`AI` players are not known to cheat.

Does the game have a combat calculator, like other Civ games have?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is no integrated combat calculator. You can use the one on the longturn.net website here:
https://longturn.net/warcalc/. You can also select an attacking unit and then middle-click over a defending
unit and in the pop-up window you will see the odds of win/loss.

Where in the game does it say what government you are currently under?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the top bar near the right side there is a :ref:`national status view <game-manual-national-status-view>`
that shows what your national budget consists of as well as what Government you are under, chance for Global
Warming, Nuclear Winter, and how far along you are with research. You can hover your mouse over any of these
icons to see more details.

What government do you start under?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You start under Despotism in :term:`LTT`. This is a ruleset configured item.

Do things that give more trade only give this bonus if there is already at least 1 trade produced on a tile?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The short answer is yes in :term:`LTT`. This is a ruleset configured item.

.. raw:: html

    <embed>
        <hr>
    </embed>

Diplomacy
---------

This subsection of the Gameplay section is a discussion around Diplomacy.

Why cannot I attack another player's units?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You have to declare war first. See the section for `How do I declare war on another player?`_ below.

.. note::
  In some rulesets, you start out at war with all players. In other rulesets, as soon as you make contact with
  a player, you enter armistice towards peace. At lower skill levels, :term:`AI` players offer you a
  cease-fire treaty upon first contact, which if accepted has to be broken before you can attack the player's
  units or cities. The main thing to remember is you have to be in the diplomatic state of war in order to
  attack an enemy.

How do I declare war on another player?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to the :ref:`Nations and Diplomacy <game-manual-nations-and-diplomacy-view>` View (F3), select the player
row, then click :guilabel:`Cancel Treaty` at the top. This drops you from :emphasis:`cease fire`,
:emphasis:`armistice`, or :emphasis:`peace` into :emphasis:`war`. If you have already signed a permanent
:emphasis:`alliance` treaty with the player, you will have to cancel treaties several times to get to
:emphasis:`war`.

See the in-game help on :title-reference:`Diplomacy` for more detail.

.. note::
  The ability to arbitrarily leave :emphasis:`peace` and go to :emphasis:`war` is also heavily dependent on
  the form of government your nation is currently ruled by. See the in-game help on
  :title-reference:`Government` for more details.

How do I do diplomatic meetings?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to the :ref:`Nations and Diplomacy <game-manual-nations-and-diplomacy-view>` View (F3), select the player
row, then choose :guilabel:`Meet` at the top. Remember that you have to either have contact with the player or
an embassy established in one of their cities with a :unit:`Diplomat`.

How do I trade money with other players?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to make a monetary exchange, first initiate a diplomatic meeting as described in the section
`How do I do diplomatic meetings?`_ above. In the diplomacy dialog, enter the amount you wish to give in
the gold input field on your side or the amount you wish to receive in the gold input field on their side.

.. Note::
  In some rulesets there might be a "tax" on gold transfers, so watch out that not all gold will make it to
  its intended destination nation.

Is there a way to tell who is allied with who?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :ref:`Nations and Diplomacy <game-manual-nations-and-diplomacy-view>` View (F3) shows diplomacy and
technology advance information if you have an embassy with the target nation. To see what is going on, select
a nation and look at the bottom of the page.

.. raw:: html

    <embed>
        <hr>
    </embed>

Game Map and Tilesets
---------------------

This subsection of the Gameplay section is a discussion around the game map and tilesets (the graphics layer).

Can one use a regular square tileset for iso-square maps and vice versa?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While that is technically possible, hex and iso-hex topologies are not directly compatible with each other, so
the result is not playable in a good (visualization) way. In the game interface you can force the change of
tileset by going to :menuselection:`Game --> Load Another Tileset`. If the game interface can change, it will
and you will be able to experiment a bit. If there is a complete discrepancy, the game interface will throw an
error and will not make the requested change.

How do I play on a hexagonal grid?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is possible to play with hexagonal instead of rectangular tiles. To do this you need to set your topology
before the game starts. Set this with Map topology index from the
:ref:`game options <game-manual-more-game-options>`, dialog or in the :guilabel:`server/chatline`:

.. code-block:: sh

    /set topology hex|iso|wrapx


This will cause the game interface to use an isometric hexagonal tileset when the game starts . Go to
:menuselection:`Game --> Set local options` to choose a different one from the drop-down. hexemplio and
isophex are included with the game.

You may also play with overhead hexagonal, in which case you want to set the topology setting to
:code:`hex|wrapx`. The hex2t tileset is supplied for this mode.

Can one use a hexagonal tileset for iso-hex maps and vice versa?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the question `Can one use a regular square tileset for iso-square maps and vice versa?`_ above.

.. raw:: html

    <embed>
        <hr>
    </embed>

Cities and Terrain
------------------

This subsection of the Gameplay section is a discussion around cities and the terrain around them.

My irrigated grassland produces only 2 food. Is this a bug?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No, it is not -- it is a feature. Your government is probably Despotism, which has a -1 output penalty
whenever a tile produces more than 2 units of food, production, or trade. You should change your government.
See the in-game help on :title-reference:`Government` for more detail to get rid of this penalty.

This feature is also not 100% affected by the form of government. There are some small and great wonders in
certain rulesets that get rid of the output penalty. See the in-game help on
:title-reference:`City Improvements` and :title-reference:`Wonders` for more information.

Can I build land over sea or transform ocean to land?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. You can do that by placing an :unit:`Engineer` in a :unit:`Transport` and going to the ocean tile you
want to build land on. Click the :unit:`Transport` to display a list of the transported :unit:`Engineers` and
activate them. Then give them the order of transforming the tile to swamp. This will take a very long time
though, so you had better try with 6 or 8 :unit:`Engineers` at a time. There must be 3 adjacent land tiles to
the ocean tile (e.g. a land corner) you are transforming for this activity to work.

Is there an enforced minimum distance between cities?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This depends on the ruleset. In :term:`LTT` there is a minimum distance of 3 empty tiles between two cities.
You can think of it as “no city can be built within the work radius of another city”, since the work radius of
a city is also 3 tiles in :term:`LTT`.

This setting, known as ``citymindist``, can be set in the server settings before a local game starts or by
changing at the :guilabel:`server/chatline`:

.. code-block:: sh

    /set citymindist 4


If your city is going to grow next turn and you rush-buy a Granary, do you still get the food savings?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. Production is “produced” before growth at turn change. This is true for all rulesets as it is part of the
standard :doc:`Turn Change </Playing/turn-change>` process.

How much population do Settlers take to build?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two in the :term:`LTT` ruleset. This is a ruleset configurable item. See the in-game help on
:title-reference:`Units` for more detail to see what the settings is for the ruleset you loaded at game
start.

Do tiles remember terraforming progress?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you change orders for the unit doing the terraforming and do not change them back within the same turn,
the terraforming progress is lost. If you change orders and then change them back, nothing special happens.
Terraforming is always processed at :term:`TC`.

How frequently do natural disasters happen?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This depends on the ruleset. For the :term:`LTT` ruleset, all natural disasters have a 1% probability to
happen each turn. The default is 10%. This setting, known as ``disasters``, can be set in the server settings
before a local game starts or by changing at the :guilabel:`server/chatline`:

.. code-block:: sh

    /set disasters 20


Does the city work area change in any way during the game?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is a ruleset configured option. In :term:`LTT` the initial value is ``15``, effectively giving 3 tiles
"out" from the city center in all directions. Varying technologies or buildings can be programmed into the
ruleset to change the vision radius (e.g. the work area) of a city.

Is it worth it to build cities on hills (potentially with rivers), or is the risk of earthquakes and floods too large?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is usually worth it, since hills and rivers have great defense values. Rivers also allow you to build an
:improvement:`Aqueduct, River` without the knowledge of :advance:`Construction` and it is much cheaper to
build and requires no upkeep.

.. note::
  It is a great strategy if you can do this to place your first city (Capital) either on a river or adjacent
  to one to get this "fresh water" effect. You can get the city up to size 16 very fast with the right growth
  strategy.

Is there a benefit to lake tiles over ocean tiles? What are their differences?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are at least the following differences:

* Lake tiles allow an adjacent city to build a cheap :improvement:`Aqueduct, Lake` with no upkeep and before
  the discovery of :advance:`Construction`. This is commonly referred to as the "fresh water" effect.
* Lake tiles give more food than ocean tiles, especially with the Fish tile special.
* Shallow ocean gives +1 production with :improvement:`Offshore Platform`. The :improvement:`Offshore Platform`
  city improvement often comes with the discovery of :advance:`Miniaturization` in most rulesets, but this is
  a ruleset configurable item.
* Some ships cannot travel on deep ocean (such as :unit:`Triremes`)
* Shallow ocean has a 10% defense bonus.
* Ocean tiles allow you to build :improvement:`Harbor`, giving +1 food. The :improvement:`Harbor` city
  improvement often comes with the discovery of :advance:`Seafaring` in most rulesets, but this is a ruleset
  configurable item.

Is “Aqueduct, River” identical to “Aqueduct, Lake”?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In most rulesets, yes. It is part of the "fresh water" effect of giving a cheap :improvement:`Aqueduct` that
has no gold upkeep and does not require the discovery of :advance:`Construction`.

Is the city tile worked for free?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. This is hard-coded in the server. In all rulesets a size 1 city will always have two tiles being worked
by the citizens of the city: the city center tile and another one in its working radius that is not being
worked by an adjacent city.

Do you get free irrigation on the city tile?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You get a “virtual” irrigation effect. It works the same way as regular irrigation for food purposes, but does
not allow you to build irrigation next to the city by itself. You will have to build regular irrigation on the
city center tile to do that. The free irrigation is lost if you build a mine on the city tile (just like
regular irrigation on a regular tile is lost with a mine). This means that a desert tile that is mined has
zero (0) food, even when on a city center tile. Since the “virtual” irrigation works like regular irrigation,
if you build a city on a tile that cannot be irrigated normally (e.g. a forest), you do not get any food
bonus.

In the late game, many rulesets have a :improvement:`Supermarket`, that comes with the discovery of
:advance:`Refrigeration`. A player can then use :unit:`Workers` or :unit:`Engineers` to add Farmland on top of
the existing irrigation for an additional food bonus. In this sense, if you want to get the Farmland food
effect on a city center tile that is already "virtually" irrigated you will have to actually irrigate the tile
and then add Farmland on top of it, just like any other regular tile.

Does the city tile have any production bonuses?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A city tile has a +1 production bonus, added after any other bonuses (such as Railroad).

Does LTT have the extra food from rivers on a desert tile when irrigated, like other rulesets have?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, an irrigated desert tile with a river gives an extra +1 food in addition to the regular irrigation food
bonus. This is a game engine (server) item and is not driven by a ruleset, such as :term:`LTT`.

Is there any penalty when changing a city production task?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are 4 “categories” of production: units, city improvements (e.g. Buildings), great wonders, and small
wonders. If you change within a “category” (e.g. :unit:`Phalanx` to :unit:`Horsemen`, or
:improvement:`Library` to :improvement:`Bank`), there is no penalty. If you change across categories (e.g.
:unit:`Archers` to :improvement:`Library`, or :wonder:`Leonardo’s Workshop` to :unit:`Frigate`), there is a
50% penalty. If you change back to the same category within the same turn, the penalty is reversed. If you
change multiple times, the penalty is only applied once, which means that if you change the production target
more than once in a turn there will be no penalty as long as you land on the same "category" as was active at
the beginning of the turn.

Is there a way to claim tiles using Diplomats?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No. In Freeciv21 there are generally 4 ways to gain tile ownership:

* Build a city and claim the tiles first.
* Grow your cities super big and much bigger than your neighbor's cities. National borders can move at Turn
  Change based on culture score.
* Build a Fortress and place a Military unit (e.g. a :unit:`Phalanx`) inside the Fortress.
* Conquer the city and take its tiles for your own.

Is there a way to create a hill other than terraforming a mountain?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can also terraform a hill from plains with :unit:`Engineers`. In some rulesets, such as :term:`LTT`, this
is very expensive in worker :term:`MP` and can take some time unless you place many :unit:`Engineers` on the
tile at the same time.

Can you build a hill under a city?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, you sure can!

.. raw:: html

    <embed>
        <hr>
    </embed>

Units - General
---------------

This subsection of the Gameplay section is a generalized discussion around units.

When does the game inform you of enemy movement within your units’ field of vision?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It depends on the status of the unit. If the unit is fortified or working on another task (e.g. irrigation)
then you will not be notified. Only the :strong:`Sentry` status will give you a notification.

If I move a unit onto a mountain, does that change how many movement points the unit has next turn?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
All units that end their turn on a mountain start with 1 less :term:`MP` the following turn. The exception to
this rule are units that ignore terrain movement completely (e.g. :unit:`Explorers`, and :unit:`Alpine
Troops`). This is known as "ignoring terrain effects".

What is a unit’s terraforming speed based on?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is based on the base amount of :term:`MP`'s for that unit and veteran level bonus. The base terraforming
duration is specified in the ruleset files.

.. todo::
  This is discussed in detail in a forthcoming LTT Gamer's Manual. Update this entry at that time.

Can workers do all land conversions? Or are most land conversions locked behind engineers?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All :unit:`Workers` can do land conversions except for major land transformations, which are available only
with :unit:`Engineers`.

Does a damaged worker work slower than normal?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No, :term:`HP`'s do not factor in a :unit:`Worker's` ability to conduct infrastructure improvements to tiles.

When terraforming, does some movement get used on the last turn of terraforming? Does the unit start with less movement points?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Terraforming does not affect a unit's :term:`MP`'s in any way.

Does damage reduce the amount of movement points the unit has?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This depends on the unit class and the ruleset configuration. If a unit’s help text specifies it is "slowed
down when damaged", then it does. If it does not say anything about it, then it does not.

Do Caravans give full production?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :unit:`Caravan` unit is a special unit that allows a player to move production (shields) from one city to
another in order to increase the speed of constructing wonders (both small and great). This effect only works
for wonders and no other city improvement. The :unit:`Caravan` unit acts like any other unit when disbanded in
a city: it gives back 50% of the shields it took to construct it in the first place. In many rulesets the
:unit:`Freight` becomes available in the late game and obsoletes the :unit:`Caravan`. The :unit:`Freight`
works the same way.

How does unit leveling work?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Freeciv21 calls this unit "Veterancy" or "Veteran Levels". You have a chance every turn for any kind of unit
to gain an upgrade via experience. The experience depends on the unit and what they are doing. For example, a
:unit:`Worker` gains experience by creating terrain infrastructure, or a :unit:`Phalanx` gains experience
during both defense and offense (attack) movements. See the following table:

+-----------------+-------------------+------------------+------------------------+
|                 |                   |                  | Promotion Chance       |
| Level           | Combat Strength   | Move Bonus       +-----------+------------+
|                 |                   |                  | In Combat | By Working |
+=================+===================+==================+===========+============+
| Green           | 1x                | 0                | 50        | 9          |
+-----------------+-------------------+------------------+-----------+------------+
| Veteran 1 (v)   | 1.5x (from Green) | 1/3 (from Green) | 45        | 6          |
+-----------------+-------------------+------------------+-----------+------------+
| Veteran 2 (vv)  | 1.75x             | 2/3              | 40        | 6          |
+-----------------+-------------------+------------------+-----------+------------+
| Veteran 3 (vvv) | 2x                | 1                | 35        | 6          |
+-----------------+-------------------+------------------+-----------+------------+
| Hardened 1 (h1) | 2.25x             | 1 1/3            | 30        | 5          |
+-----------------+-------------------+------------------+-----------+------------+
| Hardened 2 (h2) | 2.5x              | 1 2/3            | 25        | 5          |
+-----------------+-------------------+------------------+-----------+------------+
| Hardened 3 (h3) | 2.75x             | 2                | 20        | 4          |
+-----------------+-------------------+------------------+-----------+------------+
| Elite 1 (e1)    | 3x                | 2 1/3            | 15        | 4          |
+-----------------+-------------------+------------------+-----------+------------+
| Elite 2 (e2)    | 3.25x             | 2 2/3            | 10        | 3          |
+-----------------+-------------------+------------------+-----------+------------+
| Elite 3 (e3)    | 3.5x              | 3                | 0         | 0          |
+-----------------+-------------------+------------------+-----------+------------+

Is it possible to change a unit’s home city?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To be clear, a unit's "home city" is the city that produced it.

It is possible when the unit is moved to a city that is not its current home city. You then get an option to
change the home city. With the unit in a city you can either use shortcut key “h” or
:guilabel:`Unit --> Set Home City` to re-home the unit to the city it is inside.

.. Note::
  Some rulesets allow "unhomed" units. These kind of units will never have a home city and you cannot change
  it, even if you wanted to. These units have no upkeep, so they can stay unhomed.

Are queued goto commands executed before or after units and city improvements are built?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After. For example, you can beat an enemy attacking unit with a queued :term:`Goto` to your city by
rush-buying a defensive unit (it will get built first during normal :doc:`turn change processing
</Playing/turn-change>`), and the attacking unit will move after that.

.. raw:: html

    <embed>
        <hr>
    </embed>

Units - Military
----------------

This subsection of the Gameplay section is a discussion around military units specifically.

My opponent's last city is on a 1x1 island so I cannot conquer it and they will not give up. What can I do?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It depends on the ruleset, but often researching :advance:`Amphibious Warfare` will allow you to build a
:unit:`Marine`. Alternatively research :advance:`Combined Arms` and either move a :unit:`Helicopter` or
airdrop a :unit:`Paratrooper` there. When viewing the in-game help text for :title-reference:`Units`, be on
the look out for ``Can launch attack from non-native tiles``. This is the unit's feature that allows you to
attack from the ocean or air versus land, which is a native tile.

If you cannot build :unit:`Marines` yet, but you do have :unit:`Engineers`, and other land is close-by, you
can also build a land-bridge to the island (i.e. transform the ocean). If you choose this route, make sure
that your :unit:`Transport` is well defended!

Does a unit with less than 1 movement point remaining have weaker attacks?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, the base attack is multiplied by the remaining :term:`MP`'s when the unit has less than 1 :term:`MP`
left. This is commonly known as “tired attack”. As an example, a green :unit:`Knights` (base attack 6) with
6/9 :term:`MP`'s remaining will attack as if it had attack 4.

How can I tell what final defense a unit will have after applying all bonuses from terrain, fortification, city, and such?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The game interface does not show this information, so you will have to calculate it manually. Math is an
important element of all Freeciv21 games, and especially the :term:`LTT` and :term:`LTX` multi-player games
the Longturn Community enjoys playing. As in the game of Chess, the "board" does not do the math for you. You
must gauge the risk-reward ratios of your moves and counter-moves. This is the same in Freeciv21. The game
will not do the math for you. This table should help you in doing the math:

+---------------------------------------+-----------------+------------------+-----------------+----------------------+-----------+-----------+--------------------+-------------------+
| Terrain                               | Open (Sentried) | Open (Fortified) | Fortress (Open) | Fortress (Fortified) | City <= 8 | City >= 9 | City <= 8 w/ Walls | City >=9 w/ Walls |
+=======================================+=================+==================+=================+======================+===========+===========+====================+===================+
| Grass, Plains, Desert, Tundra, Desert | 1.0x            | 1.5x             | 2.0x            | 3.0x                 | 2.25x     | 3.0x      | 3.75x              | 4.5x              |
+---------------------------------------+-----------------+------------------+-----------------+----------------------+-----------+-----------+--------------------+-------------------+
| Forest, Jungle, Swamp                 | 1.25x           | 1.88x            | 2.5x            | 3.75x                | 2.81x     | 3.75x     | 4.69x              | 5.63x             |
+---------------------------------------+-----------------+------------------+-----------------+----------------------+-----------+-----------+--------------------+-------------------+
| Hills                                 | 1.5x            | 2.25x            | 3.0x            | 4.5x                 | 3.38x     | 4.5x      | 5.63x              | 6.75x             |
+---------------------------------------+-----------------+------------------+-----------------+----------------------+-----------+-----------+--------------------+-------------------+
| Mountains                             | 2.0x            | 3.0x             | 4.0x            | 6.0x                 | N/A       | N/A       | N/A                | N/A               |
+---------------------------------------+-----------------+------------------+-----------------+----------------------+-----------+-----------+--------------------+-------------------+
| w/ River                              | +1.25x on top of the other modifiers above                                                                                                   |
+---------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------+

.. Tip::
  The legacy Freeciv WiKi gives some good information in the Game Manual about Terrain here:
  https://freeciv.fandom.com/wiki/Terrain.

What is the math for upgrading units in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The basic upgrade cost is the same as disbanding the old unit in a city, and then rush-buying the new unit
from the contributed shields (production).

:strong:`Example`: :unit:`Phalanx` --> :unit:`Pikeman`

The :unit:`Phalanx` contributes 7 shields (:math:`15 \div 2` rounded down). The :unit:`Pikeman` costs 25
shields. The remaining 18 shields (:math:`25 - 7`), is bought with gold using the formula for rush-buying
units. For the math folks out there, the formula for rush-buying units (in all cases, not just for upgrades)
is: :math:`2p + \frac{p^2}{20}` where :math:`p` is the remaining production (or shields).

Are diplomats used up when investigating an enemy city?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, they are destroyed/consumed after conducting an "investigate city" action. In some rulesets (notably
:term:`LTT` and :term:`LTX`), there is a :unit:`Spy` available when you research :advance:`Espionage`. The
:unit:`Spy` is not consumed by the same actions as the :unit:`Diplomat`.

Are there any other diplomatic units other than Diplomats and Spies?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The units available is highly dependent on the ruleset. For the :term:`LTT` and :term:`LTX` rulesets there are
a couple "tech" stealing units: :unit:`Scribe` and :unit:`Scholar`. They can be used to steal (incite) units
from other players and also to steal technology. In the :term:`LTT` and :term:`LTX` rulesets, there is no
technology trading (between allies), so these two units were created as a way to allow technology trading, but
at a risk.

When my unit moves in my territory on rivers, it costs 1/3 MP per tile. If I move on a river in enemy territory, it costs the full MP for the tile. Why?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Tile improvements that affect movement (rivers, roads, railroads) only apply when the unit is on allied
territory, or on territory not owned by anyone. When moving through enemy territory, the terrain acts as if
those improvements do not exist. The server setting that controls this is called ``restrictinfra``. This value
can be set in the server settings before a local game starts or by changing at the
:guilabel:`server/chatline`:

.. code-block:: rst

    /set restrictinfra FALSE


The :term:`LTT` and :term:`LTX` rulesets used by the Longturn Community have this value set to ``TRUE``. This
is also the default setting for many of the single-player rulesets shipped with Freeciv21.

Is there a way to see potential battle odds?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. Select the unit you want to attack with and then middle-click (or Alt-click on Windows) over the
potential target and a pop-up window will show you the odds of attack and defense taking into account all
aspects of the attack (or defense) include terrain bonus, unit veterancy, etc.

When a city is captured, all units homed in that city that are currently in another city of yours are re-homed to that city. What happens to the the other units?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any units not in a native city (e.g. your own city) are lost. This includes allied cities or outside of any
city in the field.

.. raw:: html

    <embed>
        <hr>
    </embed>

Other
-----

This subsection of the Gameplay section is a catchall area for questions do not fit nicely into the other
subsections.

Can I change settings or rules to get different types of games?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Of course. Before the game is started, you may change settings through the
:ref:`Game Options <game-manual-more-game-options>` dialog. You may also change these settings or use server
commands through the :guilabel:`server/chatline` widget. If you use the :guilabel:`server/chatline`, use the:

.. code-block:: sh

    /show

command to display the most commonly-changed settings, or

.. code-block:: sh

    /help <setting>


to get help on a particular setting, or

.. code-block:: sh

    /set <setting> <value>


to change a setting to a particular value. After the game begins you may still change some settings, but not
others.

You can create rulesets or :strong:`modpacks` - alternative sets of units, buildings, and technologies.
Several different rulesets come with the Freeciv21 distribution, including a civ1 (Civilization 1
compatibility mode), and civ2 (Civilization 2 compatibility mode). Use the ``rulesetdir`` command to change
the ruleset (as in ``/rulesetdir civ2``). For more information refer to :doc:`/Modding/index`.

How compatible is Freeciv21 with the commercial Civilization games?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Freeciv21 was created as a multiplayer version of Civilization |reg| with players moving simultaneously.
Rules and elements of Civilization II |reg|, and features required for single-player use, such as :term:`AI`
players, were added later.

This is why Freeciv21 comes with several game configurations (rulesets): the civ1 and civ2 rulesets implement
game rules, elements and features that bring it as close as possible to Civilization I and Civilization II
respectively, while other rulesets such as the Classic ruleset tries to reflect the most popular settings
among Freeciv21 players. Unimplemented Civilization I and II features are mainly those that would have little
or no benefit in multi-player mode, and nobody is working on closing this gap.

Little or no work is being done on implementing features from other similar games, such as SMAC, CTP or
Civilization III+.

So the goal of compatibility is mainly used as a limiting factor in development. When a new feature is added
to Freeciv21 that makes gameplay different, it is generally implemented in such a way that the
:emphasis:`traditional` behaviour remains available as an option. However, we are not aiming for absolute
100% compatibility; in particular, we are aiming for bug-compatibility.

I want more action.
^^^^^^^^^^^^^^^^^^^

In Freeciv21, expansion is everything, even more so than in the single-player commercial Civilization games.
Some players find it very tedious to build on an empire for hours and hours without even meeting an enemy.

There are various techniques to speed up the game. The best idea is to reduce the time and space allowed for
expansion as much as possible. One idea for multiplayer mode is to add :term:`AI` players: they reduce the
space per player further, and you can toy around with them early on without other humans being aware of it.
This only works after you can beat the :term:`AI`, of course.

Another idea is to create starting situations in which the players are already fully developed. Refer to the
section on :ref:`scenarios <modding-scenarios>`.

.. raw:: html

    <embed>
        <hr>
    </embed>

Non-Gameplay Specific Questions
===============================

This section of the FAQ deals with anything not related to general gameplay aspects of Freeciv21.

Longturn Multiplayer
--------------------

How do I play multi-player?
^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can either join a network game run by someone else, or host your own. You can also join one of the many
games offered by the Longturn community.

To host your own game, we recommend starting a separate server by hand. See
:doc:`/Manuals/Advanced/on-the-server` for more information.

If all players are on the same local area network (LAN), they should launch their game interfaces, choose
:guilabel:`Connect to Network game` and then look in the :guilabel:`Internet Servers` section. You should see
the existing server listed. Double-click on it to join.

To play over the Internet, players will need to enter the hostname and port into their game interfaces. The
game admin will need to tell the other players those details. To join a longturn.net server you start by
clicking :guilabel:`Connect to Network Game` and then in the bottom-left of the dialog fill in the
:guilabel:`Connect`, :guilabel:`Port`, and :guilabel:`Username` fields provided by the game admin. Once ready,
click the :guilabel:`Connect` button at the bottom-right, fill in your longturn.net password in the
:guilabel:`Password` box and you will be added to the game.

.. note::
  Hosting an Internet server from a home Internet connection is often problematic, due to firewalls and
  network address translation (NAT) that can make the server unreachable from the wider Internet. Safely and
  securely bypassing NAT and firewalls is beyond the scope of this FAQ.


Where do I see how much time is left in the current turn?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the :doc:`/Manuals/Game/mini-map` in the bottom right of the map view, where the :guilabel:`Turn Done`
button shows for single-player games. For Longturn multi-player games will also add a count-down timer to show
when the turn will change.

When connecting to a game, is the username field case-sensitive?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, both the username and password is case-sensitive.

After typing in the hostname, port, and username, the password field is greyed out. What is up with that?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You have to click the :guilabel:`Connect` button to ask the game interface to connect to the server and then
you enter your password after connecting to authorize your entry into the game.

How do I take over an AI player?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the :guilabel:`server/chatline` you use the ``/take <playername>`` command to take over an :term:`AI`
player. You can also right-click on the player you wish to take on the
:ref:`players list table <game-manual-start-new-game-players>`.

How do I take over an idle player that was assigned to me?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Same procedure as `How do I take over an AI player?`_ above.

Does capturing work like MP2?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Unit capturing is ruleset defined. Capturing in :term:`LTT` works slightly differently than in the :term:`MP2`
ruleset. You can capture any “capturable” unit with a “capturer” unit, if the target is alone on a tile. Units
that are “capturable” have a mention of this in their help text. Units that are “capturers” also have a
mention of this in their help text.

.. Tip::
  Due to the game interface mechanics, you can capture units from boats. This cannot be done using the regular
  :term:`Goto` command, but has to be done using the number pad on your keyboard.


Where do I go to see the rules for a game? Like how big a victory alliance can be?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All rules and winning conditions are posted to the `https://forum.longturn.net/index.php <forums>`_ under the
Games index. Each game has a section for varying posts related to the game. Winning conditions are also often
posted on the Longturn Discord `https://discord.gg/98krqGm <server>`_ in the channel for the game.

Does the Nations view show whether the player is idling?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, you may have to enable the visibility of the column. Right-click the header bar to see what columns are
enabled. You are looking for the column named ``idle``.

Can you make hideouts in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hideouts are a purely :term:`FCW` thing. There is no such thing in :term:`LTT`. Other rulesets could offer
this as it is a ruleset configurable item. The Longturn Community does not like them as they are overpowered
and easily exploited.

How does research in LTT compare to MP2a/b/c?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

MP2a/b/c and :term:`LTT` are all different rulesets, so obviously this is a ruleset configured item. In
MP2a/b, all bulbs carry over to the new research. In MP2c, bulbs researched towards a technology stay with
that technology. In :term:`LTT`, bulbs do not stay with a particular technology. There is a 10% penalty when
switching research. This penalty is processed at :term:`TC`, so if you change your research again within the
same turn, you do not suffer any additional penalties. If you then change your research back to the original
technology within the same turn, you do not suffer the 10% penalty.

Is stack kill enabled in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, it is. This is a game server setting and is enabled on :term:`LTT` games as without it a player could
bring a stack of 100 units onto the same tile. With stack kill enabled, it eliminates this very overpowered
capability.

Is it really so that in LTT there is no rapture, but you get a trade bonus in celebrating cities instead?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The concept of "rapture" is a ruleset and server configured item. The :term:`LTT` ruleset does not do rapture.
Instead “celebration” is used under *Republic* and *Democracy*. In the :term:`LTX` Ruleset the *Federation*
government also allows for celebration. Under other governments, celebration does not provide any bonuses.

Are trade routes enabled in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Technically yes, they are enabled. However in reality they are not enabled, because the required city to city
distance is 999. They are overpowered and would cause game balance issues in the multi-player environments
targeted by :term:`LTT`.

.. raw:: html

    <embed>
        <hr>
    </embed>

Game Interface Configuration
----------------------------

How do I make the font bigger for help text?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can change a collection of fonts and font sizes by going to :guilabel:`Game --> Set local options` and
then clicking on the :guilabel:`Fonts` tab.

Is it possible to save login info in the game so it does not have to be entered each time?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, you can set a number of items by going to :guilabel:`Game --> Set local options` and then clicking on
the :guilabel:`Network` tab. You can set the server, port and username. You cannot save the password as that
is a security risk.

Where can I turn off “connected / disconnected” messages filling up the chat window?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can adjust a collection of things by going to :guilabel:`Game --> Messages`. Anything checked in the
``out`` column will go to the :guilabel:`server/chatline` widget of the game interface. Anything in the
``mes`` column will show in the :guilabel:`messages` widget. Lastly, anything checked in the ``pop`` column
will produce a pop-up window message.

Many players actually enable a lot of things that normally show in the :guilabel:`messages` widget and put
them in the :guilabel:`server/chatline` widget as well. You can copy text from the
:guilabel:`server/chatline`, but can not in :guilabel:`messages`. Being able to copy and paste text to your
allies comes in very handy.

Refer to :doc:`/Manuals/Game/message-options` for more information.

How do I enable/disable sound or music support?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The game can be started without sound by supplying the command-line arguments :literal:`-P none`.
The default sound plugin can also be configured in the game settings by going to
:guilabel:`Game -->Set local options` and then clicking on the :guilabel:`Sound` tab.

If the game was compiled with sound support, it will be enabled by default. All pre-compiled
packages provided by the Longturn community come with sound support enabled.

How do I use a different tileset?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the tilesets supplied with Freeciv21 do not do it for you, some popular add-on tilesets are available
through the :doc:`modpack installer utility </Manuals/modpack-installer>`. To install these, just launch the
installer from the Start menu, and choose the one you want; it should then be automatically downloaded and
made available for the current user.

If the tileset you want is not available via the modpack installer, you will have to install it by hand from
somewhere. To do that is beyond the scope of this FAQ.

How do I use a different ruleset?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Again, this is easiest if the ruleset is available through the
:doc:`modpack installer utility </Manuals/modpack-installer>` utility that is shipped with Freeciv21.

If the ruleset you want is not available via the modpack installer, you will have to install it by hand from
somewhere. To do that is beyond the scope of this FAQ.

.. raw:: html

    <embed>
        <hr>
    </embed>

Community
---------

Does Freeciv21 violate any rights of the makers of Civilization I or II?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There have been debates on this in the past and the honest answer seems to be: We do not know.

Freeciv21 does not contain any actual material from the commercial Civilization games. The Freeciv21
maintainers have always been very strict in ensuring that materials contributed to the Freeciv21
distribution or Longturn website do not violate anyone's copyright. The name of Freeciv21 is probably not a
trademark infringement. The user interface is similar, but with many (deliberate) differences. The game
itself can be configured to be practically identical to Civilization I or II, so if the rules of a game are
patentable, and those of the said games are patented, then Freeciv21 may infringe on that patent, but we
do not believe this to be the case.

Incidentally, there are good reasons to assume that Freeciv21 does not harm the sales of any of the
commercial Civilization games in any way.

How does Freeciv21 relate to other versions of Freeciv?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Freeciv21 is a code fork of Freeciv and is maintained by a community of online players called Longturn. After
using legacy Freeciv for many years for our multi-player games, the Longturn Community decided to fork Freeciv
because we felt that the development was not going in the right direction for multi-player games. Legacy
Freeciv is concentrating on single-player games for the most part.

Besides Freeciv21 and legacy Freeciv, there are also communities playing a version running in the browser,
commonly known as Freeciv Web. This version is less flexible and does not fulfill the needs of a diverse
community like Longturn.

Where can I ask questions or send improvements?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Please ask questions about the game, its installation, or the rest of this site at the Longturn Discord
Channels at https://discord.gg/98krqGm. The ``#questions-and-answers`` channel is a good start.

Patches and bug reports are best reported to the Freeciv21 bug tracking system at
https://github.com/longturn/freeciv21/issues/new/choose. For more information, have a look at
:doc:`/Contributing/bugs`.

.. raw:: html

    <embed>
        <hr>
    </embed>

Technical Stuff
---------------

I have found a bug, what should I do?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the article on `Where can I ask questions or send improvements?`_. You might want to start up a
conversation about it in the Longturn Discord channels if you are unsure.

I have started a server but the game cannot find it!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

By default, your server will be available on host :literal:`localhost` (your own machine) and port
:literal:`5556`. These are the default values your game uses when asking which game you want to
connect to.

If you do not get a connection with these values, your server is not running, or you used :literal:`-p` to
start it on a different port, or your system's network configuration is broken.

To start your local server, run :file:`freeciv21-server`. Then type :literal:`start` at the
server prompt to begin!

.. code-block:: sh

    username@computername:~/games/freeciv21/bin$ ./freeciv21-server
    This is the server for Freeciv21 version 3.0.20210721.3-alpha
    You can learn a lot about Freeciv21 at https://longturn.readthedocs.io/en/latest/index.html
    [info] freeciv21-server - Loading rulesets.
    [info] freeciv21-server - AI*1 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*2 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*3 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*4 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*5 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - Now accepting new client connections on port 5556.

    For introductory help, type 'help'.
    > start
    Starting game.


If the server is not running, you will :emphasis:`not` be able to connect to your local server.

If you cannot connect to any of the other games listed, a firewall in your organization/ISP is probably
blocking the connection. You might also need to enable port forwarding on your router.

If you are running a personal firewall, make sure that you allow communication for :file:`freeciv21-server`
and the :file:`freeciv21-client` to the trusted zone. If you want to allow others to play on your server,
allow :file:`freeciv21-server` to act as a server on the Internet zone.

For more information on running your own server refer to :doc:`/Manuals/Advanced/on-the-server`.

How do I restart a saved game?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If for some reason you cannot use the start-screen interface for loading a game, you can load one directly
through the game or server command line. You can start the game, or server, with the
:literal:`-f` option, for example:

.. code-block:: sh

    $ ./freeciv21-server -f freeciv-T0175-Y01250-auto.sav.xz


Or you can use the :literal:`/load` command inside the server before starting the game.

The server cannot save games!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In a local game started from the game interface, the games will be saved into the default Freeciv21 save
directory (typically :file:`~/.local/share/freeciv21/saves`). If you are running the server from the command
line, however, any game saves will be stored in the current directory. If the ``autosaves`` server setting is
set appropriately, the server will periodically save the game automatically, which can take a lot of disk
space. The frequency is controlled by the ``saveturns`` setting. In any case, you should check the ownership,
permissions, and disk space/quota for the directory or partition you are trying to save to.

Where are the save games located by default?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On Unix like systems (e.g. Linux), they will be in :file:`~/.local/share/freeciv21/saves`. On Windows, they
are typically found in in the :file:`Appdata\\Roaming` User profile directory. For example:

.. code-block:: bat

    > C:\Users\MyUserName\AppData\Roaming\freeciv21\saves


You could change this by setting the :literal:`HOME` environment variable, or using the :literal:`--saves`
command line argument to the server (you would have to run it separately).

I opened a ruleset file in Notepad and it is very hard to read
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ruleset files (and other configuration files) are stored with UNIX line endings which Notepad does not
handle correctly. Please use an alternative editor like WordPad, notepad2, or notepad++ instead.

What are the system requirements?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:strong:`Memory`

In a typical game the server takes about 30MB of memory and the game interface needs about 200MB. These values
may change with larger maps or tilesets. For a single-player game you need to run both the game interface and
the server.

:strong:`Processor`

We recommend at least a 1GHz processor. The server is almost entirely single-threaded, so more cores will
not help. If you find your game running too slow, these may be the reasons:

* :strong:`Too little memory`: Swapping memory pages on disc (virtual memory) is really slow. Look at the
  memory requirements above.

* :strong:`Large map`: Larger map does not necessary mean a more challenging or enjoyable game. You may try a
  smaller map.

* :strong:`Many AI players`: Again, having more players does not necessary mean a more challenging or
  enjoyable game.

* :strong:`City Governor (CMA)`: This is a really useful agent which helps you to organize your citizens.
  However, it consumes many CPU cycles. For more information on the CMA, refer to :doc:`/Playing/cma`.

* :strong:`Maps and compression`: Creating map images and/or the compression of saved games for each turn will
  slow down new turns. Consider using no compression.

* :strong:`Graphic display`: The game interface works well on 1280x1024 or higher resolutions.

* :strong:`Network`: Any modern internet connection will suffice to play Freeciv21. Even mobile hot-spots
  provide enough bandwidth.

.. raw:: html

    <embed>
        <hr>
    </embed>

Windows
-------

How do I use Freeciv21 under MS Windows?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pre-compiled binaries can be downloaded from https://github.com/longturn/freeciv21/releases. The native
Windows packages come as self-extracting installers.

OK, I have downloaded and installed it, how do I run it?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the document about :doc:`/Getting/windows-install`.

.. raw:: html

    <embed>
        <hr>
    </embed>

macOS
-----

Pre-compiled binaries in a :file:`*.dmg` file can be downloaded from https://github.com/longturn/freeciv21/releases.

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
