..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: wonder

Nation_Intelligence
*******************

.. versionadded:: 3.0
    Add the ``+Nation_Intelligence`` option to ``effects.ruleset``.

The ``Nation_Intelligence`` effect controls what kind of information about a
foreign nation is visible in the players report. A value of ``0`` or lower hides
the information, while it becomes visible when the effect evaluates to ``1`` or
bigger. For instance, the following effect repurposes the
:wonder:`United Nations` to reveal everyone's secrets to everone else:

.. code-block:: ini

    [effect_united_nations_intelligence]
    type  = "Nation_Intelligence"
    value = 1
    reqs  =
        { "type",     "name",           "range",  "present"
          "Building", "United Nations", "World",  TRUE
        }

The true power of ``Nation_Intelligence``, however, comes from its combined use
with ``NationalIntelligence`` requirements. ``NationalIntelligence`` lets you
select precisely what is visible to a player. Let us say that we want a player
owning the :wonder:`Marco Polo Embassy` to know the wonders built by any other
player it has ever met. This could be achieved as follows:

.. code-block:: ini

    [effect_marco_polo_intelligence]
    type  = "Nation_Intelligence"
    value = 1
    reqs  =
        { "type",                 "name",                 "range",  "present"
          "NationalIntelligence", "Wonders",              "Local",  TRUE
          "Building",             "Marco Polo's Embassy", "Player", TRUE
          "DiplRel",              "Never met",            "Local",  FALSE
        }

It is important to note that the effect works from the point of view of the
player who *sees* the information. It cannot depend on what the other player
does (except when using ``DiplRel`` requirements).

.. note::
    Note the ``Local`` range used in the ``DiplRel`` requirement above. This is
    very important: if we had used ``Player``, the effect would be enabled only
    once the player has met every other player in the game (no player satisfies
    ``Never met``).

The possible values for ``NationalIntelligence`` requirements are as follows:

================ ===
``Culture``      The amount of culture accumulated by a player. Not yet shown in
                 the user interface.
``Diplomacy``    Diplomatic agreements such as Peace, Alliance, and shared
                 vision.
``Infra Points`` Infrastructure points. Not yet shown in the user interface.
``Gold``         The amount of gold in the treasury.
``Government``   The other player's government.
``History``      The history accumulated by a player. Not yet shown in the user
                 interface.
``Multipliers``  The value of multipliers selected by the other player. Not yet
                 shown in the user interface.
``Mood``         For an AI player, whether it is currently farming or building
                 up an army. Not yet shown in the user interface.
``Score``        The game score.
``Tax Rates``    The rates of gold, science and luxury in the national budget.
``Techs``        The techs discovered by a player as well as the current
                 research.
``Wonders``      The list of wonders owned by the player (see also the
                 ``Wonder_Visible`` effect).
================ ===

Compatibility
=============

For legacy rulesets, the traditional rules are added automatically if the
``Nation_Intelligence`` option is not requested in ``effects.ruleset``. They are
rather complicated; to facilitate porting rulesets, we provide a ready-made file
implementing them. It can be imported directly from within ``effects.ruleset``:

.. code-block:: ini

    *include "default/nation_intelligence_effects.ruleset"

Most shipped rulesets use this technique.
