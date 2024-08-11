.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Erik Sigra <sigra@home.se>
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Map Generator
*************

This page describes how maps are generated in Freeciv21, from a technical
perspective. There are several generators the user can choose from
(``generator`` server setting), but they share a lot of code. The map generator
code is located under ``server/generator``. The entry point is
``map_fractal_generate`` in ``mapgen.cpp``.

.. note::
  See :doc:`/Manuals/Advanced/map-generator` for a less technical introduction.
  This page expects the reader to already be familiar with the settings
  governing map creation.

.. note::
  Code references are as of August 2024.

Map generation is performed in multiple passes. After some initialization, the
first step, shared between all generators, is the creation of a basic
temperature map. Since no terrain information exists at this stage, the
temperature is simply a measure of the distance to the pole (the "colatitude").
Unless ``alltemperate`` is set, the world is then divided in four regions:
frozen, cold, temperate, and tropical (``create_tmap()``).

After the temperature is set, the actual map generation starts. This depends on
the map generator chosen by the user, with various fallbacks in place. The
details of each generator are described below.
After this step, terrain is set everywhere and rivers have been generated.
Depending on the map generator, players and resources
may already have been placed (this is for instance the case with the "fair
islands" generator).

At this stage, tiny (1x1) islands are removed if disabled in the settings
(``tinyislands`` parameter, ``remove_tiny_islands()``). Water is also fine-tuned
to always have shallow ocean next to the coast and be generally smooth
(``smooth_water_depth()``). Continent numbers are assigned and small seas are
turned to lakes (``assign_continent_numbers()`` and ``regenerate_lakes()``). The
temperature map is reset after this is done.

If not done earlier, resources are then added to the map with at least one tile
between them. For each tile that gets a resource, one is picked at random from
the list of allowed resources for the terrain (``add_resources()``). Huts are
added in a similar way but with a minimum distance of 3 tiles (``make_huts()``).
The final step is to distribute players, as described in
:ref:`Player Placement <mapgen-placement>` below.

.. todo::
  This page is missing information about the Fair Islands generator. Please feel
  free to contribute!

.. _mapgen-fractions:

Terrain Fractions
-----------------

The map generators need terrain information to know what to generate. However,
the server settings control the world temperature and wetness. A conversion
is needed between them.

The generators know four types of terrain: mountains (including hills),
forests (including jungles), deserts, and swamps. Ice, tundra, plains, and
grassland are always used to fill in the gap between those, depending on the
local temperature. The fraction of tiles that will have rivers is handled in a
similar way and is thus also described in this section.

The equations used to derive the fraction of each terrain are highly arbitrary
and have likely been determined through trial and error. Since the details are
not particularly enlightening, only the general ideas are discussed below. The
interested reader can read the function ``adjust_terrain_param()`` in
``mapgen.cpp``.

The first fraction to be computed is the amount of polar terrain, decided based
on the average temperature and the size of the map (larger maps get less). Since
poles count towards the total landmass but have quite boring terrain, the
fractions of all other terrains on the rest of the land is increased to
compensate.

The fraction of mountains is directly proportional to the ``steepness``
parameter. It is about 20% for the default ``steepness`` of 30, and increases
to 60% for a ``steepness`` of 100. The fractions of all other terrain types are
reduced when there are lots of mountains, leaving room for plains and grassland.

The ``wetness`` parameter controls directly the amount of forest, which ranges
between 5% and 35%. An amount of jungle is taken away from this based on the
fraction of the map covered by hot temperatures, but is not used consistently by
map generators. ``wetness`` also sets the amount of rivers, ranging from 3% to
11% of all land including mountains.

The most complicated formulae are for swamp and desert, as they combine
``wetness`` and ``temperature``. Swamps do not appear at all when the world is
dry and cold, but their fraction increases with both ``wetness`` and
``temperature`` --- slightly faster with the former. Deserts may also not appear
if the world is very cold and wet. Their fraction increases rather quickly with
``temperature`` and is mildly reduced by ``wetness``.

.. _mapgen-height-generators:

Height-Based Generators
-----------------------

Three of the available algorithms start by building a height map for the whole
world: :ref:`mapgen-random` (``RANDOM``), :ref:`mapgen-fractal` (``FRACTAL``),
and :ref:`Fracture Map <mapgen-fracture>` (``FRACTURE``).
This height map is then used to assign tiles to continents or seas, and to
distribute terrain on the map. The algorithm used for this is shared between the
generators and is described in :ref:`mapgen-terrain-assignment`.

.. _mapgen-random:

Fully Random Height
^^^^^^^^^^^^^^^^^^^

Entry point: ``make_random_hmap()``.

This generator is extremely simple: it builds a completely random height map and
smoothes it out.
Terrain is then assigned to tiles based on their height and temperature as
described in the :ref:`terrain assignment <mapgen-terrain-assignment>` section
below, and :ref:`rivers are added on top <mapgen-height-rivers>`.

.. _mapgen-fractal:

Pseudo-Fractal Height
^^^^^^^^^^^^^^^^^^^^^

Entry point: ``make_pseudofractal1_hmap()``.

This generator works by dividing the map in blocks (five plus the number of
player islands to be created) and assigning a random height to their
corners. Each block is then processed recursively, cutting it equally in four
blocks. The height at the corners of the smaller blocks are computed by
averaging the heights at the corners of the large blocks and adding a decreasing
amount of noise. This process is repeated until blocks are one tile wide or
less.

At the end of the generation, more random noise is added on top of the generated
height map to give more variety on large maps, with warmer tiles generally
getting more noise. The generated height map is then used to assign terrain and
generate rivers as described in :ref:`mapgen-terrain-assignment` and
:ref:`mapgen-height-rivers`.

.. _mapgen-fracture:

Fracture Map
^^^^^^^^^^^^

Entry point: ``make_fracture_map()``.

The ``FRACTURE`` generator starts from points distributed randomly on the map
and grows them until they meet their neighbors. Each point is given a random
height, which is shared by all connected tiles. After adding some randomness
on top, the lowest areas are flooded and only the highest ones remain as
islands. Interesting structures with many small islands are often created in one
or two areas generated right next to the sea level.

Hills and mountains are added at the boundaries between areas in a crude mimic
of plate tectonics. This results in large mountain ranges inland and hills all
along the coasts.

The initial points used by the algorithm are distributed randomly on the map,
except that submersed points are added all along the poles and map boundaries to
prevent the land from hitting them. The number of points depends on the map
size.

The height map created from the fracture points is passed to the normal
terrain-making algorithm, except that hills and mountains are generated
differently. Their location with Fracture is not based on the absolute height of
the tiles, which would result in the highest areas being completely filled with
mountains, but instead on the local elevation --- that is, the difference in
elevation between a tile and its neighbors. The threshold for placing hills or
mountains is commonly reached at area boundaries, and mountain ranges appear
there. Mountains are explicitly removed directly along the coast as this would
result in unplayable maps.

In addition to the above, hills and mountains are added stochastically in
regions that would otherwise be too flat. To take the the ``steepness`` setting
into account, hills and mountains are placed randomly everywhere on the map
until they cover ``steepness`` percents of the total land area.

After hills and mountains have been added, the rest of the terrain selection
proceeds :ref:`as with other generators <mapgen-terrain-assignment>`. The
general flatness of the areas created by ``FRACTURE`` may result in large
patches of similar terrain.

.. _mapgen-terrain-assignment:

Terrain Assignment
^^^^^^^^^^^^^^^^^^

Generators that use a height map to generate the map share a common routine to
assign terrain to the generated tiles, whose algorithm is described in this
section. The code is in the ``make_land()`` function.

The first input is a "normalized" height map where the tile heights
range from 0 to 1000 and are spread uniformly in this range. This allows for
fast queries of the type "the lowest 20% of the world". The second input of the
algorithm is a temperature map, normalized to have only four types of tiles:
frozen, cold, temperate, and tropical.

The very first step taken by the algorithm is to reduce the height of terrain
near the polar strips, if any. This prevents generating land next to them and
disconnects land from the poles. (``normalize_hmap_poles()``)

Oceans and poles are generated next. Sea level is determined as the percentage
of sea tiles, 100 minus the ``landmass`` server setting. Any tile with a low
enough height is considered an ocean candidate. It is raised slightly if it is
next to a land tile and lowered otherwise. A suitable ocean terrain for is
chosen for the local depth and temperature: all sea tiles in the frozen region
become ice as well as 30% of the tiles in the cold region that are directly
adjacent to the frozen region. This gives the poles their slightly irregular
shapes. At this stage, all tiles above sea level are filled with a dummy land
terrain.

Having generated the "sea" poles, the lowering is reverted to allow for cold
land terrain in the area. (``renormalize_hmap_poles()``)

The temperature map is recomputed after creating oceans. In addition to the
distance from the poles, it now takes other factors into account. High ground is
made up to 30% colder and the temperature near oceans is made up to 15% less
extreme (in both the highs and lows). After this step, the map is again
simplified to four groups: frozen, cold, temperate, and tropical.

In rulesets without frozen oceans, it may happen that the poles have still not
been generated. They are marked back as land tiles by setting them to the
"unknown" terrain. (``make_polar_land()``)

The next step is to place relief, i.e. hills and mountains. This is again based
on the height map: the highest land tiles become hills or mountains. The exact
fraction of land tiles that will become a hill or mountain is governed by the
``steepness`` server setting. Large chunks of steep terrain are avoided by
randomly converting only half of the tiles and not converting tiles that are
significantly higher than one their neighbors. In addition to the above, steep
terrain is added in places that would otherwise be too flat. (``make_relief()``)

.. note::
  The ``FRACTURE`` generator uses a different logic for placing hills. The
  code is in ``make_fracture_relief()``.

Once it is decided that a tile will be steep, it is set to hilly terrain if the
tile is in the region of hot temperature, and mountains otherwise. About 70% of
the tiles in the hot region are picked with the ``green`` flag, while 70% of
the tiles at other temperatures avoid it.

The last step to generate the terrain is to fill in the gaps between the ocean
and the hills. This is done according to terrain fractions that depend on the
global ``wetness`` and ``temperature`` settings. Terrain is generated in
patches, according to properties defined in ``terrain.ruleset`` and conditions
on the tile. The following combinations are generated one at a time:

.. table:: Terrain produced by the generator and their matching to tiles
  :widths: auto
  :align: center

  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  |        | Terrain properties               | Tile properties                 |           |
  +        +-----------+-----------+----------+---------+-------------+---------+           +
  | Label  | Required  | Preferred | Avoided  | Wetness | Temp.       | Height  | Thr.      |
  +========+===========+===========+==========+=========+=============+=========+===========+
  | Forest | Foliage   | Temperate | Tropical | All     | Not frozen  | \-      | 60        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Jungle | Foliage   | Tropical  | Cold     | All     | Tropical    | \-      | 50        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Swamp  | Wet       | \-        | Foliage  | Not dry | Hot         | Low     | 50        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Desert | Dry       | Tropical  | Cold     | Dry     | Not frozen  | Not low | 80        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Desert | Dry       | Tropical  | Wet      | All     | Not frozen  | Not low | 40        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Ice    | Frozen    | \-        | Mountain | \-      | \-          | \-      | \-        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Tundra | Cold      | \-        | Mountain | \-      | \-          | \-      | \-        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+
  | Plains | Temperate | Green     | Mountain | \-      | \-          | \-      | \-        |
  +--------+-----------+-----------+----------+---------+-------------+---------+-----------+

Terrain patches expand outwards from a seed tile until the required tile
properties are no longer met or a threshold (*Thr.* in the table) in colatitude
and height difference is reached. Ice, tundra, and plains/grassland are
generated to fill in gaps and do not expand in patches. (``make_terrains()``)

The algorithm to match the desired terrain properties to the ruleset-defined
terrain types by first collecting all terrains with the required property. Then,
types without at least some of the "preferred" property and types with a
non-zero "avoided" property are removed from the set. Of the remaining terrains,
one is picked at random, with a higher chance to be selected when the required
property has a high value in the ruleset. If this search fails, it is resumed
without the "preferred" property. If this fails again, the "avoided" property is
also dropped. (``pick_terrain()``)

.. _mapgen-height-rivers:

Rivers
^^^^^^

Rivers are generated by flowing them from springs chosen randomly on the map.
Springs may not be frozen or low in the height map. They may not be ocean and
there may not be a river in the area yet. The algorithm also tries to avoid
springs in locations with many hills and mountains nearby, or in ice and
deserts (according to the corresponding terrain properties: ``mountainous``,
``frozen``, ``dry``). The entry point to generate rivers is the
``make_rivers()`` function.

Once a spring is found, the river is flown from there one tile at a time.
To decide which direction the river takes, the possible directions are tested in
a series of tests until there is only one direction left. Some tests are fatal.
This means that they can sort away all remaining directions. If they do so, the
river is aborted. Here follows a description of the test series:

Falling into itself: fatal
  This is tested by keeping track of all tiles already used or evaluated while
  creating the river. If a river comes close to one of these tiles, it is
  falling back to itself and is aborted.

Forming a 4-river-grid: fatal
  A river may not form a grid with four rivers tiles next to each other.

Highlands:
  Rivers must not flow up in mountains or hills if there are alternatives.

Adjacent ocean:
  Rivers must flow down to coastal areas when possible.

Adjacent river:
  Rivers must flow down to areas near other rivers when possible:

Adjacent highlands:
  Rivers must not flow towards highlands (terrains with a non-zero
  ``mountainous`` property) if there are alternatives.

Swamps:
  Rivers must flow down in swamps when possible.

Adjacent swamps:
  Rivers must flow towards swamps when possible.

Height map:
  Rivers must flow in the direction which takes it to the tile with the lowest
  value on the height map.

If these rules failed to decide the direction, the random number generator
makes the decision.

Once a river has been formed, it is added to the map by adding the river extra
as needed. If the river goes through terrain where rivers are forbidden, the
terrain is simply changed. The whole process is repeated until enough tiles are
covered with rivers according to the :ref:`river fraction <mapgen-fractions>`.

.. _mapgen-island:

Island-Based Generators
-----------------------

The ``ISLAND`` generator setting corresponds to three different code paths
depending on the ``startpos`` setting. They all try to fill the map with
islands, but of different sizes and in different numbers to match the desired
starting positions. The general idea is to generate island shapes randomly and
paste them somewhere on the map. If this doesn't work, the process is repeated
with a slightly smaller island until a fitting size and shape is found. Once
islands are created, the map is finalized by filling them with terrain and
creating rivers.

The three available island-based generators are as follows, selected according
to the value of ``startpos``:

``VARIABLE``:
  This generator tries to create three types of islands: 70% of the land is
  given to big islands, 20% to medium, and 10% to small islands. In an ideal
  case, each player starts on a big island and gets one medium and one small
  island. However, due to the random nature of the algorithm it is not
  guaranteed that the smaller islands will be evenly distributed.

  If it is unable to create all the big islands, this generator starts again
  with a smaller size until they all fit, increasing the size of medium islands
  accordingly. Medium and small islands are optional and are only generated if
  they can be placed on the map.

``DEFAULT`` or ``SINGLE``:
  This generator also tries to create one big island per player. Their size
  follows a slightly complicated formula. The available landmass is first
  divided by the number of players. The islands are get one third of the
  landmass per player if this number is larger than 80 tiles; else half the
  landmass per player if this is larger than 60 tiles; otherwise exactly the
  landmass per player. However, the island size is always capped to 120 tiles.
  The big islands are then created, shrinking them down to up to 10% of their
  default size if they will not fit.

  To make up for any undistributed landmass, more islands are created with
  random initial sizes up to the default island size. The generator first
  attempts to place larger islands, then resorts to smaller ones if it fails.

``2or3`` or ``ALL``:
  The generator has to create even larger islands for this starting positions
  mode. Depending on the landmass settings, these large islands take up a
  different fraction of available land: 50% for a landmass above 60%, 60% for a
  landmass above 40%, and 70% for a landmass below. This part of the landmass is
  then distributed evenly among player islands, with two or three players per
  island, but letting the islands shrink to 10% of their intended size if
  needed to place them.

  Smaller islands are then created in two different sizes, one of each per
  player, while also letting those shrink to 10% of their size if needed.

The generators become significantly slower with high ``landmass``, and simply
refuse to proceed when it is too high (above 80 or 85%). In such cases, they
fall back to ``RANDOM``, ``FRACTAL``, or ``ISLANDS`` with the ``SINGLE`` player
placement strategy.

Island Creation
^^^^^^^^^^^^^^^

To create an island, the generator first builds up its shape and then tries to
place it on the map. The shape is created by starting from a single tile and
randomly expanding the island by adding cardinally adjacent tiles until the
desired size is reached. In addition to the random expansion, all tiles with
four neighbors are automatically added to the island. This prevents gaps from
appearing in the middle.

With the island shape created, the algorithm then tries to place it on the map
by randomly selecting a tile as center and checking for collisions with the map
boundaries and other islands. This is tried many times before the algorithm
eventually gives up. Once a fitting location is found, the tiles at the map
location are set to "unknown" terrain (the map is initially only water), then
its terrain is immediately set.

Rivers and Terrain
^^^^^^^^^^^^^^^^^^

Once an island is placed, rivers are added to it. The number of river tiles is
computed worldwide and each island gets a fraction of it proportional to its
surface. Creating rivers is done by repeatedly picking a random tile in the
island. The first suitable river mouth found in this way becomes a river.
Suitable river mouths are coastal tiles with a single cardinally adjacent
oceanic tile, at most 3 ocean tiles around, and no adjacent river. Afterwards,
new river mouths are only added with a 1% probability. Instead, when the random
tile is next to an existing river, the river grows in this direction. Further
conditions for rivers to grow require that the new tile has no cardinally
adjacent ocean and at most one adjacent ocean, and furthermore at most two
adjacent rivers. Dry tiles are also penalized by dropping them with a 50%
chance. In this way the rivers slowly grow inwards.

Terrain is placed in a similar way: tiles are picked at random and terrain is
assigned to them until reaching their expected number. This is done by groups of
terrains, starting with forest-like, then desert-like, mountain-like, and
finally swamp-like. The gaps are then filled with ice, tundras, plains, and
grassland depending on the local temperature.

The placement algorithm works as follows: for a given terrain type, a line is
picked in the table below. Lines with larger weights are selected more
frequently. Each line specifies both the properties of the terrain that will be
placed and conditions on the tile wetness and temperature that need to be met.
If they are not fullfilled, the tile is rejected and another one is picked at
random. In addition, some terrain types are generated less often next to the
coast. This is controlled by the "ocean affinity" parameter. For coastal tiles,
this defines the probability that the terrain will be placed on an otherwise
suitable tile. Finally, the creation of patches of similar terrain is encouraged
by requiring that the tile passes a coin flip when the terrain would not be next
to another tile of the same type.

.. table:: Terrain classes produced by the island generators and their matching to tiles
  :widths: auto
  :align: center

  +----------+----------+-----------+-----------+----------+---------+-------------+--------+
  |          |          | Terrain properties               | Tile properties       |        |
  +          + Ocean    +-----------+-----------+----------+---------+-------------+        +
  | Type     | Affinity | Required  | Preferred | Avoided  | Wetness | Temp.       | Weight |
  +==========+==========+===========+===========+==========+=========+=============+========+
  |          |          | Foliage   | Tropical  | Dry      | All     | Tropical    | 1      |
  +          +          +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Foliage   | Temperate | \-       | All     | All         | 3      |
  + Forest   + 60%      +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Foliage   | Wet       | Frozen   | Not dry | Tropical    | 1      |
  +          +          +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Foliage   | Cold      | \-       | All     | Not frozen  | 1      |
  +----------+----------+-----------+-----------+----------+---------+-------------+--------+
  |          |          | Dry       | Tropical  | Green    | Dry     | Hot         | 3      |
  +          +          +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Dry       | Temperate | Green    | Dry     | Not frozen  | 2      |
  + Desert   + 40%      +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Cold      | Dry       | Tropical | Dry     | Not hot     | 1      |
  +          +          +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Frozen    | Dry       | \-       | Dry     | Frozen      | 1      |
  +----------+----------+-----------+-----------+----------+---------+-------------+--------+
  |          |          | Mountain  | Green     | \-       | All     | All         | 2      |
  + Mountain + 20%      +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Mountain  | \-        | Green    | All     | All         | 1      |
  +----------+----------+-----------+-----------+----------+---------+-------------+--------+
  |          |          | Wet       | Tropical  | Foliage  | Not dry | Tropical    | 1      |
  +          +          +-----------+-----------+----------+---------+-------------+--------+
  | Swamp    | 80%      | Wet       | Temperate | Foliage  | Not dry | Hot         | 2      |
  +          +          +-----------+-----------+----------+---------+-------------+--------+
  |          |          | Wet       | Cold      | Foliage  | Not dry | Not hot     | 1      |
  +----------+----------+-----------+-----------+----------+---------+-------------+--------+

.. _mapgen-placement:

Player Placement
----------------

.. table:: Mode chosen by the generator to generate start positions
  :widths: auto
  :align: right

  ============ =======
  Generator    Default
  ============ =======
  ``FAIR``     Placed by the generator
  ``FRACTURE`` ``ALL``
  ``FRACTAL``  ``ALL``
  ``ISLAND``   Always ``SINGLE``
  ``RANDOM``   ``2or3``
  Scenarios    ``ALL``
  ============ =======

The final step in the map generator is to to place players on the map. The
method used to do so is set by the user, but it can also can depend on the
generator if ``startpos`` is set to ``DEFAULT``, as listed on the right.
Starting positions are allocated using the chosen method. If a method fails,
another method is tried in the following order: ``SINGLE``, ``2or3``, ``ALL``,
and finally ``VARIABLE``.

The code is located in the ``create_start_positions()`` function.

Placement tries to find fair starting positions using a "tile value" metric,
computed as the sum of all outputs produced by the tile (food, production, and
trade). If the initial workers can build a road, irrigation, or mine, half of
the best possible bonus is counted towards the value of the tile, rounded down.
Specials are taken into account but no government bonus is applied. This gives
the following values for common terrains:

.. table:: Tile value for the most common terrains
  :widths: auto
  :align: center

  ============ =============== ====================== =====
  Terrain      Food/Prod/Trade Road + Irrigation/Mine Value
  ============ =============== ====================== =====
  Forest       1/2/0           \-                     3
  Grassland    2/0/0           1 + 1/-                3
  Ocean        1/0/2           \-                     3
  Desert       0/1/0           1 + 1/1                2
  Plains       1/0/0           1 + 1/-                2
  Hills        1/0/0           \- + 1/3               2
  Jungle       1/0/0           \-                     1
  Mountains    1/0/0           \- + -/1               1
  Swamp        1/0/0           \-                     1
  Tundra       1/0/0           \- + 1/-               1
  ============ =============== ====================== =====

The initial city radius is then taken into account. This is done by setting the
tile value to zero if, within the city radius, more tiles are worse than better.
(So the value of a wheat tile surrounded by grass and a pheasant is set to
zero because all tiles except the pheasant are strictly worse.)
After this step, the map of tile values is smoothed out using a Gaussian filter
of width 1 and the value of ocean tiles is set to zero as they cannot be used as
starting positions. The total value of every island is computed by summing over
all tiles. Finally, tile values are normalized to the range [0, 1000).

With all the values computed, actual placement can start. Here another set of
fallbacks happens depending on the number of islands: the ``SINGLE`` placement
mode requires 3 islands more than the player count; ``2or3`` requires at least
half the player count plus 4, and ``ALL`` requires enough value for the best
island. ``VARIABLE`` is used as a fallback in all cases.

For the ``SINGLE`` and ``2or3`` modes, an attempt is made at avoiding islands
with too much variation in their total value. Then a number of players is
assigned to each island according to the placement mode: all on the best island
for ``ALL``, one per island for ``SINGLE``, two per island for ``2or3`` (3 on
the best island if needed), and a variable number of players for ``VARIABLE``
(trying to have a total value of 1500 per player, or failing that to distribute
available tiles evenly).

Having determined how many players to place per island, they are then randomly
distributed on valid start positions of the island. Picked tiles must have a
value in the top-10% worldwide (this criterion is progressively loosened if not
all players can be placed). A few other conditions need to be met: one cannot
start on a hut or frozen or cold terrain. It is also possible to check that
there are enough tiles in reach (controlled by the ruleset setting
``parameters.min_start_native_area`` in ``terrain.ruleset`` but disabled by most
rulesets). Finally, players cannot start less than four tiles away from each
other (or a bit more if the value of their initial tile is lower).
