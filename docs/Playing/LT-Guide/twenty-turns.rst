.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

The Next 20 Turns
*****************

The early game is one of expansion. In most Longturn Traditional (:term:`LTT`) or Longturn Experimental
(:term:`LTX`) multiplayer rules-based games you do not need to worry so much about putting a defender(s) into
your new cities right away. Start with :unit:`Workers` to develop the land and build roads between cities. The
quicker you can upgrade the :unit:`Tribal Workers` to regular :unit:`Workers` is also important. The
:unit:`Tribal Workers` do not gain veteran status and actually have less “work points” than regular
:unit:`Workers`. Refer to `Managing your Workforce`_ below for more information.

.. note:
  Many early forms of government support martial law to help with happiness. This is definitely a place where
  some kind of military unit is very important. See `Early Government`_ section below.


Exploring the Area: Opening up the Map
======================================

Every player starts with two (2) :unit:`Explorers`. With the discovery of :advance:`Seafaring` a player can
build more if needed (or replace one that was killed by an enemy). :unit:`Explorers` are often your first form
of in game diplomacy. As you move your :unit:`Explorers` around (ideally from mountain to mountain for the
extra vision) you are going to find your neighbors. :term:`LTT`/:term:`LTX` rules enable a “contact” embassy
to be created for the current turn if your :unit:`Explorers` comes into contact with another nation’s unit (of
any kind). For an :unit:`Explorer` to come into “contact” with another unit, they simply are adjacent to each
other.

This is a good opportunity to do two things:

#. Reach out to the player in game chat or on Discord and start a dialog. Ask permission to explore their
   territory. This is the best way to A) not aggravate your neighbor and B) keep your :unit:`Explorers` alive.

#. In the game, open the diplomacy screen via the “meet” button on the
   :ref:`Nations View <game-manual-nations-and-diplomacy-view>` and ask to swap embassies. Many players will
   agree to the sharing of embassies as it gives important information that both sides can equally use. This
   is why it is important to do #1 first, so you have already started a dialog with your neighbor.

At a minimum, find out where your neighbor’s capital city and major borders are. It goes without saying that
you should not overstay your welcome. There is also nothing wrong with placing an :unit:`Explorers` on a
shared border, on a tile inside your border, to act as a sentry.

.. note::
  As opposed to typical single-player :term:`AI` games, “Armistice” is the default state between nations in
  :term:`LTT` and "War" for :term:`LTX` rulesets. When you meet a player and establish a contact embassy, you
  will automatically go into Armistice or War with that player.

  This setting comes from the ``initial_diplomatic_state`` setting in the :file:`game.ruleset`
  :ref:`ruleset file <modding-rulesets>` and is not shown in game help.


Deciding on an Initial Strategy
===============================

Early Government
----------------

Everyone starts with Despotism as their form of government. If you are land constrained there is the City
States government available with :advance:`Iron Working` (in the :term:`LTX` ruleset) that gives some nice
benefits, but also has some constraints as well (see game help).

Most players either go the Despotism → Monarchy → Democracy path or the Despotism → Republic → Democracy path.
It is easier to reach Monarchy than Republic. There is also a school of thought to abandon Despotism early and
go Tribal right away. There are some differences between the two early governments that are worth comparing.
For example Tribal has better production at the expense of science (bulbs) output.

This table provides a side-by-side comparison of Despotism vs Tribal for a complete picture.

.. list-table:: Despotism vs Tribal
  :widths: 20 40 40
  :header-rows: 1

  * - Area
    - Despotism
    - Tribal
  * - General
    - Your capital city gets a +75% bonus to gold production.
    - Each city gets 1 extra content citizen.
  * - Veteran Chance
    - No change from baseline chance.
    - Increases by half the chance of land units getting the next veteran level after a battle.
  * - Unit Support
    - Each city can **support up to 2 units for free**; further units each cost 1 **gold** per turn.
    - Each city can **support up to 2 units for free**; further units each cost 1 **shield** per turn.
  * - Aggression
    - Unlike later governments, military units do not cause unhappiness even when deployed aggressively.
    - Unlike later governments, military units do not cause unhappiness even when deployed aggressively.
  * - Corruption
    - Base **corruption is 20%**. This increases with distance from the capital (half as fast with
      :advance:`The Corporation`).
    - Base **corruption is 30%** (the highest under any government). This increases with distance from the
      capital (half as fast with :advance:`The Corporation`).
  * - Corruption Increase Per Tile Away from Capital
    - 2%
    - 2%
  * - Waste
    - Base **production waste is 10%**. This increases with distance from the capital (half as fast with
      :advance:`Trade`).
    - There is no base level of production waste, but an increasing amount with distance from the capital
      (half as fast with :advance:`Trade`).
  * - Waste Increase Per Tile Away from Capital
    - 2%
    - 2%
  * - Trade Loss
    - Trade production will suffer some losses.
    - Trade production will suffer some losses.
  * - Production Loss
    - Shield production will suffer a small amount of losses.
    - **None**.
  * - Unit Upkeep
    - Each of your cities will avoid paying **2 Gold upkeep** for your units.
    - Each of your cities will avoid paying **3 Shield upkeep** for your units.
  * - Civil War Chance
    - If you lose your capital, the chance of **civil war is 40%**.
    - If you lose your capital, the chance of **civil war is 45%**.
  * - Empire Size Penalty
    - You can have up to **10 cities** before an additional unhappy citizen appears in each city due to
      civilization size.
    - You can have up to **12 cities** before an additional unhappy citizen appears in each city due to
      civilization size.
  * - Empire Size Penalty Step
    - After the first unhappy citizen due to civilization size, for **each 10 additional cities** another
      unhappy citizen will appear.
    - After the first unhappy citizen due to civilization size, for **each 14 additional cities** another
      unhappy citizen will appear.
  * - Max Sci/Lux/Tax Rate
    - The maximum rate you can set for Science, Gold, or Luxuries is 60%.
    - The maximum rate you can set for Science, Gold, or Luxuries is 60%.
  * - Martial Law Effect
    - Your units may impose martial law. Each military unit inside a city will force **1 unhappy citizen** to
      become content.
    - Your units may impose martial law. Each military unit inside a city will force **2 unhappy citizens** to
      become content.
  * - Max Martial Law
    - A **maximum of 20 units** in each city can enforce martial law.
    - A **maximum of 3 units** in each city can enforce martial law.
  * - Despotism Penalty
    - Each worked tile that gives more than 2 Food, Shield, or Trade will suffer a -1 penalty, unless the city
      working it is celebrating. (Cities below size 3 will not celebrate.)
    - Each worked tile that gives more than 2 Food, Shield, or Trade will suffer a -1 penalty, unless the city
      working it is celebrating. (Cities below size 3 will not celebrate.)


The First Research Target
-------------------------

There are many ways to go about researching technologies depending on varying goals.

From a Small Wonder perspective (see section on `Small and Great Wonders`_ below), here are some ideas:

* :advance:`Ceremonial Burial` → :advance:`Pottery`: Gives 3 of the 4 Level 1 Small Wonders right away.

* :advance:`Alphabet` (:term:`LTT`) / :advance:`Pictography` (:term:`LTX`) → :advance:`Masonry` →
  :advance:`Mathematics`: Gives :wonder:`Pyramids`.

* :advance:`Horseback Riding` → :advance:`Polytheism`: First good attack unit (:unit:`Elephant`) plus
  :wonder:`Statue of Zeus`.

* :advance:`Mysticism`: :wonder:`Temple of Artemis`.

* :advance:`Astronomy`: :wonder:`Copernicus' Observatory`.

* :advance:`Bronze Working`: Obsoletes :wonder:`Ħal Saflieni Hypogeum` and gives :wonder:`Colossus`.

Path to Monarchy:

* Monarchy Path is :advance:`Alphabet` (:term:`LTT`) / :advance:`Pictography` (:term:`LTX`) →
  :advance:`Ceremonial Burial` → :advance:`Code of Laws` → :advance:`Monarchy`.

Early Defense and Aggression:

* :advance:`Bronze Working` (:unit:`Phalanx`) → :advance:`Horseback Riding` (:unit:`Horsemen`) →
  :advance:`The Wheel` (:unit:`Chariot`) → :advance:`Iron Working` (:unit:`Legion` or :unit:`Swordsman`) →
  :advance:`Polytheism` (:unit:`Elephant`).

The long and short of research is that every game will offer competing priorities given the nature of your
neighbors, strength of varying players, and other factors that will dictate the technology research path you
take.

.. note::
  :term:`LTT`/:term:`LTX` games have :ref:`tech leak <server-option-techleak>` enabled. This means that as
  players learn varying technologies they get cheaper for everyone else following a formula.


.. _lt-guide-settler-race:

The Settler Race
----------------

As described in the :ref:`City Planning <lt-guide-city-planning>` section, there are many ways to spread out
and plan your city placement. In the first 20 turns, you need to pump out as many :unit:`Settlers` as you can
manage and then get those cities built. You are in a race with your neighbors to grab land as quickly as your
nation can do so.

Here are some tips to keep in mind:

* :unit:`Settlers` cost two citizen population, so a city must be size 3 to complete the production of one
  :unit:`Settlers`.

* Production happens before growth during :doc:`/Playing/turn-change`. This means that if the city will grow
  to size 3 at :term:`TC` and also finish production of the :unit:`Settlers` at :term:`TC`, the city will
  grow, but the :unit:`Settlers` **will not** be produced until the next :term:`TC` with some shield waste.

* You can rush-buy :unit:`Settlers` with Gold. Many players will do this the same turn that the city grows to
  size 3 so the :unit:`Settlers` will be finished at :term:`TC`. This is a good approach to keep from running
  into the issue in the previous bullet.

* Use :unit:`Workers` to pre-build roads to planned settling spots so your :unit:`Settlers` get there faster.

* As mentioned before, use a tool such as Inkscape to help you plan out your cities. You can keep the map
  image as a layer and all the city placement objects as another layer(s) so that all you have to do is swap
  out the map image layer as you open up the map with your :unit:`Explorers`.

.. note::
  The Freeciv21 client has a feature to export a complete map to a PNG file.
  See :ref:`game manual <game-manual-game-menu>`.


Pumping Settlers
^^^^^^^^^^^^^^^^

The concept of "Settler Pumping" is probably not new to veteran players, however newer Longturn players may
need some extra information. The idea is to push :unit:`Settlers` from every city. Nothing else is produced at
this phase of the game. Produce them as quickly as possible taking into account growth and production rates.
If needed, store shields in more expensive units to allow a city to grow to size 3 and then change production.

Treat this aspect of the game similar to the United States "Manifest Destiny" period in the mid-19th century.
You want to spam :unit:`Settlers` so you can grab as much land as possible.


Small and Great Wonders
-----------------------

It cannot be emphasized enough that the collection of Small Wonders and some important Great Wonders that each
player can build are very important to a successful civilization’s early growth.

.. note::
  The built-in rulesets shipped with Freeciv21 do not break out wonders into two types. They are all Great
  Wonders, where only one player can build each. The Longturn rulesets break wonders into two types: Small
  and Great. As with the shipped rulesets, Great Wonders in :term:`LTT`/:term:`LTX` games are the same ---
  only one player can build it. Small Wonders in the Longturn rulesets allow for every player to build them.
  Many Small Wonders give empire-wide effects, just like Great Wonders do. We do this to offer some balance
  so really good players do not dominate the wonder race.

This table provides some information on the important wonders before the :advance:`Gunpowder` age.

.. list-table:: Important Early Wonders
  :widths: 6 19 25 25 25
  :header-rows: 1

  * - Tech Tree Level
    - Wonder
    - Required / Obsolete
    - Benefits
    - Notes
  * - 1
    - :wonder:`Ħal Saflieni Hypogeum`
    - :advance:`Ceremonial Burial` / :advance:`Bronze Working`
    - The city having it will get :math:`+6` additional Luxury and will celebrate after size 3.
    - Build in the city with best production as it is simply a :math:`+6` Luxury adder and not based on city
      size. The city will celebrate early by giving a small Science (bulbs) boost.
  * - 1
    - :wonder:`Mausoleum of Mausolos`
    - :advance:`Ceremonial Burial` / :advance:`The Republic`
    - :improvement:`City Walls` and :improvement:`Courthouses` each make one unhappy citizen content.
    - :improvement:`Courthouses` are a **must** to reduce waste and corruption. :improvement:`City Walls` are
      very important for defense at all stages of the game, except the late game when the power of the units
      makes them pretty much impossible to defend against.
  * - 1
    - :wonder:`Hanging Gardens`
    - :advance:`Pottery` / :advance:`Explosives`
    - Makes one unhappy citizen content in every city. This wonder also makes two content citizens happy in
      the city where it is located.
    - Good to put in a big city and really helps with empire size issues.
  * - 1
    - :wonder:`Colossus`
    - :advance:`Bronze Working` / :advance:`Invention`
    - Each tile around the city where this wonder is built that is already generating some Trade produces one
      extra Trade resource.
    - :advance:`Bronze Working` obsoletes the :wonder:`Ħal Saflieni Hypogeum`, which is often built in your
      capital city. You will want to replace the :wonder:`Ħal Saflieni Hypogeum` with the :wonder:`Colossus`
      as soon as you can to continue to get the Trade (Luxury Goods) benefits. Depending on the size of your
      capital, you may actually see a higher level of effect.
  * - 2
    - :wonder:`Pyramids`
    - :advance:`Mathematics` / :advance:`Railroad`
    - Each tile produces :math:`+1` Shield, eliminates the Despotism penalty.
    - Building :wonder:`Pyramids` is a **must** as soon as possible, especially if you do not plan to go
      straight to Monarchy. It is possible to have the :wonder:`Pyramids` by ``T15``. Build it in the highest
      production city. The :wonder:`Pyramids` are also important when you are in Anarchy switching to another
      form of government (e.g. Republic, Democracy, or Federation) so you do not suffer the Despotism penalty.
  * - 2
    - :wonder:`Temple of Artemis`
    - :advance:`Mysticism` / :advance:`Theology`
    - Makes 2 additional unhappy citizens content in every city with a :improvement:`Temple`.
    - :improvement:`Temples` are a **must** before Republic or Democracy. This wonder will help your cities to
      celebrate when the time is right.
  * - 2
    - :wonder:`Statue of Zeus`
    - :advance:`Polytheism` / :advance:`Gunpowder`
    - Eliminates 1 unhappy citizen due to military units abroad, plus each city also avoids 1 Shield of upkeep
      for units.
    - Citizen happiness is an important aspect of the early game. This wonder continues to keep your citizens
      happy as cities grow in size. The second aspect (Shield upkeep) is huge  if you are Tribal and/or
      Republic.
  * - 3
    - :wonder:`Copernicus' Observatory`
    - :advance:`Astronomy` / :advance:`University`
    - Each tile worked by the city where this wonder is built produces one extra research point.
    - Another good one for a big city to get a boost to Science (bulb) output.
  * - 4
    - :wonder:`Sun Tzu's War Academy`
    - :advance:`Feudalism` / :advance:`Metallurgy`
    - All your new military land units start with an additional veteran level.
    - This means that with a :improvement:`Barracks` in the city, all your units will be Veteran 2 (175%)
      right away after production.
  * - 5
    - :wonder:`King Richard's Crusade`
    - :advance:`Chivalry` / :advance:`Navigation`
    - Reduces the unhappiness caused by aggressively deployed military units owned by the city by 1. Under
      governments where unit upkeep is paid in gold, it gives two free gold per city towards upkeep every
      turn.
    - The primary benefit of this wonder is the reduction of gold upkeep for military units. With Monarchy,
      unit upkeep is in Gold. This wonder helps your treasury greatly.
  * - 5
    - :wonder:`Leonardo's Workshop`
    - :advance:`Invention` / :advance:`Combustion`
    - Upgrades two obsolete units per game turn.
    - At about this stage of the game, most players will have a collection of older units that have upgrades
      available. This wonder helps the player automatically upgrade old units to newer versions for free every
      turn.
  * - 5
    - :wonder:`Verrocchio's Workshop`
    - :advance:`Invention` / :advance:`Industrialization`
    - Upgrades one obsolete unit per turn.
    - This is a **Great Wonder**, so only a single player can build it. However, if you are able to get it
      first, you will have an advantage in the free unit upgrade path.


.. note::
  When thinking about Small and Great Wonders. Keep attention to what obsoletes them. For example, if you
  decide to build the :wonder:`Ħal Saflieni Hypogeum` early, you need to keep away from
  :advance:`Bronze Working` for as long as possible or you will lose its effect.

.. note::
  The table above is not an exhaustive list of all the wonders. It is a reflection of the early wonders that a
  player might want to pay attention to. See in game help for a complete list. Small Wonders will show up in
  the :title-reference:`City Improvements` section.

.. tip::
  With the discovery of :advance:`Trade`, you can build the :unit:`Caravan` unit. This allows you to transport
  50 shields of production from one city to another. These units are a great way of rapidly building expensive
  wonders by distributing the workload across many cities at once instead of keeping it all inside a single
  city’s production capacity. All good Longturn players will use them for important Wonders such as
  :wonder:`Verrocchio's Workshop`. You can start and finish a wonder in a single turn with appropriate
  planning.


Your First 10 Cities
====================

Managing Your Cities
--------------------

Using an effective micromanagement strategy with regards to managing your cities is very important, especially
in the early game. Here are a few points on things to think about:

* Concentrate :unit:`Workers` on your capital city. Get it as big as you can (up to size 16) well before you
  learn :advance:`Sanitation`. Many Longturn players will never produce :unit:`Settlers` from their capital
  and only let it grow and effectively turn it into a “wonder” city where all the Small and Great Wonders are
  constructed.

* All cities should mostly concentrate on max food for growth. Where you micromanage is around the time for
  the city to grow to the next size. Any more food that is produced at a new city size is wasted. For example:
  if you only need one food to grow, but the city is producing :math:`+2` food, then you will lose the extra
  food to waste at :term:`TC`. Instead move your citizens around in the city dialog to get the city to only
  produce :math:`+1` food and eliminate the waste.

  .. tip::
    The larger the city the more opportunity for more production. Do not drastically slow down growth simply
    for production. Concentrate on growth instead as production comes with the larger size. Larger cities also
    produce more Gold and research bulbs!

* You get a free Granary "effect" up to size 5, so be sure to keep an eye out and build a
  :improvement:`Granary` in your cities at the same time or before size 5. Production occurs before city
  growth during the turn change process. If you do not build :improvement:`Granary` your growth will stall
  significantly.

* Early governments have a martial law effect to keep citizens happy at size 5+. See in game help for more
  details. This means that with no unit in the city you can get to size 4 and have all your citizens content
  in the city (with no other improvements in the city such as a :improvement:`Temple`). At size 5 you will
  have one unhappy citizen that can be made happy with a military unit placed in the city. At size 6 you will
  need two of them and so on.

  .. tip::
    Some players build cheap :unit:`Warriors` to help with martial law instead of building happiness buildings
    or Small Wonders in the early game. The thinking is the :unit:`Warrior` can be upgraded over time to
    better units with gold or :wonder:`Leonardo’s Workshop`. :unit:`Warriors` cost 10 shields and a
    :improvement:`Temple` costs 25. So you can get 2.5 :unit:`Warriors` for every :improvement:`Temple` for
    the same effect. Also with :unit:`Warriors` available you can move them around to quickly balance out any
    unhappiness in a city while you build other items (such as finishing a :unit:`Settlers`, which will drop
    the city size down making the :improvement:`Temple` unnecessary).

* City Improvements that increase Luxury will then create bulbs, gold and happiness.

* You need :unit:`Workers`, :unit:`Workers`, and more :unit:`Workers`. Cities become very powerful the larger
  they are. The more you can put :unit:`Workers` to “work” on the tiles around your cities the better.
  Irrigate Grassland, irrigate Swamp to Grassland, cut down Forest and convert to Grassland, and then convert
  Plains to Grassland. Irrigated Grassland produces :math:`+3` food per turn and :math:`+4` with Farmland
  (with :advance:`Refrigeration`).

  .. tip::
    Do not forget to upgrade the 5 :unit:`Tribal Workers` to full :unit:`Workers` as soon as you can manage.
    Recall that :unit:`Tribal Workers` are crippled in the rulesets (in different ways in :term:`LTT` vs
    :term:`LTX`, but still less than a full :unit:`Workers`).

Some notes on determining what to build in your cities:

* Until you have at least 20 cities or are out of room to plant more cities you should be building
  :unit:`Workers` and :unit:`Settlers` as quickly as possible. Fill all available space first.

* Pay close attention to the effect varying city improvements will give you to determine if something is worth
  building or not. Think of it as a cost vs benefit analysis.

  * An example will help. Imagine you have a city of size 4 that produces :math:`+2` Trade and another city
    that is size 7 and produces :math:`+10` Trade. You have learned :advance:`Currency` and want to build a
    :improvement:`Marketplace` in all your cities (a good goal). A :improvement:`Marketplace` costs 45 Shields
    to produce and gives a 50% Tax (Trade / Luxury Goods) bonus to the city. For the first city you will only
    get :math:`+1` more Trade and the second you will get :math:`+5` more. This means you have a 1:45
    Trade:Production ratio in the first city and a 5:45 Trade:Production ratio in the second. Obviously build
    (or even buy) the :improvement:`Marketplace` in the bigger city and hold off on it in the smaller city.
    Build :unit:`Workers` instead in the smaller city as they cost 20 Shields and have more utility.

  * Another example was given earlier, but good to repeat here. A :improvement:`Temple` costs 25 Shields for a
    single happy citizen and a :unit:`Warrior` costs 10 for the same effect and has more utility.

.. note::
  If you have not figured it out yet, Longturn games are **math heavy**.

.. _lt-guide-manage-workforce:

Managing Your Workforce
-----------------------

:unit:`Workers` are a major engine for growth of your empire. There is a simple rule of thumb with regards to
:unit:`Workers` --- you can never have too many!

Let us start by talking about the veteran levels in :term:`LTT`/:term:`LTX` games. What is written here
applies to all units, but with different effects.

In the game, the tileset will place a symbol embellishment on the unit to denote its veteran level. The
embellishment will vary by tileset.

The veteran levels for :term:`LTT`/:term:`LTX` are:

* Veteran 1 (v)
* Veteran 2 (vv)
* Veteran 3 (vvv)
* Hardened 1 (h1)
* Hardened 2 (h2)
* Hardened 3 (h3)
* Elite 1 (e1)
* Elite 2 (e2)
* Elite 3 (e3)

This table shows what effect veteran levels have on all units except :unit:`Diplomats` and :unit:`Spies`.

.. table:: Veteran Levels
  :widths: auto

  +------------+-------------------+------------------+-------------------------------------+
  | Vet Level  | Combat Strength   | Move Bonus       | Promotion Chance (%)                |
  |            |                   |                  +-------------+-----------------------+
  |            |                   |                  | In Combat   | By Working (per turn) |
  +============+===================+==================+=============+=======================+
  | Green      | :math:`1` x       | :math:`0`        | 50%         | 9%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Veteran 1  | :math:`1.5` x     | :math:`^1/_3`    | 45%         | 6%                    |
  |            | (from Green)      | (from Green)     |             |                       |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Veteran 2  | :math:`1.75` x    | :math:`^2/_3`    | 40%         | 6%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Veteran 3  | :math:`2` x       | :math:`1`        | 35%         | 6%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Hardened 1 | :math:`2.25` x    | :math:`1\:^1/_3` | 30%         | 5%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Hardened 2 | :math:`2.5` x     | :math:`1\:^2/_3` | 25%         | 5%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Hardened 3 | :math:`2.75` x    | :math:`2`        | 20%         | 4%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Elite 1    | :math:`3` x       | :math:`2\:^1/_3` | 15%         | 4%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Elite 2    | :math:`3.25` x    | :math:`2\:^2/_3` | 10%         | 3%                    |
  +------------+-------------------+------------------+-------------+-----------------------+
  | Elite 3    | :math:`3.5` x     | :math:`3`        | 0           | 0                     |
  +------------+-------------------+------------------+-------------+-----------------------+

The working capacity of each :unit:`Workers` is given by the base movement points. In the
:term:`LTT`/:term:`LTX` rulesets, :unit:`Tribal Workers` have a base work rate of two (2) in :term:`LTT` and
three (3) in :term:`LTX`. Regular :unit:`Workers` have a base work rate of three (3), and :unit:`Engineers`
have a base work rate of six (6). You can see this by looking at the “moves” value for the unit in game help.
The base working rate is then multiplied by the combat strength value when promoted. For example a v1 (v)
:unit:`Workers` has a base rate of :math:`3\times1.5=4.5` and a v2 (vv) :unit:`Workers` has a base rate of
:math:`3\times1.75=5.25` and so on. This is why we ask you to upgrade your :unit:`Tribal Workers` as quickly
as you can, that extra move point is huge over the long turn.

.. note::
  :unit:`Migrants`, :unit:`Immigrants`, and :unit:`Settlers` can do work but cannot be promoted, so their work
  rate remains the same all the time.

Each terrain requires a different amount of work to build infrastructure on it. For example, if you
go to the help entry of :title-reference:`grasslands` (by going to
:menuselection:`Help --> Terrain --> Grasslands`), you will see something like the screenshot below:

.. _Work Points:
.. figure:: /_static/images/how-to-play/work-points.png
  :align: center
  :scale: 75%
  :alt: Grassland Work Points
  :figclass: align-center

  Grassland Work Points


The term “turns” here is a bit of a misnomer from a long-lost era when people used to play short-turn and some
rulesets had the :unit:`Workers` move rate set to 1. What it really means is the total work needed to modify
the terrain, either by building infrastructure or by transforming it to a different terrain. For the rest
of this page, we will refer to these “turns” as “work points”, to avoid confusion with the actual passing of
game turns.

The actual number of work points it takes for the modification of the terrain is given by the total work force
of the :unit:`Workers` and the total work points needed (e.g. you can put more than one :unit:`Workers` on a
single tile and if they do the same activity such as Irrigate they combine efforts). Let us take a particular
example: From the image above, you can see that irrigating the grassland takes 5 ”turns” (i.e. work points).
Now, If you have a v1 :unit:`Workers`, its work rate is :math:`3\times1.5=4.5`, which falls short of 5. So the
:unit:`Workers` takes 2 turns to irrigate the grassland. However, if you have a vv (v2) :unit:`Workers`, its
total work points is :math:`3\times1.75=5.25`. Therefore, the vv :unit:`Workers` can irrigate the grassland in
a single turn! Note that building a road costs two work points, so both :unit:`Tribal Workers` and
:unit:`Workers` can do it in a single turn. At this point in the game, it is really useful to optimize your
workforce to the fullest by using the right :unit:`Workers` for the right job. E.g. if you have a vv
:unit:`Workers`, a v :unit:`Tribal Worker` and a green :unit:`Workers` and want to irrigate two grasslands,
the most optimal way to do it is to let the vv :unit:`Workers` irrigate grassland alone and have the v
:unit:`Tribal Workers` and green :unit:`Workers` work together on the other grassland. Any other combination
will lead to one of the grasslands being overworked (i.e. wasting work points), and the other grassland to be
under-worked (thus needing two turns to be completed).

.. note::
  This information is not really displayed in the game. You pretty much have to do the math on your own to
  determine a complete optimal strategy for each :unit:`Workers` action per turn. You can use the middle-click
  feature to get a popup that will tell you how many turns it will take to complete the action, but it will
  not tell you how many work points are being applied to the tile. You have to do that math yourself.

  Oh, did we tell you that Longturn is math heavy?

.. tip::
  Never allow a :unit:`Workers` to be idle during a turn (not working on anything). You might miss a chance
  for promotion. If you need to move a :unit:`Workers` across the map for some reason and it will take more
  than one turn, stop along the way and build a mine (or something “expensive” in worker points). Then after
  :term:`TC`, move the :unit:`Workers` to where you wanted to go and set the appropriate task.


.. tip::
  When you get :unit:`Engineers` with :advance:`Explosives` they can do advanced terrain alterations (called
  Transform in game). With :unit:`Engineers` you can greatly influence the defense of your cities. Engineers
  can convert any terrain into Hills (certainly will take a few steps). Hills + :improvement:`City Walls`
  gives a large defensive bonus to all defending units. Use them wisely.


Early Military
--------------

Anyone who has played any kind of Longturn game knows that in the early turns, cities are small, not well
defended and do not have a lot of production capacity available. This last point means that it takes many
turns to build anything, especially military units.

In every phase of an :term:`LTT`/:term:`LTX` game, military conflict is effectively a combination of strategy
and cunning. However, one thing that gets missed sometimes is military conflict is also a factor of shield and
gold production. If you can lose fewer units (e.g. production value) than your opponent, you can often come
out on top. The caveat to this, especially during the early game, is that any production you put towards a
military conflict is not being used to grow your empire. This early aggression can cripple any growth plans,
so you have to be very sure that the military aggression path is in your best interest.

As a general rule of thumb, if you are land constrained it makes sense to go all in with military unit
production to take cities of nearby neighbors. If there is room to grow, it would be better to work to block
your opponent(s) from taking land from you instead and grow that way.

.. note::
  When going for conquest against enemy cities without :improvement:`City Walls`, remember that most land
  based units will kill off the population. :term:`LTT`/:term:`LTX` rulesets have the ``KillCitizen`` flag on
  most land based units. Only ships and aircraft do not cause population loss. If there are more units in the
  city than the city population, you will destroy the city (create Ruins) instead of taking it. Also remember
  that entering the city also kills one citizen.
