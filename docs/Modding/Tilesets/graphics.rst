Tile Specification Files
************************

Creating a Freeciv21 Tileset requires a mix of editing plain text :file:`spec` files as well as creating
graphics files saved in the :file:`.png` image format. When complete, a Tileset editor will have created the
canvas that Freeciv21 players play upon. Next to Rulesets, creating Tilesets is one of the most important
aspects of a Freeciv21 game.

Using Graphics
---------------

To use different graphics with Freeciv21, use the :code:`--tiles` argument to the Freeciv21 client. Eg, to use
the 'engels' graphics, start the client as:

.. code-block:: rst

    freeciv21-client --tiles engels


What Freeciv21 actually does in this case is look for a file called :file:`engels.tilespec` somewhere in your
Freeciv21 data path. That :file:`.tilespec` file contains information telling Freeciv21 which graphics files
to use, and what those graphics files contain.

If you don't want to use the command line to invoke Freeciv21, you can also load a different Tileset from the
client menu at :menuselection:`Game --> Load Another Tileset`. The Longturn.net community also provides some
custom Tilesets accessible from the modpack installer utility; documented in :doc:`../../General/modpack-installer`.

The rest of this file describes, though not fully, the contents of the :file:`.tilespec` file and related files.
This is intended as developer reference, and for people wanting to create/compile alternative tilesets and
modpacks for Freeciv21.

Overview
--------

The purpose of the :file:`.tilespec` file and related :file:`.spec` files is to allow the detailed layout of
the graphics within the files to be flexible and not hard-coded into Freeciv21, and to allow add-ons to
conveniently provide additional graphics. The PNG graphic files are essentially a collection of arranged
:emphasis:`sprites` combined together into a single PNG file. The :file:`.spec` files then tell Freeciv21
how to process the sprites and display them in the client. The :file:`.spec` files select which images are
used for each Freeciv21 graphic, and the :file:`.tilespec` file controls which :file:`.spec` files are used,
and how the graphics will interact with each other.

There are two layers to the :file:`.tilespec` files:

* The top-level file is named, for example, :file:`engels.tilespec`. The basename of this file (here,
  'engels') corresponds to the parameter of the :code:`--tiles` command-line argument for the Freeciv21 client,
  as described above.

* The top-level :file:`.tilespec` file contains general information on the full tileset, and a list of files
  which specify information about the individual graphics files. These filenames must be located somewhere in the
  data path. On Unix like operating systems this is :file:`[install location]/share/freeciv` and on Windows this
  is :file:`[install location]/data`. Typically the second-level :file:`.spec` and image files are in a
  sub-directory at the same level as the :file:`.tilespec` file. Note that with this system the number and
  contents of the referenced :file:`.spec` and image files are completely flexible at this level. Here is an
  example file and folder view:

.. code-block:: rst

    engels.tileset
    engels/
      terrain1.spec
      terrain1.png
      terrain2.spec
      terrain2.png
      ...


An exception is that the intro graphics must be in individual files, as listed in the :file:`.tilespec` file,
because Freeciv21 treats these specially: these graphics are freed after the game starts, and reloaded later as
necessary.

Graphics Formats
----------------

The Freeciv21 client currently uses 24 or 32 bit PNGs image files. As noted before, the PNG files are a
collection of images in a single image file. The smaller images are called :emphasis:`sprites`.

Tileset Options
---------------

In the top-level :file:`.tilespec` file you can set options for the tileset. Each of these should go within
the :code:`[tilespec]` section. Currently options include:

:strong:`Strings`

  String values are enclosed in quotes ( :code:`" "` )

* :code:`options` : A capability string, this should be :code:`+Freeciv-a.b-tilespec`, where "a.b" it the
  current Freeciv21 version.
* :code:`name` : The name of the tileset.
* :code:`type` : General type of tileset, different types have quite different format. Supported types are
  "overhead" and "isometric".
* :code:`main_intro_file` : Graphics file for the intro graphics.
* :code:`unit_default_orientation` : Specifies a direction to use for unit types in worklists etc.
  See "Unit Sprites" below.

:strong:`String Vectors`

* :code:`preferred_themes` : List of preferred client themes to use with this tileset.

:strong:`Integers`

* :code:`priority` : When user does not specify tileset, client automatically loads available compatible tileset
  with highest priority.
* :code:`normal_tile_width` : The width of terrain tiles.
* :code:`normal_tile_height` : The height of terrain tiles.
* :code:`unit_width` : Unit sprite width. Default is always ok, setting is provided just for symmetry with
  :code:`unit_height`.
* :code:`unit_height` : Unit sprite height if more than 1.5x terrain tile height in isometric tileset.
* :code:`small_tile_width` : The width of icon sprites.
* :code:`small_tile_height` : The height of icon sprites.
* :code:`fog_style` : Specifies how fog is drawn.

  * :code:`Auto` : Code automatically adds fog.
  * :code:`Sprite` :A single fog sprite is drawn on top of all other sprites for fogged tiles. The tx.fog
    sprite is used for this.
  * :code:`Darkness` : No fog, or fog from darkness_style = 4.

* :code:`darkness_style` : Specifies how "encroaching darkness" is drawn.

  * :code:`None` : No darkness.
  * :code:`IsoRect` : A single sprite can be split into 4 parts, each containing the darkness for that
    particular cardinal direction. (Iso-view only.)
  * :code:`CardinalSingle` : Four different sprites exist, each holding the darkness for a particular
    direction. Any or all of the sprites may be drawn.
  * :code:`CardinalFull` : The sprite is chosen based on the vector sum of the darkness in all 4 cardinal
    directions. 15 different sprites are needed.
  * :code:`Corner` : Corner darkness & fog, 81 sprites needed.

* :code:`unit_flag_offset_x` : Gives an offset from the tile origin at which to...
* :code:`unit_flag_offset_y` : Draw flags behind units or cities. With isometric...
* :code:`city_flag_offset_x` : Tilesets this should be non-zero so that the flag...
* :code:`city_flag_offset_y` : Is placed correctly behind the unit/city.
* :code:`occupied_offset_x` : Gives an offset from the tile origin at which to...
* :code:`occupied_offset_y` : Draw city occupied icon (in many tilesets placed above the flag).
* :code:`city_size_offset_x` : Gives an offset from the full tile origin at which to...
* :code:`city_size_offset_y` : Draw city size number.
* :code:`unit_offset_x` : Gives an offset from the tile origin at which to...
* :code:`unit_offset_y` : Draw units.
* :code:`activity_offset_x` : Gives an offset from the tile origin at which to...
* :code:`activity_offset_y` : Draw normal unit activity icons. "Auto" icons are not affected by this as they
  are usually wanted in different offset than real activity icons for both to appear simultaneously "Auto"
  icons are auto_attack, auto_settler, patrol, connect.
* :code:`select_offset_x` : Gives an offset from the tile origin at which to...
* :code:`select_offset_y` : Draw selected unit sprites.
* :code:`unit_upkeep_offset_y` : Gives an offset from the unit origin at which to draw the upkeep icons when
  they are shown along the unit. The upkeep icons can safely extend below the unit icon itself. If this value
  is omitted, normal tile height is used instead;

    * Upkeep icons appear below the unit icon if the unit icons are equal to tile height (typical in overhead
      tileset)
    * Upkeep icons overlay lower part of the unit icon, if unit icon is higher than tile height (typical in
      iso tilesets)

* :code:`unit_upkeep_small_offset_y` : Like :code:`unit_upkeep_offset_y`, but to be used in case there's only
  small space for the overall icon produced. Defaults to :code:`unit_upkeep_offset_y` - not having alternative
  layout.
* :code:`citybar_offset_y` : Gives an offset from city tile origin at which to draw city bar text.
* :code:`hex_sid` : When is_hex is specified (see is_hex, below), this value gives the length of the "extra"
  side of the hexagon. This extra side will be on the top/bottom of the tile if is_isometric (below) is given,
  or on the left/right of the tile otherwise. The actual dimensions of the hex tile are determined from the
  normal_tile_width/normal_tile_height of the tileset as well as the hex side. The "normal" dimensions give
  the X and Y offsets between adjacent tiles in the tileset - this is not the same as the dimensions of the
  tile itself. The dimension of the bounding box of the hexagonal tile will be equal to the "normal" dimension
  minus the hex_side. For instance, "normal" dimensions of 64x32 with a hex_side of 16 for an iso-hex tileset
  will give hexagons of size 48x32.

:strong:`Booleans`

  Boolena values are either FALSE or TRUE.

* :code:`is_hex` : Set to TRUE for a hexagonal tileset. If :code:`is_isometric` is also specified then you have
  an iso-hex tileset. Hex tilesets should be used with topologies 8-11 and iso-hex tilesets with topologies 12-15.

:strong:`String Lists`

  String lists are aa comma-separated list of strings.

* :code:`files` : A list of :file:`.spec` files to scan for sprites. See "individual spec files", below.


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

Extra Options
-------------

Tilespec should define style of extra graphics for each extra type in section :code:`[extras]` like:

.. code-block:: ini

    [extras]
    styles =
        { "name",          "style"
          "road",          "RoadAllSeparate"
          "rail",          "RoadAllSeparate"
          "river",         "River"
          "tx.irrigation", "Cardinals"
        }


* :code:`RoadAllSeparate` : A single sprite is drawn for every connection the tile has; only 8 sprites are needed.
* :code:`RoadParityCombined` : A single sprite is drawn for all cardinal connections and a second sprite is
  drawn for all diagonal connections; 32 sprites are needed.
* :code:`RoadAllCombined` : One sprite is drawn to show roads in all directions. There are thus 256 sprites (64
  for a hex tileset).
* :code:`River` : Cardinal connections are drawn, as well as delta at the coast
* :code:`Single1` : Single sprite at layer :code:`Special1`.
* :code:`Single2` : Single sprite at layer :code:`Special2`.
* :code:`3Layer` : 3 Sprites, tagged :code:`<name>_bg`, :code:`<name>_mg`, and :code:`<name>_fg`.
* :code:`Cardinals` : Sprite for each cardinal connection.

Individual Spec Files
---------------------

Each :file:`.spec` file describes one graphics file as specified in the spec file. The graphics file must be
in the Freeciv21 data path, but not necessarily in the same location as the :file:`.spec` file. Note you can
have multiple spec files using a single graphics file in different ways.

The main data described in the :file:`.spec` file is in sections named :code:`[grid_*]`, where :code:`*` is
some arbitrary tag (but unique within each file). A grid corresponds to a regular rectangular array of tiles.
In general one may have multiple grids in one file, but the default tilesets usually only have one per file.
Multiple grids would be useful to have different size tiles in the same file. Each grid defines an origin (top
left) and spacing, both in terms of pixels, and then refers to individual tiles of the grid by row and column.
The origin, and rows and columns, are counted as (0,0) = top left.

* :code:`x_top_left` : X-coordinate of the leftmost pixel of the leftomost cell.
* :code:`y_top_left` : Y-coordinate of the topmost pixel of the topmost cell.
* :code:`dx` : Cell width.
* :code:`dy` : Cell height.
* :code:`pixel_border` : Number of pixels between cells, unless overridden by axis specific value.
* :code:`pixel_border_x` : Number of pixels between cells in x-direction, overrides :code:`pixel_border`.
* :code:`pixel_border_y` : Number of pixels between cells in y-direction, overrides :code:`pixel_border`.
* :code:`tiles`: Table of tags, each line having "row", "column", and "tag".

.. code-block:: ini

    [grid_example]
    x_top_left   = 1   ; Border (in x=0) also in left side of the entire grid
    y_top_left   = 1   ; Border (in y=0) also in top side of the entire grid
    dx           = 96
    dy           = 48
    pixel_border = 1
    tiles = { "row", "column", "tag"
    0, 0, "tag1"
    0, 1, "tag2"
    1, 0, "tag3"
    1, 1, "tag4"
    }


Each individual tile is given a "tag", which is a string which is referenced in the code and/or from ruleset
files. A grid may be sparse, with some elements unused (simply don't mention their row and column), and a
single tile may have multiple tags (eg, to use the same graphic for multiple purposes in the game): just
specify a list of comma-separated strings.

If a given tag appears multiple times in the spec files, the *last* such tag is used. That is, in the order of
files listed in the tilespec file, and order within each file. This allows selected graphics to be "overridden"
by listing a replacement spec file near the end of the 'files' list in the top-level tilespec file, without
having to modify earlier files in the list.

Tag Prefixes
------------

To help keep the tags organised, there is a rough prefix system used for standard tags:

* :code:`f.` : National flags.
* :code:`r.` : Road/rail.
* :code:`s.` : General "small".
* :code:`u.` : Unit images.
* :code:`t.` : Basic terrain types (with :code:`_n0s0e0w0` to :code:`_n1s1e1w1`).
* :code:`ts.` : Terrain special resources.
* :code:`tx.` : Extra terrain-related.
* :code:`gov.` : Government types.
* :code:`unit.` : Unit overlays: hp, stack, activities (goto, fortify etc.).
* :code:`upkeep.` : Unit upkeep and unhappiness.
* :code:`city.` : City related (city, size, sq.-prod., disorder, occupied).
* :code:`cd.` : City defaults.
* :code:`citizen.` : Citizens, including specialists.
* :code:`explode.` : Explosion graphics (nuke, units).
* :code:`spaceship.` : Spaceship components.
* :code:`treaty.` : Treaty thumbs.
* :code:`user.` : Crosshairs (in general: user interface?).

In general, graphics tags hard-wired into Freeciv21 :strong:`must` be provided by the :file:`.spec` files, or
the client will refuse to start. Graphics tags provided by ruleset files (at least for the "standard"
rulesets) should also be provided, but generally the client will continue even if they are not, though the
results may not be satisfactory for the user. To work properly tags should correspond to appropriately sized
graphics. The basic size may vary, as specified in the top-level :file:`.tilespec` file, but the individual
tiles should be consistent with those sizes and/or the usage of those graphics.

Sprites
-------

Depending on the information given here the tileset must/may contain certain sprites.

Theme Sprites
-------------

Citizen Sprites
  This provides citizen graphics. Each citizen has one or more sprites which are shown in the city dialog. The
  types of citizen are "happy", "content", "unhappy", and "angry". The tag name is :code:`citizen.<type>_<n>`.
  :code:`<type>` is one of the listed types. :code:`<n>` is the number of the graphic (numbered starting with
  0, unlike most other graphics) which allows more than one sprite to be used. No more than 6 sprites per
  citizen may be used.

  Currently the citizen and specialist sprites may not have any transparency, as this is ignored in much of
  the drawing. This is considered a bug.

Specialist Sprites
  These provide specialist graphics just like the citizen graphics. However, specialist types come from the
  ruleset and may be changed in modpacks. The sprite name is :code:`specialist.<type>_<n>`. Again :code:`<type>`
  is the type of specialist (currently "elvis", "scientist", "taxman") while :code:`<n>` is the sprite number.
  See "citizen sprites" above.

Progress Indicators
  There are three types of progress indicator. :code:`science_bulb` indicates progress toward the current
  research target. :code:`warming_sun` indicates progress toward global warming. :code:`cooling_flake`
  indicates progress toward nuclear winter. Each indicator should have 8 states, numbered 0 (least) through 7
  (most). The sprite names are :code:`s.<type>_<n>`.

Government Icons
  There should be one icon for each government. Its name is :code:`gov.<gov>`, where :code:`<gov>` is the
  government name. Government types come from :file:`governments.ruleset` (currently "anarchy", "despotism",
  "monarchy", "communism", "fundamentalism", "republic", "democracy"). Ruleset modders can create other
  governments, the aforementioned list is not static.

Tax Icons
  One icon for each tax type. These are used to show the tax rates. The sprites are :code:`s.tax_luxury`,
  :code:`s.tax_science`, :code:`s.tax_gold`. Commonly the specialist sprites are reused for this.

Right Arrow
  A sprite :code:`s.right_arrow` is used on the panel when more units are present than can be shown.


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


Terrain Special Sprites
-----------------------

Farmland/Irrigation
  :code:`tx.farmland` and :code:`tx.irrigation` provide the basic sprites for farmland and irrigation.
  Additionally, there is support for drawing continuous farmland and irrigation (as is used in Civ3). Here
  there are 16 irrigation sprites (and the same for farmland), starting with :code:`tx.irrigation_n0s0e0w0`
  and running through :code:`tx.irrigation_n1s1e1w1`. An appropriate sprite will be chosen depending on which
  adjacent tiles also have farmland/irrigation. If any of these sprites are not present, the default sprite
  will be used as a fallback.

Unit Sprites
------------

Units sprites can be either unoriented or oriented, in which case the sprite that is displayed depends on the
direction the unit is facing (it turns when it moves or fights).

Unoriented sprites are specified as :code:`u.phalanx`. Oriented sprites have a direction suffix:
:code:`u.phalanx_s`, :code:`u.phalanx_nw` and so on. For each unit type, either an unoriented sprite or a full
set of the oriented sprites needed for the tileset topology must be provided (you can also provide both, see
below).

The game sometimes needs to draw a sprite for a unit type that doesn't correspond to a specific unit, so is
not facing a particular direction. There are several options for oriented tilesets:

* If the :code:`unit_default_orientation` is specified for the tileset, the game will by default use that directional
  sprite. (The direction doesn't have to be a valid one for the tileset.)

* Specific unit types may override this by providing an unoriented sprite as well as the oriented ones; this
  doesn't have to be distinct, so it can point to one of the oriented sprites, allowing choice of the best
  orientation for each individual unit type. If unit_default_orientation is not specified, an unoriented sprite
  must be specified for *every* unit.
