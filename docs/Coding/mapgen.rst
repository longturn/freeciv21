.. SPDX-License-Identifier: GPL-3.0-or-later
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
(``generator`` server setting), but they share a lot of code.

Map generation is performed in multiple passes. After some initialization, the
first step, shared between all generators, is the creation of a basic
temperature map. Since no terrain information exists at this stage, the
temperature is simply a measure of the distance to the pole (the "colatitude").
Unless ``alltemperate`` is set, the world is then divided in four regions:
frozen, cold, temperate, and tropical.

After the temperature is set, the actual map generation starts. This depends on
the map generator chosen by the user, with various fallbacks in place. The
details of each generator are described below.  After this step, at least
terrain is set everywhere. Depending on the map generator, players and specials
may already have been placed (this is for instance the case with the "fair
islands" generator).

At this stage, tiny (1x1) islands are removed if disabled in the settings
(``tinyislands``) parameter. Water is also fine-tuned to always have shallow
ocean next to the coast and be generally smooth. Continent numbers are assigned
and small seas are turned to lakes. The temperature map is reset after this is
done.

If not done earlier, resources are then added to the map with at least one tile
between them. For each tile that gets a resource, one is picked at random from
the list of allowed resources for the terrain. Huts are added in a similar way
but with a minimum distance of 3 tiles. The final step is to distribute players,
as described in :ref:`Player Placement <mapgen-placement>` below.

.. _mapgen-placement:
Player placement
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
for ``ALL``, one per island for ``SINGLE``, two per island for ``2to3`` (3 on
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
