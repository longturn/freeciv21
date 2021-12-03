The Turn Change Sequence
************************

This page has been adapted from the legacy Freeciv `Wiki <https://freeciv.fandom.com/wiki/Turn>`_.

Freeciv21 is a turn-based game. A turn is a period of the game when all players (human and AI) are allowed to
make a specific number of game actions. Longturn online multi-player games have turns that are often set to a
counter of 23 hours. This allows players all over the world an opportunity to log in and make thier moves at a
reasonable time. Single player games (one human against the AI) do not have a counter set. The turn ends when
all moves have been completed and then the human player will manually clicks the :guilabel:`End Turn` button.

Turns are typically enumerated from ``T1``.

The period of a turn when a specific player or player group can act in a game is called a *phase*. Most often
Freeciv21 is played in the concurrent mode - there is only one phase per turn for everybody (AIs do their
moving only at phase beginning). But in the server options another mode is available: all players move in a
queue, or all teams move in a queue. Most things happen to an empire either at the start or at the end of its
phase (since the default mode is one-phase, these periods are commonly referred as ``turn start`` or
``turn end``, but in alternate playing they will happen to different players in different time) [#f1]_.
Understanding the order in which things happen can give a player considerable advantage.

Cities
  Depending on the ruleset, cities will typically produce one item per turn depending on the surplus shields.
  Some rulesets enable a slots feature and with sufficient surplus shields can produce more than one unit per
  turn. Only one building can be produced per turn. Surplus shields will be saved between turns.

Units
  Units have a specific number of move points (MP) allotted per turn. Veterancy levels can increase the number
  of MPs a unit is allotted per turn. Once the number of moves have been used, there are no more until the
  next turn. If some move points are left over at the end of the turn, they are lost.

Multi-Player vs AI
  As previously noted, online multi-player games have a turn defined as a certain number of hours. This means
  that "moves" by a player can happen at any time during the turn. One player may login and play his moves at
  the beginning of the turn, another may play her moves mid-turn and a third may play near the end of the
  turn. The main point here is that in this scenario, just because you have made your moves does not mean that
  all moves for all players have completed. During single player games, the AI will make its moves at the
  beginning of the turn and will be finished. The human player will then make his moves.


Turn Change Events
==================

There are a number of functions that are called at Turn Change (TC) that explains the sequence of events that
occurs when either a human player clicks the :guilabel:`End Turn` button or a Longturn multi-player game's
timer expires and the turn ends.

Turn start
  Function: ``begin_turn()``

  * A ``turn_started`` signal (turn, year) is emitted.
  * Player scores are calculated.
  * If fog of war setting was changed, the change is applied.
  * In the concurrent mode, random sequence of players is defined for the things that happen to them in a
    queue. Below, "For Each Player" (FEP) means iterating players in this order
    (``phase_players_iterate(pplayer)`` macro). In teams-alternating mode, player sequence within a team is
    permanent through the game. After the procedure exits, ``begin_turn`` packet is sent to the clients.
    Then, the phases of the game follow in the order of teams' or players' numbers as shown on the Nations
    report.

Phase Start
  Function: ``begin_phase()``

  * All players get available info on each other. Without contact or embassy, it's at least dead/alive, and
    great wonders. Add diplomatic states with players you are in contact with.
  * A ``start_phase`` packet is sent to the clients.
  * FEP: Unit's activities (mining, etc.) are updated for each player's units in their default order: Unit
    activity rate is calculated (if the unit has at least one movement fragment, the rate is proportional to
    its basic move rate and veteranship power factor) that is added to its activity counter.
  * For performing certain activities, the unit might have a chance to become veteran.
  * Unit's move points (MP) are restored.
  * Autoexplorers explore.
  * Counters of the units on the tile that do the same are summed to look if the activity is done. If so,
    changes are applied to the tile with side effects like bouncing ships from ocean converted to swamp.

    * Units doing the same changing activity on the tile become idle. For example, even if we have irrigated
      swamp to grassland and could irrigate more.
    * Adjacent units are also checked for if they haven't they lost their only irrigation source.
    * Fortifying and converting units do the thing.

  * FEP: Unit orders are executed, units are processed by the default player's unit list (not necessarily in
    command order).
  * FEP (human only): Building advisor prepares advice for cities.
  * FEP: City data are sent.
  * FEP (alive): Revolution finishes, if it's the time and target government is specified (if not, a
    notification is sent).
  * FEP (AI): Diplomatic meetings are considered.
  * FEP (AI): Do turn-begin activities (move units, etc.)
  * Players that don't have units or cities die from the game.
  * The timer is started.
  * Global game info is sent to the clients.

  Since this moment, turn is considered new even if the game was restarted from savegame. If turn autosaves of
  game/map image are enabled, now one happens.

Running a phase
  Function: ``server_sniff_all_input()``

  * Server watches for all packets clients send to it. Phase players can play until phase time runs off or
    every human presses :guilabel:`End Turn` button. After that moment, timer is cleared and phase end starts.

Phase End
  Function: ``phase_end()``

  During phase end, server-client packages go to a buffer, that is unbuffered when the phase ends to end.

  * An ``end_phase`` packet is sent.
  * FEP: Techs updated:

    * If a player has not set what to research, a tech towards his or her goal is selected, or random tech (by
      game random tech setting) if no goal.
    * If a tech is going to be lost, future techs are reduced (if any), or random losable tech (holes
      allowed/not) is lost.

  * At this moment, city state updates are stopped due to many things that leave them in intermediate
    out-of-the-game state.
  * FEP (AI): Unit end turn AI activities (no movement)
  * FEP:

    * Auto-settlers do their move to work terrain.
    * For AIs, governments, techs, taxes, cities and space program are handled.
    * The ``Tech_Parasite`` (Great Library) effect may bring techs known to others (depending on ruleset).
    * Auto-upgrade (Leonardo's Workshop depending on ruleset)

  * For each player's unit:

    * Hit ponts (HP) regenerate/shrink;
    * If they shrink to zero or below, the unit dies;
    * Fueled units running out of fuel try to seek a resort automatically within left movepoints.

  * Fueled units are refueled if possible, or their fuel is reduced, and they crash if it goes zero or below.
  * Spaceship parts autoplaced.
  * Cities are updated. For cities in their normal order:

    * Citizen assimilation handled;
    * Traderoutes that no longer can exist are cancelled.

  * Now, for the player's cities in a randomized order:

    * City is refreshed [#f2]_. Workers are auto-arranged if radius has changed significantly.
    * Unit upkeep is recalculated.
    * If something changes, the workers are arranged by some default manager.
    * If not enough shields, units upkept with them are disbanded (in city units list order). If it does not
      balance without touching undisbandable units, a citizen is spent on the upkeep of each such unit. The
      city may be destroyed in effect. If, otherwise, some surplus shields remain, they are added to the
      shield stock.
    * The production is remembered for the case it is changed to another genus.
    * City tries to produce something:

      * For Mint-like buildings, remained shields are converted to gold, then, if something else is on the
        plan, the production changes.
      * For other improvements, here production target is upgraded if it becomes obsolete (to the ``replaced_by``
        building).
      * If you still can't in principle build the improvement any more (your techs don't allow it, your
        spaceship is finished etc.), here you get a notification and a signal is emitted.
      * Otherwise, if your shield stock [#f3]_ is greater or equal to the improvement cost:

        * For small wonders that can be built, another instance of this small wonder in the player's empire is
          removed.
        * For space parts, they are produced [#f4]_. Other improvements appear in the city; wonders are updated
          right this moment to the cache used by requirements; for global wonders, notifications are sent to
          everybody. Then shield stocks are reduced on the used cost, and the ``building_built`` signal is
          emitted (as any signal, might potentially destroy the city right here).
        * City vision radius is updated.
        * Darwin's Voyage effect for the building may give techs.
        * If it was a space part, corresponding information is sent around. Otherwise, the city is
          refreshed [#f2]_ and the workers are auto-arranged if the radius has changed.
        * Production is changed according to the worklist. If no worklist, then, if the building is a special
          one that can be built again (Gold), it is started again, otherwise new one is chosen by the advisor.

  * If a unit is produced:

    * If the production can be changed (the city has not bought the former turn) and the unit is obsolete,
      city switches to the obsoleting unit.
    * If the city does not fulfil the units requirements (tech, improvement, unit has no ``NoBuild`` flag...)
      and the player is not barbarian, it is notified and a signal is emitted: city surival is not checked.
    * Otherwise, if we have enough shields to build the unit:

      * If it is a settler consuming last population, we have at least two cities and the city setting
        enabling this is on, the city is disbanded (otherwise, a notification and a signal of not being able
        to produce happens), its units, including the newly created one, are transferred to the nearest city,
        and the city processing is finished here.
      * Otherwise, the city remembers that we have built last this turn.
      * If some population cost is paid, the city size is reduced and the city is updated (citizens,
        borders...) and refreshed [#f2]_ with workers auto-arranging if the workable tiles change.
      * Shields are reduced on paid cost. Notifications and ``unit_built`` signal are emitted.
      * If we have additional building slots, the unit we build does not cost pop and is not unique, it can
        be produced more than one time. If the city has worklist, to use the full ``City_Build_Slots``
        effect, the unit should be repeated at the top of the list so many times (the positions will be
        removed). For floor (``shield_stock`` / ``unit_cost``), similar units are built with corresponding
        shield stock reduction (cycle breaks only if the city is destroyed in process).

  * Here it's checked if the game is over and it's time to leave the game.
  * If the city is big enough and was happy before, it celebrates and gets its rapture counter up, otherwise
    any celebrations are cancelled and the counter is zeroed. Then, the city's "was happy" switch is updated
    from its feeling level [#f5]_.
  * Plague is checked.
  * City gets its food surplus into its granaries.
  * If the full granary size is achieved or overdone and the plague has not just struck, or the city has
    rapture grow this turn, the city tries to grow. If no necessary aqueduct, it just loses some food (but
    granary building effect reduces this loss); any way, all that does not fit into the (new) food stock is
    lost. A grown city is refreshed [#f2]_.
  * City claimed borders are updated.
  * Food in the city is balanced. If not enough, food-upkept units are disbanded, then a citizen is lost (may
    destroy the city). Granary food left after a shrink is calculated from the granary size of the reduced
    city but with granary preserving effect of the city before shrink.
  * Sell, buy and airlift counters are cleared.
  * Bulbs are harvested to the player's research. (If it has negative bulbs, a tech can be lost here, but
    unlikely).
  * Gold is harvested to the player's treasury, then gold upkeep to buildings and units is paid. If the
    treasury appears in debt after this calculation, balancing happens according to gold upkeep style (not
    here in style 2).
  * If the city is in disorder, notification is sent and disorder turns counter is increased.
  * Pollution is checked (production is calculated without disorder fine)
  * If you rebel enough turns to overthrow your government, it happens.
  * The city is finally refreshed [#f2]_ and workers are auto-arranged.
  * National gold upkeeps are balanced, according to the upkeep style. Nationally supported improvements (not
    wonders) are put on list, and a random one is sold until the gold is positive. The same happens to
    gold-upkept units (transports are disbanded only after all their cargo is checked for disbanding, units
    that can not be "sold" don't return their upkeep into the treasury).
  * If expenses exceed 150% of the treasury, warning "LOW on FUNDS!" is sent to the player.
  * Pay tech upkeep and check for obtaining/losing techs.
  * FEP: Refresh cities vision.
  * At this moment, the players in "dying" state leave the gameboard.
  * Internet or Apollo Program revealing effects show things (depending on ruleset).
  * Marco Polo's Embassy effect gives contact to other players (depending on ruleset).
  * FEP: Phase finished AI function (for human players, also needed to initialize :doc:`cma`).
  * Now connections are unbuffered. If the game is over for this moment, other phases of the turn are not
    started. After done with phases, the turn begins to end.

Turn end
  Function: ``end_turn()``

  * An ``end_turn`` packet is sent.
  * Borders are updated over the map.
  * Barbarians are summoned.
  * If migrations are enabled, they happen, and all cities are sent to players.
  * City disasters.
  * Global warming.
  * Nuclear winter.
  * Diplomatic states are updated (e.g. ceasefires run out). Players are iterated in a two-floor loop, on both
    levels in their main order. This includes moving or destroying illegally positioned units.
  * Historians may do their reports.
  * Settings turn.
  * Voting turn.
  * In-game date advances.
  * Timeout is updated.
  * Game and players info is sent to the clients.
  * Year is sent to the clients.

  Then, the metaserver info is sent. The game is checked to be over by rules or stopped manually; if so, players
  are ranked.

Game over
  Client connections are thawed, and the turn timer is cleared. The scores are calculated and go to the
  scorelog. Map is shown to everybody. Server resends info to the metaserver, and saves the game on gameover.

.. rubric:: Footnotes

.. [#f1] A granary influents food stock if built on the growing turn. Barracks won't regenerate all HP of
   units resting in the city the turn they are built.
.. [#f2] The ``City_Build_Slots`` effect works for making units only.
.. [#f3] Units that obsolete another but have fallen not available for e.g. government change are not
   "downgraded" (but also are not produced, even if you have paid for them!).
.. [#f4] A "``unit_built``" signal is not emitted if you disband a city; ``city_destroyed`` with nil as the
   destroyer parameter is instead. By the way, the unit will have the former city's ``Veteran_Build`` rank
   as a last memory of it.
.. [#f5] City happiness is not immediately updated with building a unit unless it costs population.
