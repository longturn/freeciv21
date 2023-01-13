..
    SPDX-License-Identifier:  GPL-3.0-or-later
    SPDX-FileCopyrightText: 1999-2021 Freeciv and Freeciv21 contributors
    SPDX-FileCopyrightText: 2022 louis94 <m_louis30@yahoo.com>

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

.. todo::
    Documentation is still missing for:

    * Sprite dimensions, in particular in hexagonal mode where it's counter-intuitive.
    * Blending
    * The mask sprite and what happens at the edge of the map
    * A general introduction to the structure (``[layer*]`` and ``[tile_*]`` sections). Some of this is
      already there at the bottom of but would need reformatting.


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
| One, same as ``match_type``    | ``t.l1.hills_n0e1s0w1``        | ``t.l0.floor_cell_u011``       | Not implemented                |
|                                | (:ref:`doc <single-match>`)    | (:ref:`doc <corner-same>`)     |                                |
+--------------------------------+--------------------------------+--------------------------------+                                +
| One, different from            | Not implemented                | ``t.l1.coast_cell_u_g_g_g``    |                                |
| ``match_type``                 |                                | (:ref:`doc <corner-pair>`)     |                                |
+--------------------------------+                                +--------------------------------+--------------------------------+
| Two or more                    |                                | ``t.l0.cellgroup_g_g_g_g``     | ``t.l0.hex_cell_right_g_g_g``, |
|                                |                                | (:ref:`doc <corner-general>`)  | ``t.l0.hex_cell_left_g_g_g``   |
|                                |                                |                                | (:ref:`doc <hex_corner>`)      |
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
:ref:`hex_corner <hex_corner>` instead.

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

This mode is used when a single matching group is specified in the ``matches_with`` list, and it is the same
as ``match_type``. This is often used to draw beaches, because they are drawn where neighboring tiles are
anything but water. 32 sprites are required for each tag, with the following naming convention:

.. code-block:: xml

    t.l<n>.<tag>_cell_<direction><01><01><01>

The value ``<n>`` is replaced with the layer number, and ``<tag>`` with the terrain tag. Sprites must be
provided for each of the four possible values of ``<direction>``: ``u``, ``d``, ``l``, and ``r``, that
indicate which corner the sprites are for. The three remaining parts, ``<01>``, each correspond to the
matching status of one of the adjacent tiles, counting clockwise. ``0`` means that the tile is not matched,
and ``1`` that it is.

For instance, the suffix of ``u011`` corresponds to the following situation, where blue represents the group
of the tile being rendered (black frame) and green is some other terrain:

.. figure:: /_static/images/tileset-reference/example-corner-same-1.png
    :alt: A diagram illustrating what the u011 corresponds to in terms of adjacent tiles.
    :align: center

Example
"""""""

Simple coasts can be drawn as follows:

.. code-block:: ini

    [layer0]
    match_types = "water"

    [tile_coast]
    tag = "coast"
    num_layers = 1
    layer0_match_type = "water"
    layer0_match_with = "water"
    layer0_sprite_type = "corner"

    [tile_floor]
    tag = "floor"
    num_layers = 1
    layer0_match_type = "water"
    layer0_match_with = "water"
    layer0_sprite_type = "corner"

    [tile_lake]
    tag = "lake"
    num_layers = 1
    layer0_match_type = "water"
    layer0_match_with = "water"
    layer0_sprite_type = "corner"

This requires 96 sprites, 32 for each tile type.

.. _corner-pair:

Matching a pair of groups
^^^^^^^^^^^^^^^^^^^^^^^^^

This mode is used when a single matching group is specified in the ``matches_with`` list, and it is different
from ``match_type``: a neighbor tile matches only if it is in the specified group. This can be used in a
similar role as :ref:`matching with the same group <corner-same>`, but is sometimes more convenient
(especially when a layer starts to have many groups). This mode requires 32 sprites per tag and uses the
following naming convention:

.. code-block:: xml

    t.l<n>.<tag>_cell_<direction>_<g>_<g>_<g>

The value ``<n>`` is replaced with the layer number, and ``<tag>`` with the terrain tag. Sprites must be
provided for each of the four possible values of ``<direction>``: ``u``, ``d``, ``l``, and ``r``, that
indicate which corner the sprites are for. The three remaining parts, ``<g>``, each correspond to the first
letter of a matching group of one of the adjacent tiles, counting clockwise. If there was a match, the first
letter of the group in ``matches_with`` is used; otherwise, it is the first letter of ``match_type``.

.. warning::
    Extra care is needed when drawing sprites for this mode; see the example for guidance.

Example
"""""""

Suppose that you have a tileset where mountains are drawn as solid rock. It would then make sense to draw
cliffs instead of beaches where the mountains meet water, as below:

.. figure:: /_static/images/tileset-reference/example-corner-pair-1.png
    :alt: The meeting point of four tiles, from left to right and top to bottom: mountains, water, plains,
        and water. A cliff is drawn between the mountains and the water.
    :align: center

    Cliffs

This can be achieved by drawing the mountains and the sea normally in the first layer, and overlaying the
cliffs in the second layer. In this example, the cliffs are drawn on top of the water (the mountains advance
into the sea):

.. code-block:: ini

    [layer2]
    match_types = "water", "mountains"

    [tile_coast]
    tag = "coast"
    num_layers = 2
    layer1_match_type = "water"
    layer1_match_with = "mountains"
    layer1_sprite_type = "corner"

    [tile_mountains]
    tag = "mountains"
    num_layers = 1
    layer1_match_type = "mountains"

The sprite shown above would be called ``t.l1.coast_cell_l_w_w_m`` (left side, water, water, and mountains
when enumerating clockwise): even though the tile on the left is not water, it is still identified as such
because it is not in the group given in ``match_with``.

Because the tile on the left is identified with water, there is no way to distinguish between the following
situations:

.. figure:: /_static/images/tileset-reference/example-corner-pair-2.png
    :alt: On the left, the same drawing as above. On the right, the same drawing with water instead of the
        plains.
    :align: center

    Indistinguishable cases when using pair matching.

Because of this, sprites need to be designed to work in several cases (the tile at the bottom could also be
either land or water). In the example above, the cliff vanishes at the corner, which allows it to merge with
the land and is also a plausible behavior when there is only water around.


.. _corner-general:

General matching
^^^^^^^^^^^^^^^^

When more than one element is present in the ``matches_with`` list, general matching is used. This mode uses
sprites that cover the intersection between four tiles:

.. figure:: /_static/images/tileset-reference/sprite-corner-general.png
    :alt: The meeting point of four tiles, with the area covered by the sprites highlighted.
    :align: center

The sprites have the same size as a normal tile, but are drawn with an offset equal to one half of a tile,
such that they are centered around the meeting point of the tiles.

The sprite naming convention uses only the names of the four groups the tiles are in. Unlike with other
modes, the terrain tag is not used:

.. code-block:: xml

    t.l<n>.cellgroup_<g>_<g>_<g>_<g>

The value ``<n>`` is replaced with the layer number. The four remaining parts, ``<g>``, each correspond to
the first letter of one of the groups specified in ``matches_with``, specified in clockwise order starting
from top (referring to the above schema, ``u``, ``r``, ``d``, and ``l``).

.. note::
    General matching is a very flexible mode that lets one draw very complex terrain, but this comes at the
    cost of a large number of sprites: for three groups, 81 sprites are needed; for four groups, it raises to
    256; and to use four groups, one would need to draw 625 sprites.


.. _hex_corner:

Sprite type ``hex_corner``
--------------------------

.. versionadded:: 3.0-beta1

    Use the ``+hex_corner`` option in tilesets requiring this feature.

The ``hex_corner`` sprite type provides functionality similar to ``corner``, using a geometry optimized for
isometric hexagonal tilesets. Hexagonal corner sprites cover one half of the height of the hexagons and are
centered vertically within the tiles. They come in two types: "left" corners cover the left hand side of an
hexagon and the right hand side of the border between two others; "right" corners have a similar geometry,
but are flipped horizontally. When drawn in a checkerboard pattern, left and right sprites reconstruct the
complete hexagons.

.. figure:: /_static/images/tileset-reference/sprite-hex-corners.png
    :alt: An illustration of the geometry explained above.
    :align: center

    The geometry of hexagonal corner sprites.

Matching for ``hex_corner`` sprites is always performed based on terrain groups, as in
:ref:`the general mode for square tilesets <corner-general>`. The naming convention for "left" sprites is as
follows:

.. code-block:: xml

    t.l<n>.hex_cell_left_<g>_<g>_<g>

For "right" sprites, simply replace ``left`` with ``right``. The value of ``<n>`` gives the layer number, and
the three ``<g>`` each correspond to the first letter of a matching group. For "left" sprites, the first
group corresponds to the tile of the right, the second to the tile at the top left, and the third group is the
one of the tile at the bottom left. For "right" sprites, the tile on the left comes first, followed by the
one at the top right and the tile at the bottom right. The order is indicated by the letters ``a``, ``b``, and
``c`` in the figure above.

Example
^^^^^^^

.. figure:: /_static/images/tileset-reference/example-hex-corners.png
    :alt: Four hexagons, two of which are water and the others land. The coast is highlighted and the
        boundaries of two corner sprites are shown.
    :align: center

    Coasts using ``hex_corner`` sprites.

To draw coasts using ``hex_corner``, one starts by defining two matching groups ``land`` and ``water``:

.. code-block:: ini

    [layer0]
    match_types = "land", "water"

Each land terrain must be declared within the ``land`` matching group, while seas and lakes go to ``water``:

.. code-block:: ini

    [tile_coast]
    tag = "coast"
    num_layers = 1
    layer0_match_type = "water"
    layer0_match_with = "land", "water"
    layer0_sprite_type = "hex_corner"

    [tile_plains]
    tag = "plains"
    num_layers = 1
    layer0_match_type = "land"
    layer0_match_with = "land", "water"
    layer0_sprite_type = "hex_corner"

    ; etc

With these settings, the two sprites shown in the figure are called ``t.l0.hex_cell_right_w_w_l`` for the one
above (white), and ``t.l0.hex_cell_left_l_w_l`` for the one below (red).


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
