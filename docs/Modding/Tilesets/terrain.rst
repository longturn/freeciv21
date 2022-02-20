Terrain
*******

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
* :code:`layerN_is_tall` : Left right corner of terrain sprites is not based on normal_tile_width and
  normal_tile_height, but to corner of the full tile.
* :code:`layerN_offset_x` : Offset for terrain sprites
* :code:`layerN_offset_y` : Offset for terrain sprites
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

* The matched group is the same as the tag group. For :code:`CELL_WHOLE`, this requires sprites with names
  like :code:`t.l0.grassland_n1e0s0w0`, where the n1 indicates that the terrain in the north direction is in the
  same group as the tile that is being drawn, and the 0's indicate that other terrains are different. Sprites
  must be provided for all 16 combinations of 0's and 1's. Amplio2 forests and hills are drawn this way.

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
