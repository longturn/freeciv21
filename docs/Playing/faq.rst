Frequently Asked Questions (FAQ)
********************************

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

The following page has a listing of frequenty asked questions with answers about Freeciv21.

Acronymns
=========

Common acronymns you will find in this FAQ as well as on the Longturn Community's Discord server and other
documentation:

* :strong:`FC`: Freeciv - Classic legacy Freeciv
* :strong:`FC21`: Freeciv21 - A new fork of Freeciv v3.0 concentrating on multiplayer games and a single client.
* :strong:`FCW`: Freeciv Web - a web client and highly customized version of the legacy Freeciv server.
  Not affiliated directly with Freeciv, Freeciv21 or Longturn.
* :strong:`LT`: Longturn
* :strong:`LTT`: Longturn Tradtional Ruleset - LT's standard ruleset
* :strong:`LTX`: Longturn Experimental Ruleset - LT's experimental rulset
* :strong:`MP2`: Multiplayer 2 Ruleset. There are many MP2 rulesets and they are numbered: MP2a, MP2b, MP2c, etc.
* :strong:`MP`: Move Point - the number of moves a unit has available.
* :strong:`HP`: Hit Point - the amount of health a unit has avalable. When this goes to zero the unit is killed.
* :strong:`FP`: Fire Power - The amount of damage an attacking unit can inflict on a defending unit.
* :strong:`TC`: Turn Change - A period of time when the server processes end of turn events in a specific
  :doc:`order <../Playing/turn-change>`.


Gameplay
========

This section of the FAQ is broken into a collection of sub-sections all surrounding general GamePlay aspects
of Freeciv21:

* Gameplay General
* Diplomacy
* Game Map and Tilesets
* Cities and Terrain
* Units - General
* Units - Military
* Other


Gameplay General
----------------

This subsection of the Gameplay section is a generalized discussion of questions.

OK, so I installed Freeciv21. How do I play?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Start the client. Depending on your system, you might choose it from a menu, double-click on the
:file:`freeciv21-client` executable program, or type :file:`freeciv21-client` in a terminal window.

Once the client starts, to begin a single-player game, select :guilabel:`Start new game`. Now edit your
game settings (the defaults should be fine for a beginner-level single-player game) and press the
:guilabel:`Start` button.

Freeciv21 is a client/server system. However, in most cases you don't have to worry about this; the client
starts a server automatically for you when you start a new single-player game.

Once the game is started you can find information in the :guilabel:`Help` menu. If you have never played a
Civilization-style game before you may want to look at help on :title-reference:`Strategy and Tactics`.

You can continue to change the game settings through the :menuselection:`Game --> Server Options` menu.
Type :literal:`/help` in the chatline (or server command line) to get more information about server commands.

A more detailed explanation of how to play Freeciv21 is available in :doc:`../Playing/how-to-play`, and in the
in-game help.

Where is the chatline you are talking about, how do I chat?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are two widgets on the main map area: Chatline and Messages. The messages widget can be toggled
visible/not-visible via a button on the top bar. The chatline is another widget on the game map and is always
visible.

The chatline can be used for normal chatting between players, for issuing server commands by typing a
forward-slash :literal:`/` followed by the server command and seeing server output messages.

See the in-game help on :title-reference:`Chatline` for more detail.

That sounds complicated and tedious. Isn’t there a better way to do this?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No, there’s no other better/GUI way. This is a big reason why the Longturn Community prefers using Discord.
There are plans to improve this, but it is not implemented yet.

Is there a way to send a message to all your allies?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the client, there’s a checkbox to the far right of the chatline widget. When selected, any messages typed
will only got to your allies.

.. Note:: This option only shows up if you are playing an online Longturn Community game with a remote server.
  If you are playing a local single-player game against AI, this option does not show up since you cannot
  chat with the AI.

How do I find out about the available units, improvements, terrain types, and technologies?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is extensive help on this in the :guilabel:`Help` menu, but only once the game has been started. This is
because the in-game help is generated at run-time based on the server and ruleset settings as configured.

The game comes with an interactive tutorial scenario. To run it, select :guilabel:`Start Scenario Game` from
the main menu, then load the :strong:`tutorial` scenario.

How can I change the way a Freeciv21 game is ended?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A standard Freeciv21 game ends when only allied players/teams are left alive; when a player's spaceship
arrives at Alpha Centauri; or when you reach the ending turn -- whichever comes first.

For Longturn multi-player games, the winning conditions are announced before the game begins and can vary
widely between games.

For local single-player games, you can change the default ending turn by changing the ``endturn`` setting.
You can do this through the :menuselection:`Game --> Server Options` menu or by typing into the chatline
something like:

.. code-block:: rst

    /set endturn 300


You can end a running game immediately with:

.. code-block:: rst

    /endgame


For more information, try:

.. code-block:: rst

    /help endgame
    /help endturn


If you want to avoid the game ending by space race, or require a single-player/team to win, you can change
the victories setting -- again either through the Server Options dialog or through the chatline. For instance
this changes from the default setting ``spacerace|allied`` victory to disallow allied victory and space race:

.. code-block:: rst

    /set victories ""


You can instead allow spaceraces without them ending the game by instead changing the ``endspaceship`` setting.

A single-player who defeats all enemies will always win the game. This conquest victory condition cannot be
changed.

In rulesets which support it, a cultural domination victory can be enabled, again with the victories setting.

How do I play against computer players?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Refer to the `How do I create teams of AI or human players?`_ section below.

In most cases when you start a single-player game you can change the number of players, and their
difficulty, directly through the spinbutton.

.. note:: The number of players here includes human players (an ``aifill`` of ``5`` adds AI players until the
  total number of players becomes 5).

If you are playing on a remote server, you'll have to do this manually. Change the ``aifill`` server option
through the :guilabel:`Game --> Server Options` options dialog, or do it on the chatline with something like:

.. code-block:: rst

    /set aifill 30


Difficulty levels are set with the ``/cheating``, ``/hard``, ``/normal``, ``/easy``, ``/novice``, and
``/handicapped`` commands.

You may also create AI players individually. For instance, to create one hard and one easy AI player, enter:

.. code-block:: rst

    /create ai1
    /hard ai1
    /create ai2
    /easy ai2
    /list


More details are in :doc:`../Playing/how-to-play`, and in the in-game help.

How do I create teams of AI or human players?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The client has a GUI for setting up teams - just right click on any player and assign them to any team.

You may also use the command-line interface (through the chatline.)

First of all try the ``/list`` command. This will show you all players created, including human
players and AI players (both created automatically by aifill or manually with ``/create``).

Now, you're ready to assign players to teams. To do this you use the team command. For example, if there's
one human player and you want two more AI players on the same team, you can do to create two AI players and
put them on the same team you can do:

.. code-block:: rst

    /set aifill 2
    /team AI*2 1
    /team AI*3 1


You may also assign teams for human players, of course. If in doubt use the ``/list`` command again;
it will show you the name of the team each player is on. Make sure you double-check the teams before
starting the game; you can't change teams after the game has started.

Can I build up the palace or throne room as in the commercial Civilization games?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No. This feature is not present in Freeciv21, and will not be until someone draws the graphics and writes the
client related code for it. Feel free to :doc:`contribute <../Contributing/index>`.

My opponents seem to be able to play two moves at once!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

They are not; it only seems that way. Freeciv21's multiplayer facilities are asynchronous: during a turn,
moves from connected clients are processed in the order they are received. Server managed movement is executed
in between turns (e.g. at TC) This allows human players to surprise their opponents by clever use of goto or
quick fingers.

A turn in a Longturn game typically lasts 23 hours and it's always possible that they managed to log in twice
between your two consecutive logins. However, firstly, there is a mechanic that slightly limits this (known as
unit wait time), and secondly, this can't happen every time because now they have already played their move
this turn and now need to wait for the Turn Change to make their next move. So, in the next turn, if you log
in before them, now it was you who made your move twice. If not, they can't :emphasis:`move twice` until you
do.

The primary server setting to mitigate this problem is ``unitwaittime``, which imposes a minimum time between
moves of a single unit on successive turns. This setting is used to prevent a varying collection of what the
community calls "turn change shenanigans". For example, one such issue is moving a :unit:`Worker` into enemy
territory just before Turn Change and giving it orders to build a road. After Turn Change you go in and
capture a city using the road for move benefit. Without ``unitwaittime`` you would be able to move the
:unit:`Worker` back to safety immediately, thereby prevent it from being captured or destroyed. With
``unitwaittime`` enabled, you have to wait the requisite amount of time. This makes the game harder, but also
more fair since not everyone can be online at every Turn Change.

.. Note:: The ``unitwaittime`` setting is really only used in Longturn multi-player games and is not
  enabled/used for any of the single-player rulesets shipped with Freeciv21

Why are the AI players so hard on 'novice' or 'easy'?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Short answer is... You are not expanding fast enough.

You can also turn off Fog of War. That way, you will see the attacks of the AI. Just type
:code:`/set fogofwar disabled` on the chatline before the game starts.

Why are the AI players so easy on 'hard'?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Several reasons. For example, the AI is heavily play tested under and customized to the default ruleset and
server settings. Although there are several provisions in the code to adapt to changing rules, playing under
different conditions is quite a handicap for it. Though mostly the AI simply doesn't have a good, all
encompassing strategy besides :strong:`"eliminate nation x"`.

To make the game harder, you could try putting some or all of the AI into a team. This will ensure that they
will waste no time and resources negotiating with each other and spend them trying to eliminate you. They
will also help each other by trading techs. See the question `How do I create teams of AI or human players?`_.

You can also form more than one AI team by using any of the different predefined teams, or put some AI
players teamed with you. Another alternative is to create AIs that are of differing skill levels. The stronger
AIs will then go after the weaker ones.

What distinguishes AI players from humans? What do the skill levels mean?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

AI players in Freeciv21 operate in the server, partly before all client moves, partly afterwards. Unlike the
client, they can in principle observe the full state of the game, including everything about other players,
although most levels deliberately restrict what they look at to some extent.

All AI players can change production without penalty. Some levels (generally the harder ones) get other
exceptions from game rules; conversely, easier levels get some penalties, and deliberately play less well in
some regards.

For more details about how the skill levels differ from each other, see the help for the relevant server
command (for instance :code:`/help hard`).

Other than as noted here, the AI players are not known to cheat.

Does the client have a combat calculator, like other Civ games have?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is no integrated combat calculator. You can use the one on the longturn.net website here:
https://longturn.net/warcalc/. You can also select an attacking unit and then middle-click over a defending
unit and in the pop-up window will see the odds of win/loss.

Where in the client does it say what government you’re currently under?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the topbar near the right side there is a bank of graphics that show what your economy consists of (Tax,
Sci , or Lux) as well as what Government you are under, chance for Global Warming, Nuclear Winter and how far
along you are with research. You can hover your mouse over any of these icons to see more details.

What government do you start under?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You start under Despotism in LTT. This is a ruleset configured item.

Do things that give more trade (certain governments, wonders) only give this bonus if there’s already at least 1 trade produced on a tile?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The short answer is yes in LTT. This is a ruleset configured item.


Diplomacy
---------

This subsection of the Gameplay section is a discussion around Diplomacy.

Why can't I attack another player's units?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You have to declare war first. See the section for `How do I declare war on another player?`_ below.

.. note:: In some rulesets, you start out at war with all players. In other rulesets, as soon as you
    make contact with a player, you enter armistise towards peace. At lower skill levels, AI players offer
    you a cease-fire treaty upon first contact, which if accepted has to be broken before you can attack
    the player's units or cities. The main thing to remember is you have to be in the diplomatic state of war
    in order to attack an enemy.

How do I declare war on another player?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to the :guilabel:`Nations` page (F3), select the player row, then click :guilabel:`Cancel Treaty` at the
top. This drops you from :emphasis:`cease fire`, :emphasis:`armistice`, or :emphasis:`peace` into
:emphasis:`war`. If you've already signed a permanent :emphasis:`alliance` treaty with the player, you will
have to cancel treaties several times to get to :emphasis:`war`.

See the in-game help on :title-reference:`Diplomacy` for more detail.

.. note:: The ability to arbitrarily leave :emphasis:`peace` and go to :emphasis:`war` is also heavily
    dependent on the form of governement your nation is currently ruled by. See the in-game help on
    :title-reference:`Government` for more details.

How do I do diplomatic meetings?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to the :guilabel:`Nations` page (F3), select the player row, then choose :guilabel:`Meet` at the top.
Remember that you have to either have contact with the player or an embassy established in one of their cities
with a :unit:`Diplomat`.

How do I trade money with other players?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to make a monetary exchange, first initiate a diplomatic meeting as described in the section
about `How do I do diplomatic meetings?`_ above. In the diplomacy dialog, enter the amount you wish to give in
the gold input field on your side or the amount you wish to receive in the gold input field on their side.
With the focus in either input field, press :guilabel:`Enter` to insert the clause into the treaty.

.. Note:: In some rulsets there might be a "tax" on gold transfers, so watch out that not all gold will make
  it to its intended destination nation.

Is there a way to tell who’s allied with who?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :guilabel:`Nations` Page (F3) shows diplomacy and tech information if you have an embassy with the target
nation. To see what is going on, select a nation and look at the bottom of the screen.


Game Map and Tilesets
---------------------

This subsection of the Gameplay section is a discussion around the game map and tilesets (the graphics layer).

Can one use a regular square tileset for iso-square maps and vice versa?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While that’s technically possible, hex and iso-hex topologies aren’t directly compatible with each other, so
the result isn’t playable in a good (visualization) way. In the client you can force the change of tileset by
going to :menuselection:`Game --> Load Another Tileset`. If the client can change, it will and you will be
able to experiment a bit. If there is a complete discrepency, the client will throw an error and won't make
the requested change.

How do I play on a hexagonal grid?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is possible to play with hexagonal instead of rectangular tiles. To do this you need to set your topology
before the game starts; set this with Map topology index from the game settings, or in the chatline:

.. code-block:: rst

    /set topology hex|iso|wrapx


This will cause the client to use an isometric hexagonal tileset when the game starts (go to
:menuselection:`Game --> Set local options` to choose a different one from the drop-down;
hexemplio and isophex are included with the game).

You may also play with overhead hexagonal, in which case you want to set the topology setting to
:code:`hex|wrapx`; the hex2t tileset is supplied for this mode.

Can one use a hexagonal tileset for iso-hex maps and vice versa?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the question `Can one use a regular square tileset for iso-square maps and vice versa?`_ above.


Cities and Terrain
------------------

This subsection of the Gameplay section is a discussion around cities and the terrain around them.

My irrigated grassland produces only 2 food. Is this a bug?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No, it's not -- it's a feature. Your government is probably Despotism, which has a -1 output penalty whenever
a tile produces more than 2 units of food, production, ortrade. You should change your government (See the
in-game help on :title-reference:`Government` for more detail) to get rid of this penalty.

This feature is also not 100% affected by the form of government. There are some small and great wonders
in certain rulesets that get rid of the output penalty.

Can I build land over sea/transform ocean to land?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. You can do that by placing :unit:`Engineer` in a :unit:`Transport` and going to the ocean tile you want
to build land on. Click the :unit:`Transport` to display a list of the transported :unit:`Engineers` and
activate them. Then give them the order of transforming the tile to swamp. This will take a very long time
though, so you'd better try with 6 or 8 :unit:`Engineers` at a time. There must be 3 adjacent land tiles to
the ocean tile (e.g. a land corner) you are transforming for this activiy to work.

Is there an enforced minimum distance between cities?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This depends on the ruleset. In LTT there is a minimum distance of 3 empty tiles between two cities. You can
think of it as “no city can be built within the work radius of another city”, since the work radius of a city
is also 3 tiles in LTT.

This setting, knowns as ``citymindist``, can be set in the server settings before a local game starts or by
changing at the chatline:

.. code-block:: rst

    /set citymindist 4


If your city is going to grow next turn and you rush-buy a Granary, do you still get the food savings?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. Production is “produced” before growth at turn change. This is true for all rulsets as it is part of the
standard :doc:`Turn Change <../Playing/turn-change>` process.

How much population do Settlers take to build?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two (2) in the LTT ruleset. This is a ruleset configurable item. See the in-game help on
:title-reference:`Units` for more detail to see what the settings is for the ruleset you loaded at game
start.

Do tiles remember terraforming progress?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you change orders for the unit doing the terraforming and don’t change them back within the same turn, the
terraforming progress is lost. If you change orders and then change them back, nothing special happens.
Terraforming is always processed at Turn Change.

How frequently do natural disasters happen?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This depends on the ruleset. For the LTT ruleset, all natural disasters have a 1% probability to happen each
turn. The default is 10%. This setting, known as ``disasters``, can be set in the server settings before a
local game starts or by changing at the chatline:

.. code-block:: rst

    /set disasters 20


Does the city work area change in any way during the game?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is a ruleset configured option. In LTT the intial value is ``15``, effectively giving 3 tiles "out" from
the city center in all directions. Varying technologies or buildings can be programmed into the ruleset to
change the vision radius (e.g. the work area) of a city.

Is it worth it to build cities on hills (potentially with rivers), or is the risk of earthquakes and floods too large?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It’s usually worth it, since hills and rivers have great defense values. Rivers also allow you to build an
:improvement:`Aqueduct, River` without the knowledge of Construction and it is much cheaper to build and
requires no upkeep.

.. note:: It is a great strategy if you can do this to place your first city (Capital) either on a river or
  adjacent to one to get this "fresh water" effect. You can get the city up to size 16 very fast with the right
  growth strategy.

Is there a benefit to lake tiles over ocean tiles? What are their differences?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are at least the following differences:

* Lake tiles allow an adjacent city to build a cheap :improvement:`Aqueduct, Lake` with no upkeep and before the
  discovery of Construction. This is commonly referred to as the "fresh water" effect.
* Lake tiles give more food than ocean tiles, especially with the Fish tile special.
* Shallow ocean gives +1 production with :improvement:`Offshore Platform`. The :improvement:`Offshore Platform`
  city improvement often comes with the discovery of Miniturization in most rulesets, but this is a ruleset
  configurable item.
* Some ships can’t travel on deep ocean (such as Triremes)
* Shallow ocean has a 10% defense bonus.
* Ocean tiles allow you to build :improvement:`Harbor`, giving +1 food. The :improvement:`Harbor` city
  improvement often comes with the discovery of Seafaring in most rulsets, but this is a ruleset configurable
  item.

Is “Aqueduct, River” identical to “Aqueduct, Lake”?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In most rulesets, yes. It is part of the "fresh water" effect of giving a cheap :improvement:`Aqueduct` that
has no gold upkeep and does not require the discovery of Construction.

Is the city tile worked for free?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. This is hardcoded in the server. In all rulesets a size 1 city will always have two tiles being worked
by the citizens of the city: the city center tile and another one in its vision radius that is not being
worked by an adjacent city.

Do you get free irrigation on the city tile?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You get a “virtual” irrigation effect. It works the same way as regular irrigation for food purposes, but
doesn’t allow you to build irrigation next to the city by itself. You’ll have to build regular irrigation on
the city center tile to do that. The free irrigation is lost if you build a mine on the city tile (just like
regular irrigation on a regular tile is lost with a mine). This means that a desert tile that is mined has
zero (0) food, even when on a city center tile. Since the “virtual” irrigation works like regular irrigation,
if you build a city on a tile that can’t be irrigated normally (e.g. a forest), you don’t get any food bonus.

In the late game, many rulesets have a :improvement:`Supermarket`, that comes with the disovery of
Refrigeration. A player can then use :unit:`Workers` or :unit:`Engineers` to add Farmland on top of the
existing irrigation for an addition food bonus. In this sense, if you want to get the Farmland food effect on
a city center tile that is already "virtually" irrigated you will have to actually irrigate the tile and then
add Farmland on top of it, just like any other regular tile.

Does the city tile have any production bonuses?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A city tile has a +1 production bonus, added after any other bonuses (such as railroad).

Does LTT have the extra food from rivers on a desert tile when irrigated, like other rulesets have?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, an irrigated desert tile with a river gives an extra +1 food in addition to the regular irrigation food
bonus. This is a game engine (server) item and is not driven by a ruleset, such as LTT.

Is there any penalty when changing a city production task?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are 4 “categories” of production: units, city improvements (e.g. Buildings), great wonders, and small
wonders. If you change within a “category” (e.g. :unit:`Phalanx` to :unit:`Horsemen`, or
:improvement:`Library` to :improvement:`Bank`), there is no penalty. If you change across categories (e.g.
:unit:`Archers` to :improvement:`Library`, or :wonder:`Leonardo’s Workshop` to :unit:`Frigate`), there’s a 50%
penalty. If you change back to the same category within the same turn, the penalty is reversed. If you change
multiple times, the penalty is only applied once, which means that if you change the production target more
than once in a turn there will be no penalty as long as you land on the same "category" as was active at the
beginning of the turn.

Is there a way to claim tiles using Diplomats?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No. In Freeciv21 there are generally 4 ways to gain tile ownership:

* Build a city and claim the tiles first.
* Grow your cities super big and much bigger than your neighbor's cities. National borders can move at Turn
  Change based on culture score.
* Build a Fortress and place a Military unit (e.g. a :unit:`Phalanx`) inside the Fortress.
* Conquer the city and take its tiles for your own.

Is there a way to create a hill other than terraforming a mountain?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can also terraform a hill from plains with :unit:`Engineers`. In some rulesets, such as LTT, this is very
expensive in workker move points and can take some time unless you place many :unit:`Engineers` on the tile
at the same time.

Can you build a hill under a city?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, you sure can!


Units - General
---------------

This subsection of the Gameplay section is a generalized discussion around units.

When does the game inform you of enemy movement within your units’ field of vision?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It depends on the status of the unit. If the unit is fortified or working on another task (e.g. irrigation)
then you will not be notified. Only the :strong:`Sentry` status will give you a notification.

If I move a unit onto a mountain, does that change how many movement points the unit has next turn?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All units that end their turn on a mountain start with 1 less MP the following turn. The exception to this
rule are units that ignore terrain movement completely (e.g. :unit:`Explorers`, and :unit:`Alpine Troops`).
This is knowns as "ignoring terrain effects".

What is a unit’s terraforming speed based on?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It’s based on the base amount of movement points for that unit and veteran level bonus. The base terraforming
duration is specified in the ruleset files.

.. todo:: This is discussed in detail in a forthcoming LTT Gamer's Manual. Update this entry at that time.

Can workers do all land conversions? Or are most land conversions locked behind engineers?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All :unit:`Workers` can do land conversions except for major land transformations, which are available only
with :unit:`Engineers`.

Does a damaged worker work slower than normal?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No, Hit Points do not factor in a :unit:`Worker's` ability to conduct infrastructure improvements to tiles.

When terraforming, does some movement get used on the last turn of terraforming? Does the unit start with less movement points?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Terraforming doesn’t affect a unit's Movement Points in any way.

Does damage reduce the amount of movement points the unit has?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This depends on the unit class and the ruleset configuration. If a unit’s help text specifies it is "slowed
down when damaged", then it does. If it doesn’t say anything about it, then it doesn’t.

Do Caravans give full production?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :unit:`Caravan` unit is a special unit that allows a player to move production from one city to another in
order to increase the speed of constructing wonders (both small and great). This effect only works for wonders
and no other city improvement. The :unit:`Caravan` unit acts like any other unit when disbanded in a city: it
gives back 50% of the shields it took to construct it in the first place. In many rulesets the :unit:`Freight`
becomes available in the late game and obsoletes the :unit:`Caravan`. The :unit:`Freight` works the same way.

How does unit leveling work?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Freeciv21 calls this unit "Veterancy" or "Veteran Levels". You have a chance every turn for any kind of unit to
gain an upgrade via experience. The experience depends on the unit and what they are doing. For example,
a :unit:`Worker` gains experience by creating terrain infrastructure, or a :unit:`Phalanx` gains experience
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

It is possible when the unit is moved to a city that isn’t its current home city. You then get an option to
change the home city. With the unit in a city you can either use hotkey “h” or
:guilabel:`Unit --> Set Home City` to rehome the unit to the city it is inside.

.. Note:: Some rulsets allow "unhomed" units. These kind of units will never have a home city and you cannot
  change it, even if you wanted to. These units have no upkeep, so they can stay unhomed.

Are queued goto commands executed before or after units and city improvements are built?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After. For example, you can beat an enemy attacking unit with a queued goto to your city by rush-buying a
defensive unit (it will get built first during normal :doc:`turn change processing <../Playing/turn-change>`),
and the attacking unit will move after that.


Units - Military
----------------

This subsection of the Gameplay section is a discussion around military units specifically.

My opponent's last city is on a 1x1 island so I cannot conquer it, and they won't give up. What can I do?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It depends on the ruleset, but often researching Amphibious Warfare will allow you to build a
:unit:`Marine`. Alternatively research Combined Arms and either move a :unit:`Helicopter` or airdrop a
:unit:`Paratrooper` there. When viewing the in-game help text for :title-reference:`Units`, be on the look out
for ``Can launch attack from non-native tiles``. This is the unit's feature that allows you to attack from
the ocean or air versus land, which is a native tile.

If you can't build :unit:`Marines` yet, but you do have :unit:`Engineers`, and other land is close-by, you
can also build a land-bridge to the island (i.e. transform the ocean). If you choose this route, make sure
that your :unit:`Transport` is well defended!

Does a unit with less than 1 movement point remaining have weaker attacks?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, the base attack is multiplied by the remaining movement points when the unit has less than 1 MP left.
This is commonly known as “tired attack”. As an example, a green :unit:`Knights` (base attack 6) with 6/9
movement points remaining will attack as if it had attack 4.

How can I tell what final defense a unit will have after applying all bonuses from terrain, fortification, city, and such?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The client doesn’t show this information, so you’ll have to calculate manually. Math is an important element of
all Freeciv21 games, and especialy the LTT and LTX multi-player games the Longturn Community enjoys playing. As
in the game of Chess, the "board" does not do the math for you. You much gauge the risk-reward ratios of your
moves and counter-moves. This is the same in Freeciv21. The game will not do the math for you. This table should
help you in doing the math:

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

.. Tip:: The legacy Freeciv WiKi gives some good information in the Game Manual about Terrain here:
  https://freeciv.fandom.com/wiki/Terrain.

What is the math for upgrading units in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The basic upgrade cost is the same as disbanding the old unit in a city, and then rush-buying the new unit from
the contributed shields (production).

:strong:`Example`: :unit:`Phalanx` --> :unit:`Pikeman`

The :unit:`Phalanx` contributes 7 shields (15 / 2 rounded down). The :unit:`Pikeman` costs 25 shields. The
remaining 18 shields (25 - 7), is bought with gold using the formula for rush-buying units. For the math folks
out there, the formula for rush-buying units (in all cases, not just for upgrades) is:
``2 * p + (p * p) / 20`` where ``p`` is the remaining production (or shields).

Are diplomats used up when investigating an enemy city?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, they are destroyed/consumed after conducting an "investigate city" action. In some rulesets (notably LTT
and LTX), there is a :unit:`Spy` available when you research Espionage. The :unit:`Spy` is not consumed by the
same actions as the :unit:`Diplomat`.

Are there any other diplomatic units, other than Diplomats and Spies?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The units available is highly dependent on the ruleset. For the LTT and LTX rulesets there are a couple "tech"
stealing units: :unit:`Scribe` and :unit:`Scholar`. They can be used to steal (incite) units from other players
and also to steal technology. In the LTT and LTX rulesets, there is no technology trading (between allies), so
these two units were created as a way to allow technology trading, but at a risk.

When my unit moves in my territory on rivers, it costs 1/3 MP per tile. If I move on a river in enemy territory, it costs the full MP for the tile. Why?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Tile improvements that affect movement (rivers, roads, railroads) only apply when the unit is on allied
territory, or on territory not owned by anyone. When moving through enemy territory, the terrain acts as if
those improvements don’t exist. The server setting that controls this is called ``restrictinfra``. This value
can be set in the server settings before a local game starts or by changing at the chatline:

.. code-block:: rst

    /set restrictinfra FALSE


The LTT and LTX rulesets used by the Longturn Community have this value set to ``TRUE``. This is also the
default setting for many of the single-player rulesets shipped with Freeciv21.

Is there a way to see potential battle odds?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. Select the unit you want to attack with and then middle-click (or Alt-click on Windows) over the potential
target and a pop-up window will show you the odds of attack and defense taking into account all aspects of
the attack (or defense) include terrain bonus, unit veterancy, etc.

When a city is captured, all units homed in that city that are currently in another city of yours are re-homed to that city. What happens to the the other units?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any units not in a native city (e.g. your own city) are lost. This includes allied cities or outside of any
city in the field.


Other
-----

This subsection of the Gameplay section is a catchall area for questions don't fit nicely into the other
subsections.

Can I change settings or rules to get different types of games?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Of course. Before the game is started, you may change settings through the :guilabel:`Server Options`
dialog. You may also change these settings or use server commands through the chatline. If you use the
chatline, use the:

.. code-block:: rst

    /show

command to display the most commonly-changed settings, or

.. code-block:: rst

    /help <setting>


to get help on a particular setting, or

.. code-block:: rst

    /set <setting> <value>


to change a setting to a particular value. After the game begins you may still change some settings, but not
others.

You can create rulesets or :strong:`modpacks` - alternative sets of units, buildings, and technologies.
Several different rulesets come with the Freeciv21 distribution, including a civ1 (Civilization 1
compatibility mode), and civ2 (Civilization 2 compatibility mode). Use the ``rulesetdir`` command to change
the ruleset (as in ``/rulesetdir civ2``). For more information refer to :doc:`../Modding/index`.

How compatible is Freeciv21 with the commercial Civilization games?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Freeciv21 was created as a multiplayer version of Civilization |reg| with players moving simultaneously.
Rules and elements of Civilization II |reg|, and features required for single-player use, such as AI
players, were added later.

This is why Freeciv21 comes with several game configurations (rulesets): the civ1 and civ2 rulesets implement
game rules, elements and features that bring it as close as possible to Civilization I and Civilization II
respectively, while other rulesets such as the default Classic ruleset tries to reflect the most popular
settings among Freeciv21 players. Unimplemented Civilization I and II features are mainly those that would
have little or no benefit in multi-player mode, and nobody is working on closing this gap.

Little or no work is being done on implementing features from other similar games, such as SMAC, CTP or
Civilization III+.

So the goal of compatibility is mainly used as a limiting factor in development. When a new feature is added
to Freeciv21 that makes gameplay different, it is generally implemented in such a way that the
:emphasis:`traditional` behaviour remains available as an option. However, we're not aiming for absolute
100% compatibility; in particular, we're aiming for bug-compatibility.

I want more action.
^^^^^^^^^^^^^^^^^^^

In Freeciv21, expansion is everything, even more so than in the single-player commercial Civilization games.
Some players find it very tedious to build on an empire for hours and hours without even meeting an enemy.

There are various techniques to speed up the game. The best idea is to reduce the time and space allowed for
expansion as much as possible. One idea for multiplayer mode is to add AI players: they reduce the space per
player further, and you can toy around with them early on without other humans being aware of it. This only
works after you can beat the AI, of course.

Another idea is to create starting situations in which the players are already fully developed. Refer to the
section on :strong:`scenarios` in :doc:`../Modding/index`.


Non-Gameplay Specific Questions
===============================

This section of the FAQ deals with anything not related to general gameplay aspects of Freeciv21.

Longturn Multiplayer
--------------------

How do I play multi-player?
^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can either join a network game run by someone else, or host your own. You can also join one of the many
games offered by the Longturn community.

To join an open network game, choose :guilabel:`Connect to network game` and then
:guilabel:`Internet servers`. A list of active servers should come up; double-click one to join it.

To host your own game, we recommend starting a separate server by hand.

To start the server, enter :file:`freeciv21-server` in a terminal or by double-clicking on the executable.
This will start up a text-based console interface.

If all players are on the same local area network (LAN), they should launch their clients, choose
:guilabel:`Connect to Network game` and then look in the :guilabel:`Local servers` section. You should see
the existing server listed; double-click on it to join.

To play over the Internet, players will need to enter the hostname and port into their clients, so the game
admin will need to tell the other players those details. To join a longturn.net server you start by clicking
:guilabel:`Connect to Network Game` and then in the bottom-left of the dialog fill in the
:guilabel:`Connect`, :guilabel:`Port`, and :guilabel:`Username` fields provided by the game admin. Once
ready, click the :guilabel:`Connect` button at the botton-right, fill in your longturn.net password in the
:guilabel:`Password` box and you will be added to the game.

.. note:: Hosting an Internet server from a home Internet connection is often problematic, due to
    firewalling and network address translation (NAT) that can make the server unreachable from the wider
    Internet. Safely and securely bypassing NAT and firewalls is beyond the scope of this FAQ.


Where do I see how much time is left in the current turn?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the minimap in the bottom right of the main map, where the :guilabel:`Turn Done` button shows for
single-player games. For Longturn multi-player games will also add a count-down timer to show when the turn
will change.

When connecting to a game, is the username field case-sensitive?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, both the username and password is case-sensitive.

After typing in the hostname, port, and username, the password field is greyed out. What’s up with that?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You have to click the :guilabel:`Connect` button to ask the client to connect to the server and then you
enter your password after connecting to authorize your entry into the game.

How do I take over an AI player?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the chatline you use the ``/take <playername>`` command to take over an AI player.

How do I take over an idle player that was assigned to me?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Same procedure as `How do I take over an AI player?`_ above.

Does capturing work like MP2?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Unit capturing is ruleset defined. Capturing in LTT works slightly differently than in the MP2 ruleset. You
can capture any “capturable” unit with a “capturer” unit, if the target is alone on a tile. Units that are
“capturable” have a mention of this in their help text. Units that are “capturers” also have a mention of this
in their help text.

.. Tip:: Due to the client mechanics, you can capture units from boats. This can’t be done using the regular
  “goto” command, but has to be done using the numpad.


Where do I go to see the rules for a game? Like how big a victory alliance can be?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All rules and winning conditions are posted to the `https://forum.longturn.net/index.php <forums>`_ under the
Games index. Each game has a section for varying posts related to the game. Winning conditions are also often
posted on the Longturn Discord `https://discord.gg/98krqGm <server>`_ in the channel for the game.

Does the “Nations” page show whether the player is idling?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, you may have to enable the visibility of the column. Right-click the header bar to see what columns are
enabled. You are looking for the column named ``idle``.

Can you make hideouts in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hideouts are a purely FCW thing. There’s no such thing in LTT. Other rulesets could offer this as it is a
ruleset configurable item. The Longturn Community does not like them as they are overpowered and easily
exploited.

How does research in LTT compare to MP2a/b/c?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

MP2a/b/c and LTT are all different rulesets, so obviously this is a ruleset configured item. In MP2a/b, all
bulbs carry over to the new research. In MP2c, bulbs researched towards a technology stay with that
technology. In LTT, bulbs don’t stay with a particular technology. There’s a 10% penalty when switching
research. This penalty is processed at Turn Change, so if you change your research again within the same turn,
you don’t suffer any additional penalties. If you then change your research back to the original technology
within the same turn, you don’t suffer the 10% penalty.

Is stack kill enabled in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, it is. This is a game server setting and is enabled on LTT games as without it a player could bring a
stack of 100 units onto the same tile. With stack kill enabled, it eliminates this very overpowered capability.

Is it really so that in LTT there’s no rapture, but you get a trade bonus in celebrating cities instead?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The concept of "rapture" is a ruleset and server configured item. The LTT ruleset does not do rapture. Instead
“celebration” is used under Republic and Democracy. In the LTX Ruleset the Federation government also allows
for celebration. Under other governments, celebration doesn’t provide any bonuses.

Are trade routes enabled in LTT?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Techncially yes, they are enabled. However is reality they are not enabled, because the required city to city
distance is 999. They are overpowered and would cause game balance issues in the multi-player environments
targeted by LTT.


Client Configuration
--------------------

How do I make the font bigger for help text?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can change a collection of fonts and font sizes by going to :guilabel:`Game --> Set local options` and
then clicking on the :guilabel:`Fonts` tab.

Is it possible to save login info in the client so it doesn’t have to be entered each time?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, you can set a number of items by going to :guilabel:`Game --> Set local options` and then clicking on
the :guilabel:`Network` tab. You can set the server, port and username. You cannot save the password as that
is a security risk.

Where can I turn off “connected / disconnected” messages filling up the chat window?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can adjust a collection of things by going to :guilabel:`Game --> Messages`. Anything checked in the ``out``
column will go to the chatline widget of the client. Anything in the ``mes`` column will show in the messages
widget. Lastly, anything checked in the ``pop`` column will produce a pop-up window message.

Many players actually enable a lot of things that normally show in the messages widget and put them in the
chatline widget as well. You can copy text from the chatline, but can not in messages. Being able to copy and
paste text to your allies comes in very handy.

How do I enable/disable sound or music support?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The client can be started without sound by supplying the commandline arguments :literal:`-P none`. The
default sound plugin can also be configured in the client settings by going to
:guilabel:`Game --> Set local options` and then clicking on the :guilabel:`Sound` tab.

If the client was compiled with sound support, it will be enabled by default.

How do I use a different tileset?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the tilesets supplied with Freeciv21 don't do it for you, some popular add-on tilesets are available
through the modpack installer utility. To install these, just launch the installer from the Start menu, and
choose the one you want; it should then be automatically downloaded and made available for the current user.
For more information, refer to :doc:`/Manuals/modpack-installer`.

If the tileset you want is not available via the modpack installer, you'll have to install it by hand from
somewhere. To do that is beyond the scope of this FAQ.

How do I use a different ruleset?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Again, this is easiest if the ruleset is available through the :strong:`Freeciv21 Modpack Installer` utility
that's shipped with Freeciv21.

If the ruleset you want is not available via the modpack installer, you'll have to install it by hand from
somewhere. To do that is beyond the scope of this FAQ.


Community
---------

Does Freeciv21 violate any rights of the makers of Civilization I or II?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There have been debates on this in the past and the honest answer seems to be: We don't know.

Freeciv21 doesn't contain any actual material from the commercial Civilization games. (The Freeciv21
maintainers have always been very strict in ensuring that materials contributed to the Freeciv21
distribution or Longturn website do not violate anyone's copyright.) The name of Freeciv21 is probably not a
trademark infringement. The user interface is similar, but with many (deliberate) differences. The game
itself can be configured to be practically identical to Civilization I or II, so if the rules of a game are
patentable, and those of the said games are patented, then Freeciv21 may infringe on that patent, but we
don't believe this to be the case.

Incidentally, there are good reasons to assume that Freeciv21 doesn't harm the sales of any of the
commercial Civilization games in any way.

How does Freeciv21 relate to other versions of Freeciv?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Freeciv21 is a code fork of Freeciv and is maintained by a community of online players called Longturn. After
using legacy Freeciv for many years for our multi-player games, the Longturn Community decided to fork Freeciv
because we felt that the development was not going in the right direction for multi-player games. Legacy
Freeciv is concentrating on single-player games for the most part.

Besides Freeciv21 and legacy Freeciv, there are also communities playing a version running in the browser,
commonly known as Freeciv Web. This version is less flexible and doesn't fulfill the needs of a diverse
community like Longturn.

Where can I ask questions or send improvements?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Please ask questions about the game, its installation, or the rest of this site at the Longturn Discord
Channels at https://discord.gg/98krqGm. The ``#questions-and-answers`` channel is a good start.

Patches and bug reports are best reported to the Freeciv21 bug tracking system at
https://github.com/longturn/freeciv21/issues/new/choose. For more information, have a look at
:doc:`../Contributing/bugs`.


Technical Stuff
---------------

I've found a bug, what should I do?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the article on `Where can I ask questions or send improvements?`_. You might want to start up a
conversation about it in the Longturn Discord channels if you are unsure.

I've started a server but the client cannot find it!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

By default, your server will be available on host :literal:`localhost` (your own machine), port
:literal:`5556`; these are the default values your client uses when asking which game you want to connect to.

So if you don't get a connection with these values, your server isn't running, or you used :literal:`-p` to
start it on a different port, or your system's network configuration is broken.

To start your local server, run :file:`freeciv21-server`. Then type :literal:`start` at the
server prompt to begin!

.. code-block:: rst

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

If you can't connect to any of the other games listed, a firewall in your organization/ISP is probably
blocking the connection. You might also need to enable port forwarding on your router.

If you are running a personal firewall, make sure that you allow communication for :file:`freeciv21-server`
and the :file:`freeciv21-client` to the trusted zone. If you want to allow others to play on your server,
allow :file:`freeciv21-server` to act as a server on the Internet zone.

How do I restart a saved game?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If for some reason you can't use the start-screen interface for loading a game, you can load one directly
through the client or server command line. You can start the client, or server, with the :literal:`-f`
option, for example:

.. code-block:: rst

    freeciv21-server -f freeciv-T0175-Y01250-auto.sav.bz2


Or you can use the :literal:`/load` command inside the server before starting the game.

The server cannot save games!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In a local game started from the client, the games will be saved into the default Freeciv21 save directory
(typically :file:`~/.local/share/freeciv21/saves`). If you are running the server from the command line,
however, any savegames will be stored in the current directory. If the autosaves server setting is set
appropriately, the server will periodically save the game automatically (which can take a lot of disk space
in some cases); the frequency is controlled by the :literal:`saveturns` setting. In any case, you should
check the ownership, permissions, and disk space/quota for the directory or partition you're trying to save
to.

Where are the save games located by default?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On Unix like systems (e.g. Linux), they will be in :file:`~/.local/share/freeciv21/saves`. On Windows, they
are typically found in in the :file:`Appdata\\Roaming` User profile directory. For example:

.. code-block:: rst

    C:\Users\MyUserName\AppData\Roaming\freeciv21\saves


You could change this by setting the :literal:`HOME` environment variable, or using the :literal:`--saves`
command line argument to the server (you would have to run it separately).

I opened a ruleset file in Notepad and it is very hard to read
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ruleset files (and other configuration files) are stored with UNIX line endings which Notepad doesn't
handle correctly. Please use an alternative editor like WordPad, notepad2, or notepad++ instead.

What are the system requirements?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:strong:`Memory`

In a typical game the server takes about 30MB of memory and the client needs about 200MB. These values may
change with larger maps or tilesets. For a single-player game you need to run both the client and the server.

:strong:`Processor`

We recommend at least a 1GHz processor. The server is almost entirely single-threaded, so more cores will
not help. If you find your game running too slow, these may be the reasons:

* :strong:`Too little memory`: Swapping memory pages on disc (virtual memory) is really slow. Look at the
  memory requirements above.

* :strong:`Large map`: Larger map doesn't necessary mean a more challenging or enjoyable game. You may try a
  smaller map.

* :strong:`Many AI players`: Again, having more players doesn't necessary mean a more challenging or enjoyable
  game.

* :strong:`City Governor (CMA)`: This is a really useful client side agent which helps you to organize our
  citizens. However, it consumes many CPU cycles. For more information on the CMA, refer to
  :doc:`../Playing/cma`.

* :strong:`Maps and compression`: Creating map images and/or the compression of saved games for each turn will
  slow down new turns. Consider using no compression.

* :strong:`Graphic display`: The client works well on 1024x800 or higher resolutions. On smaller screens you
  may want to enable the Arrange widgets for small displays option under Interface tab in local options.

* :strong:`Network`: Any modern internet connection will suffice to play Freeciv21. Even mobile hotspots
  provide enough bandwidth.


Windows
-------

How do I use Freeciv21 under MS Windows?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Precompiled binaries can be downloaded from https://github.com/longturn/freeciv21/releases. The native
Windows packages come as self-extracting installers.

OK, I've downloaded and installed it, how do I run it?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the document about :doc:`/Getting/windows-install`.


macOS
-----

Precompiled binaries in a :file:`*.dmg` file can be downloaded from https://github.com/longturn/freeciv21/releases.

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
