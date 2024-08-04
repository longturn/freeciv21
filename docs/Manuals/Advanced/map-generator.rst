.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Controlling the Map Generator
*****************************

The generated map and player placement for a game can be manipulated by changing varying
:doc:`/Manuals/Server/options`. This page aims to give the reader some recipes or options on how to manipulate
the varying options to create the map you want to play. It is assumed that the game master is using a
:doc:`/Manuals/Server/settings-file`.

.. note::
  See :doc:`/Coding/mapgen` for a detailed description of the inner workings of the generator.

The server options that impact what kind of map the generator creates are:

* ``alltemperate``
* ``flatpoles``
* ``generator``
* ``huts``
* ``landmass``
* ``mapseed``
* ``mapsize``
* ``revealmap``
* ``separatepoles``
* ``singlepole``
* ``size``
* ``specials``
* ``startpos``
* ``steepness``
* ``teamplacement``
* ``temperature``
* ``tilesperplayer``
* ``tinyisles``
* ``topology``
* ``wetness``
* ``xsize``
* ``ysize``

Some server options are more impacting than others to the makeup of the map. Let us start with talking about
the options that are least impacting to the overall map.

These options change the makeup of tiles and not the map itself:

* ``alltemperate`` -- When ``enabled``, all tiles have a similar temperature. Players will all have similar
  tiles to work. Set to ``disabled`` if you want to control with the ``temperature`` and ``wetness`` options.
* ``huts`` -- How many huts you want on the map. Longturn games typically have ``0`` huts.
* ``specials`` -- How many specials (tile resources) you want on the map.
* ``steepness`` -- Bigger numbers produce hilly and mountainous terrain, lower numbers do not.
* ``temperature`` -- You can create a hot or cold world with this option.
* ``tinyisles`` -- Set to ``enabled`` if you want small single-tile (1x1) islands on the map.
* ``wetness`` -- This option allows you to create a dry or wet map. Lower numbers are dry, higher numbers are
  wet.
* ``revealmap`` -- This setting is used to reveal the whole map at game start. Good to use for testing, but
  not for real games.


Poles and Base Topology
=======================

The concept of poles and overall topology is important in how you want to create your world map. There are
some options that when toggled will create a pole at the top and bottom, no poles whatsoever, or poles in the
map that you can work and navigate around.

The settings that define the poles and base topology are:

* ``flatpoles`` -- A low setting (``0``) will give poles with water and a high (``100``) will fill in with
  mostly glacier tiles on the poles.
* ``separatepoles`` -- When ``enabled`` will break poles up and when ``disabled`` will keep them together.
* ``singlepole`` -- When ``enabled`` will allow for a single pole and when ``disabled`` will give two poles.
* ``topology`` -- Sets the wrap (``X``, ``Y``), the orientation (isometric or overhead) and the tile type
  (squares vs hexes).

:strong:`Recipe: Blocking poles on the map`

Set ``topology "WRAPX|ISO"``, ``separatepoles enabled``, and ``singlepole disabled``. This gives you a map
with a blocking (cannot navigate around it) pole at the top and bottom of the map. You navigate East and
West. If you set ``singlepole enabled`` here, you will get a really thick blocking border.

You can create blocking poles on the left and right by changing ``WRAPX`` to ``WRAPY``. You would then
navigate North and South.

Lastly, you can create a map that is like a game board. If you remove both ``WRAPX`` and ``WRAPY`` on
``topology``, you will get regular poles at the top and bottom and "black" on the left and right. You can only
navigate inside the box.

:strong:`Recipe: A set of navigable poles on the map`

Set ``topology "WRAPX|WRAPY|ISO"``, ``separatepoles enabled``, and ``singlepole disabled``. This gives you two
poles on the map, which looks and acts like Earth where you can navigate around them.

If you set ``singlepole enabled`` instead of ``disabled`` you will get a single pole on the map instead of
two.

Lastly, if you set ``flatpoles 0`` you will get navigable poles with passages of water inside of them.

:strong:`Recipe: No poles on the map`

Set ``topology "WRAPX|WRAPY|ISO"``, ``separatepoles disabled``, and ``singlepole disabled``. This eliminates
all poles on the map. You get a world with continents and islands, but no poles. There is no edge to the map
as you can navigate in all directions indefinitely.

:strong:`Recipe: Overhead square tiles`

Set ``topology "WRAPX|WRAPY"``

:strong:`Recipe: Hex tiles`

Set ``topology "WRAPX|WRAPY|ISO|HEX"`` will give isometric hex tiles. Removing the ``ISO`` will give overhead
hex tiles.


Sizing Your Map
===============

The overall size of the map (total number of X and Y tiles) is driven by a collection of settings. They are:

* ``mapsize``
* ``size``
* ``tilesperplayer``
* ``xsize``
* ``ysize``

The ``mapsize`` option is the driver and has three possible configurations:

#. ``FULLSIZE`` -- When used, you must also have the ``size`` option set. The value is simply a number (in
   thousands) of tiles.
#. ``PLAYER`` -- When used, you must also have the ``tilesperplayer`` option set. The map generator will take
   this into account and try its best to give each player a similar number of tiles to settle.
#. ``XYSIZE`` -- When used, you must also have the ``xsize`` and ``ysize`` options set. These values are
   similar to the ``size`` option. Give the map generator very specific number of tiles on the two axis.

No recipes here. As a game master, you can figure out how big or small you want your map. Longturn games use
the ``tilesperplayer`` option for their games as a reference.


Finalizing the Map and Player Placement
=======================================

This is probably one of the hardest aspects of map generation. Actually getting the map you want as a game
master, but also ensuring that player placement (or even team placement) is done the way you want is not
exactly directly mapped. There is a bit of randomness involved.

These options are the last piece to defining a game map:

* ``generator``
* ``landmass``
* ``mapseed``
* ``startpos``
* ``teamplacement``

As with the ``revealmap`` option, discussed earlier, the ``mapseed`` option is used during testing to keep
randomness from creeping into your testing. By setting a value, you eliminate the RNG in the server from
impacting the game map you want to create. If a game master is also playing the game, it is recommended to
disable (remove or comment out) this option when the game starts so even the game master does not have
knowledge of the map at game start.

We will get to some recipes in a bit, but before we do that, let us talk about the ``generator`` and
``startpos`` options. They work in tandem with each other.

First up, ``generator`` has the following configurations:

* ``SCENARIO`` -- This configuration is for Scenario games only. This is a special use case.
* ``RANDOM`` -- The default. As the name implies, there is a dependency on the built-in Random Number
  Generator (RNG) in the server. The generator will attempt to create equally spaced, relatively small
  islands. Player placement will be impacted by the ``landmass`` option. The larger the value the bigger the
  continents/islands. This option is also impacted by the ``mapsize`` option. Best to use the ``FULLSIZE`` or
  the ``XYSIZE`` configuration.
* ``FRACTAL`` -- This is the setting most Longturn games use. This configuration will create earth-like maps.
  By default, all players are placed on the same continent. The ``landmass`` option can also impact placement.
* ``ISLAND`` -- Each player is placed on their own island. Each island is similar in size, but not shape.
* ``FAIR`` -- Every player gets the exact same island.
* ``FRACTURE`` -- Similar to ``FRACTAL``, however this configuration often places mountains on the coasts.

Now let us discuss ``startpos``, which has the following configurations:

* ``DEFAULT`` -- The default. This configuration uses the ``generator`` configuration to place players.
* ``SINGLE`` -- One player per island/continent.
* ``2or3`` -- As the configuration name implies, the ``startpos`` will place 2 or 3 players together on an
  island/continent.
* ``ALL`` -- Everyone is placed on the same continent. Make sure you give enough tiles when using this
  configuration. The ``landmass`` and ``tilesperplayer`` will come in handy.
* ``VARIABLE`` -- The server will use the RNG to give a bit of randomness to player placement. The size of the
  continents will be taken into account.

:strong:`Recipe: Large Pangea-like world`

To create a gigantic single continent and have every player start there, begin with the no poles recipe above.
Then set ``generator`` to ``RANDOM``, or ``FRACTAL``, or ``FRACTURE`` and set ``landmass 85`` (the max).
Finally set ``startpos "ALL"``.

If you want more ocean or poles, you can reduce the ``landmass`` setting and add poles with the recipe above.
Longturn games use a ``landmass`` setting of ``40`` with poles for many games.

:strong:`Recipe: An archipelago with players on their own island`

To create an archipelago and start each player on their own island, begin with the navigable poles on the map
recipe above. This is recommended from a realism perspective. If you do not want poles, you can skip it. Set
``generator`` to ``ISLAND`` or ``FAIR`` (``ISLAND`` is recommended) and then set ``startpos "SINGLE"``.
Finally set ``landmass`` to 15 or 20 (minimum is 15). This will allow for some small random non-populated
islands on the map.

This recipe introduces a great use of the ``tilesperplayer`` option. Each player will get a similar sized
island of the number of tiles you define. Do not forget to change the ``mapsize`` option to ``PLAYER`` if you
go this route.

By playing around with the varying values, you can create many different kinds of maps. Let us move to
discussing team games and team placement.

Team games are a special use case. Most games are what the Longturn community calls Free For All (FFA). The
idea of an FFA game is there are no preset alliances at game start. Those form in game. Team games are the
opposite of FFA games. Alliances (e.g. teams) are defined before the game starts. Getting player placement
correct when teams are involved is quite important!

The ``teamplacement`` option has the following configurations:

* ``DISABLED`` -- If set, then the option configuration is ignored.
* ``CLOSEST`` -- The default. The name implies what happens.
* ``CONTINENT`` -- Everyone on the same continent. This requires tuning ``landmass``, ``generator``, and
  ``startpos`` to fit how you want the teams to get placed on the same continent.
* ``HORIZONTAL`` -- Place team players in a East-West alignment.
* ``VERTICAL`` -- Place team players in a North-South alignment.

:strong:`Recipe: Two team game with each team on their own continent`

Follow the :doc:`steps <players>` to create a :file:`players.serv` file.

Add a ``read players.serv`` entry to your :doc:`/Manuals/Server/settings-file`.

Set ``generator "FRACTURE"``, ``landmass`` to 30 or 40, ``mapsize "PLAYER"``, ``tilesperplayer`` to something
between 1 and 1000 (Longturn uses 500), ``teamplacement "CONTINENT"``, and ``startpos "ALL"``.

You can add poles to the map with the recipe above.

.. note:: Do not be surprised if you need to play around with some of the other settings to get the map you
  are looking for. Every Longturn game goes through a process of generating test maps for players to evaluate
  and vote for. Play around with the settings and you will get the map you eventually want!
