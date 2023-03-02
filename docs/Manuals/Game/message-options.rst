.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: 2023 James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Game Message Options
********************

As discussed in the :ref:`Message Options <game-manual-message-options>` section of the game manual, there is
a large collection of message output options available. This page lists them in detail. Here is a sample of
the top of the message options dialog.

.. _Message Options Dialog2:
.. figure:: /_static/images/gui-elements/message-options.png
  :scale: 65%
  :align: center
  :alt: Freeciv21 Message Options dialog
  :figclass: align-center

  Message Options Dialog


.. note::
  Messages are essentially a visual response to an event that occur during a turn or turn change in a game.

The column headings do the following:

* :strong:`Out` -- The event message will be placed into the :guilabel:`Server Chat/Command Line` widget on
  the main map.
* :strong:`Mes` -- The event message will be placed into the :guilabel:`Messages` widget as described in
  :ref:`Messages <game-manual-messages>`.
* :strong:`Pop` -- The event message will be displayed in a pop-up message box.

The following is a detailed view of all of the message options available in Freeciv21. Each entry provides
a short description of the message, the default setting and the name of the event.

AI Debug messages
  This event outputs AI calculations and other messages related to the in-game AI (e.g. the computer running
  one or more nations in a game).

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_AI_DEBUG``

Broadcast Report
  This event outputs a historian report at given turn intervals. Historian reports are often a widget on the
  main map,  however there is an associated message generated at the same time.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_BROADCAST_REPORT``

Caravan actions
  This event outputs information on an action a :unit:`Caravan` has taken. Examples include establishing a
  trade route or helping to build a wonder.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CARAVAN_ACTION``

Chat error messages
  This event is associated with any kind of error message related to commands entered in the
  :guilabel:`Server Chat/Command Line` widget on the main map.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_CHAT_ERROR``

Chat messages
  This is simply chat messages between the public (all players on a server) and allies (all players on the
  same team or allied together).

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_CHAT_MSG``

City: Building Unavailable Item
  You will receive this message if you attempt to build something in a city, but cannot build it for some
  reason. There are a number of reasons when this event can come up. Some examples are: you added future
  targets to your city worklist, but do not have the technology available to build the item; you already have
  the building in your city and accidentally asked the city to build it again; you already have a unique unit
  built and you ask the city to build another one.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_CANTBUILD``

City: Captured/Destroyed
  The city named in the message has either been captured by your enemy or has been destroyed and turned into
  ruins.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_LOST``

City: Celebrating
  The named city's citizens are so happy that they are celebrating in your honor! Some tilesets show a
  fireworks effect over the city when this event occurs to help you identify the city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_LOVE``

City: City Map Changed
  The working radius of the named city has changed. This can happen when certain city improvements are
  constructed in the city or with a certain technology being researched, which are often ruleset dependent.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_RADIUS_SQ``

City: Civil Disorder
  The citizens of the named city are so angry that the city is in disorder. Most tilesets show a raised fist
  icon over the city when this event occurs to help you identify the city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_DISORDER``

City: Disaster
  The named city has experienced some kind of disaster. Some examples include: a fire has destroyed a city
  improvement; a flood has destroyed all food saved in the city's granary; an explosion in a
  :improvement:`Factory` has caused some damage.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_DISASTER``

City: Famine
  The named city has experienced famine. Famine occurs when the city is producing less food than is required
  to maintain (feed) the citizens within the city and the granary is empty. When this event occurs, the city
  will be reduced in size by one.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_FAMINE``

City: Famine Feared
  Very similar to the message item above. If a city is close to experiencing famine, this message will alert
  you to the impending event. You will have time to act on the named city to prevent famine if possible when
  you see this message.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_FAMINE_FEARED``

City: Growth
  The named city has grown! This event occurs when the accumulated food in the city's granary has met or
  exceeded the amount needed for the next food box size. You can see this information in the
  :doc:`city-dialog` box on the general tab. Look for the granary stats and the
  food surplus.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_GROWTH``

City: Has Plague
  The named city has experienced a plague. A plague can cause population loss or prevent the city from growing
  to the next size. Some rulesets allow for city improvements that can reduce the chance of plague in a city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_PLAGUE``

City: May Soon Grow
  The named city is close to filling its granary and will grow to the next size.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_CITY_MAY_SOON_GROW``

City: Needs Improvement to Grow
  The named city is trying to grow, but cannot due to the lack of an improvement. In many rulesets, the
  :improvement:`Aqueduct` and :improvement:`Sewer System` are needed for cities to grow beyond certain sizes.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_IMPROVEMENT``

City: Needs Improvement to Grow, Being Built
  The named city will soon grow and needs the city improvement that is currently being produced. The
  message may show many turns in advance, giving you ample time to determine if it needs to be rush bought or
  can finish on its own.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_IMPROVEMENT_BLDG``

City: Normal
  A city that was previously in disorder is now no longer in disorder.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_NORMAL``

City: Nuked
  The named city has been hit with a :unit:`Nuclear` bomb or other similar type of unit.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_NUKED``

City: Production changed
  The named city has changed what is at the top of the worklist.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_CITY_PRODUCTION_CHANGED``

City: Released from citizen governor
  The named city was previously under control of the :doc:`/Playing/cma` and can no longer fulfill the
  requirements.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_CMA_RELEASE``

City: Suggest Growth Throttling
  The named city is producing a :improvement:`Granary` and may grow before the improvement is complete. You
  will want to ensure that the :improvement:`Granary` is completed before the turn when the city will grow, or
  you will lose the benefits of the improvement for one city growth cycle.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_GRAN_THROTTLE``

City: Transfer
  The named city has been transferred as part of a diplomatic agreement.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CITY_TRANSFER``

City: Was Built
  The named city has been founded by :unit:`Settlers`.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_CITY_BUILD``

City: Worklist Events
  The named city has had some kind of worklist change. This often occurs when you change the type of item
  being produced such as going from an improvement to a unit.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_WORKLIST``

Connect/disconnect messages
  Outputs when users connect and disconnect from a game server. These are often seen on Longturn multiplayer
  games.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_CONNECTION``

Deprecated Modpack syntax warnings
  An installed Modpack uses syntax that may stop working in future versions of the game.

  .. hlist::
    :columns: 2

    * Default: Mes and Pop
    * Event Name: ``E_DEPRECATION_WARNING``

Diplomat Action: Bribe
  Your :unit:`Diplomat` or :unit:`Spy` was successful in bribing an enemy unit.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_BRIBE``

Diplomat Action: Caused Incident
  Your :unit:`Diplomat` or :unit:`Spy` was successful in causing an incident in a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_DIPLOMATIC_INCIDENT``

Diplomat Action: Embassy
  Your :unit:`Diplomat` or :unit:`Spy` was successful in establishing an embassy with another nation.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_EMBASSY``

Diplomat Action: Escape
  Your :unit:`Diplomat` or :unit:`Spy` was successful in escaping detection from the enemy nation.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_ESCAPE``

Diplomat Action: Failed
  Your :unit:`Diplomat` or :unit:`Spy` was unsuccessful in the named action taken.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_FAILED``

Diplomat Action: Gold Theft
  Your :unit:`Spy` was successful in stealing gold from a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_SPY_STEAL_GOLD``

Diplomat Action: Incite
  Your :unit:`Diplomat` or :unit:`Spy` was successful in inciting a targeted city to revolt.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_INCITE``

Diplomat Action: Map Theft
  Your :unit:`Spy` was successful in stealing maps from an enemy nation.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_SPY_STEAL_MAP``

Diplomat Action: Poison
  Your :unit:`Diplomat` or :unit:`Spy` was successful in poisoning the citizens of a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_POISON``

Diplomat Action: Sabotage
  Your :unit:`Diplomat` or :unit:`Spy` was successful in sabotaging the production of a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_SABOTAGE``

Diplomat Action: Suitcase Nuke
  Your :unit:`Spy` was successful in deploying a suitcase tactical nuclear device in a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_SPY_NUKE``

Diplomat Action: Theft
  Your :unit:`Diplomat` or :unit:`Spy` was successful in stealing a technology advance from an enemy nation.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_DIPLOMAT_THEFT``

Diplomatic Message
  This message appears when some kind diplomatic event has occurred. Examples include: accepting or canceling
  a diplomatic meeting; in-game allied AI asks for assistance; in-game AI threatens to kill you.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_DIPLOMACY``

Enemy Diplomat: Bribe
  An enemy's :unit:`Diplomat` or :unit:`Spy` was successful in bribing one of your units.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_BRIBE``

Enemy Diplomat : Embassy
  An enemy's :unit:`Diplomat` or :unit:`Spy` was successful in establishing an embassy with your nation.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_EMBASSY``

Enemy Diplomat: Failed
  An enemy's :unit:`Diplomat` or :unit:`Spy` was unsuccessful in the named action taken.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_FAILED``

Enemy Diplomat: Gold Theft
  An enemy's :unit:`Spy` was successful in stealing gold from a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_SPY_STEAL_GOLD``

Enemy Diplomat: Incite
  An enemy's :unit:`Diplomat` or :unit:`Spy` was successful in inciting a targeted city to revolt.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_INCITE``

Enemy Diplomat: Map Theft
  An enemy's :unit:`Spy` was successful in stealing your maps.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_SPY_STEAL_MAP``

Enemy Diplomat: Poison
  An enemy's :unit:`Diplomat` or :unit:`Spy` was successful in poisoning the citizens of a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_POISON``

Enemy Diplomat: Sabotage
  An enemy's :unit:`Diplomat` or :unit:`Spy` was successful in sabotaging the production of a targeted city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_SABOTAGE``

Enemy Diplomat: Suitcase Nuke
  An enemy's :unit:`Spy` was successful in deploying a suitcase tactical nuclear device in a targeted
  city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_SPY_NUKE``

Enemy Diplomat: Theft
  An enemy's :unit:`Diplomat` or :unit:`Spy` was successful in stealing a technology advance from you.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ENEMY_DIPLOMAT_THEFT``

Error message from bad command
  This message appears when any kind of incorrect command you give the game occurs.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_BAD_COMMAND``

Extra Appears or Disappears
  This message appears when you use :unit:`Workers` or :unit:`Engineers` to terraform terrain that had a
  special "extra" on the tile or when you terraform it back to the original terrain.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_SPONTANEOUS_EXTRA``

Game Ended
  The game has ended. The final player report is shown.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_GAME_END``

Game Started
  The game has started.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_GAME_START``

Global: Eco-Disaster
  Global Warming or Nuclear Winter has occurred.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_GLOBAL_ECO``

Global: Nuke Detonated
  A player has detonated a :unit:`Nuclear` device on the map. Coordinates are given in the message.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_NUKE``

Help for beginners
  Messages to aid new players.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_BEGINNER_HELP``

Hut: Barbarians in a Hut Roused
  One of your units has entered a hut on the map and roused :unit:`Barbarians`.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_BARB``

Hut: City Founded from Hut
  One of your units has entered a hut on the map and founded a city for you at that location.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_CITY``

Hut: Gold Found in Hut
  One of your units has entered a hut on the map and found gold inside. The message will contain the amount of
  gold found.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_GOLD``

Hut: Killed by Barbarians in a Hut
  One of your units has entered a hut on the map and was killed by :unit:`Barbarians`.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_BARB_KILLED``

Hut: Mercenaries Found in Hut
  One of your units has entered a hut on the map and mercenaries were found that join your nation. Mercenaries
  are often the best attacking unit that you have the technology for.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_MERC``

Hut: Settler Found in Hut
  One of your units has entered a hut on the map and found :unit:`Settlers` inside that can be used to build
  a city at a location of your choice.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_SETTLER``

Hut: Tech Found in Hut
  One of your units has entered a hut on the map and found `scrolls of wisdom` containing a technology
  advance.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_TECH``

Hut: Unit Spared by Barbarians
  One of your units has entered a hut on the map and was not killed by a band of :unit:`Barbarians`.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_HUT_BARB_CITY_NEAR``

Improvement: Bought
  You have rush bought with gold a city improvement in the named city.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_IMP_BUY``

Improvement: Built
  The named city has completed construction of the listed city improvement.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_IMP_BUILD``

Improvement: Forced to Sell
  Your national treasury did not have enough gold to maintain the upkeep of all of your city improvements at
  turn change, so the game sold one or more of them.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_IMP_AUCTIONED``

Improvement: New Improvement Selected
  You did not tell a city to build a specific city improvement, so the in-game `Advisor` selected one for you.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_IMP_AUTO``

Improvement: Sold
  You manually sold a named city improvement.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_IMP_SOLD``

Message from server operator
  The server operator has sent a broadcast message to all players. Longturn multiplayer games will use this
  feature sometimes.

  .. hlist::
    :columns: 2

    * Default: Mes and Pop
    * Event Name: ``E_MESSAGE_WALL``

Nation Selected
  You have selected (taken) a nation. This message typically comes up during Longturn multiplayer games when
  you are taking control of a nation either by picking up a idle player or acting as the regent for a team
  mate. In single player games, you can also take control of any of the in-game AI players as well and this
  message will show at that time too.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_NATION_SELECTED``

Nation: Achievements
  Your nation has crossed an achievement boundary. Different rulesets have varying types of achievements
  available.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ACHIEVEMENT``

Nation: Barbarian Uprising
  There has been a :unit:`Barbarian` uprising in the game. Ensure you have sufficient defensive units in your
  cities as they will attack when they find you.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UPRISING``

Nation: Civil War
  Your nation has been broken apart due to Civil War. Some of your cities have broken away from your nation
  and formed a new nation controlled by an in-game AI.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_CIVIL_WAR``

Nation: Collapse to Anarchy
  Too many cities are in disorder and you can no longer maintain a functioning government. Your nation has
  fallen into anarchy.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_ANARCHY``

Nation: First Contact
  One of your units has come into contact with the first enemy nation in the game.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_FIRST_CONTACT``

Nation: Learned New Government
  Your scientists have learned a new technology advance that also allows for a new form of government. A good
  example is learning :advance:`Republic` allows you to form a new government of the same name.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_NEW_GOVERNMENT``

Nation: Low Funds
  Your national treasury is low in gold. If you do not correct the issue, city improvements will be sold at
  turn change.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_LOW_ON_FUNDS``

Nation: Multiplier changed
  Certain effects in a ruleset can create bonuses (e.g. multipliers). This message occurs when one or more
  multipliers has changed values. Details are given as part of the message.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MULTIPLIER``

Nation: Pollution
  The named city's production has caused pollution on a tile in its working radius.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_POLLUTION``

Nation: Revolution Ended
  You started a revolution to form a new government and it is now over. The new form of government is given in
  the message.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_REVOLT_DONE``

Nation: Revolution Started
  You have started a revolution to form a new government and are now in anarchy.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_REVOLT_START``

Nation: Spaceship Events
  One or more of your cities has constructed a :improvement:`Space Component`, :improvement:`Space Module`, or
  :improvement:`Space Structural` for your spaceship. This message will also come up when you launch your
  spaceship, and when the spaceship arrives at Alpha Centauri, or is destroyed along the way and does not make
  it.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_SPACESHIP``

Player Destroyed
  Either you or another player has completely destroyed a player in the game.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_DESTROYED``

Report
  You have asked for a non-modal report such as Demographics or Top Five Cities.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_REPORT``

Scenario/ruleset script message
  This is a message from a ``Lua`` script inside of a scenario or a ruleset. The game tutorial uses these
  extensively.

  .. hlist::
    :columns: 2

    * Default: Mes and Pop
    * Event Name: ``E_SCRIPT``

Server Aborting
  There is a very bad error occurring on the serer and it is aborting/shutting down.

  .. hlist::
    :columns: 2

    * Default: Mes and Pop
    * Event Name: ``E_LOG_FATAL``

Server Problems
  The server is experiencing some problems that are not fatal.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_LOG_ERROR``

Server settings changed
  The server settings have changed. The game admins of Longturn multiplayer games will sometimes have to alter
  the settings after a game has started.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_SETTING``

Technology: Acquired New Tech
  Your nation has acquired a new named technology advance. This can be through a diplomatic agreement, or
  from a Great Wonder such as the :wonder:`Great Library`.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TECH_GAIN``

Technology: Learned New Tech
  Your scientists have researched a new named technology advance for you.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TECH_LEARNED``

Technology: Lost a Tech
  Your scientists are not able to maintain enough research (bulbs) to maintain knowledge and have now
  forgotten/lost a named technology advance.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TECH_LOST``

Technology: Other Player Gained/Lost a Tech
  Your embassy with another player relays a message that the player has gained or lost a named technology
  advance.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TECH_EMBASSY``

Technology: Selected new Goal
  You have given your scientists a new goal to research on the technology tree.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TECH_GOAL``

Treaty: Alliance
  You have formed an alliance pact with the named player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TREATY_ALLIANCE``

Treaty: Broken
  You have broken a named diplomatic pact with a given player. For example you break a peace treaty and go to
  war.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TREATY_BROKEN``

Treaty: Cease-fire
  You have entered into a cease-fire pact with the named player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TREATY_CEASEFIRE``

Treaty: Embassy
  You have established an embassy with the named player. This can occur via a :unit:`Diplomat` or from a
  diplomatic meeting once contact has been established.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TREATY_EMBASSY``

Treaty: Peace
  You have entered into a peace pact with the named player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TREATY_PEACE``

Treaty: Shared Vision
  You have granted shared vision with the named player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TREATY_SHARED_VISION``

Turn Bell
  This event gives a message of the turn number and year when the turn changes.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_TURN_BELL``

Unit: Action Failed
  A named action by a unit has failed.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ACTION_FAILED``

Unit: Attack Failed
  You tried to attack an enemy unit and your unit has been destroyed in the process. The game will give a
  detailed results message that looks like this:

  .. code-block:: rst

    Your attacking {veteran level} {unit name} [id:{number} D:{defense}
    HP:{hit points}] failed against the {enemy nation} {veteran level}
    {unit name} [id:{number} lost {hit points} HP, {hit points} HP
    remaining]!


  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_UNIT_LOST_ATT``

Unit: Attack Succeeded
  Your unit attacked an enemy unit and won the battle. The game will give a detailed results message that
  looks like this:

  .. code-block:: rst

    Your attacking {veteran level} {unit name} [id:{number} A:{attack}
    lost {hit points} HP, has {hit points} remaining] succeeded against
    the {enemy nation} {veteran level} {unit name} [id:{number}
    HP:{hit points remaining}].


  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_UNIT_WIN_ATT``

Unit: Bought
  You have rush bought with gold a unit in the named city.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_UNIT_BUY``

Unit: Built
  The named city has completed construction of the listed unit.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_BUILT``

Unit: Built unit with population cost
  The named city has completed construction of the listed unit that also cost city population. This is often
  :unit:`Settlers` or :unit:`Migrants`.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_BUILT_POP_COST``

Unit: Defender Destroyed
  Your unit has been attacked by an enemy player and while acting as a defender has been destroyed in the
  process. The game will give a detailed results message that looks like this:

  .. code-block:: rst

    Your {veteran level} {unit name} [id:{number} D:{defense}
    HP:{hit points}] lost to an attack by the {enemy nation}
    {veteran level} {unit name} [id:{number} A:{attack} lost
    {hit points} HP, has {hit points} HP remaining].


  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_LOST_DEF``

Unit: Defender Survived
  Your unit has been attacked by an enemy player and while acting as a defender has survived. The game will
  give a detailed results message that looks like this:

  .. code-block:: rst

    Your {veteran level} {unit name} [id:{number} D:{defense} lost
    {hit points} HP, {hit points} HP remaining] survived the pathetic
    attack from the {enemy nation} {veteran level} {unit name}
    [id:{number} A:{attack} HP:{hit points remaining}].


  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_WIN_DEF``

Unit: Did Expel
  You have successfully expelled an enemy unit to its nation's capital city.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_DID_EXPEL``

Unit: Lost outside battle
  This message name can be a bit misleading. You can lose a unit in varying scenarios that do not involve
  direct conflict. Examples include: you leave a unit inside the borders of a nation that you entered into a
  peace pact with; you transfer a city to another player, which also includes any units that are supported by
  that city; a unit is on a transporter unit such as a :unit:`Galleon` and the ship was sunk in an attack.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_LOST_MISC``

Unit: Orders / goto events
  This event occurs when you give units advanced orders using the :menuselection:`Unit --> Goto and...` menu
  option.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ORDERS``

Unit: Production Upgraded
  The named city is producing a unit that has been obsoleted by a technology advance. The newer unit is now
  being constructed. For example: a city is building a :unit:`Phalanx`, however your nation has recently
  discovered :advance:`Feudalism`. The city will change to producing :unit:`Pikemen` instead of
  :unit:`Phalanx`.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_UPGRADED``

Unit: Promoted to Veteran
  One of your units has been promoted to a higher veteran level. The message will give the veteran level.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_BECAME_VET``

Unit: Relocated
  One or more of your units has been relocated on the map. This is often caused when you use :unit:`Engineers`
  loaded on a :unit:`Transport` to terraform ocean to swamp. Global Warming can also cause units to be
  relocated.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_RELOCATED``

Unit: Sentried units awaken
  A unit you have sentried has observed an enemy unit in its vision radius. The message will give details on
  the enemy unit that was observed.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_WAKE``

Unit: Unit did
  Currently unused.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ACTION_TARGET_OTHER``

Unit: Unit did heal
  A unit was healed, e.g. gained hit points.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_UNIT_DID_HEAL``

Unit: Unit did to you
  An enemy unit has taken an action against city. For example, when an enemy :unit:`Spy` sabotages production
  of a city improvement.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ACTION_TARGET_HOSTILE``

Unit: Unit escaped
  An enemy unit has escaped.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ESCAPED``

Unit: Unit illegal action
  This message will appear when you attempt to take an action with a unit that is not possible. For example,
  trying to capture a unit that is not able to be captured.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ILLEGAL_ACTION``

Unit: Unit was healed
  One of your units has been completely healed, e.g. 100% of its hit points has been restored.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_MY_UNIT_WAS_HEALED``

Unit: Was Expelled
  One of your units was expelled by an enemy nation and has been returned to your capital.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_WAS_EXPELLED``

Unit: Your unit did
  A unit of yours was able to take an action against another unit. This message is typically related to
  :unit:`Diplomat` and :unit:`Spy` units.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ACTION_ACTOR_SUCCESS``

Unit: Your unit failed
  A unit of yours was not able to take an action against another unit. This message is typically related to
  :unit:`Diplomat` and :unit:`Spy` units.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_UNIT_ACTION_ACTOR_FAILURE``

Vote: New vote
  The in-game voting process has been activated and you are asked to vote on a topic.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_VOTE_NEW``

Vote: Vote canceled
  The player who initiated the in-game voting process has canceled the vote on a topic.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_VOTE_ABORTED``

Vote: Vote resolved
  The in-game voting process has completed.

  .. hlist::
    :columns: 2

    * Default: Out
    * Event Name: ``E_VOTE_RESOLVED``

Wonder: Finished
  The named Great Wonder has been completed by the listed player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_WONDER_BUILD``

Wonder: Made Obsolete
  One of your named wonders (both Great and Small) has had its effect removed due to becoming obsolete. This
  often occurs when a new technology advance has been discovered or another wonder is constructed.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_WONDER_OBSOLETE``

Wonder: Started
  Construction of the named Great Wonder has been started by the listed player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_WONDER_STARTED``

Wonder: Stopped
  Construction of the named Great Wonder has been stopped by the listed player.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_WONDER_STOPPED``

Wonder: Will Finish Next Turn
  Construction of the named Great Wonder will be finished at the end of the turn by the listed player. If you
  are also building the same wonder, it will become obsolete and you cannot built it.

  .. hlist::
    :columns: 2

    * Default: Mes
    * Event Name: ``E_WONDER_WILL_BE_BUILT``

Year Advance
  This event shows a message that the year has advanced at turn change.

  .. hlist::
    :columns: 2

    * Default: Off/None
    * Event Name: ``E_NEXT_YEAR``
