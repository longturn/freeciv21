Effects
*******

The :file:`effects.ruleset` file contains all effects in play in a Freeciv21 ruleset. They have the following
form (this is perhaps the most complicated example I could find):

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

Value is integral amount parameter for many effects (must be in the range -32767 to 32767).

Requirement range may be one of: ``None``, ``Local``, ``CAdjacent`` (Cardinally Adjacent), ``Adjacent``,
``City``, ``Continent``, ``Player``, ``Allied``, ``World``. Some requirement types may only work at certain
ranges; this is described below. In particular, at present, ``Continent`` effects can affect only cities and
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

* MinSize is the minimum size of a city required.
* AI is ai player difficulty level.
* TerrainClass is either "Land" or "Oceanic".
* CityTile is either "Center" (city center) or "Claimed" (owned).
* CityStatus is "OwnedByOriginal"
* DiplRel is a diplomatic relationship.
* MaxUnitsOnTile is about the number of units present on a tile.
* UnitState is "Transported", "Transporting", "OnNativeTile", "OnLivableTile", "InNativeExtra",
  "OnDomesticTile", "MovedThisTurn" or "HasHomeCity".
* Activity is "Idle", "Pollution", "Mine", "Irrigate", "Fortified", "Fortress", "Sentry", "Pillage",
  "Goto", "Explore", "Transform", "Fortifying", "Fallout", "Base", "Road", "Convert", "Cultivate", or "Plant".
* MinMoveFrags is the minimum move fragments the unit must have left.
* MinCalFrag is the minimum sub-year division the calendar must have reached, if enabled (see
  [calendar].fragments in game.ruleset).
* Nationality is fulfilled by any citizens of the given nationality present in the city.
* ServerSetting is if a Boolean server setting is enabled. The setting must be visible to all players and
  affect the game rules.

Details about requirement types
===============================

The DiplRel requirement type
----------------------------

Look for the diplomatic relationship "Never met", "War", "Cease-fire", "Armistice", "Peace", "Alliance",
"Team", "Gives shared vision", "Receives shared vision", "Hosts embassy", "Has embassy", "Hosts real
embassy" (not from an effect), "Has real embassy", "Has Casus Belli" (reason for war), "Provided Casus
Belli" or "Is foreign".

A DiplRel is considered fulfilled for the range:

* world if some player in the world has the specified diplomatic relationship to some other living player.
* player if the player has the specified diplomatic relationship to some other living player.
* local if the first player has the specified relationship to the second player. Example: When testing a
  build requirement for an extra the first player is the owner of the unit and the second player the owner
  of the terrain the extra is built on.

Only the exact relationship required fulfills it. Example: An alliance or an armistice agreement won't
fulfill a "Peace" requirement.

It is possible to create a requirement that in some situations won't have a player to check. In those cases
the requirement will always be considered unfulfilled. This applies to both present and not present
requirements. The ranges Alliance, Team, Player and Local needs a player. The Local range also needs the
player the first player's relationship is to.

Example: The requirements below are about the relationship to the owner of a tile. The table shows in what
situations a requirement is fulfilled.

+---------------------------------------------+----------+-----------+---------+
|                                             | Fulfilled when the tile is     |
| Requirement                                 +----------+-----------+---------+
|                                             | Domestic | Unclaimed | Foreign |
+=============================================+==========+===========+=========+
| ``"DiplRel", "Is foreign", "Local", TRUE``  | no       | no        | yes     |
+---------------------------------------------+----------+-----------+---------+
| ``"DiplRel", "Is foreign", "Local", FALSE`` | yes      | no        | no      |
+---------------------------------------------+----------+-----------+---------+

The MaxUnitsOnTile requirement type
-----------------------------------

Check the number of units present on a tile. Is true if no more than the specified number of units are
present on a single tile.

.. tip:: By using negation ("not present") it is possible to check if a tile has more than the given numbers.
    It is possible to combine a negated and a non negated requirement to specify a range.

The UnitState requirement type
------------------------------

Transported
    is fulfilled if the unit is transported by another unit.

Transporting
    is fulfilled if the unit is transporting another unit.

OnNativeTile
    is fulfilled if the unit is on a tile with native terrain or with a native Extra. Doesn't care about
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

The NationalIntelligence requirement type
-----------------------------------------

This is only used with the :doc:`Nation_Intelligence effect <Effects/Nation_Intelligence>`.

Effect types
------------

Tech_Parasite
    Gain any advance known already by amount number of other teams, if team_pooled_research is enabled,
    or amount number of other players otherwise.

.. note::
   If you have two such effects, they add up (the number of players required to gain an advance is increased).

Airlift
    Allow airlift to/from a city. The value tells how many units per turn can be airlifted, unless server
    setting 'airlifttingstyle' sets the number unlimited for either source or destination city. If airlifts
    are set to unlimited, they are enabled by any positive value of this effect.

Any_Government
    Allow changing to any form of government regardless of tech prerequisites.

Capital_City
    The city with positive value is a capital city. Player's city with highest Capital_City value (or
    random among those with equal positive value) is the primary capital. Cities with lesser positive value
    are secondary capitals.

Gov_Center
    The city with this effect is governmental center. Corruption and waste depends on distance to nearest
    such city.

Enable_Nuke
    Allows the production of nuclear weapons.

Enable_Space
    Allows the production of space components.

Specialist_Output
    Specify what outputs a specialist is producing. Should be used with an OutputType requirement.

Output_Bonus
    City production is increased by amount percent.

Output_Bonus_2
    City production is increased by amount percent after Output_Bonus, so is multiplicative with it.

Output_Add_Tile
    Add amount to each worked tile.

Output_Inc_Tile
    Add amount to each worked tile that already has at least 1 output.

Output_Per_Tile
    Increase tile output by amount percent.

Output_Tile_Punish_Pct
    Reduce the output of a tile by amount percent. The number of units to remove is rounded down. Applied
    after everything except a city center's minimal output.

Output_Waste_Pct
    Reduce waste by amount percent.

Force_Content
    Make amount' unhappy citizens content. Applied after martial law and unit penalties.

Give_Imm_Tech
    Give amount techs immediately.

Conquest_Tech_Pct
    Percent chance that a player conquering a city learns a tech from the former owner.

Growth_Food
    Food left after cities grow or shrink is amount percent of the capacity of he city's foodbox. This also
    affects the 'aqueductloss' penalty.

Have_Contact
    If value > 0, gives contact to all the other players.

Have_Embassies
    If value > 0, gives an embassy with all the other players owner has ever had contact with.

Irrigation_Pct
    The tile gets value % of its terrain's irrigation_food_incr bonus.

.. note:: This is how irrigation-like extras have an effect.

Mining_Pct
    The tile gets value % of its terrain's mining_shield_incr bonus.


.. note:: This is how mine-like extras have an effect.

Make_Content
    Make amount unhappy citizens content. Applied before martial law and unit penalties.

Make_Content_Mil
    Make amount unhappy citizens caused by units outside of a city content.

Make_Content_Mil_Per
    Make amount per unit of unhappy citizens caused by units outside of a city content.

Make_Happy
    Make amount citizens happy.

Enemy_Citizen_Unhappy_Pct
    There will be one extra unhappy citizen for each value/100 citizens of enemy nationality in the city.

No_Anarchy
    No period of anarchy between government changes.

.. note:: This also neuters the Has_Senate effect.

Nuke_Proof
    City is nuke proof.

Pollu_Pop_Pct
    Increases pollution caused by each unit of population by amount percent (adds to baseline of 100%,
    i.e. 1 pollution per citizen).

Pollu_Pop_Pct_2
    Increases pollution caused by each unit of population by amount percent (adds to baseline of 100%,
    i.e. 1 pollution per citizen). This factor is applied after Pollu_Pop_Pct, so is multiplicative with it.

Pollu_Prod_Pct
    Increases pollution caused by shields by amount percent.

Health_Pct
    Reduces possibility of illness (plague) in a city by amount percent.

Reveal_Cities
    Immediately make all cities known.

Reveal_Map
    Immediately make entire map known.

Border_Vision
    Give vision on all tiles within the player's borders. Happens during turn change. Does nothing unless the
    borders setting is set to "Enabled". You can lock it if border vision rules are important to your ruleset.

Incite_Cost_Pct
    Increases revolt cost by amount percent.

Unit_Bribe_Cost_Pct
    Increases unit bribe cost by amount percent. Requirements are from the point of view of the target unit,
    not the briber.

Max_Stolen_Gold_Pm
    The upper limit on the permille of the players gold that may be stolen by a unit doing the "Steal Gold"
    and the "Steal Gold Escape" actions. Evaluated against the city stolen from.

Thiefs_Share_Pm
    The permille of the gold stolen by a unit doing the "Steal Gold" and the "Steal Gold Escape" actions
    that is lost before it reaches the player ordering it. Evaluated against the actor unit.

Maps_Stolen_Pct
    The probability (in percent) that the map of a tile is stolen in the actions "Steal Maps" and "Steal Maps
    Escape". DiplRel reqs are unit owner to city owner. Requirements evaluated against tile or city not
    supported. Default value: 100%

Illegal_Action_Move_Cost
    The number of move fragments lost when the player tries to do an action that turns out to be illegal.
    Only applied when the player wasn't aware that the action was illegal and its illegality therefore
    reveals new information.

Illegal_Action_HP_Cost
    The number of hit points lost when the player tries to do an action that turns out to be illegal. Only
    applied when the player wasn't aware that the action was illegal and its illegality therefore reveals new
    information. Can kill the unit. If the action always causes the actor unit to end up at the target tile
    two consolation prizes are given. An area with the radius of the actor unit's vision_radius_sq tiles is
    revealed. The player may also get contact with the owners of units and cites adjacent to the target tile.

Action_Success_Actor_Move_Cost
    The number of move fragments lost when a unit successfully performs an action. Evaluated and done after
    the action is successfully completed. Added on top of any movement fragments the action itself subtracts.

Action_Success_Target_Move_Cost
    The number of move fragments subtracted from a unit when someone successfully performs an action on it.
    Evaluated and done after the action is successfully completed. Added on top of any movement fragments the
    action itself subtracts. Only supported for actions that targets an individual unit.
    (See doc/README.actions)

Casus_Belli_Caught
    Checked when a player is caught trying to do an action. Will cause an incident with the intended victim
    player if the value is 1 or higher. The incident gives the intended victim a casus belli against the
    actor player. A value of 1000 or higher is international outrage. International outrage gives every other
    player a casus belli against the actor.

Casus_Belli_Success
    Checked when a player does an action to another player. Will cause an incident with the intended victim
    player if the value is 1 or higher. The incident gives the intended victim a casus belli against the actor
    player. A value of 1000 or higher is international outrage. International outrage gives every other player
    a casus belli against the actor.

Casus_Belli_Complete
    Checked when a player completes an action that takes several turns against another player. Will cause an
    incident with the intended victim player if the value is 1 or higher. The incident gives the intended
    victim a casus belli against the actor player. A value of 1000 or higher is international outrage.
    International outrage gives every other player a casus belli against the actor. Only "Pillage" is
    currently supported.

Action_Odds_Pct
    Modifies the odds of an action being successful. Some actions have a  risk: the actor may get caught
    before he can perform it. This effect  modifies the actor's odds. A positive value helps him. A negative
    value  makes it more probable that he will get caught. Currently supports the  actions "Incite City",
    "Incite City Escape", "Steal Gold", "Steal Gold Escape", "Steal Maps", "Steal Maps Escape", "Suitcase
    Nuke",  "Suitcase Nuke Escape", "Sabotage City", "Sabotage City Escape", "Targeted Sabotage City",
    "Targeted Sabotage City Escape", "Sabotage City Production", "Sabotage City Production Escape",
    "Surgical Strike Building", "Surgical Strike Production", "Steal Tech", "Steal Tech Escape Expected",
    "Targeted Steal Tech", "Targeted Steal Tech Escape Expected" and "Spread Plague".

Size_Adj
    Increase maximum size of a city by amount.

Size_Unlimit
    Make the size of a city unlimited.

Unit_Slots
    Number of unit slots city can have units in. New units cannot be built, nor can homecity be changed so
    that maintained units would use more slots than this. Single unit does not necessarily use single slot -
    that's defined separately for each unit type.

SS_Structural, SS_Component, SS_Module
    A part of a spaceship; this is a "Local" ranged effect. It (for now) applies to improvements which
    cannot be built unless "Enable_Space" is felt. Buildings which have this effect should probably not be
    given any other effects.

Spy_Resistant
    In diplomatic combat defending diplomatic units will get an AMOUNT percent bonus. All Spy_Resistant's
    are summed before being applied.

Building_Saboteur_Resistant
    If a spy specifies a target for sabotage, then she has an AMOUNT percent chance to fail.

Stealings_Ignore
    When determining how difficult it is to steal a tech from enemy, AMOUNT previous times tech has been
    stolen from the city is ignored. Negative amount means that number of times tech has already been stolen
    from target city does not affect current attempt at all. With this effect it's possible to allow
    diplomats to steal tech multiple times from the same city, or make it easier for spies.

Move_Bonus
    Add amount movement to units. Use UnitClass' requirement with range of 'Local' to give it a specific
    class of units only.

Unit_No_Lose_Pop
    No population lost when a city's defender is lost.

Unit_Recover
    Units recover amount extra hitpoints per turn.

Upgrade_Unit
    Upgrade amount obsolete units per turn.

Upkeep_Free
    Improvements with amount or less upkeep cost become free to upkeep (others are unaffected).

Tech_Upkeep_Free
    If this value is greater than 0, the tech upkeep is reduced by this value. For tech upkeep style
    "Basic" this is total reduction, for tech upkeep style "Cities" this reduction is applied to every city.

No_Unhappy
    No citizens in the city are ever unhappy.

Veteran_Build
    Increases the veteran class of newly created units of this type. The total amount determines the veteran
    class (clipped at the maximum for the unit).

Veteran_Combat
    Increases the chance of units of this type becoming veteran after combat by amount percent.

Combat_Rounds
    Maximum number of rounds combat lasts. Unit is the attacker. Zero and negative values mean that combat
    continues until either side dies.

HP_Regen
    Units that do not move recover amount percentage (rounded up) of their full hitpoints per turn.

    .. note::
        This effect is added automatically to implement HP recovery in cities. This behavior can be turned
        off by requiring the ``+HP_Regen_Min`` option in ``effects.ruleset``.

HP_Regen_Min
    Lower limit on "HP_Regen".  That is, the recovery percentage is the larger of "HP_Regen" and "HP_Regen_Min".

    .. note::
        This effect is added automatically to implement HP recovery in cities. This behavior can be turned
        off by requiring the ``+HP_Regen_Min`` option in ``effects.ruleset``.

City_Vision_Radius_Sq
    Increase city vision radius in squared distance by amount tiles.

    .. note::
        This effect is added automatically for VisionLayers other than Main,
        with a value of 2, and a VisionLayer=Main requirement is added to any
        existing instances of this effect.
        This behaviour can be turned off by requiring the ``+VisionLayer``
        option in ``effects.ruleset``, allowing you to use VisionLayer
        requirements to specify which layer (Main, Stealth or Subsurface)
        the effect applies to.

Unit_Vision_Radius_Sq
    Increase unit vision radius in squared distance by amount tiles.

    .. note::
        A VisionLayer=Main requirement is added automatically to any
        existing instances of this effect.
        This behaviour can be turned off by requiring the ``+VisionLayer``
        option in ``effects.ruleset``, allowing you to use VisionLayer
        requirements to specify which layer (Main, Stealth or Subsurface)
        the effect applies to.

Defend_Bonus
    Increases defensive bonuses of units. Any unit requirements on this effect will be applied to the
    _attacking_ unit. Attackers with "BadWallAttacker" flag will have their firepower set to 1.

Attack_Bonus
    Increases offensive bonuses of units. Unit requirements on this effect are the attacking unit itself.

Fortify_Defense_Bonus
    Percentage defense bonus multiplicative with Defend_Bonus, usually given to fortified units. Unit
    requirements on this effect are the defending unit itself.

Gain_AI_Love
    Gain amount points of "AI love" with AI(s).

Turn_Years
    Year advances by AMOUNT each turn unless Slow_Down_Timeline causes it to be less.

Turn_Fragments
    Year fragments advance by AMOUNT each turn.

Slow_Down_Timeline
    Slow down the timeline based on the AMOUNT. If AMOUNT >= 3 the timeline will be max 1 year/turn; with
    AMOUNT == 2 it is max 2 years/turn; with AMOUNT == 1 it is max 5 years/turn; with AMOUNT <= 0 the
    timeline is unaffected. The effect will be ignored if game.spacerace isn't set.

Civil_War_Chance
    Base chance in per cent of a nation being split by civil war when its capital is captured is increased
    by this amount. This percentage is in- creased by 5 for each city in civil disorder and reduced by 5 for
    each one celebrating.

City_Unhappy_Size
    The maximum number of citizens in each city that are naturally content; in larger cities, new citizens
    above this limit start out unhappy. (Before Empire_Size_Base/Step are applied.)

Empire_Size_Base
    Once your civilization has more cities than the value of this effect, each city gets one more unhappy
    citizen. If the sum of this effect and Empire_Size_Step is zero, there is no such penalty.

Empire_Size_Step
    After your civilization reaches Empire_Size_Base size, it gets one more unhappy citizen for each amount
    of cities it gets above that. Set to zero to disable. You can use Empire_Size_Step even if
    Empire_Size_Base is zero.

Max_Rates
    The maximum setting for each tax rate is amount.

Martial_Law_Each
    The amount of citizens pacified by each military unit giving martial law.

Martial_Law_Max
    The maximum amount of units that will give martial law in city.

Rapture_Grow
    Can rapture grow cities.

Revolution_Unhappiness
    If value is greater than zero, it tells how many turns citizens will tolerate city disorder before
    government falls. If value is zero, government never falls.

Has_Senate
    Has a senate that prevents declarations of war in most cases.

Inspire_Partisans
    Partisan units (defined in units.ruleset) may spring up when this player's cities are taken.

Happiness_To_Gold
    Make all Make_Content and Force_Content effects instead generate gold.

Max_Trade_Routes
    Number of trade routes that city can establish. This is forced on trade route creation only. Existing
    trade routes are never removed due to reduction of effect value. This is to avoid micro-management, need
    to create same trade routes again after their max number has been temporarily down.

Fanatics
    Units with "Fanatics" flag incur no upkeep.

No_Diplomacy
    Cannot use any diplomacy.

Not_Tech_Source
    Tech cannot be received from this player by any means.

Trade_Revenue_Bonus
    One time trade revenue bonus is multiplied by :math:`2^{(\texttt{amount} \div 1000)}`. The amount value is
    taken from the caravan's home city.

Trade_Revenue_Exponent
    One time trade revenue bonus is raised to the :math:`1 + \frac{\texttt{amount}}{1000}` power.
    This is applied before ``Trade_Revenue_Bonus``.

Traderoute_Pct
    Percentage bonus for trade from traderoutes. This bonus applies after the value of the traderoute is
    already calculated. It affects one end of the traderoute only.

Unhappy_Factor
    Multiply unhappy unit upkeep by amount.

Upkeep_Factor
    Multiply unit upkeep by amount.

Unit_Upkeep_Free_Per_City
    In each city unit upkeep is deducted by this amount. As usual, you can use
    with OutputType requirement to specify which kind of upkeep this should be.

Output_Waste
    Base amount in percentage that each city has in waste. Waste can be used
    with any output type, use an OutputType requirement to specify which.

Output_Waste_By_Distance
    For each tile in real distance that a city is from nearest
    Government Center, it gets :math:`\frac{\texttt{amount}}{100}` of extra waste.

Output_Waste_By_Rel_Distance
    City gets extra waste based on distance to nearest Government Center, relative
    to world size. The amount of this extra waste is
    :math:`\frac{\texttt{distance}\,\times\,\texttt{amount}}{100\,\times\,\texttt{max_distance}}`

Output_Penalty_Tile
    When a tile yields more output than amount, it gets a penalty of -1.

Output_Inc_Tile_Celebrate
    Tiles get amount extra output when city working them is celebrating.

Upgrade_Price_Pct
    Increases unit upgrade cost by amount percent. This effect works at player level. You cannot adjust
    upgrade costs for certain unit type or for units upgraded in certain city.

Unit_Shield_Value_Pct
    Increase the unit's value in shields by amount percent. When this effect is used to determine how many
    shields the player gets for the actions "Recycle Unit" and "Help Wonder" it gets access to unit state.
    When it is used to influence the gold cost of "Upgrade Unit" it only has access to unit type.

Retire_Pct
    The chance that unit gets retired (removed) when turn changes. Retirement only happens if there are no
    enemy units or cities within a few tiles. (This exists mainly to implement barbarian behavior.)

Visible_Wall
    Instruct client to show specific buildings version of the city graphics.
    Zero or below are considered normal city graphics.

Tech_Cost_Factor
    Factor for research costs.

Building_Build_Cost_Pct
    Percentage added to building building cost.

Building_Buy_Cost_Pct
    Percentage added to building buy cost.

Unit_Build_Cost_Pct
    Percentage added to unit building cost.

Unit_Buy_Cost_Pct
    Percentage added to unit buy cost.

Nuke_Improvement_Pct
    Percentage chance that an improvement would be destroyed while nuking the city
    Only regular improvements (not wonders) are affected. Improvements protected from Sabotage (Eg: City Walls)
    aren't affected.

Nuke_Infrastructure_Pct
    Percentage chance that an extra located within a nuclear blast area gets destroyed.
    Only "Infra" extras such as roads and irrigation are affected, and rmreqs are also checked.

    Note that an `Extra` requirement will match any extra on the tile, not only the one
    considered for destruction.

Shield2Gold_Factor
    Factor in percent for the conversion of unit shield upkeep to gold upkeep. A value of 200 would transfer
    1 shield upkeep to 2 gold upkeep. The range of this effect must be player or world. Note that only units
    with the "Shield2Gold" flag will be affected by this.

Tile_Workable
    If value > 0, city can work target tile.

Migration_Pct
    Increase the calculated migration score for the a city by amount in percent.

City_Radius_Sq
    Increase the squared city radius by amount. Currently, this can only usefully have "MinSize", "Building",
    or "Tech" requirements.

City_Build_Slots
    Increase the number of units with no population cost a city can build in a turn if there are enough
    shields.

City_Image
    The index for the city image of the given city style.

Victory
    Positive value means that player wins the game.

Performance
    Value is how much performance type culture city produces.

History
    Value is how much history type (cumulative) culture city produces.

National_Performance
    Value is how much performance type culture, not tied to any specific city, nation produces.

National_History
    Value is how much history type (cumulative) culture, not tied to any any specific city, nation produces.

Infra_Points
    City increases owner's infra points by value each turn. If overall points are negative after all cities
    have been processed, they are set to 0.

Bombard_Limit_Pct
    Bombardment may only reduce units to amount percent (rounded up) of their total hitpoints.  Unit
    requirements on this effect are the defending unit itself.

    .. note::
        This effect is added automatically with a value of 1 and no reqs. This behavior can be turned
        off by requiring the ``+Bombard_Limit_Pct`` option in ``effects.ruleset``.

Wonder_Visible
    If the value of this effect is larger than 0 for a small wonder, the wonder will be visible to all
    players and reported in the intelligence screen. Great wonders are always visible to everyone through the
    wonders report. When a small wonder is lost (for instance, because the city it is in is lost or some of
    its requirements become invalid), it also becomes visible to everyone (this is a limitation of the
    server).

    .. note::
        This effect is added automatically with a value of 1 for great wonders (since they are shown in the
        wonders report anyway). This behavior can be turned off by requiring the ``+Wonder_Visible`` option
        in ``effects.ruleset``.

Nation_Intelligence
    Controls the information available in the Nations View. :doc:`See the
    detailed description. <Effects/Nation_Intelligence>`

    .. toctree::
        :hidden:

        Effects/Nation_Intelligence
