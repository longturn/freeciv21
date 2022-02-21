Terrain
*******

Drawing terrain with realistic (or not) transitions between adjacent tiles is a complex problem. Freeciv21
comes with several patterns that can be used to build complex terrain schemes. They are specified in the
main :file:`.tilespec` file of the tileset, under the ``[tile_*]`` and ``[layer*]`` sections.

Terrain rendering is done in several independent passes, each of them having its own layer in the main
tileset configuration file, ``Terrain1`` through ``Terrain3``. This allows several sprites to be combined in
the drawing of a single tile, which is one of the ways to break monotony.

Each terrain specified in a ruleset must have a ``graphic`` tag name, and optionally a ``graphic_alt`` tag.
This information is used to search for compatible sprites in the tileset, first using ``graphic`` and, if
it fails, ``graphic_alt``.

Sprite tags
-----------

The names used to reference the sprites depend on the chosen cell type and on the number of groups included
in the matching. The table below summarizes the naming scheme; available options are discussed in detail in
the corresponding sections.

+--------------------------------+--------------------------------+--------------------------------+--------------------------------+
| Matched groups                 | ``single``                     | ``corner``                     | ``hex_corner``                 |
+================================+================================+================================+================================+
| None                           | ``t.l0.grassland1``            | Avoid                          | Avoid                          |
|                                | (:ref:`doc <single-simple>`)   | (:ref:`doc <corner-simple>`)   |                                |
+--------------------------------+--------------------------------+--------------------------------+--------------------------------+
| One, same as ``match_type``    | ``t.l1.hills_n0e1s0w1``        | ``t.l0.floor_cell_u011``       |                                |
|                                | (:ref:`doc <single-match>`)    | (:ref:`doc <corner-same>`)     |                                |
+--------------------------------+--------------------------------+--------------------------------+                                +
| One, different from            | Not implemented                | ``t.l1.coast_cell_u_g_g_g``    | ``t.l0.hex_cell_right_g_g_g``, |
| ``match_type``                 |                                | (:ref:`doc <corner-pair>`)     | ``t.l0.hex_cell_left_g_g_g``   |
+--------------------------------+                                +--------------------------------+                                +
| Two or more                    |                                | ``t.l0.cellgroup_g_g_g_g``     |                                |
|                                |                                | (:ref:`doc <corner-general>`)  |                                |
+--------------------------------+--------------------------------+--------------------------------+--------------------------------+


Sprite type ``single``
----------------------

In this mode, each tile is drawn using a single sprite. The sprites should have dimensions
``normal_tile_width`` times ``normal_tile_height``. It is possible to augment the size by setting
``layerN_is_tall`` to ``TRUE``, in which case the height is expanded by 50% above the tile. This can be used
to render graphical elements like trees and mountains that hide terrain behind them.

It is possible to set arbitrary offsets on a per-terrain basis using ``layerN_offset_x`` (positive values move
the sprite to the right) and ``layerN_offset_y`` (positive values move the sprite down). These options should
be used with caution, because pixels drawn outside of the area covered by a "tall" tile will confuse the
renderer and cause artifacts.

.. note:: ``whole`` is a synonym for ``single``; ``single`` is preferred.

.. _single-simple:

Without matching
^^^^^^^^^^^^^^^^

The name of the sprites used by sprite type ``single`` depend on the number of terrain groups included in
``matches_with``. When no matching is performed, sprites names are built according to the following pattern:

.. code-block:: xml

    t.l<n>.<tag><i>

The value ``<n>`` is replaced with the layer number, and ``<tag>`` with the terrain tag. The last element,
``<i>``, is a number starting from 1: if several sprites are provided with numbers 1, 2, ..., the renderer
will pick one at random for every tile. This can be used to provide some variation, either by changing the
base terrain sprite or by overlaying decorations on top.

Example
"""""""

The following is the minimal definition for a terrain type: no matching is performed, and a single sprite is
sufficient:

.. code-block:: ini

    [tile_desert]
    tag = "desert"
    num_layers = 1

The base sprite would have tag ``t.l0.desert1``; additional sprites called ``t.l0.desert2``, ``t.l0.desert3``,
etc., can also be added, in which case one will be picked at random for every tile.

.. _single-match:

With matching
^^^^^^^^^^^^^

Sprite type ``single`` also supports matching against the `same` group as the represented terrain is in. For
instance, if one group is used for land, a second group for sea tiles, and a third group for ice, the sprite
used for ice tiles can depend on the presence of ice on adjacent tiles --- but when there is no ice, one
cannot know whether the other tile is land or water. In this case, the pattern is as follows:

.. code-block:: xml

    t.l<n>.<tag>_<directions>

Like in the unmatched case, ``<n>`` is replaced with the layer number and ``<tag>`` with the terrain tag. The
``<directions>`` part indicated which in which directions a match has been achieved, as a list of directions
followed by ``0`` (no match) or ``1`` (match). The directions depend on the tileset geometry:

* For square tilesets, they are North, East, South, and West, and thus the ``<directions>`` part looks like
  ``n0e1s1w0``. There are 16 sprites in total.
* Isometric hexagonal tilesets also have South-East and North-West, and the ``<directions>`` part looks like
  ``n0e1ne0s1w0nw0``. There are 64 sprites.
* Non-isometric hexagonal tilesets use North-East and South-West instead, for instance ``n0ne0e1s1sw1w0``.
  There are also 64 sprites.

Example
"""""""

In many tilesets, the sprites used for hills and mountains depend on the presence of other hills and mountains
on adjacent tiles. This is achieved by putting them in a single matching group, usually called ``hills``:

.. code-block:: ini

    [layer1]
    match_types = "hills"

We use layer 1 in this example because something is typically drawn under the hills for coasts and blending.
The next step is to put hills and mountains in the group and enable matching:

.. code-block:: ini

    [tile_hills]
    tag = "hills"
    num_layers = 2
    layer1_match_type = "hills"
    layer1_match_with = "hills"

    [tile_mountains]
    tag = "mountains"
    num_layers = 2
    layer1_match_type = "hills"
    layer1_match_with = "hills"

With these settings, both hills and mountains will match adjacent tiles if they have hills or mountains.

.. figure:: /_static/images/tileset-reference/example-single-match.png
  :alt: Amplio2 hills and mountains in two different layouts
  :align: center

  Hills and mountains in ``amplio2`` use the pattern described above.


Sprite type ``corner``
----------------------

The ``corner`` sprite type divides each tile in four smaller parts that are adjacent to only three tiles.
This allows matching with diagonal tiles, which would be impractical in ``single`` mode due to the large
number of sprites required. Corner mode was developed primarily for square isometric tilesets, but it can
also be used with other topologies, as shown in the diagram below:

.. figure:: /_static/images/tileset-reference/sprite-corners.png
    :alt: A diagram showing how the corners are defined
    :align: center

    Definition of the corners for the four tileset topologies: square isometric (top left), square (top
    right), hexagonal isometric (bottom left), and hexagonal (bottom right).

For square topologies, the corner sprites (colored rectangles) cover a slice of the tile area adjacent to
three other tiles. Matching takes place with respect to each of them, which enables complicated designs while
requiring comparatively small numbers of small sprites. For isometric hexagonal tilesets, some slices are
adjacent to two tiles and some to three; it is recommended that new tilesets use
:ref:`iso_corner <iso_corner>` instead.

The four corners are identified using the letters shown in the diagram, which stand for their location in
isometric mode: up, down, left, and right. The names used in the :file:`.spec` files depends on the number of
groups listed in ``matches_with`` and, when a single group is listed, of the group matching is performed
against. This naming scheme is explained in the next sections.

.. _corner-simple:

Without matching
^^^^^^^^^^^^^^^^

.. warning::
    Using corner sprites without matching is fully equivalent to a ``single`` sprite, except that performance
    is likely to be worse and the naming convention is harder to track. Avoid using this mode.

When no matching is performed, four ``corner`` sprites are required for each terrain. They are expected to be
half the size of a normal tile in both dimensions, and use the following naming scheme:

.. code-block:: xml

    t.l<n>.<tag>_cell_<direction>

The value ``<n>`` is replaced with the layer number, and ``<tag>`` with the terrain tag. The last part,
``<direction>``, indicates which corner the sprite refers to.

.. _corner-same:

Matching with the same group
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: xml

    t.l<n>.<tag>_cell_<direction><01><01><01>

.. _corner-pair:

Matching a pair of groups
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: xml

    t.l<n>.<tag>_cell_<direction>_<g>_<g>_<g>

.. _corner-general:

General matching
^^^^^^^^^^^^^^^^

.. code-block:: xml

    t.l<n>.cellgroup_<g>_<g>_<g>_<g>


.. _iso_corner:

Sprite type ``iso_corner``
--------------------------

.. todo::
    Documentation for this mode has yet to be written.


Terrain Options
---------------

The top-level :file:`.tilespec` file also contains information on how to draw each terrain type (grassland,
ocean, swamp, etc.). For each terrain type include a section :code:`[tile_xxx]`. This section contains
information on how to draw this terrain type. The terrain types are specified in the server :file:`ruleset`
file.

:code:`[tile_XXX]` options:

* :code:`tag` : Tag of the terrain this drawing information refers to. That must match the "graphic" or
  "graphic_alt" field given in the ruleset file.
* :code:`blend_layer` : If non-zero, given layer of this terrain will be blended with adjacent terrains.
  Blending is done civ2-style with a dither mask. Only iso-view currently supports blending. Only the base
  graphic will be blended. The blending mask has sprite :code:`t.dither_tile`.
* :code:`num_layers` : The number of layers in the terrain. This value must be 1, 2 or 3. Each layer is drawn
  separately. The layerN options below control the drawing of each layer (N should be 0, 1 or 2)
* :code:`layerN_match_type` : If 0 or unset, no terrain matching will be done and the base sprite will be drawn
  for the terrain. If non-zero, then terrain matching will be done. A matched sprite will be chosen that
  matches all cardinally adjacent tiles whose terrain has the same match_type.
* :code:`layerN_match_with` : List of match_types to match against
* :code:`layerN_sprite_type` : With traditional tilesets each tile is drawn using one sprite. This default
  :code:`sprite_type` is "whole". Which sprite to use may be specified using a :code:`match_group`, and there
  may be multiple layers (each having one sprite). This method corresponds to :code:`sprite_type` "single". A
  more sophisticated drawing method breaks the tile up into 4 rectangles. Each rectangular cell is adjacent to
  3 different tiles. Each adjacency is matched, giving 8 different sprites for each of the 4 cells. This
  :code:`sprite_type` is "corner".

Additionally the top-level :file:`.tilespec` file should contain information about the drawing of each layer.
This is needed because the way each layer is drawn must be consistent between different terrain types. You may
not have more than 3 layers (either in this section or in the [tile_XXX] sections).

:code:`[layerN]` Options:

* :code:`match_types` : Gives a string list of all different match types. This list must include every possible
  match_type used by terrains for this layer. First letter of the match_type must be unique within layer.

Terrain Sprites
---------------

To use tag matching, one first defines a number of terrain groups ("match type" in spec files). Groups are
created with :code:`create_matching_group`. Every tag must be in a group, set with
:code:`set_tag_matching_group`. The first letter of group names must be unique within a layer. Each tag can
then be matched against any number of groups. There will be one sprite for each combination of groups next to
the tile of interest.

The simplest matching is no matching, in which case the sprite used doesn't depend on adjacent terrain. This
is available for both :code:`CELL_WHOLE` and :code:`CELL_CORNER`, although there is little use for the second.
The sprite names for :code:`CELL_WHOLE` are formed like :code:`t.l0.grassland1`, where 0 is the layer number,
grassland appears in the name of the :file:`.tilespec` section, and 1 is an index (when there are several
sprites with indices 1, 2, 3, ..., one is picked at random). For :code:`CELL_CORNER`, the names are like
:code:`t.l0.grassland_cell_u`, where u ("up") indicates the direction of the corner. It can be u ("up"), d
("down"), r ("right"), or l ("left"). When a tag is matched against one group, there are two possibilities:

* For :code:`CELL_CORNER`, this requires 24 sprites with names like :code:`t.l0.grassland_cell_u010`.
  :code:`t.l0.grassland_cell_u` is like in the no match case, and 010 indicates which sides of the corner match
  the terrain being drawn. Amplio2 ice uses this method. The matched group is different from the tag group (only
  supported for :code:`CELL_CORNER`). There are again 24 sprites, this time with names like
  :code:`t.l0.grassland_cell_u_a_a_b`. The first letter, in this example u, is the direction of the corner. The
  other three indicate which terrains are found on the three external sides of the corner. They are the first
  letter of the name of a matching group: the group being matched against if the adjacent terrain is of that
  group, and otherwise the group of the sprite being drawn. The coasts of Amplio2 lakes use this method.

When more than one group is used, which is only supported for :code:`CELL_CORNER`, the sprites are named like
:code:`t.l0.cellgroup_a_b_c_d`. Each sprite represents the junction of four tiles with the group names first
letters a, b, c, and d. Each sprite is split in four to provide four corner sprites. Amplio2 coasts are drawn
this way.

Base Sprite
  If the terrain has no match type or is layered, a base sprite is needed. This sprite has tag :code:`t.<terrain>1`
  (e.g., :code:`t.grassland1`). More than one such sprite may be given (:code:`t.grassland2`, etc.) in which
  case one will be chosen at random for each tile.

Matched Sprites
  If the terrain has a match type or is layered, a set of matched sprites is needed. This consists of 16
  sprites with tags :code:`t.<terrain>_n<V>e<V>s<V>w<V>` (e.g., :code:`t.hills_n0e0s1w0`. Each direcional value
  :code:`<V>` is either 0 or 1. Note that the directions are in map coordinates, so n (north) in iso-view is
  northeast on the mapview. (Note this only applies for cell_type "single".)

Cell Sprites
  For matched terrains that have cell_type "rect", 32 different sprites are needed. Each sprite is a rectangle
  corresponding to one cell, and there are 8 different sprites per cell. Each sprite has a name like
  :code:`t.ocean_cell_u110` where "ocean" is the terrain, "u" means up (north on the map) and 110 indicates
  which of the adjacent tiles are mismatched. For instance u110 means:

.. code-block:: rst

      |      /\
      |     /B \
      |    /\ 1/\
      |   / A\/C \
      |   \1 /\ 0/
      |    \/D \/
      |     \  /
      |      \/


a matching terrain exists at C but not at A or B. In this case D is the current tile.

  Examples:

.. code-block:: ini

    ; This specifies a civ2-like grassland tile. A single sprite
    ; t.grassland is needed; it will be drawn blended.
    [terrain_grassland]
    blend_layer = 1
    num_layers  = 1
    layer0_match_type = 0

    ; This specifies a civ1-like mountain tile. 16 sprites
    ; t.mountains_n0s0e0w0 ... t.mountains_n1s1e1w1 are needed. One of them
    ; will be drawn to match the adjacent tiles. Assuming only mountains
    ; has this match_type, adjacent mountains will match.
    [terrain_mountains]
    blend_layer = 0
    num_layers  = 1
    layer0_match_type = 7

    ; This specifies a civ2-like hills tile. A base sprite t.hills will be
    ; needed, plus 16 matching sprites. The base sprite will be drawn,
    ; dithered with adjacent base sprites, and the matching sprite will be
    ; drawn on top. (In most civ2 tilesets the base sprite is the grassland
    ; sprite).
    [terrain_hills]
    blend_layer = 1
    num_layers  = 2
    layer0_match_type = 0
    layer1_match_type = 8

    ; This specifies a civ2-like ocean tile. Ocean is drawn via a cell-based
    ; system as explained above.
    [terrain_ocean]
    blend_layer = 1
    num_layers  = 1
    layer0_match_type = 6
    layer0_cell_type = "rect"
