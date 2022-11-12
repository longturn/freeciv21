..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

Technology Advance Flags
************************

Every technology advance in a ruleset can be given zero (``0``) or more flags by ruleset editors. Each flag
gives additional features to the technology advance that has been assigned the flag. A reader can find the
flags by looking at the :file:`techs.ruleset` file in any ruleset directory. The following flags are used in
all of the rulesets shipped by the Freeciv21 developers: Alien, Civ1, Civ2, Civ2Civ3, Classic, Experimental,
Granularity, Multiplayer, Royale, and Sandbox.

:strong:`Bonus_Tech`
  The player gets extra technology if reached first.

:strong:`Bridge`
  ``Settler`` unit types can build roads with ``RequiresBridge`` flag over roads with ``PreventsOtherRoads``
  flag (rivers).

:strong:`Build_Airborne`
  From now on player can build air units (for use by AI).

:strong:`Claim_Ocean`
  Player claims ocean tiles even if they are not adjacent to border source.

:strong:`Claim_Ocean_Limited`
  Oceanic border sources claim ocean tiles even if they are not adjacent to border source.
