.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: improvement

Effects
*******

The :file:`effects.ruleset` file contains all effects in play in a Freeciv21 ruleset. They have the following
form (this is perhaps the most complicated example we could find):

.. code-block:: ini

    [effect_hydro_plant]
    type  = "Output_Bonus"
    value = 25
    reqs  =
        { "type", "name", "range", "present", "quiet"
          "Building", "Factory", "City", TRUE, FALSE
          "Building", "Hydro Plant", "City", TRUE, FALSE
          "OutputType", "Shield", "Local", TRUE, TRUE
          "Building", "Hoover Dam", "Player", FALSE, FALSE
          "Building", "Nuclear Plant", "City", FALSE, FALSE
        }


The text in the brackets is the entry name, which just has to be unique, but is otherwise not used. The
``type`` field tells Freeciv21 which effect you are defining.  The ``value`` is the effect's value, which
depends on which effect it is. The ``reqs`` table contain a list of requirements for this effect being in
effect. You need to satisfy all requirements listed here for this effect to take effect in the game.
Requirements with ``present = TRUE`` must be present, those with ``present = FALSE`` must not be present.

Value is integral b parameter for many effects (must be in the range -32767 to 32767).

Requirement range may be one of: ``None``, ``Local``, ``CAdjacent`` (Cardinally Adjacent), ``Adjacent``,
``City``, ``Continent``, ``Player``, ``Allied``, ``World``. Some requirement types may only work at certain
ranges. This is described below. In particular, at present, ``Continent`` effects can affect only cities and
units in cities.

A requirement may have a ``survives`` field, and if this ``TRUE``, the effect survives destruction. This is
supported for only a few conditions and ranges: wonders (at world or player range), nations, and advances
(both at world range only).

A requirement may have a ``present`` field, and if this is ``FALSE``, the requirement is negated (the
condition must not be true for the req to be met).

A requirement may have a ``quiet`` field, and if this is ``TRUE``, the help system does not try to
autogenerate text about that requirement. This can be used if the help system's text is unclear or
misleading, or if you want to describe the requirement in your own words. The ``quiet`` field has no effect
on the game rules.


Requirement types and supported ranges
======================================

.. _Effect Flags:
.. table:: Effect Flags
  :widths: auto
  :align: left

  ======================== ================
  Requirement              Supported ranges
  ======================== ================
  ``Tech``                 ``World``, ``Alliance``, ``Team``, ``Player``
  ``TechFlag``             ``World``, ``Alliance``, ``Team``, ``Player``
  ``MinTechs``             ``World``, ``Player``
  ``Achievement``          ``World``, ``Alliance``, ``Team``, ``Player``
  ``Gov``                  ``Player``
  ``Building``             ``World``, ``Alliance``, ``Team``, ``Player``, ``Continent``, ``Traderoute``, ``City``, ``Local``
  ``BuildingGenus``        ``Local``
  ``Extra``                ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``BaseFlag``             ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``RoadFlag``             ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``ExtraFlag``            ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``Terrain``              ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``Good``                 ``City``
  ``UnitType``             ``Local``
  ``UnitFlag``             ``Local``
  ``UnitClass``            ``Local``
  ``UnitClassFlag``        ``Local``
  ``Nation``               ``World``, ``Alliance``, ``Team``, ``Player``
  ``NationGroup``          ``World``, ``Alliance``, ``Team``, ``Player``
  ``Nationality``          ``Traderoute``, ``City``
  ``DiplRel``              ``World``, ``Alliance``, ``Team``, ``Player``, ``Local``
  ``Action``               ``Local``
  ``OutputType``           ``Local``
  ``Specialist``           ``Local``
  ``MinYear``              ``World``
  ``MinCalFrag``           ``World``
  ``Topology``             ``World``
  ``ServerSetting``        ``World``
  ``Age`` (of unit)        ``Local``
  ``Age`` (of city)        ``City``
  ``Age`` (of player)      ``Player``
  ``MinSize``              ``Traderoute``, ``City``
  ``MinCulture``           ``World``, ``Alliance``, ``Team``, ``Player``, ``Traderoute``, ``City``
  ``MinForeignPct``        ``Traderoute``, ``City``
  ``AI``                   ``Player``
  ``MaxUnitsOnTile``       ``Local``, ``Adjacent``, ``CAdjacent``
  ``TerrainClass``         ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``TerrainFlag``          ``Local``, ``Adjacent``, ``CAdjacent``, ``Traderoute``, ``City``
  ``TerrainAlter``         ``Local``
  ``CityTile``             ``Local``, ``Adjacent``, ``CAdjacent``
  ``CityStatus``           ``Traderoute``, ``City``
  ``Style``                ``Player``
  ``UnitState``            ``Local``
  ``Activity``             ``Local``
  ``MinMoveFrags``         ``Local``
  ``MinVeteran``           ``Local``
  ``MinHitPoints``         ``Local``
  ``VisionLayer``          ``Local``
  ``NationalIntelligence`` ``Local``
  ======================== ================

.. raw:: html

    <p>&nbsp;</p>

* ``MinSize`` is the minimum size of a city required.
* :term:`AI` is the :term:`AI` player difficulty level.
* ``TerrainClass`` is either "Land" or "Oceanic".
* ``CityTile`` is either "Center" (city center) or "Claimed" (owned).
* ``CityStatus`` is "OwnedByOriginal".
* ``DiplRel`` is a diplomatic relationship as shown in
  :ref:`nations view <game-manual-nations-and-diplomacy-view>`.
* ``MaxUnitsOnTile`` is about the number of units present on a tile.
* ``UnitState`` is "Transported", "Transporting", "OnNativeTile", "OnLivableTile", "InNativeExtra",
  "OnDomesticTile", "MovedThisTurn", or "HasHomeCity".
* ``Activity`` is "Idle", "Pollution", "Mine", "Irrigate", "Fortified", "Fortress", "Sentry", "Pillage",
  "Goto", "Explore", "Transform", "Fortifying", "Fallout", "Base", "Road", "Convert", "Cultivate", or "Plant".
* ``MinMoveFrags`` is the minimum move fragments (:term:`MP`) the unit must have left.
* ``MinCalFrag`` is the minimum sub-year division the calendar must have reached, if enabled (see
  ``[calendar].fragments`` in :file:`game.ruleset`).
* ``Nationality`` is fulfilled by any citizens of the given nationality present in the city.
* ``ServerSetting`` is if a Boolean server setting is enabled. The setting must be visible to all players and
  affect the game rules.

Details about requirement types
===============================

The "DiplRel" requirement type
------------------------------

Look for the diplomatic relationship "Never met", "War", "Cease-fire", "Armistice", "Peace", "Alliance",
"Team", "Gives shared vision", "Receives shared vision", "Hosts embassy", "Has embassy", "Hosts real
embassy" (not from an effect), "Has real embassy", "Has Casus Belli" (reason for war), "Provided Casus
Belli" or "Is foreign".

A ``DiplRel`` is considered fulfilled for the range:

* "world" if some player in the world has the specified diplomatic relationship to some other living player.
* "player" if the player has the specified diplomatic relationship to some other living player.
* "local" if the first player has the specified relationship to the second player. Example: When testing a
  build requirement for an extra the first player is the owner of the unit and the second player the owner
  of the terrain the extra is built on.

Only the exact relationship required fulfills it. Example: An Alliance or an Armistice agreement will not
fulfill a "Peace" requirement.

It is possible to create a requirement that in some situations will not have a player to check. In those cases
the requirement will always be considered unfulfilled. This applies to both present and not present
requirements. The ranges "Alliance", "Team", "Player" and "Local" needs a player. The Local range also needs
the player the first player's relationship is to.

Example: The requirements below are about the relationship to the owner of a tile. The table shows in what
situations a requirement is fulfilled.

.. _effect-deplrel-example:
.. table:: DiplRel Exampe
  :align: left

  +---------------------------------------------+----------+-----------+---------+
  |                                             | Fulfilled when the tile is     |
  | Requirement                                 +----------+-----------+---------+
  |                                             | Domestic | Unclaimed | Foreign |
  +=============================================+==========+===========+=========+
  | ``"DiplRel", "Is foreign", "Local", TRUE``  | no       | no        | yes     |
  +---------------------------------------------+----------+-----------+---------+
  | ``"DiplRel", "Is foreign", "Local", FALSE`` | yes      | no        | no      |
  +---------------------------------------------+----------+-----------+---------+

.. raw:: html

    <p>&nbsp;</p>

The "MaxUnitsOnTile" requirement type
-------------------------------------

Check the number of units present on a tile. Is TRUE if no more than the specified number of units are
present on a single tile.

.. tip::
  By using negation ("not present") it is possible to check if a tile has more than the given numbers.
  It is possible to combine a negated and a non negated requirement to specify a range.

The "UnitState" requirement type
--------------------------------

Transported
    is fulfilled if the unit is transported by another unit.

Transporting
    is fulfilled if the unit is transporting another unit.

OnNativeTile
    is fulfilled if the unit is on a tile with native terrain or with a native Extra. Does not care about
    details like cities and safe tiles.

OnLivableTile
    is fulfilled if the unit is on a tile where it can exist outside of a transport.

InNativeExtra
    is fulfilled if the unit is on a tile with an extra native to it.

OnDomesticTile
    is fulfilled if the unit is on a tile owned by its player.

MovedThisTurn
    is fulfilled if the unit has moved this turn.

HasHomeCity
    is fulfilled if the unit has a home city.

The "NationalIntelligence" requirement type
-------------------------------------------

This is only used with the :doc:`Nation_Intelligence effect <Effects/Nation_Intelligence>`.

Effect types
------------

.. _effect-tech-parasite:

Tech_Parasite
    Gain any advance known already by AMOUNT number of other teams, if ``team_pooled_research`` is enabled,
    or AMOUNT number of other players otherwise.

.. note::
   If you have two such effects, they add up (the number of players required to gain an advance is increased).

.. _effect-airlift:

Airlift
    Allow airlift to/from a city. The value tells how many units per turn can be airlifted, unless server
    setting ``airlifttingstyle`` sets the number unlimited for either source or destination city. If airlifts
    are set to unlimited, they are enabled by any positive value of this effect.

.. _effect-any-government:

Any_Government
    Allow changing to any form of government regardless of tech prerequisites.

.. _effect-capital-city:

Capital_City
    The city with a positive value is a capital city. Player's city with highest ``Capital_City`` value (or
    random among those with equal positive value) is the primary capital. Cities with lesser positive value
    are secondary capitals.

.. _effect-gov-center:

Gov_Center
    The city with this effect is governmental center. Corruption and waste depends on distance to nearest
    such city.

.. _effect-enable-nuke:

Enable_Nuke
    Allows the production of nuclear weapons.

.. _effect-enable-space:

Enable_Space
    Allows the production of space components.

.. _effect-specialist-output:

Specialist_Output
    Specify what outputs a specialist is producing. Should be used with an ``OutputType`` requirement.

.. _effect-output-bonus:

Output_Bonus
    City production is increased by AMOUNT percent.

.. _effect-output-bonus-2:

Output_Bonus_2
    City production is increased by AMOUNT percent after :ref:`Output_Bonus <effect-output-bonus>`, so is
    multiplicative with it.

.. _effect-output-add-tile:

Output_Add_Tile
    Add AMOUNT to each worked tile.

.. _effect-output-inc-tile:

Output_Inc_Tile
    Add AMOUNT to each worked tile that already has at least 1 output.

.. _effect-output-per-tile:

Output_Per_Tile
    Increase tile output by AMOUNT percent.

.. _effect-output-tile-punish-pct:

Output_Tile_Punish_Pct
    Reduce the output of a tile by AMOUNT percent. The number of units to remove is rounded down. Applied
    after everything except a city center's minimal output.

.. _effect-output-waste-pct:

Output_Waste_Pct
    Reduce waste by AMOUNT percent.

.. _effect-force-content:

Force_Content
    Make AMOUNT unhappy citizens content. Applied after martial law and unit penalties.

.. _effect-give-imm-tech:

Give_Imm_Tech
    Give AMOUNT techs immediately.

.. _effect-conquest-tech-pct:

Conquest_Tech_Pct
    Percent chance that a player conquering a city learns a tech from the former owner.

.. _effect-groth-food:

Growth_Food
    Saves some food in the granary when a city grows (or shrinks). This effect controls how much food there
    will be in the city's granary after growing, as a percentage of the ``foodbox`` at the new size, provided
    there was sufficient food before growing. This also reduces the ``aqueductloss`` penalty in the same
    fraction.

    .. note::
      This is traditionally used for the :improvement:`Granary`.

.. _effect-growth-surplus-pct:

Growth_Surplus_Pct
    .. versionadded:: 3.1

    How much of the excess food is kept when a city growth, as a percentage. For example, with a value of
    100, a city with a granary full at 18/20 and generating 5 food would start the next turn with 3 bushels
    in its granary. With a value of 50, it would have only 1 bushel.

.. _effect-have-contact:

Have_Contact
    If value > 0, gives contact to all the other players.

.. _effect-have-embassies:

Have_Embassies
    If value > 0, gives an embassy with all the other players owner has ever had contact with.

.. _effect-irrigation-pct:

Irrigation_Pct
    The tile gets value % of its terrain's ``Irrigation_Food_Incr`` bonus
    value.

    .. note::
      This is how irrigation-like extras have an effect.

.. _effect-mining-pct:

Mining_Pct
    The tile gets value % of its terrain's ``Mining_Shield_Incr`` bonus.

    .. note::
      This is how mine-like extras have an effect.

.. _effect-make-content:

Make_Content
    Make AMOUNT unhappy citizens content. Applied before martial law and unit penalties.

.. _effect-make-content-mil:

Make_Content_Mil
    Make AMOUNT unhappy citizens caused by units outside of a city content.

.. _effect-make-content-mil-per:

Make_Content_Mil_Per
    Make AMOUNT per unit of unhappy citizens caused by units outside of a city content.

.. _effect-make-happy:

Make_Happy
    Make AMOUNT citizens happy.

.. _effect-enemy-citizen-unhappy-pct:

Enemy_Citizen_Unhappy_Pct
    There will be one extra unhappy citizen for each value/100 citizens of enemy nationality in the city.

.. _effect-no-anarchy:

No_Anarchy
    No period of anarchy between government changes.

    .. note::
      This also neuters the :ref:`Has_Senate <effect-has-senate>` effect.

.. _effect-nuke-proof:

Nuke_Proof
    City is nuke proof.

.. _effect-pollu-pop-pct:

Pollu_Pop_Pct
    Increases pollution caused by each unit of population by AMOUNT percent (adds to baseline of 100%,
    i.e. 1 pollution per citizen).

.. _effect-pollu-pop-pct-2:

Pollu_Pop_Pct_2
    Increases pollution caused by each unit of population by AMOUNT percent (adds to baseline of 100%,
    i.e. 1 pollution per citizen). This factor is applied after :ref:`Pollu_Pop_Pct <effect-pollu-pop-pct>`,
    so is multiplicative with it.

.. _effect-pollu-prod-pct:

Pollu_Prod_Pct
    Increases pollution caused by shields by AMOUNT percent.

.. _effect-health-pct:

Health_Pct
    Reduces possibility of illness (plague) in a city by AMOUNT percent.

.. _effect-reveal-cities:

Reveal_Cities
    Immediately make all cities known.

.. _effect-reveal-map:

Reveal_Map
    Immediately make entire map known.

.. _effect-border-vision:

Border_Vision
    Give vision on all tiles within the player's borders. Happens during turn change. Does nothing unless the
    borders setting is set to "Enabled". You can lock it if border vision rules are important to your ruleset.

.. _effect-incite-cost-pct:

Incite_Cost_Pct
    Increases revolt cost by AMOUNT percent.

.. _effect-unit-bribe-cost-pct:

Unit_Bribe_Cost_Pct
    Increases unit bribe cost by AMOUNT percent. Requirements are from the point of view of the target unit,
    not the briber.

.. _effect-max-stolen-gold-pm:

Max_Stolen_Gold_Pm
    The upper limit on the permille of the players gold that may be stolen by a unit doing the
    :ref:`Steal Gold <action-steal-gold>` and the :ref:`Steal Gold Escape <action-steal-gold-escape>` actions.
    Evaluated against the city stolen from.

.. _effect-thiefs-share-pm:

Thiefs_Share_Pm
    The permille of the gold stolen by a unit doing the :ref:`Steal Gold <action-steal-gold>` and the
    :ref:`Steal Gold Escape <action-steal-gold-escape>` actions that is lost before it reaches the player
    ordering it. Evaluated against the actor unit.

.. _effect-maps-stolen-pct:

Maps_Stolen_Pct
    The percent probability that the map of a tile is stolen in the actions
    :ref:`Steal Maps <action-steal-maps>` and :ref:`Steal Maps Escape <action-steal-maps-escape>`. DiplRel
    reqs are unit owner to city owner. Requirements evaluated against tile or city not supported.
    Default value: 100%

.. _effect-illegal-action-move-cost:

Illegal_Action_Move_Cost
    The number of move fragments lost when the player tries to do an action that turns out to be illegal.
    Only applied when the player was not aware that the action was illegal and its illegality therefore
    reveals new information.

.. _effect-illegal-action-hp-cost:

Illegal_Action_HP_Cost
    The number of hit points (:term:`HP`) lost when the player tries to do an action that turns out to be
    illegal. Only applied when the player was not aware that the action was illegal and its illegality
    therefore reveals new information. Can kill the unit. If the action always causes the actor unit to end up
    at the target tile two consolation prizes are given. An area with the radius of the actor unit's
    ``vision_radius_sq`` tiles is revealed. The player may also get contact with the owners of units and cites
    adjacent to the target tile.

.. _effect-action-success-actor-move-cost:

Action_Success_Actor_Move_Cost
    The number of move fragments lost when a unit successfully performs an action. Evaluated and done after
    the action is successfully completed. Added on top of any movement fragments the action itself subtracts.

.. _effect-action-success-target-move-cost:

Action_Success_Target_Move_Cost
    The number of move fragments subtracted from a unit when someone successfully performs an action on it.
    Evaluated and done after the action is successfully completed. Added on top of any movement fragments the
    action itself subtracts. Only supported for actions that targets an individual unit.

.. _effect-casus-belli-caught:

Casus_Belli_Caught
    Checked when a player is caught trying to do an action. Will cause an incident with the intended victim
    player if the value is 1 or higher. The incident gives the intended victim a casus belli against the
    actor player. A value of 1000 or higher is international outrage. International outrage gives every other
    player a casus belli against the actor.

.. _effect-casus-belli-success:

Casus_Belli_Success
    Checked when a player does an action to another player. Will cause an incident with the intended victim
    player if the value is 1 or higher. The incident gives the intended victim a casus belli against the actor
    player. A value of 1000 or higher is international outrage. International outrage gives every other player
    a casus belli against the actor.

.. _effect-casus-belli-complete:

Casus_Belli_Complete
    Checked when a player completes an action that takes several turns against another player. Will cause an
    incident with the intended victim player if the value is 1 or higher. The incident gives the intended
    victim a casus belli against the actor player. A value of 1000 or higher is international outrage.
    International outrage gives every other player a casus belli against the actor. Only
    :ref:`Pillage <action-pillage>` is currently supported.

.. _effect-action-odds-pct:

Action_Odds_Pct
    Modifies the odds of an action being successful. Some actions have a risk: the actor may get caught
    before he can perform it. This effect modifies the actor's odds. A positive value helps him. A negative
    value  makes it more probable that he will get caught. Currently supports the actions
    :ref:`Incite City <action-incite-city>`, :ref:`Incite City Escape <action-incite-city-escape>`,
    :ref:`Steal Gold <action-steal-gold>`, :ref:`Steal Gold Escape <action-steal-gold-escape>`,
    :ref:`Steal Maps <action-steal-maps>`, :ref:`Steal Maps Escape <action-steal-maps-escape>`,
    :ref:`Suitcase Nuke <action-suitcase-nuke>`, :ref:`Suitcase Nuke Escape <action-suitcase-nuke-escape>`,
    :ref:`Sabotage City <action-sabotage-city>`, :ref:`Sabotage City Escape <action-sabotage-city-escape>`,
    :ref:`Targeted Sabotage City <action-targeted-sabotage-city>`,
    :ref:`Targeted Sabotage City Escape <action-targeted-sabotage-city-escape>`,
    :ref:`Sabotage City Production <action-sabotage-city-production>`,
    :ref:`Sabotage City Production Escape <action-sabotage-city-production-escape>`,
    :ref:`Surgical Strike Building <action-surgical-strike-building>`,
    :ref:`Surgical Strike Production <action-surgical-strike-production>`,
    :ref:`Steal Tech <action-steal-tech>`,
    :ref:`Steal Tech Escape Expected <action-steal-tech-escape-expected>`,
    :ref:`Targeted Steal Tech <action-targeted-steal-tech>`,
    :ref:`Targeted Steal Tech Escape Expected <action-targeted-steal-tech-escape-expected>`, and
    :ref:`Spread Plague <action-spread-plague>`.

.. _effect-size-adj:

Size_Adj
    Increase maximum size of a city by AMOUNT.

.. _effect-size-unlimit:

Size_Unlimit
    Make the size of a city unlimited.

.. _effect-unit-slots:

Unit_Slots
    Number of unit slots city can have units in. New units cannot be built, nor can homecity be changed so
    that maintained units would use more slots than this. Single unit does not necessarily use single slot -
    that is defined separately for each unit type.

.. _effect-spaceship:

SS_Structural, SS_Component, SS_Module
    A part of a spaceship; this is a "Local" ranged effect. It (for now) applies to improvements which
    cannot be built unless :ref:`Enable_Space <effect-enable-space>` is felt. Buildings which have this effect
    should probably not be given any other effects.

.. _effect-spy-resistant:

Spy_Resistant
    In diplomatic combat defending diplomatic units will get an AMOUNT percent bonus. All ``Spy_Resistant``'s
    are summed before being applied.

.. _effect-building-saboteur-resistant:

Building_Saboteur_Resistant
    If a spy specifies a target for sabotage, then she has an AMOUNT percent chance to fail.

.. _effect-stealings-ignore:

Stealings_Ignore
    When determining how difficult it is to steal a tech from enemy, AMOUNT previous times tech has been
    stolen from the city is ignored. Negative AMOUNT means that number of times tech has already been stolen
    from target city does not affect current attempt at all. With this effect it is possible to allow
    diplomats to steal tech multiple times from the same city, or make it easier for spies.

.. _effect-move-bonus:

Move_Bonus
    Add AMOUNT movement to units. Use "UnitClass" requirement with range of "Local" to give it a specific
    class of units only.

.. _effect-unit-no-lose-pop:

Unit_No_Lose_Pop
    No population lost when a city's defender is lost.

.. _effect-unit-recover:

Unit_Recover
    Units recover AMOUNT extra hitpoints (:term:`HP`) per turn.

.. _effect-upgrade-unit:

Upgrade_Unit
    Upgrade AMOUNT obsolete units per turn.

.. _effect-upkeep-free:

Upkeep_Free
    Improvements with AMOUNT or less upkeep cost become free to upkeep (others are unaffected).

.. _effect-tech-upkeep-free:

Tech_Upkeep_Free
    If this value is greater than 0, the tech upkeep is reduced by this value. For tech upkeep style
    "Basic" this is total reduction, for tech upkeep style "Cities" this reduction is applied to every city.

.. _effect-no-unhappy:

No_Unhappy
    No citizens in the city are ever unhappy.

.. _effect-veteran-build:

Veteran_Build
    Increases the veteran class of newly created units of this type. The total AMOUNT determines the veteran
    class (clipped at the maximum for the unit).

.. _effect-veteran-combat:

Veteran_Combat
    Increases the chance of units of this type becoming veteran after combat by AMOUNT percent.

.. _effect-combat-rounds:

Combat_Rounds
    Maximum number of rounds combat lasts. Unit is the attacker. Zero and negative values mean that combat
    continues until either side dies.

.. _effect-hp-regen:

HP_Regen
    Units that do not move recover AMOUNT percentage (rounded up) of their full hitpoints (:term:`HP`) per
    turn.

    .. note::
      This effect is added automatically to implement :term:`HP` recovery in cities. This behavior can be
      turned off by requiring the ``+HP_Regen_Min`` option in :file:`effects.ruleset`.

.. _effect-hp-regen-min:

HP_Regen_Min
    Lower limit on :ref:`HP_Regen <effect-hp-regen>`. That is, the recovery percentage is the larger of
    :ref:`HP_Regen <effect-hp-regen>` and ``HP_Regen_Min``.

    .. note::
      This effect is added automatically to implement HP recovery in cities. This behavior can be turned
      off by requiring the ``+HP_Regen_Min`` option in :file:`effects.ruleset`.

.. _effect-city-vision-radius-sq:

City_Vision_Radius_Sq
    Increase city vision radius in squared distance by AMOUNT tiles.

    .. note::
        This effect is added automatically for VisionLayers other than Main, with a value of 2, and a
        VisionLayer=Main requirement is added to any existing instances of this effect. This behaviour can be
        turned off by requiring the ``+VisionLayer`` option in :file:`effects.ruleset`, allowing you to use
        VisionLayer requirements to specify which layer (Main, Stealth or Subsurface) the effect applies to.

.. _effect-unit-vision-radius-sq:

Unit_Vision_Radius_Sq
    Increase unit vision radius in squared distance by AMOUNT tiles.

    .. note::
        A VisionLayer=Main requirement is added automatically to any existing instances of this effect. This
        behaviour can be turned off by requiring the ``+VisionLayer`` option in :file:`effects.ruleset`,
        allowing you to use VisionLayer requirements to specify which layer (Main, Stealth or Subsurface) the
        effect applies to.

.. _effect-defend-bonus:

Defend_Bonus
    Increases defensive bonuses of units. Any unit requirements on this effect will be applied to the
    _attacking_ unit. Attackers with "BadWallAttacker" flag will have their firepower set to 1.

.. _effect-attack-bonus:

Attack_Bonus
    Increases offensive bonuses of units. Unit requirements on this effect are the attacking unit itself.

.. _effect-fortify-defense-bonus:

Fortify_Defense_Bonus
    Percentage defense bonus multiplicative with :ref:`Defend_Bonus <effect-defend-bonus>`, usually given to
    :ref:`fortified <action-fortify>` units. Unit requirements on this effect are the defending unit itself.

.. _effect-gain-ai-love:

Gain_AI_Love
    Gain AMOUNT points of "AI love" with :term:`AI`'s.

.. _effect-turn-years:

Turn_Years
    Year advances by AMOUNT each turn unless Slow_Down_Timeline causes it to be less.

.. _effect-turn-fragments:

Turn_Fragments
    Year fragments advance by AMOUNT each turn.

.. _effect-slow-down-timeline:

Slow_Down_Timeline
    Slow down the timeline based on the AMOUNT. If AMOUNT >= 3 the timeline will be max 1 year/turn; with
    AMOUNT == 2 it is max 2 years/turn; with AMOUNT == 1 it is max 5 years/turn; with AMOUNT <= 0 the
    timeline is unaffected. The effect will be ignored if game.spacerace isn't set.

.. _effect-civil-war-chance:

Civil_War_Chance
    Base chance in per cent of a nation being split by civil war when its capital is captured is increased
    by this AMOUNT. This percentage is in- creased by 5 for each city in civil disorder and reduced by 5 for
    each one celebrating.

.. _effect-city-unhappy-size:

City_Unhappy_Size
    The maximum number of citizens in each city that are naturally content, In larger cities, new citizens
    above this limit start out unhappy. Before :ref:`Empire_Size_Base <effect-empire-size-base>` and
    :ref:`Empire_Size_Step <effect-empire-size-step>` are applied.

.. _effect-empire-size-base:

Empire_Size_Base
    Once your civilization has more cities than the value of this effect, each city gets one more unhappy
    citizen. If the sum of this effect and :ref:`Empire_Size_Step <effect-empire-size-step>` is zero, there is
    no such penalty.

.. _effect-empire-size-step:

Empire_Size_Step
    After your civilization reaches :ref:`Empire_Size_Base <effect-empire-size-base>` size, it gets one more
    unhappy citizen for each AMOUNT of cities it gets above that. Set to zero to disable. You can use
    ``Empire_Size_Step`` even if  :ref:`Empire_Size_Base <effect-empire-size-base>` is zero.

.. _effect-map-rates:

Max_Rates
    The maximum setting for each tax rate is AMOUNT.

.. _effect-martial-law-each:

Martial_Law_Each
    The AMOUNT of citizens pacified by each military unit giving martial law.

.. _effect-martial-law-max:

Martial_Law_Max
    The maximum AMOUNT of units that will give martial law in city.

.. _effect-rapture-grow:

Rapture_Grow
    Can rapture grow cities.

.. _effect-revolution-unhappiness:

Revolution_Unhappiness
    If value is greater than zero, it tells how many turns citizens will tolerate city disorder before
    government falls. If value is zero, government never falls.

.. _effect-has-senate:

Has_Senate
    Has a senate that prevents declarations of war in most cases.

.. _effect-inspire-partisans:

Inspire_Partisans
    Partisan units (defined in :file:`units.ruleset`) may spring up when this player's cities are taken.

.. _effect-happiness-to-gold:

Happiness_To_Gold
    Make all :ref:`Make_Content <effect-make-content>` and :ref:`Force_Content <effect-force-content>` effects
    instead generate gold.

.. _effect-max-trade-routes:

Max_Trade_Routes
    Number of trade routes that city can establish. This is forced on trade route creation only. Existing
    trade routes are never removed due to reduction of effect value. This is to avoid micro-management, need
    to create same trade routes again after their max number has been temporarily down.

.. _effect-fanatics:

Fanatics
    Units with "Fanatics" flag incur no upkeep.

.. _effect-no-diplomacy:

No_Diplomacy
    Cannot use any diplomacy.

.. _effect-not-tech-source:

Not_Tech_Source
    Tech cannot be received from this player by any means.

.. _effect-trade-revenue-bonus:

Trade_Revenue_Bonus
    One time trade revenue bonus is multiplied by :math:`2^{(\texttt{amount} \div 1000)}`. The AMOUNT value is
    taken from the caravan's home city.

.. _effect-trade-revenue-exponent:

Trade_Revenue_Exponent
    One time trade revenue bonus is raised to the :math:`1 + \frac{\texttt{amount}}{1000}` power.
    This is applied before :ref:`Trade_Revenue_Bonus <effect-trade-revenue-bonus>`.

.. _effect-traderoute-pct:

Traderoute_Pct
    Percentage bonus for trade from traderoutes. This bonus applies after the value of the traderoute is
    already calculated. It affects one end of the traderoute only.

.. _effect-unhappy-factor:

Unhappy_Factor
    Multiply unhappy unit upkeep by AMOUNT.

.. _effect-upkeep-factor:

Upkeep_Factor
    Multiply unit upkeep by AMOUNT.

.. _effect-unit-upkeep-free-per-city:

Unit_Upkeep_Free_Per_City
    In each city unit upkeep is deducted by this AMOUNT. As usual, you can use with "OutputType" requirement
    to specify which kind of upkeep this should be.

.. _effect-output-waste:

Output_Waste
    Base AMOUNT in percentage that each city has in waste. Waste can be used with any output type, use an
    "OutputType" requirement to specify which.

.. _effect-output-waste-by-distance:

Output_Waste_By_Distance
    For each tile in real distance that a city is from nearest Government Center, it gets
    :math:`\frac{\texttt{amount}}{100}` of extra waste.

.. _effect-output-waste-by-rel-distance:

Output_Waste_By_Rel_Distance
    City gets extra waste based on distance to nearest Government Center, relative to world size. The AMOUNT
    of this extra waste is
    :math:`\frac{\texttt{distance}\,\times\,\texttt{amount}}{100\,\times\,\texttt{max\_distance}}`

.. _effect-output-penalty-tile:

Output_Penalty_Tile
    When a tile yields more output than AMOUNT, it gets a penalty of -1.

.. _effect-output-inc-tile-celebrate:

Output_Inc_Tile_Celebrate
    Tiles get AMOUNT extra output when city working them is celebrating.

.. _effect-upgrade-price-pct:

Upgrade_Price_Pct
    Increases unit upgrade cost by AMOUNT percent. This effect works at player level. You cannot adjust
    upgrade costs for certain unit type or for units upgraded in certain city.

.. _effect-unit-shield-value-pct:

Unit_Shield_Value_Pct
    Increase the unit's value in shields by AMOUNT percent. When this effect is used to determine how many
    shields the player gets for the actions :ref:`Recycle Unit <action-recycle-unit>` and
    :ref:`Help Wonder <action-help-wonder>` it gets access to unit state. When it is used to influence the
    gold cost of :ref:`Upgrade Unit <action-upgrade-unit>` it only has access to unit type.

.. _effect-retire-pct:

Retire_Pct
    The chance that unit gets retired (removed) when turn changes. Retirement only happens if there are no
    enemy units or cities within a few tiles. This exists mainly to implement barbarian behavior.

.. _effect-visible-wall:

Visible_Wall
    Instruct the game to show specific buildings version of the city graphics. Zero or below are considered
    normal city graphics.

.. _effect-tech-cost-factor:

Tech_Cost_Factor
    Factor for research costs.

.. _effect-building-build-cost-pct:

Building_Build_Cost_Pct
    Percentage added to building building cost.

.. _effect-building-buy-cost-pct:

Building_Buy_Cost_Pct
    Percentage added to building buy cost.

.. _effect-unit-build-cost-pct:

Unit_Build_Cost_Pct
    Percentage added to unit building cost.

.. _effect-unit-buy-cost-pct:

Unit_Buy_Cost_Pct
    Percentage added to unit buy cost.

.. _effect-nuke-improvement-pct:

Nuke_Improvement_Pct
    Percentage chance that an improvement would be destroyed while nuking the city. Only regular improvements
    (not wonders) are affected. Improvements protected from Sabotage (Eg: :improvement:`City Walls`) are not
    affected.

.. _effect-nuke-infrastructure-pct:

Nuke_Infrastructure_Pct
    Percentage chance that an extra located within a nuclear blast area gets destroyed. Only "Infra" extras
    such as roads and irrigation are affected, and ``rmreqs`` are also checked. Note that an `Extra`
    requirement will match any extra on the tile, not only the one considered for destruction.

.. _effect-shield2gold-factor:

Shield2Gold_Factor
    Factor in percent for the conversion of unit shield upkeep to gold upkeep. A value of 200 would transfer
    1 shield upkeep to 2 gold upkeep. The range of this effect must be player or world. Note that only units
    with the ``Shield2Gold`` flag will be affected by this.

.. _effect-tile-workable:

Tile_Workable
    If value > 0, city can work target tile.

.. _effect-migration-pct:

Migration_Pct
    Increase the calculated migration score for the a city by AMOUNT in percent.

.. _effect-city-radius-sq:

City_Radius_Sq
    Increase the squared city radius by AMOUNT. Currently, this can only usefully have "MinSize", "Building",
    or "Tech" requirements.

.. _effect-city-build-slots:

City_Build_Slots
    Increase the number of units with no population cost a city can build in a turn, if there are enough
    shields.

.. _effect-city-image:

City_Image
    The index for the city image of the given city style.

.. _effect-victory:

Victory
    Positive value means that player wins the game.

.. _effect-performance:

Performance
    Value is how much performance type culture city produces.

.. _effect-history:

History
    Value is how much history type (cumulative) culture city produces.

.. _effect-national-performance:

National_Performance
    Value is how much performance type culture, not tied to any specific city, the nation produces.

.. _effect-national-history:

National_History
    Value is how much history type (cumulative) culture, not tied to any any specific city, the nation
    produces.

.. _effect-infra-points:

Infra_Points
    City increases owner's infra points by value each turn. If overall points are negative after all cities
    have been processed, they are set to 0.

.. _effect-bombard-limit-pct:

Bombard_Limit_Pct
    Bombardment may only reduce units to AMOUNT percent (rounded up) of their total hitpoints (:term:`HP`).
    Unit requirements on this effect are the defending unit itself.

    .. note::
        This effect is added automatically with a value of 1 and no reqs. This behavior can be turned
        off by requiring the ``+Bombard_Limit_Pct`` option in :file:`effects.ruleset`.

.. _effect-wonder-visible:

Wonder_Visible
    If the value of this effect is larger than 0 for a small wonder, the wonder will be visible to all
    players and reported in the intelligence panel in the
    :ref:`nations view <game-manual-nations-and-diplomacy-view>`. Great wonders are always visible to everyone
    through the wonders report (F7). When a small wonder is lost (for instance, because the city it is in is
    lost or some of its requirements become invalid), it also becomes visible to everyone (this is a
    limitation of the server).

    .. note::
        This effect is added automatically with a value of 1 for great wonders (since they are shown in the
        wonders report anyway). This behavior can be turned off by requiring the ``+Wonder_Visible`` option
        in :file:`effects.ruleset`.

.. _effect-nation-intelligence:

Nation_Intelligence
    Controls the information available in the Nations View.
    :doc:`See the detailed description. <Effects/Nation_Intelligence>`

    .. toctree::
        :hidden:

        Effects/Nation_Intelligence.rst
