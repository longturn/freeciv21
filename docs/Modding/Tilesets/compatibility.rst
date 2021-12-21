Tileset compatibility
*********************

The tileset format evolves when new Freeciv21 releases are published. As a rule of thumb, we try
to maintain compatibility with tilesets designed for previous releases, or at least to make the
update as straightforward as possible. When new features are introduced, we provide a flag that
lets tilesets require them: a tileset using such a flag won't load in versions of Freeciv21 that
don't have the feature. This lets tileset authors request the exact features needed by their
tileset.

Feature flags are requested by adding them to the ``options`` string in the main ``.tilespec``
file. For instance, the following options require ``precise-hp-bars`` in addition to the main
Freeciv21 capability:

.. code-block:: ini

   options = "+Freeciv-tilespec-Devel-2019-Jul-03 +precise-hp-bars"

Notice the ``+`` in front of the flag name.

The rest of this page lists the available flags and their meaning.

.. option:: duplicates_ok

    When a graphics tag is specified appears several times, the lattermost tag is used.

.. option:: precise-hp-bars

    Unit HP bars with a precision different from 10% can be used (sprites ``unit.hp_*``). The
    sprites must still be equally spaced: for instance, providing the following set of sprites will work as
    expected: ``unit.hp_0``, ``unit.hp_25``, ``unit.hp_50``, ``unit.hp_75``, ``unit.hp_100``.
    On the other hand, specifying only the following sprites will give wrong results:
    ``unit.hp_0``, ``unit.hp_90``, ``unit.hp_100``.

.. option:: unlimited-unit-select-frames

    The number of sprites used in the animation under the selected unit is no longer fixed to four
    (sprites ``unit.select*``). Also introduces the setting ``select_step_ms``.

.. option:: unlimited-upkeep-sprites

    The number of sprites used to display unit upkeep is no longer limited to 10
    (sprites ``upkeep.unhappy*``, ``upkeep.output*``).
