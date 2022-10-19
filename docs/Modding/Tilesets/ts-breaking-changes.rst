..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Tileset Breaking Changes
************************

This page lists in reverse chronilogical order breaking changes merged into the ``master`` branch of the
GitHub repository to aid Tileset modders keep thier tilesets up to date.

Post Beta.4 before Beta.5
=========================

* Removed the need for ``main_intro_file`` for all :file:`*.tilespec` files. Tileset modders no longer need to
  include this tag in their tilesets.
* Wonder sprites removed from :file:`data/misc/buildings.spec` and moved into :file:`data/misc/wonders.spec`.
  Tileset modders will need to include a new file to get the small wonder sprites.
* Removed :file:`data/amplio2/units.spec`. Consolidated all unit sprites into :file:`data/misc/units.spec`.
  Tileset modders utilizing the Amplio2 unit sprites will want to instead use the new ``units.spec`` file. It
  is the new home of the Amplio style units utilized by the Tier 1 tilesets in Freeciv21: Amplio2 and
  Hexemplio.
