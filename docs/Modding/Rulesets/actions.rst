Actions
*******

An action is something a player can do to achieve something in the game. It has properties like cost, chance
of success and effects. Some of those properties are configurable using effects and other rule set settings.
To learn how to change them read :doc:`effects` and the rule set files of classic. An action enabler allows a
player to do an action.

Generalized Action Enablers
===========================

Some actions have generalized action enablers. An action like that can have zero, one or more action enablers
defined for it in the ruleset. The player can do the action only when at least one generalized action enabler
says that the action is enabled (and all its hard requirements are fulfilled). A ruleset author can therefore
disable an action by not defining any action enablers for it in his ruleset.

A generalized action enabler lives in :file:`game.ruleset`. It consists of the action it enables and two
requirement vectors. The first requirement vector, :code:`actor_reqs`, applies to the entity doing the action.
The second, :code:`target_reqs`, applies to its target. If both requirement vectors are fulfilled the action
is enabled as far as the action enabler is concerned. Note that an action's hard requirements still may make
it impossible.

In some situations an action controlled by generalized action enablers may be impossible because of
limitations in Freeciv21 itself. Those limitations are called hard requirements. The hard requirements of each
action are documented below in the section called "Actions and their hard coded requirements".

If the player doesn't have the knowledge required to find out if an action is enabled or not the action is
shown to the player in case it is possible. The client will indicate the uncertainty to the player.

Should the player order a unit to do an illegal action the server will inform the player that his unit was
unable to perform the action. The actor unit may also lose a ruleset configurable amount of move fragments.

Example:

.. code-block:: rst

    [actionenabler_veil_the_threat_of_terror]
    action = "Incite City"
    actor_reqs    =
        { "type",    "name",                 "range", "present"
          "DiplRel", "Has Casus Belli",      "Local", TRUE
          "DiplRel", "Provided Casus Belli", "Local", FALSE
          "DiplRel", "Is foreign",           "Local", TRUE
        }

Another Example:

.. code-block:: rst

    [actionenabler_go_bind_your_sons_to_exile]
    action = "Incite City"
    actor_reqs    =
        { "type",    "name",       "range",  "present"
          "Tech",    "Flight",     "Player", TRUE
          "DiplRel", "Is foreign", "Local",  TRUE
        }
    target_reqs    =
        { "type",   "name",    "range",  "present"
          "Tech",   "Writing", "Player", False
        }


Above are two action enablers. They both enable the action "Incite City". If all the conditions of at least
one of them are fulfilled it will be enabled. No information is given to the player about what action enabler
enabled an action.

The first action enabler, :code:`actionenabler_veil_the_threat_of_terror`, is simple. It allows a player to
incite a city if he has a reason to declare war on its owner AND the cities owner don't have a reason to
declare war on him.

The second action enabler, :code:`actionenabler_go_bind_your_sons_to_exile`, is more complex. It allows a
player that has Flight to bribe the cities of civilizations that don't have Writing. The complexity is caused
by the requirement that the target don't know Writing. If the civilization of the target city knows Writing or
not may be unknown to the acting player. To avoid this complexity a requirement that the acting player has an
embassy to the target cities civilization (and therefore knowledge about its techs) can be added.

Requirement Vector Rules
========================

An action enabler has two requirement vectors that must be true at the same time. This creates some corner
cases you won't find with single requirement vectors. The rules below tries to deal with them.

A :code:`DiplRel` requirement with the range :code:`Local` should always be put in the actor requirements.

* A local :code:`DiplRel` requirement can always be expressed as an actor requirement.
* Only having to care about local :code:`DiplRel` requirements in the actor requirements allows the Freeciv21
  code responsible for reasoning about  action enablers to be simpler and faster.
* If player A having a diplomatic relationship to player B implies that player B has the same relationship to
  player A the relationship is symmetric. Examples: :code:`Is foreign` and :code:`War`
* Symmetric local :code:`DiplReal` requirements can be moved directly from the target requirement vector to
  the actor requirement vector.
* Asymmetric local :code:`DiplReal` requirements must test for the same thing in the opposite direction.
  Example: :code:`Hosts embassy -> Has embassy`

Actions And Lua
===============

Right before an action is executed, but after it is known to be legal, a signal is emitted to Lua. It has
access to the same information as the server. It obviously don't have access to the result of the action since
it isn't done yet.

The signal's name starts with :code:`action_started_`, then the actor kind, then another :code:`_` and in the
end the target kind. The signal that is emitted when a unit performs an action on a city is therefore
:code:`action_started_unit_city`.

The signal has three parameters. The first parameter is the action that is about to get started. The second is
the actor. The third parameter is the target. The parameters of :code:`action_started_unit_city` is therefore
:code:`action`, :code:`actor_unit` and finally :code:`target city`.

To get the rule name of an action, that is the name used in action enablers, you can use the method
:code:`rule_name()`. To get a translated name that is nice to show to players use :code:`name_translation()`.

Example 1
  The following Lua code will log all actions done by any unit to a city, to another unit, to a unit stack, to
  a tile or to itself:

.. code-block:: rst

    function action_started_callback(action, actor, target)
      local target_owner
      if target == nil then
        target_owner = "it self"
      elseif target.owner == nil then
        target_owner = "unowned"
      else
        target_owner = target.owner.nation:plural_translation()
      end

      log.normal(_("%d: %s (rule name: %s) performed by %s %s (id: %d) on %s"),
                game.current_turn(),
                action:name_translation(),
                action:rule_name(),
                actor.owner.nation:plural_translation(),
                actor.utype:rule_name(),
                actor.id,
                target_owner)
    end

    signal.connect("action_started_unit_city", "action_started_callback")
    signal.connect("action_started_unit_unit", "action_started_callback")
    signal.connect("action_started_unit_units", "action_started_callback")
    signal.connect("action_started_unit_tile", "action_started_callback")
    signal.connect("action_started_unit_self", "action_started_callback")


Example 2
  The following Lua code will make a player that poisons the population of cities risk civil war:

.. code-block:: rst

    function action_started_callback(action, actor, target)
      if action:rule_name() == "Poison City" then
        edit.civil_war(actor.owner, 5);
      end
    end

    signal.connect("action_started_unit_city", "action_started_callback")


Actions And Their Hard Requirements
===================================

Freeciv21 can only allow a player to perform an action when the action's hard requirements are fulfilled.
Some, but not all, hard requirements can be expressed in an action enabler. Putting them there makes it
clearer what the rule actually is. Parts of Freeciv21 reasons about action enablers. Examples are self
contradicting rule detection and the help system. Including the hard requirements rules in each enabler of its
action is therefore obligatory for some hard requirements. Those hard requirements are marked with an
exclamation mark (!).

Actions Done By A Unit Against A City
=====================================

Establish Embassy
  Establish a real embassy to the target player

  Rules:

  * UI name can be set using :code:`ui_name_establish_embassy`
  * actor must be aware that the target exists
  * actor can't have a real embassy to the target player (!)
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Establish Embassy Stay
  Establish a real embassy to the target player

  Rules:

  * UI name can be set using :code:`ui_name_establish_embassy`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor can't have a real embassy to the target player (!)
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Investigate City
  Look at the city dialog of a foreign city

  Rules:

  * UI name can be set using :code:`ui_name_investigate_city`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Investigate City Spend Unit
  Look at the city dialog of a foreign city

  Rules:

  * UI name can be set using :code:`ui_name_investigate_city`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Sabotage City
  Destroy a building or the production in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_sabotage_city`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Sabotage City Escape
  Destroy a building or the production in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_sabotage_city_escape`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Targeted Sabotage City
  Destroy a building in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_targeted_sabotage_city`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Targeted Sabotage City Escape
  Destroy a building in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_targeted_sabotage_city_escape`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Sabotage City Production
  Sabotage the city's produciton.

  Rules:

  * UI name can be set using :code:`ui_name_sabotage_city_production`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Sabotage City Production Escape
  Sabotage the city's produciton.

  Rules:

  * UI name can be set using :code:`ui_name_sabotage_city_production_escape`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Poison City
  Kill a citizen in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_poison_city`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Poison City Escape
  Kill a citizen in the target city and escape.

  Rules:

  * UI name can be set using :code:`ui_name_poison_city_escape`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Spread Plague
  Bio-terrorism. Infect the target city with an illness.

  Rules:

  * UI name can be set using :code:`ui_name_spread_plague`
  * set if the actor unit is spent with :code:`spread_plague_actor_consuming_always`
  * may infect trade route connected cities if :code:`illness.illness_on` is TRUE
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Steal Tech
  Steal a random tech from the targets owner.

  Rules:

  * UI name can be set using :code:`ui_name_steal_tech`
  * spends the actor unit
  * will always fail when the tech theft is expected. Tech theft is expected when the number of previous tech
    thefts from the target city is above the limit set by the :code:`Stealings_Ignore` effect.
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Steal Tech Escape Expected
  Escape version of the above.

  Rules:

  * UI name can be set using :code:`ui_name_steal_tech_escape`
  * more likely to fail when the tech theft is expected. Tech theft is expected when the number of previous
    tech thefts from the target city is above the limit set by the :code:`Stealings_Ignore` effect.
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Targeted Steal Tech
  Steal a specific tech from the targets owner.

  Rules:

  * UI name can be set using :code:`ui_name_targeted_steal_tech`
  * spends the actor unit
  * will always fail when the tech theft is expected. Tech theft is expected when the number of previous tech
    thefts from the target city is above the limit set by the :code:`Stealings_Ignore` effect.
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Targeted Steal Tech Escape Expected
  Escape version of the above.

  Rules:

  * UI name can be set using :code:`ui_name_targeted_steal_tech_escape`
  * more likely to fail when the tech theft is expected. Tech theft is expected when the number of previous
    tech thefts from the target city is above the limit set by the :code:`Stealings_Ignore` effect.
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Incite City
  Pay the target city to join the actors owners side.

  Rules:

  * UI name can be set using :code:`ui_name_incite_city`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Incite City Escape
  Pay the target city to join the actors owners side.

  Rules:

  * UI name can be set using :code:`ui_name_incite_city_escape`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Steal Gold
  Steal some gold from the owner of the target city.

  Rules:

  * UI name can be set using :code:`ui_name_steal_gold`
  * adjustable with the :code:`Max_Stolen_Gold_Pm` effect and with the :code:`Thiefs_Share_Pm` effect
  * spends the actor unit
  * actor must be aware that the target exists
  * the targets owner must have more than 0 gold.
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Steal Gold Escape
  Steal some gold from the owner of the target city.

  Rules:

  * UI name can be set using :code:`ui_name_steal_gold_escape`
  * adjustable with the :code:`Max_Stolen_Gold_Pm` effect and with the :code:`Thiefs_Share_Pm` effect
  * actor must be aware that the target exists
  * the targets owner must have more than 0 gold.
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Steal Maps
  Steal parts of the owner of the target city's map.

  Rules:

  * UI name can be set using :code:`ui_name_steal_maps`
  * adjustable with the effect :code:`Maps_Stolen_Pct` and the ruleset setting :code:`steal_maps_reveals_all_cities`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Steal Maps Escape
  Steal parts of the owner of the target city's map.

  Rules:

  * UI name can be set using :code:`ui_name_steal_maps_escape`
  * adjustable with the effect :code:`Maps_Stolen_Pct` and the ruleset setting :code:`steal_maps_reveals_all_cities`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)

Suitcase Nuke
  Cause a nuclear explosion in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_suitcase_nuke`
  * spends the actor unit
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Suitcase Nuke Escape
  Cause a nuclear explosion in the target city.

  Rules:

  * UI name can be set using :code:`ui_name_suitcase_nuke_escape`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Destroy City
  Destroys the target city.

  Rules:

  * UI name can be set using :code:`ui_name_destroy_city`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.

Establish Trade Route
  Establish a trade route to the target city.

  Rules:

  * UI name can be set using :code:`ui_name_establish_trade_route`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target or on the tile next to it.
  * actor must have a home city. (!)
  * target must be foreign or :code:`trademindist` tiles away from that home city.
  * trade route type pct (see "Trade settings") can't be 0%.
  * it is possible to establish a trade route between the cities as far as the two cities them self are
    concerned. Example: If one of the cities can't have any trade routes at all it is impossible to establish
    a new one.

Enter Marketplace
  Get a one time bounus without creating a trade route.

  Rules:

  * UI name can be set using :code:`ui_name_enter_marketplace`
  * actor must be aware that the target exists
  * if :code:`force_trade_route` is TRUE "Establish Trade Route" must be impossible
  * actor must be on the same tile as the target or on the tile next to it.
  * actor must have a home city. (!)
  * target must be foreign or :code:`trademindist` tiles away from that home city.
  * trade route type (see Trade settings) can't be 0%.

Help Wonder
  Add the shields used to build the actor to the target city.

  Rules:

  * UI name can be set using :code:`ui_name_help_wonder`
  * adjustable with the :code:`Unit_Shield_Value_Pct` effect
  * actor must be aware that the target exists
  * actor must be on the same tile as the target unless :code:`help_wonder_max_range` allows it to be further
    away. Default :code:`help_wonder_max_range` is 1
  * target city must need the extra shields to complete its production.

Recycle Unit
  Add half the shields used to build the unit to target

  Rules:

  * UI name can be set using :code:`ui_name_recycle_unit`
  * adjustable with the :code:`Unit_Shield_Value_Pct` effect
  * actor must be aware that the target exists
  * "Help Wonder" must be impossible
  * actor must be on the same tile as the target unless :code:`recycle_unit_max_range` allows it to be further
    away. Default :code:`recycle_unit_max_range` is 1
  * target city must need the extra shields to complete its production.

Join City
  Add the actor to the target city's population.

  Rules:

  * UI name can be set using :code:`ui_name_join_city`
  * actor must be aware that the target exists
  * actor must have population to add (set in :code:`pop_cost`)
  * actor must be on the same tile as the target or on the tile next to it.
  * target city population must not become higher that the :code:`add_to_size_limit` setting permits.
  * target must be able to grow to the size that adding the unit would result in.

Home City
  Set target city as the actor unit's new home city

  Rules:

  * UI name can be set using :code:`ui_name_home_city`
  * actor must be aware that the target exists
  * actor must be on the same tile as the target
  * actor must not have the :code:`NoHome` unit type flag. (!)
  * can't set existing home city as new home city
  * target city has enough unused unit maintenance slots to support the actor unit. No problem if the actor
    unit spends 0 city slots.

Upgrade Unit
  Upgrade the actor unit using the target's facilities.

  Rules:

  * UI name can be set using :code:`ui_name_upgrade_unit`.
  * adjustable with the :code:`Unit_Shield_Value_Pct` effect
  * actor must be aware that the target exists
  * actor must be on the same tile as the target.
  * actor player must have enough gold to pay for the upgrade.
  * actor unit must have a type to upgrade to (:code:`obsoleted_by`).
  * actor unit's upgraded form must be able to exist at its current location.
  * actor unit's upgraded form must have room for its current cargo.
  * target player must be able to build the unit upgraded to
  * target city must be domestic. (!)

Airlift Unit
  Airlift actor unit to target city.

Rules:

  * UI name can be set using :code:`ui_name_airlift_unit`
  * max legal distance to the target can be set using :code:`airlift_max_range`
  * actor must be aware that the target exists
  * the actor unit isn't transporting another unit (!)
  * the actor unit isn't inside the target city
  * the actor unit can exist in the target city (outside a transport)
  * the actor unit is in a city (!)
  * the city the actor unit is in:

    * is domestic or, if :code:`airliftingstyle` permits it, allied
    * has Airlift (see the :code:`Airlift` effect and the :code:`airliftingstyle` setting)

  * the target city is domestic or, if :code:`airliftingstyle` permits it, allied
  * the target city has :code:`Airlift`

Nuke City
  Detonate in the target city. Cause a nuclear explosion.

  Rules:

  * UI name can be set using :code:`ui_name_nuke_city`
  * if :code:`force_capture_units` is TRUE "Capture Units" must be impossible
  * if :code:`force_bombard` is TRUE "Bombard", "Bombard 2" and "Bombard 3" must be impossible.
  * the actor unit must be on a tile next to the target unless :code:`nuke_city_max_range` allows it to be
    further away.

Conquer City
  Conquer the target city.

  Rules:

  * UI name can be set using :code:`ui_name_conquer_city`
  * actor must be aware that the target exists
  * if :code:`force_capture_units` is TRUE "Capture Units" must be impossible
  * if :code:`force_bombard` is TRUE "Bombard", "Bombard 2" and "Bombard 3" must be impossible
  * if :code:`force_explode_nuclear` is TRUE "Explode Nuclear", "Nuke Units" and "Nuke City" must be impossible
  * "Attack" must be impossible
  * the actor unit must be on a tile next to the target.
  * the actor player's nation can't be an animal barbarian. (!)
  * the actor unit's current transport, if the actor unit is transported, must be in a city or in a base
    native to the current transport if the current transport's unit class has the :code:`Unreachable` unit
    class flag and the actor's unit type doesn't list the current transport's unit class in disembarks.
  * the actor unit doesn't have the :code:`CoastStrict` unit type flag or the target city is on or adjacent to
    a tile that doesn't have the :code:`UnsafeCoast` terrain flag.
  * the actor unit can't be diplomatically forbidden from entering the tile of the target city.
  * the actor unit has the :code:`CanOccupyCity` unit class flag (!)
  * the actor can't have the :code:`NonMil` unit type flag (!)
  * the actor unit has at least one move fragment left (!)
  * the actor's relationship to the target is War (!)
  * actor unit must be able to exist outside of a transport at the target's tile.
  * the target must be foreign (!)
  * the target city contains 0 units (!)

Conquer City 2
  Conquer the target city.

  Rules:

  * UI name can be set using :code:`ui_name_conquer_city_2`
  * A copy of "Conquer City".
  * See "Conquer City" for everything else.

Surgical Strike Building
  Destroy a specific building.

  Rules:

  * UI name can be set using :code:`ui_name_surgical_strike_building`
  * actor must be aware that the target exists
  * the actor unit must be on a tile next to the target.

Surgical Strike Production
  Destroy the city production.

  Rules:

  * UI name can be set using :code:`ui_name_surgical_strike_production`
  * actor must be aware that the target exists
  * the actor unit must be on a tile next to the target.

Actions Done By A Unit Against Another Unit
===========================================

Sabotage Unit
  Halve the target unit's hit points.

  Rules:

  * UI name can be set using :code:`ui_name_sabotage_unit`
  * spends the actor unit
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be visible for the actor.

Sabotage Unit Escape
  Halve the target unit's hit points.

  Rules:

  * UI name can be set using :code:`ui_name_sabotage_unit_escape`
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be visible for the actor.

Bribe Unit
  Make the target unit join the actors owners side.

  Rules:

  * UI name can be set using :code:`ui_name_bribe_unit`
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be foreign. (!)
  * target must be visible for the actor.

Expel Unit
  Expel the target unit to its owner's capital.

  Rules:

  * UI name can be set using :code:`ui_name_expel_unit`
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be visible for the actor.
  * target's owner must have a capital

Heal Unit
  Restore the target unit's health.

  Rules:

  * UI name can be set using :code:`ui_name_heal_unit`
  * actor must be on the same tile as the target or on the tile next to it.
  * target must be visible for the actor.

Transport Alight
  Exit target transport to same tile.

  Rules:

  * UI name can be set using :code:`ui_name_transport_alight`
  * actor must be on the same tile as the target
  * actor must be transported (!)
  * actor must be on a livable tile (!)
  * target must be transporting (!)
  * target must be in a city or in a native base if the target's unit class has the :code:`Unreachable` unit
    class flag and the actor's unit type doesn't list the target's unit class in disembarks.
  * target must be visible for the actor.

Transport Unload
  Unload the target unit to same tile.

  Rules:

  * UI name can be set using :code:`ui_name_transport_unload`
  * actor must be on the same tile as the target
  * actor must have a transport_cap greater than 0
  * actor must be transporting (!)
  * actor must be in a city or in a native base if the actor's unit class has the :code:`Unreachable` unit
    class flag and the target's unit type doesn't list the actor's unit class in disembarks.
  * target must be transported (!)
  * target must be on a livable tile (!)
  * target must be visible for the actor.

Transport Board
  Enter target transport on the same tile.

  Rules:

  * UI name can be set using :code:`ui_name_transport_board`
  * the actor unit can't currently be transported by the target unit.
  * the actor unit's current transport, if the actor unit is transported, must be in a city or in a base
    native to the current transport if the current transport's unit class has the :code:`Unreachable` unit
    class flag and the actor's unit type doesn't list the current transport's unit class in the disembarks field.
  * the actor's unit class must appear in the target unit type's cargo field.
  * the actor unit unit type must be different from the target unit type
  * the actor unit or its (recursive) cargo, if it has cargo, must be unable  to transport itself, the target
    unit or the target unit's transporters. See :code:`unit_transport_check()`.
  * boarding won't cause a situation with more than 5 recursive transports
  * the target unit must be domestic, allied or on the same team as the actor unit is. (!)
  * target must be in a city or in a base native to it if the target's unit class has the :code:`Unreachable`
    unit class flag and the actor's unit type doesn't list the target's unit class in the embarks field.
  * the target must be transporting fewer units than its unit type's :code:`transport_cap` field.
  * target must be visible to the actor.

Transport Embark
  Enter target transport on a different tile.

  Rules:

  * UI name can be set using :code:`ui_name_transport_embark`
  * the actor unit must be on a tile next to the target.
  * the actor unit has at least one move fragment left (!)
  * the actor unit can't currently be transported by the target unit.
  * the actor unit's current transport, if the actor unit is transported, must be in a city or in a base
    native to the current transport if the current transport's unit class has the :code:`Unreachable` unit
    class flag and the actor's unit type doesn't list the current transport's unit class in the disembarks field.
  * the actor unit's type must be the target tile's terrain animal if the player's nation is an animal barbarian.
  * the actor unit can't be diplomatically forbidden from entering the target tile.
  * the actor unit doesn't have the :code:`CoastStrict` unit type flag or the target city is on or adjacent to
    a tile that doesn't have the :code:`UnsafeCoast` terrain flag.
  * actor unit must be able to move to the target tile.
  * the actor's unit class must appear in the target unit type's cargo field.
  * the actor unit unit type must be different from the target unit type
  * the actor unit or its (recursive) cargo, if it has cargo, must be unable to transport itself, the target
    unit or the target unit's transporters. See :code:`unit_transport_check()`.
  * boarding won't cause a situation with more than 5 recursive transports
  * the target unit must be domestic, allied or on the same team as the actor unit is. (!)
  * target must be in a city or in a base native to it if the target's unit class has the :code:`Unreachable`
    unit class flag and the actor's unit type doesn't list the target's unit class in the embarks field.
  * the target must be transporting fewer units than its unit type's transport_cap field.
  * the target tile can't contain any city or units not allied to the actor unit and all its cargo.
  * target must be visible to the actor.

Actions Done By A Unit Against All Units At A Tile
==================================================

Capture Units
  Steal the target units.

  Rules:

  * UI name can be set using :code:`ui_name_capture_units`
  * actor must be on a tile next to the target.
  * target must be foreign. (!)
  * target cannot be transporting other units. (!)

Bombard
  Bombard the units (and city) at the tile without killing them.

  Rules:

  * UI name can be set using :code:`ui_name_bombard`
  * if force_capture_units is true "Capture Units" must be impossible
  * actor must have a bombard_rate > 0
  * actor must have an attack > 0
  * actor must be on a tile next to the target or, if :code:`bombard_max_range` allows it, futher away.
  * target can't be in a city the actor player isn't at war with.
  * target owner must be at war with actor. (!)

Bombard 2
  Bombard the units (and city) at the tile without killing them.

  Rules:

  * UI name can be set using :code:`ui_name_bombard_2`
  * actor must be on a tile next to the target or, if :code:`bombard_2_max_range` allows it, futher away.
  * A copy of "Bombard".
  * See "Bombard" for everything else.

Bombard 3
  Bombard the units (and city) at the tile without killing them.

  Rules:

  * UI name can be set using :code:`ui_name_bombard_3`
  * actor must be on a tile next to the target or, if :code:`bombard_3_max_range` allows it, futher away.
  * A copy of "Bombard".
  * See "Bombard" for everything else.

Attack
  Attack an enemy unit, possibly survive

  Rules:

  * UI name can be set using :code:`ui_name_attack`
  * if :code:`force_capture_units` is TRUE "Capture Units" must be impossible
  * if :code:`force_bombard` is TRUE "Bombard", "Bombard 2" and "Bombard 3" must be impossible
  * if :code:`force_explode_nuclear` is TRUE "Explode Nuclear", "Nuke Units" and "Nuke City" must be impossible
  * the actor must be on the tile next to the target.
  * the actor's attack must be above 0
  * the actor can't have the :code:`NonMil` unit type flag (!)
  * the actor must be native to the target tile unless it has the :code:`AttackNonNative` unit class flag and
    not the :code:`Only_Native_Attack` unit type flag.
  * the target tile has no non enemy units. (!)
  * the target tile has no non enemy city.
  * one or all (unreachableprotects) non transported units at the target tile must be reachable. A unit is
    reachable if any of the following is true:

    * it doesn't have the "Unreachable" unit class flag
    * it is listed in the actor unit's targets
    * it is in a city
    * it is on a tile with a native Extra

Suicide Attack
  Attack an enemy unit, die immediately

  Rules:

  * UI name can be set using :code:`ui_name_suicide_attack`
  * if :code:`force_capture_units` is TRUE "Capture Units" must be impossible
  * if :code:`force_bombard` is TRUE "Bombard", "Bombard 2" and "Bombard 3" must be impossible
  * if :code:`force_explode_nuclear` is TRUE "Explode Nuclear", "Nuke Units" and "Nuke City" must be impossible
  * the actor must be on the tile next to the target.
  * the actor's attack must be above 0
  * the actor can't have the :code:`NonMil` unit type flag (!)
  * the actor must be native to the target tile unless it has the :code:`AttackNonNative` unit class flag and
    not the :code:`Only_Native_Attack` unit type flag.
  * the target tile has no non enemy units. (!)
  * the target tile has no non enemy city.
  * one or all (unreachableprotects) non transported units at the target tile must be reachable. A unit is
    reachable if any of the following is true:

    * it doesn't have the "Unreachable" unit class flag
    * it is listed in the actor unit's targets
    * it is in a city
    * it is on a tile with a native Extra

Nuke Units
  Detonate at the target unit stack. Cause a nuclear explosion.

  Rules:

  * UI name can be set using :code:`ui_name_nuke_units`
  * if :code:`force_capture_units` is TRUE "Capture Units" must be impossible
  * if :code:`force_bombard` is TRUE "Bombard", "Bombard 2" and "Bombard 3" must be impossible
  * the actor unit must be on a tile next to the target unless :code:`nuke_units_max_range` allows it to be
    further away.
  * one or all (unreachableprotects) non transported units at the target tile must be reachable. A unit is
    reachable if any of the following is true:

    * it doesn't have the "Unreachable" unit class flag
    * it is listed in the actor unit's targets
    * it is in a city
    * it is on a tile with a native Extra

Spy Attack
  Trigger a diplomatic battle to eliminate tile defenders.

  Rules:

  * UI name can be set using :code:`ui_name_spy_attack`
  * the actor must be on the tile next to the target.
  * the target tile must have at least 1 diplomatic defender.

Actions Done By A Unit Against A Tile
=====================================

Found City
  Found a city at the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_found_city`
  * city name must be legal
  * the scenario setting :code:`prevent_new_cities` must be false.
  * actor must be on the same tile as the target.
  * target must not have the :code:`NoCities` terrain flag. (!)
  * target must not be closer than :code:`citymindist` to nearest city.

Explode Nuclear
  Detonate at the target tile. Cause a nuclear explosion.

  Rules:

  * UI name can be set using :code:`ui_name_explode_nuclear`
  * if :code:`force_capture_units` is TRUE "Capture Units" must be impossible
  * if :code:`force_bombard` is TRUE "Bombard", "Bombard 2" and "Bombard 3" must be impossible
  * actor must be on the same tile as the target unless :code:`explode_nuclear_max_range` allows it to be
    further away.

Paradrop Unit
  Move the actor unit to the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_paradrop_unit`
  * can result in the conquest of the city at the target tile if:

    * the actor player isn't an animal barbarian.
    * the actor unit has the :code:`CanOccupyCity` unit class flag
    * the actor don't have the :code:`NonMil` unit type flag
    * the actor's relationship to the target is War
    * the target city contains 0 units

  * the distance between actor and target is from 1 to :code:`paratroopers_range`
  * the actor unit hasn't paradropped this turn
  * the actor unit isn't transporting another unit (!)
  * the actor unit can't be diplomatically forbidden from entering the target tile. (!)
  * the target tile is known (doesn't have to be seen) by the actor
  * if the target tile is seen:

    * the actor unit must be able to exist outside a transport on the target tile if the target tile doesn't
      have a visible transport the actor unit is able to load into on landing
    * the target tile can't contain a city belonging to a player the actor has peace, cease-fire or armistice
      with.
    * the target tile can't contain any seen unit belonging to a player the actor player has peace, cease-fire
      or armistice with.

Transform Terrain
  Transform tile terrain type.

  Rules:

  * UI name can be set using :code:`ui_name_transform_terrain`
  * the actor unit has :code:`Settlers` flag (!)
  * terrain type must be one that can be transformed

Cultivate
  Transform tile terrain type by irrigating

  Rules:

  * UI name can be set using :code:`ui_name_cultivate_tf`
  * the actor unit has :code:`Settlers` flag (!)
  * terrain type must be one that can be transformed by irrigating

Plant
  Transform tile terrain type by planting

  Rules:

  * UI name can be set using :code:`ui_name_plant`
  * the actor unit has :code:`Settlers` flag (!)
  * terrain type must be one that can be transformed by mining

Pillage
  Pillage extra from tile

  Rules:

  * UI name can be set using :code:`ui_name_pillage`
  * terrain type must be one where pillaging is possible
  * the target extra must be present at the target tile
  * the terrain of the target tile must have a non 0 pillage_time
  * no other unit can be pillaging the target extra
  * the target extra must have the Pillage removal cause
  * the target extra's rmreqs must be fulfilled
  * the target extra can't be a dependency of another extra present at the target tile
  * the target extra can't have the :code:`AlwaysOnCityCenter` extra flag if the target tile has a city
  * the target extra can't have the :code:`AutoOnCityCenter` extra flag if the target tile has a city and the
    city's owner can rebuild it
  * the target extra must be the rule chosen extra if the civstyle section's :code:`pillage_select` is FALSE

Clean Pollution
  Clean extra from the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_clean_pollution`
  * actor must be on the same tile as the target.
  * the actor unit has the :code:`Settlers` unit type flag (!)
  * the target extra must be present at the target tile
  * the terrain of the target tile must have a non 0 clean_pollution_time
  * the target extra must have the CleanPollution removal cause
  * the target extra's rmreqs must be fulfilled
  * the target extra can't have the :code:`AlwaysOnCityCenter` extra flag if the target tile has a city
  * the target extra can't have the :code:`AutoOnCityCenter` extra flag if the target tile has a city and the
    city's owner can rebuild it

Clean Fallout
  Clean extra from the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_clean_fallout`
  * actor must be on the same tile as the target.
  * the actor unit has the :code:`Settlers` unit type flag (!)
  * the target extra must be present at the target tile
  * the terrain of the target tile must have a non 0 clean_fallout_time
  * the target extra must have the CleanFallout removal cause
  * the target extra's rmreqs must be fulfilled
  * the target extra can't have the :code:`AlwaysOnCityCenter` extra flag if the target tile has a city
  * the target extra can't have the :code:`AutoOnCityCenter` extra flag if the target tile has a city and the
    city's owner can rebuild it

Build Road
  Build road at the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_road`
  * actor must be on the same tile as the target.
  * the actor unit has the :code:`Settlers` unit type flag (!)
  * the target tile can't have an extra that the target extra must bridge over (see extra type's bridged_over)
    unless the actor player knows a tech with the "Bridge" tech flag
  * the target extra (the extra to be built) is an road
  * the target tile doesn't already have the target extra
  * the target extra is buildable (see extra type's buildable)
  * the target tile's terrain's road_time isn't 0
  * if the target extra is both a road and a base the target tile's terrain's base_time isn't 0
  * if the target extra is both a road and a base the target extra can't claim land (see base type's
    border_sq) if the target tile has a city
  * to begin a road the build requirements of the target road (see road type's first_reqs) must be fulfilled.
    Building a road when no (cardinal) adjacent tile has the target extra is considered beginning it.
  * the build requirements of the target extra (see extra type's reqs) must be fulfilled

Build Base
  Build base at the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_build_base`
  * the actor unit has the :code:`Settlers` unit type flag (!)
  * the target tile can't have an extra that the target extra must bridge over (see extra type's bridged_over)
    unless the actor player knows a tech with the "Bridge" tech flag
  * the target extra (the extra to be built) is an base
  * the target tile doesn't already have the target extra
  * the target extra is buildable (see extra type's buildable)
  * the target tile's terrain's base_time isn't 0
  * the target extra can't claim land (see base type's border_sq) if the target tile has a city
  * if the target extra is both a road and a base the target tile's terrain's road_time isn't 0
  * the build requirements of the target extra (see extra type's reqs) must be fulfilled

Build Mine
  Build mine at the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_build_mine`
  * actor must be on the same tile as the target.
  * the actor unit has the :code:`Settlers` unit type flag (!)
  * the target tile can't have an extra that the target extra must bridge over (see extra type's bridged_over)
    unless the actor player knows a tech with the "Bridge" tech flag
  * the target extra (the extra to be built) is a mine
  * the target tile doesn't already have the target extra
  * the target extra is buildable (see extra type's buildable)
  * the target tile's terrain's mining_time isn't 0
  * the target tile's terrain's mining_result is "yes"
  * if the target extra is both a mine and a base the target tile's terrain's base_time isn't 0
  * if the target extra is both a mine and a base the target extra can't claim land (see base type's
    border_sq) if the target tile has a city
  * if the target extra is both a mine and a road the target tile's terrain's road_time isn't 0
  * the build requirements of the target extra (see extra type's reqs) must be fulfilled

Build Irrigation
  Build irrigation at the target tile.

  Rules:

  * UI name can be set using :code:`ui_name_irrigate`
  * actor must be on the same tile as the target.
  * the actor unit has the :code:`Settlers` unit type flag (!)
  * the target tile can't have an extra that the target extra must bridge over (see extra type's bridged_over)
    unless the actor player knows a tech with the "Bridge" tech flag
  * the target extra (the extra to be built) is an irrigation
  * the target tile doesn't already have the target extra
  * the target extra is buildable (see extra type's buildable)
  * the target tile's terrain's irrigation_time isn't 0
  * the target tile's terrain's irrigation_result is "yes"
  * if the target extra is both an irrigation and a base the target tile's terrain's base_time isn't 0
  * if the target extra is both an irrigation and a base the target extra can't claim land (see base type's
    border_sq) if the target tile has a city
  * if the target extra is both an irrigation and a road the target tile's terrain's road_time isn't 0
  * the build requirements of the target extra (see extra type's reqs) must be fulfilled

Transport Disembark
  Exit transport to target tile.

  Rules:

  * UI name can be set using :code:`ui_name_transport_disembark`
  * the actor unit must be on a tile next to the target.
  * the actor unit has at least one move fragment left (!)
  * actor must be transported (!)
  * the actor unit's transport must be in a city or in a native base if the transport's unit class has the
    :code:`Unreachable` unit class flag and the actor's unit type doesn't list the target's unit class in disembarks.
  * the actor unit doesn't have the "CoastStrict" unit type flag or the target city is on or adjacent to a
    tile that doesn't have the :code:`UnsafeCoast` terrain flag.
  * the actor unit can't be diplomatically forbidden from entering the target tile.
  * the actor unit's type must be the target tile's terrain animal if the player's nation is an animal barbarian.
  * actor unit must be able to exist outside of a transport at the target tile.
  * actor unit must be able to move to the target tile.
  * the target tile isn't blocked for the actor unit by some other unit's zone of control (ZOC)
  * the target tile can't contain any city or units not allied to the actor unit and all its cargo.

Transport Disembark 2
  Exit transport to target tile.

  Rules:

  * UI name can be set using :code:`ui_name_transport_disembark_2`
  * A copy of "Transport Disembark".
  * See "Transport Disembark" for everything else.

Actions Done By A Unit To It Self
=================================

Disband Unit
  Disband the unit.

  Rules:

  * spends the actor unit. Gives nothing in return. No shields spent to build the unit is added to the shield
    stock of any city even if the unit is located inside it.
  * UI name can be set using ui_name_disband_unit
  * "Help Wonder" must be impossible
  * "Recycle Unit" must be impossible

Fortify
  Fortify at tile

  Rules:

  * UI name can be set using :code:`ui_name_fortify`
  * the actor unit can't already be fortified (!)

Convert Unit
  Convert the unit to another unit type.

  Rules:

  * UI name can be set using :code:`ui_name_convert_unit`
  * actor unit must have a type to convert to (convert_to).
  * actor unit's converted form must be able to exist at its current location.
  * actor unit's converted form must have room for its current cargo.

Ruleset Defined Actions
=======================

User actions are "blank". The ruleset does everything they do. The following ruleset variables allows user
action number n to be further customized:

* :code:`ui_name_user_action_n`: The UI name shown to the user in the action selection dialog.
* :code:`user_action_n_target_kind`: The kind of target the action is done to. See target_reqs.
  Legal values: "individual cities", "individual units", "unit stacks", "tiles" or "itself"
* :code:`user_action_n_min_range` and :code:`user_action_n_max_range`: What distance from the actor to the
  target is permitted for the action.
* :code:`user_action_n_actor_consuming_always`: TRUE if Freeciv21 should make sure that the actor is spent
  after the action is successfully done.

The current ruleset defined actions are "User Action 1", "User Action 2" and "User Action 3".
