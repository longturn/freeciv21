..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Technology Advance Flags
************************

Every technology advance in a ruleset can be given zero or more :ref:`flags <Effect Flags>` by ruleset
:doc:`modders </Modding/index>`. Each flag gives additional features to the technology advance that has been
assigned the flag. A reader can find the flags by looking at the :file:`techs.ruleset` file in any ruleset
directory. The following flags are used in all of the rulesets shipped by the Freeciv21 developers.

:strong:`Bonus_Tech`
  The player gets extra technology when reaching the technology with this flag first.

:strong:`Bridge`
  ``Settler`` unit types can build roads with the ``RequiresBridge`` flag over roads with the
  ``PreventsOtherRoads`` flag. This is typically on Rivers.

:strong:`Build_Airborne`
  This flag is mostly to help the AI (e.g. an AI hint). From now on the player can build air units.

:strong:`Claim_Ocean`
  With this flag, the player can claim ocean tiles even if they are not adjacent to a border source.

:strong:`Claim_Ocean_Limited`
  With this flag, the player's oceanic border sources claim ocean tiles even if they are not adjacent to a
  border source.
