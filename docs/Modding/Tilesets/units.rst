.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv contributors
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

Units
*****

In addition to the unit sprite itself, the following are drawn:

* For selected units, an animated selection indicator is displayed underneath the unit. Optionally, the
  selected units can blink instead. `select_offset_x` and `select_offset_y`.
* The unit flag is also drawn under the unit picture (unless Freeciv21 is configured to display a solid color
  behind units, in which case that color is shown instead). `unit_flag_offset_x`, `unit_flag_offset_y`.
* Then, the sprite for the unit type is added. It can depend on the unit orientation. `unit_offset_x` and
  `unit_offset_y`.
* Loaded transports are marked as such.
* A sprite is added to represent the unit activity. `activity_offset_x` and `activity_offset_y`.
* Units on autosettler and autoexplorer commands are tagged. `activity_offset_x` and `activity_offset_y` for
  autoexplore.
* The patrol indicator is added for units with repeating orders, or the "connect" indicator for units with
  this kind of orders, or the "G" sprite for units on goto (with `activity_offset_x` and `activity_offset_y`).
* Units awaiting a decision by the players are marked with a question mark sprite. `activity_offset_x` and
  `activity_offset_y`.
* The battlegroup sprite is added.
* Low fuel.
* Tired.
* The "plus" symbol is added for stacked units and loaded transports.
* Veterancy.
* :term:`HP`.

Unless mentioned otherwise, the sprites are drawn like "full sprites".

Unit Sprites
------------

Units sprites can be either unoriented or oriented, in which case the sprite that is displayed depends on the
direction the unit is facing (it turns when it moves or fights).

Unoriented sprites are specified as :code:`u.phalanx`. Oriented sprites have a direction suffix:
:code:`u.phalanx_s`, :code:`u.phalanx_nw` and so on. For each unit type, either an unoriented sprite or a full
set of the oriented sprites needed for the tileset topology must be provided (you can also provide both, see
below).

The game sometimes needs to draw a sprite for a unit type that does not correspond to a specific unit, so is
not facing a particular direction. There are several options for oriented tilesets:

* If the :code:`unit_default_orientation` is specified for the tileset, the game will by default use that
  directional sprite. The direction does not have to be a valid one for the tileset.

* Specific unit types may override this by providing an unoriented sprite as well as the oriented ones; this
  does not have to be distinct, so it can point to one of the oriented sprites, allowing choice of the best
  orientation for each individual unit type. If unit_default_orientation is not specified, an unoriented sprite
  must be specified for *every* unit.

