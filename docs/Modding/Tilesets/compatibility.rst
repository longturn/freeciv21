.. SPDX-License-Identifier:  GPL-3.0-or-later
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

Tileset compatibility
*********************

The tileset format evolves when new Freeciv21 releases are published. As a rule of thumb, we try
to maintain compatibility with tilesets designed for previous releases, or at least to make the
update as straightforward as possible. When new features are introduced, we provide a flag that
lets tilesets require them: a tileset using such a flag will not load in versions of Freeciv21 that
do not have the feature. This lets tileset authors request the exact features needed by their
tileset.

Feature flags are requested by adding them to the ``options`` string in the main ``.tilespec``
file. For instance, the following options require ``precise-hp-bars`` in addition to the main
Freeciv21 capability:

.. code-block:: ini

   options = "+Freeciv-tilespec-Devel-2019-Jul-03 +precise-hp-bars"

Notice the ``+`` in front of the flag name.

The rest of this page lists the available flags and their meaning.

duplicates_ok
    When a graphics tag is specified appears several times, the lattermost tag is used.

precise-hp-bars
    Unit :term:`HP` bars with a precision different from 10% can be used (sprites ``unit.hp_*``). The
    sprites must still be equally spaced: for instance, providing the following set of sprites will work as
    expected: ``unit.hp_0``, ``unit.hp_25``, ``unit.hp_50``, ``unit.hp_75``, ``unit.hp_100``.
    On the other hand, specifying only the following sprites will give wrong results:
    ``unit.hp_0``, ``unit.hp_90``, ``unit.hp_100``.

unlimited-unit-select-frames
    The number of sprites used in the animation under the selected unit is no longer fixed to four
    (sprites ``unit.select*``). Also introduces the setting ``select_step_ms``.

unlimited-upkeep-sprites
    The number of sprites used to display unit upkeep is no longer limited to 10
    (sprites ``upkeep.unhappy*``, ``upkeep.output*``).

hex_corner
    Support for this option signals the availability of the new ``hex_corner`` sprite type for terrain.
