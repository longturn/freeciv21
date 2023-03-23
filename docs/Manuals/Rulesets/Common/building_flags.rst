..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Building Flags
**************

Every building (e.g. city improvement) in a ruleset can be given zero or more :ref:`flags <Effect Flags>` by
ruleset :doc:`modders </Modding/index>`. Each flag gives additional features to the building that has been
assigned the flag. A reader can find the flags by looking at the :file:`buildings.ruleset` file in any ruleset
directory. The following flags are used in all of the rulesets shipped by the Freeciv21 developers.

:strong:`VisibleByOthers`
  Anyone who can see your city knows whether it has this improvement. Great and small wonders are always
  visible. :improvement:`City Walls` are often visible by others as well.

:strong:`SaveSmallWonder`
  If you lose the city with this building in it, and the ``savepalace`` server setting is enabled, another
  :improvement:`Palace` will be built for free in a random city. This flag should only be used with building
  genus ``SmallWonder``.

:strong:`Gold`
  This is not a real real building, per se. The city's shield production is turned into gold indefinitely.
  Most rulesets call this :improvement:`Capitalization` or :improvement:`Coinage`. The building's genus should
  be ``Special``.

:strong:`DisasterProof`
  Natural disasters never destroy a building with this flag. This flag is important for building genus
  ``Improvement`` as others are automatically disaster proof.
