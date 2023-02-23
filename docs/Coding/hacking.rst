..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022-2023 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 Pranav Sampathkumar <pranav.sampathkumar@gmail.com>
    SPDX-FileCopyrightText: 2022 NIKEA-SOFT
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Freeciv21 Hacker's Guide
************************

This guide is intended to be a help for developers, wanting to mess with Freeciv21 programming.


The Server
==========

Server Autogame Testing
-----------------------

Code changes should always be tested before submission for inclusion into the GitHub source tree. It is
useful to run the client and server as autogames to verify either a particular savegame no longer shows a
fixed bug, or as a random sequence of games in a while loop overnight.

To start a server game with all AI players, create a file (below named :file:`civ.serv`) with lines such as
the following:

.. code-block:: sh

    # set gameseed 42       # repeat a particular game (random) sequence
    # set mapseed 42        # repeat a particular map generation sequence
    # set timeout 3         # run a client/server autogame
    set timeout -1          # run a server only autogame
    set minplayers 0        # no human player needed
    set ec_turns 0          # avoid timestamps in savegames
    set aifill 7            # fill to 7 players
    hard                    # make the AI do complex things
    create Caesar           # first player (with known name) created and
                            # toggled to AI mode
    start                   # start game

The commandline to run server-only games can be typed as variations of:

.. code-block:: sh

    $ while( time build/freeciv21-server -r civ.serv ); do date; done
    ---  or  ---
    $ build/freeciv21-server -r civ.serv -f buggy1534.sav.gz


To attach one or more clients to an autogame, remove the :code:`start` command, start the server program and
attach clients to created AI players. Or type :code:`aitoggle <player>` at the server command prompt for each
player that connects. Finally, type :code:`start` when you are ready to watch the show.

.. note::
    The server will eventually flood a client with updates faster than they can be drawn to the screen,
    thus it should always be throttled by setting a timeout value high enough to allow processing of the large
    update loads near the end of the game.


If you plan to compare results of autogames the following changes can be helpful:

* :code:`define __FC_LINE__` to a constant value in :file:`./utility/log.h`.
* deactivation of the event cache (:code:`set ec_turns 0`).


Old Lists
=========

For variable length list of units and cities Freeciv21 uses a :code:`genlist`, which is implemented in
:file:`utility/genlist.cpp`. By some macro magic type specific macros have been defined, creating a lot of
trouble for C++ programmers. These macro-based lists are being phased out in favor of STL containers; in the
meantime, we preserve here an explanation of how to use them.

For example a ``tile`` struct (the pointer to it we call :code:`ptile`) has a ``unit`` list,
:code:`ptile->units`; to iterate though all the units on the tile you would do the following:

.. code-block:: cpp

    unit_list_iterate(ptile->units, punit) {
      // In here we could do something with punit, which is a pointer to a
      // unit struct
    } unit_list_iterate_end;

Note that the macro itself declares the variable :code:`punit`. Similarly there is a

.. code-block:: cpp

    city_list_iterate(pplayer->cities, pcity) {
      // Do something with pcity, the pointer to a city struct
    } city_list_iterate_end;


There are other operations than iterating that can be performed on a list; inserting, deleting, or sorting
etc. See :file:`utility/speclist.h`. Note that the way the :code:`*_list_iterate macro` is implemented means
you can use "continue" and "break" in the usual manner.

One thing you should keep in the back of your mind. Say you are iterating through a unit list, and then
somewhere inside the iteration decide to disband a unit. In the server you would do this by calling
:code:`wipe_unit(punit)`, which would then remove the unit node from all the relevant unit lists. However, by
the way :code:`unit_list_iterate` works, if the removed unit was the following node :code:`unit_list_iterate`
will already have saved the pointer, and use it in a moment, with a segfault as the result. To avoid this, use
:code:`unit_list_iterate_safe` instead.

Graphics
========

Currently the graphics is stored in the PNG file format.

If you alter the graphics, then make sure that the background remains transparent. Failing to do this means
the mask-pixmaps will not be generated properly, which will certainly not give any good results.

Each terrain tile is drawn in 16 versions, all the combinations with a green border in one of the main
directions. Hills, Mountains, Forests, and Rivers are treated in special cases.

Isometric tilesets are drawn in a similar way to how civ2 draws (that is why civ2 graphics are compatible). For
each base terrain type there exists one tile sprite for that terrain. The tile is blended with nearby tiles to
get a nice-looking boundary. This is erroneously called "dither" in the code.

Non-isometric tilesets draw the tiles in the "original" Freeciv21 way, which is both harder and less pretty.
There are multiple copies of each tile, so that a different copy can be drawn depending on the terrain type of
the adjacent tiles. It may eventually be worthwhile to convert this to the civ2 system or another one
altogether.

Map Structure
=============

The map is maintained in a pretty straightforward C array, containing :math:`X\times Y` tiles. You can use the
function :code:`struct tile *map_pos_to_tile(x, y)` to find a pointer to a specific tile. A tile has various
fields; see the struct in :file:`common/map.h`.

You may iterate tiles, you may use the following methods:

.. code-block:: cpp

    whole_map_iterate(tile_itr) {
      // do something
    } whole_map_iterate_end;


for iterating all tiles of the map;

.. code-block:: cpp

    adjc_iterate(center_tile, tile_itr) {
      // do something
    } adjc_iterate_end;


for iterating all tiles close to ``center_tile``, in all *valid* directions for the current topology (see
below);

.. code-block:: cpp

    cardinal_adjc_iterate(center_tile, tile_itr) {
      // do something
    } cardinal_adjc_iterate_end;


for iterating all tiles close to ``center_tile``, in all *cardinal* directions for the current topology (see
below);

.. code-block:: cpp

    square_iterate(center_tile, radius, tile_itr) {
      // do something
    } square_iterate_end;


for iterating all tiles in the radius defined ``radius`` (in real distance, see below), beginning by
``center_tile``;

.. code-block:: cpp

    circle_iterate(center_tile, radius, tile_itr) {
      // do something
    } square_iterate_end;


for iterating all tiles in the radius defined ``radius`` (in square distance, see below), beginning by
``center_tile``;

.. code-block:: cpp

    iterate_outward(center_tile, real_dist, tile_itr) {
      // do something
    } iterate_outward_end;


for iterating all tiles in the radius defined ``radius`` (in real distance, see below), beginning by
``center_tile``. Actually, this is the duplicate of square_iterate, or various tricky ones defined in
:file:`common/map.h`, which automatically adjust the tile values. The defined macros should be used whenever
possible, the examples above were only included to give people the knowledge of how things work.

Note that the following:

.. code-block:: cpp

    for (x1 = x-1; x1 <= x+1; x1++) {
      for (y1 = y-1; y1 <= y+1; y1++) {
        // do something
      }
    }


is not a reliable way to iterate all adjacent tiles for all topologies, so such operations should be avoided.


Also available are the functions calculating distance between tiles. In Freeciv21, we are using 3 types of
distance between tiles:

* The :code:`map_distance(ptile0, ptile1)` function returns the *Manhattan* distance between tiles, i.e. the
  distance from :code:`ptile0` to :code:`ptile1`, only using cardinal directions. For example,
  :math:`|dx| + |dy|` for non-hexagonal topologies.

* The :code:`real_map_distance(ptile0, ptile1)` function returns the *normal* distance between tiles, i.e. the
  minimal distance from :code:`ptile0` to :code:`ptile1` using all valid directions for the current topology.

* The :code:`sq_map_distance(ptile0, ptile1)` function returns the *square* distance between tiles. This is a
  simple way to make Pythagorean effects for making circles on the map for example. For non-hexagonal
  topologies, it would be :math:`dx^2 + dy^2`. Only useless square root is missing.


Different Types of Map Topology
-------------------------------

Originally Freeciv21 supports only a simple rectangular map. For instance a 5x3 map would be conceptualized as

.. code-block:: rst

    <- XXXXX ->
    <- XXXXX ->
    <- XXXXX ->


and it looks just like that under "overhead" (non-isometric) view. The arrows represent an east-west
wrapping. But under an isometric-view client, the same map will look like:

.. code-block:: rst

    <-   X     ->
    <-  X X    ->
    <- X X X   ->
    <-  X X X  ->
    <-   X X X ->
    <-    X X  ->
    <-     X   ->


where "north" is to the upper-right and "south" to the lower-left. This makes for a mediocre interface.

An isometric-view client will behave better with an isometric map. This is what Civ2, SMAC, Civ3, etc. all
use. A rectangular isometric map can be conceptualized as

.. code-block:: rst

   <- X X X X X  ->
   <-  X X X X X ->
   <- X X X X X  ->
   <-  X X X X X ->


North is up and it will look just like that under an isometric-view client. Of course under an overhead-view
client it will again turn out badly.

Both types of maps can easily wrap in either direction: north-south or east-west. Thus there are four types
of wrapping: flat-earth, vertical cylinder, horizontal cylinder, and torus. Traditionally Freeciv21 only wraps
in the east-west direction.


Topology, Cardinal Directions and Valid Directions
--------------------------------------------------

A *cardinal* direction connects tiles per a *side*. Another *valid* direction connects tiles per a *corner*.

In non-hexagonal topologies, there are 4 cardinal directions, and 4 other valid directions. In hexagonal
topologies, there are 6 cardinal directions, which matches exactly the 6 valid directions.

Note that with isometric view, the direction named "North" (``DIR8_NORTH``) is actually not from the top to
the bottom of the screen view. All directions are turned a step on the left (e.g. :math:`\pi \div 4` rotation
with square tiles and :math:`\pi \div 3` rotation for hexagonal tiles).


Different Coordinate Systems
----------------------------

In Freeciv21, we have the general concept of a "position" or "tile". A tile can be referred to in any of
several coordinate systems. The distinction becomes important when we start to use non-standard maps (see
above).

Here is a diagram of coordinate conversions for a classical map.

.. code-block:: rst

      map        natural      native       index

      ABCD        ABCD         ABCD
      EFGH  <=>   EFGH     <=> EFGH   <=> ABCDEFGHIJKL
      IJKL        IJKL         IJKL


Here is a diagram of coordinate conversions for an iso-map.

.. code-block:: rst

      map          natural     native       index

        CF        A B C         ABC
       BEIL  <=>   D E F   <=>  DEF   <=> ABCDEFGHIJKL
      ADHK        G H I         GJI
       GJ          J K L        JKL


Below each of the coordinate systems are explained in more detail. Note that hexagonal topologies are always
considered as isometric.

Map (or "Standard") Coordinates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All of the code examples above are in map coordinates. These preserve the local geometry of square tiles,
but do not represent the global map geometry well. In map coordinates, you are guaranteed, so long as we use
square tiles, that the tile adjacency rules

.. code-block:: rst

    |  (map_x-1, map_y-1)    (map_x, map_y-1)   (map_x+1, map_y-1)
    |  (map_x-1, map_y)      (map_x, map_y)     (map_x+1, map_y)
    |  (map_x-1, map_y+1)    (map_x, map_y+1)   (map_x+1, map_y+1)


are preserved, regardless of what the underlying map or drawing code looks like. This is the definition of
the system.

With an isometric view, this looks like:

.. code-block:: rst

    |                           (map_x-1, map_y-1)
    |              (map_x-1, map_y)            (map_x, map_y-1)
    | (map_x-1, map_y+1)          (map_x, map_y)              (map_x+1, map_y-1)
    |             (map_x, map_y+1)            (map_x+1, map_y)
    |                           (map_x+1, map_y+1)


Map coordinates are easiest for local operations (e.g. 'square_iterate' and friends, translations, rotations,
and any other scalar operation) but unwieldy for global operations.

When performing operations in map coordinates (like a translation of tile :code:`(x, y)` by :code:`(dx, dy)`
-> :code:`(x + dx, y + dy)`), the new map coordinates may be unsuitable for the current map. In case, you
should use one of the following functions or macros:

* :code:`map_pos_to_tile()`: return ``NULL`` if normalization is not possible;

* :code:`normalize_map_pos()`: return ``TRUE`` if normalization have been done (wrapping X and Y coordinates
  if the current topology allows it);

* :code:`is_normal_map_pos()`: return ``TRUE`` if the map coordinates exist for the map;

* :code:`CHECK_MAP_POS()`: assert whether the map coordinates exist for the map.

Map coordinates are quite central in the coordinate system, and they may be easily converted to any other
coordinates: :code:`MAP_TO_NATURAL_POS()`, :code:`MAP_TO_NATIVE_POS()`, or :code:`map_pos_to_index()`
functions.

Natural Coordinates
^^^^^^^^^^^^^^^^^^^

Natural coordinates preserve the geometry of map coordinates, but also have the rectangular property of
native coordinates. They are unwieldy for most operations because of their sparseness. They may not have
the same scale as map coordinates and, in the iso case, there are gaps in the natural representation of a
map.

With classical view, this looks like:

.. code-block:: rst

      (nat_x-1, nat_y-1)    (nat_x, nat_y-1)   (nat_x+1, nat_y-1)
      (nat_x-1, nat_y)      (nat_x, nat_y)     (nat_x+1, nat_y)
      (nat_x-1, nat_y+1)    (nat_x, nat_y+1)   (nat_x+1, nat_y+1)


With an isometric view, this looks like:

.. code-block:: rst

    |                            (nat_x, nat_y-2)
    |             (nat_x-1, nat_y-1)          (nat_x+1, nat_y-1)
    | (nat_x-2, nat_y)            (nat_x, nat_y)              (nat_x+2, nat_y)
    |             (nat_x-1, nat_y+1)          (nat_x+1, nat_y+1)
    |                            (nat_x, nat_y+2)


Natural coordinates are mostly used for operations which concern the user view. It is the best way to
determine the horizontal and the vertical axis of the view.

The only coordinates conversion is done using the :code:`NATURAL_TO_MAP_POS()` function.

Native Coordinates
^^^^^^^^^^^^^^^^^^

With classical view, this looks like:

.. code-block:: rst

      (nat_x-1, nat_y-1)    (nat_x, nat_y-1)   (nat_x+1, nat_y-1)
      (nat_x-1, nat_y)      (nat_x, nat_y)     (nat_x+1, nat_y)
      (nat_x-1, nat_y+1)    (nat_x, nat_y+1)   (nat_x+1, nat_y+1)


With an isometric view, this looks like:

.. code-block:: rst

    |                            (nat_x, nat_y-2)
    |            (nat_x-1, nat_y-1)          (nat_x, nat_y-1)
    | (nat_x-1, nat_y)            (nat_x, nat_y)            (nat_x+1, nat_y)
    |           (nat_x-1, nat_y+1)          (nat_x, nat_y+1)
    |                            (nat_x, nat_y+2)


Neither is particularly good for a global map operation such as map wrapping or conversions to or from map
indexes. Something better is needed.

Native coordinates compress the map into a continuous rectangle. The dimensions are defined as
:code:`map.xsize x map.ysize`. For instance, the above iso-rectangular map is represented in native
coordinates by compressing the natural representation in the X axis to get the 3x3 iso-rectangle of

.. code-block:: rst

    ABC       (0,0) (1,0) (2,0)
    DEF  <=>  (0,1) (1,1) (2,1)
    GHI       (0,2) (1,2) (3,2)


The resulting coordinate system is much easier to use than map coordinates for some operations. These
include most internal topology operations (e.g., :code:`normalize_map_pos`, or :code:`whole_map_iterate`) as
well as storage (in ``map.tiles`` and savegames, for instance).

In general, native coordinates can be defined based on this property; the basic map becomes a continuous
(gap-free) cardinally-oriented rectangle when expressed in native coordinates.

Native coordinates can be easily converted to map coordinates using the :code:`NATIVE_TO_MAP_POS()` function,
to index using the code:`native_pos_to_index()` function and to tile (shortcut) using the
:code:`native_pos_to_tile()` function.

After operations, such as the :code:`FC_WRAP(x, map.xsize)` function, the result may be checked with the
:code:`CHECK_NATIVE_POS()` function.

Index Coordinates
^^^^^^^^^^^^^^^^^

Index coordinates simply reorder the map into a continuous (filled-in) one-dimensional system. This
coordinate system is closely tied to the ordering of the tiles in native coordinates, and is slightly
easier to use for some operations (like storage) because it is one-dimensional. In general you cannot assume
anything about the ordering of the positions within the system.

Indexes can be easily converted to native coordinates using the :code:`index_to_native_pos()` function or to
map positions (shortcut) using the :code:`index_to_map_pos()` function.

A map index can tested using the :code:`CHECK_INDEX` macro.

With a classical rectangular map, the first three coordinate systems are equivalent. When we introduce
isometric maps, the distinction becomes important, as demonstrated above. Many places in the code have
introduced :code:`map_x/map_y` or :code:`nat_x/nat_y` to help distinguish whether map or native coordinates
are being used. Other places are not yet rigorous in keeping them apart, and will often just name their
variables :code:`x` and :code:`y`. The latter can usually be assumed to be map coordinates.

Note that if you don't need to do some abstract geometry exploit, you will mostly use tile pointers, and give
to map tools the ability to perform what you want.

Note that :code:`map.xsize` and :code:`map.ysize` define the dimension of the map in :code:`_native_`
coordinates.

Of course, if a future topology does not fit these rules for coordinate systems, they will have to be refined.

Native Coordinates on an Isometric Map
--------------------------------------

An isometric map is defined by the operation that converts between map (user) coordinates and native
(internal) ones. In native coordinates, an isometric map behaves exactly the same way as a standard one. See
`Native Coordinates`_, above.

Converting from map to native coordinates involves a :math:`\pi \div 2` rotation (which scales in each
dimension by :math:`\sqrt{2}`) followed by a compression in the :code:`X` direction by a factor of 2. Then a
translation is required since the "normal set" of native coordinates is defined as :math:`(x, y)` where
:math:`\{x \mid 0\leq x < \texttt{map.xsize}\}` and :math:`\{y \mid 0\leq y < \texttt{map.ysize}\}` while the
normal set of map coordinates must satisfy :math:`x \geq 0` and :math:`y \geq 0`.

Converting from native to map coordinates (a less cumbersome operation) is the opposite.

.. code-block:: rst

    |                                       EJ
    |          ABCDE     A B C D E         DIO
    | (native) FGHIJ <=>  F G H I J <=>   CHN  (map)
    |          KLMNO     K L M N O       BGM
    |                                   AFL
    |                                    K

Note that:

.. code-block:: cpp

  native_to_map_pos(0, 0) == (0, map.xsize-1)
  native_to_map_pos(map.xsize-1, 0) == (map.xsize-1, 0)
  native_to_map_pos(x, y+2) = native_to_map_pos(x,y) + (1,1)
  native_to_map_pos(x+1, y) = native_to_map_pos(x,y) + (1,-1)


The math then works out to:

.. math::
  x_\texttt{map} &= \left\lceil \dfrac{y_\texttt{nat}}{2} \right\rceil + x_\texttt{nat} \\
  y_\texttt{map} &= \left\lfloor \dfrac{y_\texttt{nat}}{2} \right\rfloor - x_\texttt{nat} + x_\texttt{size} - 1

.. math::
  y_\texttt{nat} &= x_\texttt{map} + y_\texttt{map} - x_\texttt{size} \\
  x_\texttt{nat} &= \left\lfloor x_\texttt{map} - y_\texttt{map} + \dfrac{x_\texttt{size}}{2} \right\rfloor

which leads to the macros :code:`NATIVE_TO_MAP_POS()`, and :code:`MAP_TO_NATIVE_POS()` that are defined in
:file:`map.h`.

Unknown Tiles and Fog of War
----------------------------

In :file:`common/player.h`, there are several fields:

.. code-block:: cpp

    struct player {
      ...
      struct dbv tile_known;

      union {
        struct {
          ...
        } server;

    struct {
        struct dbv tile_vision[V_COUNT];
        } client;
      };
    };


While :code:`tile_get_known()` returns:

.. code-block:: cpp

    // network, order dependent
    enum known_type {
    TILE_UNKNOWN = 0,
    TILE_KNOWN_UNSEEN = 1,
    TILE_KNOWN_SEEN = 2,
    };


The values :code:`TILE_UNKNOWN` and :code:`TILE_KNOWN_SEEN` are straightforward. :code:`TILE_KNOWN_UNSEEN` is
a tile of which the user knows the terrain, but not recent cities, roads, etc.

:code:`TILE_UNKNOWN` tiles never are (nor should be) sent to the client. In the past, :code:`UNKNOWN` tiles that
were adjacent to :code:`UNSEEN` or :code:`SEEN` were sent to make the drawing process easier, but this has now
been removed. This means exploring new land may sometimes change the appearance of existing land (but this is
not fundamentally different from what might happen when you transform land). Sending the extra info, however,
not only confused the goto code but allowed cheating.

Fog of War is the fact that even when you have seen a tile once you are not sent updates unless it is inside
the sight range of one of your units or cities.

We keep track of Fog of War by counting the number of units and cities of each client that can see the tile.
This requires a number per player, per tile, so each :code:`player_tile` has a :code:`short[]`. Every time a
unit, city, or somthing else can observe a tile 1 is added to its player's number at the tile, and when it
cannot observe any more (killed/moved/pillaged) 1 is subtracted. In addition to the initialization/loading of
a game this array is manipulated with the :code:`void unfog_area(struct player *pplayer, int x, int y, int
len)` and :code:`void fog_area(struct player *pplayer, int x, int y, int len)` functions. The :code:`int len`
variable is the radius of the area that should be fogged/unfogged, i.e. a ``len`` of 1 is a normal unit. In
addition to keeping track of Fog of War, these functions also make sure to reveal :code:`TILE_UNKNOWN` tiles
you get near, and send information about :code:`TILE_UNKNOWN` tiles near that the client needs for drawing.
They then send the tiles to the :code:`void send_tile_info(struct player *dest, int x, int y)` function, which
then sets the correct ``known_type`` and sends the tile to the client.

If you want to just show the terrain and cities of the square the function :code:`show_area()` does this. The
tiles remain fogged. If you play without Fog of War all the values of the seen arrays are initialized to 1. So
you are using the exact same code, you just never get down to 0. As changes in the "fogginess" of the tiles
are only sent to the client when the value shifts between zero and non-zero, no redundant packages are sent.
You can even switch Fog of War on or off in game just by adding or subtracting 1 to all the tiles.

We only send city and terrain updates to the players who can see the tile. So a city, or improvement, can
exist in a square that is known and fogged and not be shown on the map. Likewise, you can see a city in a
fogged square even if the city does not exist. It will be removed when you see the tile again. This is done by
1) only sending info to players who can see a tile and 2) keeping track of what info has been sent so the game
can be saved. For the purpose of 2), each player has a map on the server (consisting of ``player_tile`` and
``dumb_city`` fields) where the relevant information is kept.

The case where a player ``p1`` gives map info to another player ``p2`` requires some extra information.
Imagine a tile that neither player sees, but which ``p1`` has the most recent information on. In that case the
age of the players' information should be compared, which is why the player tile has a ``last_updated`` field.
This field is not kept up to date as long as the player can see the tile and it is unfogged, but when the tile
gets fogged the date is updated.

There is a Shared Vision feature, meaning that if ``p1`` gives Shared Vision to ``p2``, every time a function
like :code:`show_area()`, :code:`fog_area()`, :code:`unfog_area()`, or
:code:`give_tile_info_from_player_to_player()` is called on ``p1``, ``p2`` also gets the information. Note
that if ``p2`` gives Shared Vision to ``p3``, ``p3`` also gets the informtion for ``p1``. This is controlled
by ``p1's`` really_gives_vision bitvector, where the dependencies will be kept.

National Borders
----------------

For the display of national Borders (similar to those used in Sid Meier's Alpha Centauri) each map tile also
has an ``owner`` field, to identify which nation lays claim to it. If :code:`game.borders` is non-zero, each
city claims a circle of tiles :code:`game.borders` in Vision Radius. In the case of neighbouring enemy cities,
tiles are divided equally, with the older city winning any ties. Cities claim all immediately adjacent tiles,
plus any other tiles within the border radius on the same continent. Land cities also claim ocean tiles if
they are surrounded by 5 land tiles on the same continent. This is a crude detection of inland seas or Lakes,
which should be improved upon.

tile ownership is decided only by the server, and sent to the clients, which draw border lines between tiles
of differing ownership. Owner information is sent for all tiles that are known by a client, whether or not
they are fogged.
