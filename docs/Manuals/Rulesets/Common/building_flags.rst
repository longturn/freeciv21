..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

Building Flags
**************

Every building (e.g. city improvement) in a ruleset can be given zero (``0``) or more flags by ruleset
editors. Each flag gives additional features to the building that has been assigned the flag. The following
flags are used in all of the rulesets shipped by the Freeciv21 developers: Alien, Civ1, Civ2, Civ2Civ3,
Classic, Experimental, Granularity, Multiplayer, Royale, and Sandbox.

* :strong:`VisibleByOthers`: Anyone who can see your city knows whether it has this improvement. Great and
  small wonders are always visible.
* :strong:`SaveSmallWonder`: If you lose the city with this building in, and the ``savepalace`` server setting
  is enabled, another will be built for free in a random city. Should only be used with genus ``SmallWonder``.
* :strong:`Gold`: Not a real building; production turned into gold indefinitely (capitalization/coinage).
  Genus should be ``Special``.
* :strong:`DisasterProof`: Disasters never destroy this building. Is meaningful only for genus ``Improvement``
  buildings as others are automatically disaster proof.
