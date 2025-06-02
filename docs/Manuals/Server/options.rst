.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>


.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance

Server Options
**************

Server Options are a collection of flags that can be set from the server command-line. The
:doc:`/Manuals/Game/index` refers to a :ref:`menu setting <game-manual-game-menu>` called
:guilabel:`Game Options`. In reality, the dialog box allows a user to use a graphical interface to set
specific gameplay options. This page provides a detailed description of all of the options that a game admin
can set from the server command line.

Refer to :doc:`settings-file` for information on how to place all the options in a settings file to call
from the :doc:`command-line`.

To place a setting value on any of these settings use the ``/set <option-name> <value>``
:ref:`server command <set-option-name-value>`.

.. note::
  The default value in the "Value (min, max)" section below is based on the Classic ruleset. The default
  value is often set by the ruleset, but can be overridden before the game starts.

``aifill``
  :strong:`Default Value (Min, Max)`: 5 (0, 500)

  :strong:`Description`: If set to a positive value, then AI players will be automatically created or removed
  to keep the total number of players at this amount. As more players join, these AI players will be replaced.
  When set to zero, all AI players will be removed.

``airliftingstyle``
  :strong:`Default Value`: empty value / not set

  :strong:`Description`: This setting affects airlifting units between cities. It can be a set of the
  following values:

    * ``FROM_ALLIES``: Allows units to be airlifted from allied cities.
    * ``TO_ALLIES``: Allows units to be airlifted to allied cities.
    * ``SRC_UNLIMITED``: Unlimited units from source city. :strong:`Note` that airlifting from a city does not
      reduce the airlifted counter, but still needs airlift capacity of at least 1.
    * ``DEST_UNLIMITED``: Unlimited units to destination city. :strong:`Note` that airlifting to a city does
      not reduce the airlifted counter, and does not need any airlift capacity.

.. _server-option-allowtake:

``allowtake``
  :strong:`Default Value`: "HAhadOo"

  :strong:`Description`: This should be a string of characters, each of which specifies a type or status of a
  civilization (player). Clients will only be permitted to take or observe those players which match one of
  the specified letters. This only affects future uses of the ``/take`` or ``/observe`` commands. It is not
  retroactive. The characters and their meanings are:

    * o,O = Global observer
    * b   = Barbarian players
    * d   = Dead players
    * a,A = AI players
    * h,H = Human players

  The first description on this list which matches a player is the one which applies. Thus ``d`` does not
  include dead barbarians, ``a`` does not include dead :term:`AI` players, and so on. Upper case letters apply
  before the game has started, lower case letters afterwards.

  Each character above may be followed by one of the following numbers to allow or restrict the manner of
  connection:

  * (none) = Controller allowed, observers allowed, can displace connections. (Displacing a connection means
    that you may take over a player, even when another user already controls that player.)
  * 1 = Controller allowed, observers allowed, cannot displace connections.
  * 2 = Controller allowed, no observers allowed, can displace connections.
  * 3 = Controller allowed, no observers allowed, cannot displace connections.
  * 4 = No controller allowed, observers allowed.

  The limits do not affect clients with HACK access level.

``alltemperate``
  :strong:`Default Value`: disabled

  :strong:`Description`: If this setting is enabled, the temperature will be equivalent everywhere on the map.
  As a result, the poles will not be generated.

``animals``
  :strong:`Default Value (Min, Max)`: 20 (0, 500)

  :strong:`Description`: Number of animals initially created on terrains defined for them in the ruleset (if
  the ruleset supports it). The server variable's scale is animals per thousand tiles.

``aqueductloss``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: If a city would expand, but it cannot because it lacks some prerequisite
  (traditionally an :improvement:`Aqueduct` or :improvement:`Sewer System`), this is the base percentage of
  its foodbox that is lost each turn. The penalty may be reduced by buildings or other circumstances,
  depending on the ruleset.

``autoattack``
  :strong:`Default Value`: disabled

  :strong:`Description`: If set to on, units with moves left will automatically consider attacking enemy units
  that move adjacent to them.

``autosaves``
  :strong:`Default Value`: ``TURN|GAMEOVER|QUITIDLE|INTERRUPT``

  :strong:`Description`: This setting controls which autosave types get generated:

    * ``TURN``: Save when turn begins, once every ``saveturns`` turns.
    * ``GAMEOVER``: Final save when game ends.
    * ``QUITIDLE``: Save before server restarts due to lack of players.
    * ``INTERRUPT``: Save when server quits due to interrupt.
    * ``TIMER``: Save every ``savefrequency`` minutes.

``autotoggle``
    :strong:`Default Value`: disabled

    :strong:`Description`: If enabled, :term:`AI` status is turned off when a player connects, and on when a
    player disconnects.

``barbarians``
  :strong:`Default Value`: ``NORMAL``

  :strong:`Description`: This setting controls how frequently the :unit:`Barbarians` appear in the game.
  See also the ``onsetbarbs`` setting. Possible values:

    * ``DISABLED``: No barbarians.
    * ``HUTS_ONLY``: Only in huts.
    * ``NORMAL``: Normal rate of appearance.
    * ``FREQUENT``: Frequent barbarian uprising.
    * ``HORDES``: Raging hordes.

``borders``
  :strong:`Default Value`: ``ENABLED``

  :strong:`Description`: If this is not disabled, then any land tiles around a city or border-claiming extra
  (like the classic ruleset's Fortress base) will be owned by that nation. Possible values:

    * ``SEE_INSIDE``: See everything inside borders.
    * ``EXPAND``: Borders expand to unknown, revealing tiles.
    * ``ENABLED``: Will, in some rulesets, grant the same visibility if certain conditions are met.
    * ``DISABLED``: Disabled
    * ``ENABLED``: Enabled

``caravan_bonus_style``
  :strong:`Default Value`: ``CLASSIC``

  :strong:`Description`: The formula for the bonus when a :unit:`Caravan` enters a city. Possible values:

    * ``CLASSIC``: Bonuses are proportional to distance and trade of source and destination with multipliers
      for overseas and international destinations.
    * ``LOGARITHMIC``: Bonuses are proportional to :math:`log^2(distance + trade)`.
    * ``LINEAR``: Bonuses are similar to ``CLASSIC``, but (like ``LOGARITHMIC``) use the max trade of the city
      rather than current.
    * ``DISTANCE``: Bonuses are proportional only to distance.

``citymindist``
  :strong:`Default Value (Min, Max)`: 2 (1, 11)

  :strong:`Description`: Minimum distance between cities. When a player attempts to found a new city, it is
  prevented if the distance from any existing city is less than this setting. For example, when this setting
  is 3, there must be at least two clear tiles in any direction between all existing cities and the new city
  site. A value of 1 removes any such restriction on city placement.

``citynames``
  :strong:`Default Value`: ``PLAYER_UNIQUE``

  :strong:`Description`: Allowed city names. Possible values:

    * ``NO_RESTRICTIONS``: No restrictions. Players can have multiple cities with the same names.
    * ``PLAYER_UNIQUE``: Unique to a player. One player cannot have multiple cities with the same name.
    * ``GLOBAL_UNIQUE``: Globally unique. All cities in a game have to have different names.
    * ``NO_STEALING``: No city name stealing. Like "Globally unique", but a player is not allowed to use a
      default city name of another nation unless it is a default for their nation also.

``civilwarsize``
  :strong:`Default Value (Min, Max)`: 10 (2, 1000)

  :strong:`Description`: Minimum number of cities for civil war. A civil war is triggered when a player has at
  least this many cities and the player's capital is captured. If this option is set to the maximum value,
  civil wars are turned off altogether.

``compresstype``
  :strong:`Default Value`: ``XZ``

  :strong:`Description`: Compression library to use for savegames. Possible values:

    * ``PLAIN``: No compression.
    * ``LIBZ``: Using zlib (gzip format).
    * ``BZIP2``: Using bzip2 (deprecated).
    * ``XZ``: Using xz.

``conquercost``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Penalty when getting tech from conquering. For each technology you gain by conquering
  an enemy city, you lose research points equal to this percentage of the cost to research a new technology.
  If this is non-zero, you can end up with negative research points.

``contactturns``
  :strong:`Default Value (Min, Max)`: 20 (0, 100)

  :strong:`Description`: Turns until player contact is lost. Players may meet for diplomacy this number of
  turns after their units have last met, even when they do not have an embassy. If set to zero, then players
  cannot meet unless they have an embassy.

``demography``
  :strong:`Default Value`: "NASRLPEMOCqrb"

  :strong:`Description`: What is shown in the Demographics report. This should be a string of characters,
  each of which specifies the inclusion of a line of information in the Demographics report. The characters
  and their meanings are:

    * s = include Score
    * z = include League Score
    * N = include Population
    * n = include Population in Citizen Units
    * c = include Cities
    * i = include Improvements
    * w = include Wonders
    * A = include Land Area
    * S = include Settled Area
    * L = include Literacy
    * a = include Agriculture
    * P = include Production
    * E = include Economics
    * g = include Gold Income
    * R = include Research Speed
    * M = include Military Service
    * m = include Military Units
    * u = include Built Units
    * k = include Killed Units
    * l = include Lost Units
    * O = include Pollution
    * C = include Culture

  Additionally, the following characters control whether or not certain columns are displayed in the report:

    * q = display "quantity" column
    * r = display "rank" column
    * b = display "best nation" column

  The order of characters is not significant, but their capitalization is.

``diplbulbcost``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Penalty when getting tech from treaty. For each technology you gain from a diplomatic
  treaty, you lose research points equal to this percentage of the cost to research a new technology. If this
  is non-zero, you can end up with negative research points.

``diplchance``
  :strong:`Default Value (Min, Max)`: 80 (40, 100)

  :strong:`Description`: Base chance for diplomats and spies to succeed. The base chance of a :unit:`Spy`
  returning from a successful mission and the base chance of success for :unit:`Diplomat` and :unit:`Spy`
  units.

``diplgoldcost``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Penalty when getting gold from treaty. When transferring gold in diplomatic treaties,
  this percentage of the agreed sum is lost to both parties. It is deducted from the donor, but not received
  by the recipient.

``diplomacy``
  :strong:`Default Value`: ``ALL``

  :strong:`Description`: Ability to do diplomacy with other players. This setting controls the ability to do
  diplomacy with other players. Possible values:

    * ``ALL``: Enabled for everyone.
    * ``HUMAN``: Only allowed between human players.
    * ``AI``: Only allowed between AI players.
    * ``NOAI``: Only allowed when human involved.
    * ``NOMIXED``: Only allowed between two humans, or two AI players.
    * ``TEAM``: Restricted to teams.
    * ``DISABLED``: Disabled for everyone.

``disasters``
  :strong:`Default Value (Min, Max)`: 10 (0, 1000)

  :strong:`Description`: Frequency of disasters. Affects how often random disasters happen to cities, if any
  are defined by the ruleset. The relative frequency of disaster types is set by the ruleset. Zero prevents
  any random disasters from occurring and higher values create more opportunities for disasters to occur.

``dispersion``
  :strong:`Default Value (Min, Max)`: 0 (0, 10)

  :strong:`Description`: Area where initial units are located. This is the radius within which the initial
  units are dispersed at game start.

``ec_chat``
  :strong:`Default Value`: enabled

  :strong:`Description`: Save chat messages in the event cache. If turned on, chat messages will be saved in
  the event cache.

``ec_info``
  :strong:`Default Value`: disabled

  :strong:`Description`: Print turn and time for each cached event. If turned on, all cached events will be
  marked by the turn and time of the event like ``(T2 - 15:29:52)``.

``ec_max_size``
  :strong:`Default Value (Min, Max)`: 256 (10, 20000)

  :strong:`Description`: Size of the event cache. This defines the maximal number of events in the event
  cache.

``ec_turns``
  :strong:`Default Value (Min, Max)`: 1 (0, 32768)

  :strong:`Description`: Event cache for this number of turns. Event messages are saved for this number of
  turns. A value of 0 deactivates the event cache.

``endspaceship``
  :strong:`Default Value`: enabled

  :strong:`Description`: Should the game end if the spaceship arrives? If this option is turned on, the game
  will end with the arrival of a spaceship at Alpha Centauri.

``endturn``
  :strong:`Default Value (Min, Max)`: 5000 (1, 32767)

  :strong:`Description`: Turn the game ends. The game will end at the end of the given turn.

``first_timeout``
  :strong:`Default Value (Min, Max)`: -1 (-1, 8639999)

  :strong:`Description`: First turn timeout. If greater than 0, T1 will last for ``first_timeout`` seconds.
  If set to 0, T1 will not have a timeout. If set to -1, the special treatment of T1 will be disabled. See
  also ``timeout``.

``fixedlength``
  :strong:`Default Value`: disabled

  :strong:`Description`: Fixed-length turns play mode. If this is turned on the game turn will not advance
  until the timeout has expired, even after all players have clicked on :guilabel:`Turn Done`.

``flatpoles``
  :strong:`Default Value (Min, Max)`: 100 (0, 100)

  :strong:`Description`: How much the land at the poles is flattened. Controls how much the height of the
  poles is flattened during map generation, preventing a diversity of land terrain there. 0 is no flattening,
  100 is maximum flattening. Only affects the ``RANDOM`` and ``FRACTAL`` map generators.

``foggedborders``
  :strong:`Default Value`: disabled

  :strong:`Description`: Whether fog of war applies to border changes. If this setting is enabled, players
  will not be able to see changes in tile ownership if they do not have direct sight of the affected tiles.
  Otherwise, players can see any or all changes to borders as long as they have previously seen the tiles.

``fogofwar``
  :strong:`Default Value`: enabled

  :strong:`Description`: Whether to enable fog of war. If this is enabled, only those units and cities within
  the vision range of your own units and cities will be revealed to you. You will not see new cities or
  terrain changes in tiles not observed.

``foodbox``
  :strong:`Default Value (Min, Max)`: 100 (1, 10000)

  :strong:`Description`: Food required for a city to grow. This is the base amount of food required to grow a
  city. This value is multiplied by another factor that comes from the ruleset and is dependent on the size of
  the city.

``freecost``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Penalty when getting a free tech. For each technology you gain "for free" (other than
  covered by ``diplcost`` or ``conquercost``: for instance, from huts or from :wonder:`Great Library` effects),
  you lose research points equal to this percentage of the cost to research a new technology. If this is
  non-zero, you can end up with negative research points.

``fulltradesize``
  :strong:`Default Value (Min, Max)`: 1 (1, 50)

  :strong:`Description`: Minimum city size to get full trade. There is a trade penalty in all cities smaller
  than this value. The penalty is 100% (no trade at all) for sizes up to ``notradesize``, and decreases
  gradually to 0% (no penalty except the normal corruption) for ``size = fulltradesize``. See also
  ``notradesize``.

``gameseed``
  :strong:`Default Value (Min, Max)`: 0 (0, 2147483647)

  :strong:`Description`: Game random seed. For zero (the default) a seed will be chosen based on the current
  time.

``generator``
  :strong:`Default Value`: ``RANDOM``

  :strong:`Description`: Method used to generate map. Specifies the algorithm used to generate the map. If the
  default value of the ``startpos`` option is used, then the chosen generator chooses an appropriate
  ``startpos`` setting. Otherwise, the generated map tries to accommodate the chosen ``startpos`` setting.

    * ``SCENARIO``: Scenario map. Indicates a pre-generated map. By default, if the scenario does not specify
      start positions, they will be allocated depending on the size of continents.
    * ``RANDOM``: Fully random height. Generates maps with a number of equally spaced, relatively small
      islands. By default, start positions are allocated depending on continent size.
    * ``FRACTAL``: Pseudo-fractal height. Generates Earthlike worlds with one or more large continents and a
      scattering of smaller islands. By default, players are all placed on a single continent.
    * ``ISLAND``: Island-based. Generates *fair* maps with a number of similarly-sized and -shaped islands,
      each with approximately the same ratios of terrain types. By default, each player gets their own island.
    * ``FAIR``: Fair islands. Generates the exact copy of the same island for every player or every team.
    * ``FRACTURE``: Fracture map. Generates maps from a fracture pattern. Tends to place hills and mountains
      along the edges of the continents. If the requested generator is incompatible with other server
      settings, the server may fall back to another generator.

``globalwarming``
  :strong:`Default Value`: enabled

  :strong:`Description`: Global warming. If turned off, global warming will not occur as a result of
  pollution. This setting does not affect pollution.

``globalwarming_percent``
  :strong:`Default Value (Min, Max)`: 100 (1, 10000)

  :strong:`Description`: Global warming percent. This is a multiplier for the rate of accumulation of global
  warming.

``gold``
  :strong:`Default Value (Min, Max)`: 50 (0, 50000)

  :strong:`Description`: Starting gold per player. At the beginning of the game, each player is given this
  much gold.

``happyborders``
  :strong:`Default Value`: ``NATIONAL``

  :strong:`Description`: Units inside borders cause no unhappiness. If this is set, units will not cause
  unhappiness when inside your borders, or even allies borders, depending on value. Possible values:

    * ``DISABLED``: Borders are not helping.
    * ``NATIONAL``: Happy within own borders.
    * ``ALLIED``: Happy within allied borders.

``homecaughtunits``
  :strong:`Default Value`: enabled

  :strong:`Description`: Give caught units a homecity. If unset, caught units will have no homecity and will
  be subject to the ``killunhomed`` option.

``huts``
  :strong:`Default Value (Min, Max)`: 15 (0, 500)

  :strong:`Description`: Amount of huts (bonus extras). Huts are tile extras that usually may be investigated
  by units. The server variable's scale is huts per thousand tiles.

``incite_gold_capt_chance``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Probability of gold capture during inciting revolt. When the unit trying to incite a
  revolt is eliminated and loses its gold, there is chance that this gold would be captured by city defender.
  Ruleset defined transfer tax would be applied, though. This setting is irrelevant, if
  ``incite_gold_loss_chance`` is zero.

``incite_gold_loss_chance``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Probability of gold loss during inciting revolt. When the unit trying to incite a
  revolt is eliminated, half of the gold (or quarter, if unit was caught), prepared to bribe citizens, can be
  lost or captured by enemy.

``kicktime``
  :strong:`Default Value (Min, Max)`: 1800 (0, 86400)

  :strong:`Description`: Time before a kicked user can reconnect. Gives the time in seconds before a user
  kicked using the ``/kick`` server :ref:`command <server-command-kick>` may reconnect. Changing this setting
  will affect users kicked in the past.

``killcitizen``
  :strong:`Default Value`: enabled

  :strong:`Description`: Reduce city population after attack. This flag indicates whether a city's population
  is reduced after a successful attack by an enemy unit. If this is disabled, population is never reduced.
  Even when this is enabled, only some units may kill citizens.

``killstack``
  :strong:`Default Value`: enabled

  :strong:`Description`: Do all units in tile die with defender? If this is enabled, each time a defender unit
  loses in combat, and is not inside a city or suitable base, all units on the same tile are destroyed along
  with the defender. If this is disabled, only the defender unit is destroyed.

``killunhomed``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Slowly kill units without home cities (e.g., starting units). If greater than 0, then
  every unit without a homecity will lose :term:`HP` each turn. The number of hitpoints lost is given by
  ``killunhomed`` percent of the HP of the unit type. At least one HP is lost every turn until the death of
  the unit.

``landmass``
  :strong:`Default Value (Min, Max)`: 30 (15, 100)

  :strong:`Description`: Percentage of the map that is land. This setting gives the approximate percentage of
  the map that will be made into land.

``mapseed``
  :strong:`Default Value (Min, Max)`: 0 (0, 2147483647)

  :strong:`Description`: Map generation random seed. The same seed will always produce the same map. For zero
  (the default) a seed will be chosen based on the time to give a random map.

``mapsize``
  :strong:`Default Value`: ``FULLSIZE``

  :strong:`Description`: Map size definition. Chooses the method used to define the map size. Other options
  specify the parameters for each method.

    * ``FULLSIZE``: Number of tiles. Map area (option ``size``).
    * ``PLAYER``: Tiles per player. Number of (land) tiles per player (option ``tilesperplayer``).
    * ``XYSIZE``: Width and height. Map width and height in tiles (options ``xsize`` and ``ysize``).

``maxconnectionsperhost``
  :strong:`Default Value (Min, Max)`: 4 (0, 1024)

  :strong:`Description`: Maximum number of connections to the server per host. New connections from a given
  host will be rejected if the total number of connections from the very same host equals or exceeds this
  value. A value of 0 means that there is no limit, at least up to the maximum number of connections supported
  by the server.

``maxplayers``
  :strong:`Default Value (Min, Max)`: 500 (1, 500)

  :strong:`Description`: Maximum number of players. The maximal number of human and :term:`AI` players who can
  be in the game. When this number of players are connected in the pregame state, any new players who try to
  connect will be rejected. When playing a scenario which defines player start positions, this setting cannot
  be set to greater than the number of defined start positions.

``metamessage``
  :strong:`Default Value`: ""

  :strong:`Description`: Set user defined metaserver info line. If parameter is omitted, previously set
  ``metamessage`` will be removed. For most of the time user defined ``metamessage`` will be used instead of
  automatically generated messages, if it is available.

``mgr_distance``
  :strong:`Default Value (Min, Max)`: 0 (-5, 6)

  :strong:`Description`: Maximum distance citizens may migrate. This setting controls how far citizens may
  look for a suitable migration destination when deciding which city to migrate to. The value is added to the
  candidate target city's radius and compared to the distance between the two cities. If the distance is lower
  or equal, migration is possible. So with a setting of 0, citizens will only consider migrating if their
  city's center is within the destination city's working radius. This setting has no effect unless migration
  is enabled by the ``migration`` setting.

``mgr_foodneeded``
  :strong:`Default Value`: enabled

  :strong:`Description`: Whether migration is limited by food. If this setting is enabled, citizens will not
  migrate to cities which would not have enough food to support them. This setting has no effect unless
  migration is enabled by the ``migration`` setting.

``mgr_nationchance``
  :strong:`Default Value (Min, Max)`: 50 (0, 100)

  :strong:`Description`: Percent probability for migration within the same nation. This setting controls how
  likely it is for citizens to migrate between cities owned by the same player. Zero indicates migration will
  never occur, 100 means that migration will always occur if the citizens find a suitable destination. This
  setting has no effect unless migration is activated by the ``migration`` setting.

``mgr_turninterval``
  :strong:`Default Value (Min, Max)`: 5 (1, 100)

  :strong:`Description`: Number of turns between migrations from a city. This setting controls the number of
  turns between migration checks for a given city. The interval is calculated from the founding turn of the
  city. So for example if this setting is 5, citizens will look for a suitable migration destination every
  five turns from the founding of their current city. Migration will never occur the same turn that a city is
  built. This setting has no effect unless migration is enabled by the ``migration`` setting.

``mgr_worldchance``
  :strong:`Default Value (Min, Max)`: 10 (0, 100)

  :strong:`Description`: Percent probability for migration between foreign cities. This setting controls how
  likely it is for migration to occur between cities owned by different players. Zero indicates migration will
  never occur, 100 means that citizens will always migrate if they find a suitable destination. This setting
  has no effect if migration is not enabled by the ``migration`` setting.

``migration``
  :strong:`Default Value`: disabled

  :strong:`Description`: Whether to enable citizen migration. This is the master setting that controls whether
  citizen migration is active in the game. If enabled, citizens may automatically move from less desirable
  cities to more desirable ones. The *desirability* of a given city is calculated from a number of factors.
  In general larger cities with more income and improvements will be preferred. Citizens will never migrate
  out of the capital, or cause a wonder to be lost by disbanding a city.

``minplayers``
  :strong:`Default Value (Min, Max)`: 1 (0, 500)

  :strong:`Description`: Minimum number of players. There must be at least this many players (connected human
  players) before the game can start.

``multiresearch``
  :strong:`Default Value`: disabled

  :strong:`Description`: Allow researching multiple technologies. Allows switching to any technology without
  wasting old research. Bulbs are never transfered to new technology. Techpenalty options are inefective after
  enabling that option.

``nationset``
  :strong:`Default Value`: ""

  :strong:`Description`: Set of nations to choose from. Controls the set of nations allowed in the game. The
  choices are defined by the ruleset. Only nations in the set selected here will be allowed in any
  circumstances, including new players and civil war. Small sets may thus limit the number of players in a
  game. If this is left blank, the ruleset's default nation set is used. See ``/list nationsets`` for possible
  choices for the currently loaded ruleset.

``naturalcitynames``
  :strong:`Default Value`: enabled

  :strong:`Description`: Whether to use natural city names. If enabled, the default city names will be
  determined based on the surrounding terrain. See :doc:`/Modding/Rulesets/nations`.

``netwait``
  :strong:`Default Value (Min, Max)`: 4 (0, 20)

  :strong:`Description`: Max seconds for network buffers to drain. The server will wait for up to the value of
  this parameter in seconds, for all client connection network buffers to unblock. Zero means the server will
  not wait at all.

``notradesize``
  :strong:`Default Value (Min, Max)`: 0 (0, 49)

  :strong:`Description`: Maximum size of a city without trade. Cities do not produce any trade at all unless
  their size is larger than this amount. The produced trade increases gradually for cities larger than
  ``notradesize`` and smaller than ``fulltradesize``. See also ``fulltradesize``.

``nuclearwinter``
  :strong:`Default Value`: enabled

  :strong:`Description`: Nuclear winter. If turned off, nuclear winter will not occur as a result of nuclear
  fallout.

``nuclearwinter_percent``
  :strong:`Default Value (Min, Max)`: 100 (1, 10000)

  :strong:`Description`: Nuclear winter percent. This is a multiplier for the rate of accumulation of nuclear
  winter.

``occupychance``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Chance of moving into tile after attack. If set to 0, combat is Civ1/2-style (when
  you attack, you remain in place). If set to 100, attacking units will always move into the tile they
  attacked when they win the combat (and no enemy units remain in the tile). If set to a value between 0 and
  100, this will be used as the percent chance of "occupying" territory.

``onsetbarbs``
  :strong:`Default Value (Min, Max)`: 60 (1, 32767)

  :strong:`Description`: Barbarian onset turn. Barbarians will not appear before this turn.

``persistentready``
  :strong:`Default Value`: ``DISABLED``

  :strong:`Description`: When the Readiness of a player gets autotoggled off. In pre-game, usually when new
  players join or old ones leave, those who have already accepted game to start by toggling "Ready" get that
  autotoggled off in the changed situation. This setting can be used to make readiness more persistent.
  Possible values:

    * ``DISABLED``: Disabled.
    * ``CONNECTED``: As long as connected.

``phasemode``
  :strong:`Default Value`: ``ALL``

  :strong:`Description`: Control of simultaneous player/team phases. This setting controls whether players may
  make moves at the same time during a turn. Change in setting takes effect next turn. Currently, at least to
  the end of this turn, mode is "All players move concurrently". Possible values:

    * ``ALL``: All players move concurrently.
    * ``PLAYER``: All players alternate movement.
    * ``TEAM``: Team alternate movement.

``pingtime``
  :strong:`Default Value (Min, Max)`: 20 (1, 1800)

  :strong:`Description`: Seconds between PINGs. The server will poll the clients with a PING request each time
  this period elapses.

``pingtimeout``
  :strong:`Default Value`: 60 (60, 1800)

  :strong:`Description`: Time to cut a client. If a client does not reply to a PING in this time the client is
  disconnected.

``plrcolormode``
  :strong:`Default Value`: ``PLR_ORDER``

  :strong:`Description`: How to pick player colors. This setting determines how player colors are chosen.
  Player colors are used in the :ref:`Nations View <game-manual-nations-and-diplomacy-view>`, for national
  borders on the map, and so on.

    * ``PLR_ORDER``: Per-player, in order. Colors are assigned to individual players in order from a list
      defined by the ruleset.
    * ``PLR_RANDOM``: Per-player, random. Colors are assigned to individual players randomly from the set
      defined by the ruleset.
    * ``PLR_SET``: Set manually. Colors can be set with the ``/playercolor``
      :ref:`command <server-command-playercolor>` before the game starts. These are not restricted to the
      ruleset colors. Any players for which no color is set when the game starts get a random color from the
      ruleset.
    * ``TEAM_ORDER``: Per-team, in order. Colors are assigned to teams from the list in the ruleset. Every
      player on the same team gets the same color.
    * ``NATION_ORDER``: Per-nation, in order. If the ruleset defines a color for a player's nation, the
      player takes that color. Any players whose nations don't have associated colors get a random color from
      the list in the ruleset.

  Regardless of this setting, individual player colors can be changed after the game starts with the
  ``/playercolor`` command.

.. _server-option-rapturedelay:

``rapturedelay``
  :strong:`Default Value (Min, Max)`: 1 (1, 99)

  :strong:`Description`: Number of turns between rapture effect. Sets the number of turns between rapture
  growth of a city. If set to :math:`n` a city will grow after celebrating for :math:`n+1` turns.
  See also the `Rapture_Grow effect <effect-rapture-grow>`.

``razechance``
  :strong:`Default Value (Min, Max)`: 20 (0, 100)

  :strong:`Description`: Chance for conquered building destruction. When a player conquers a city, each city
  improvement has this percentage chance to be destroyed.

``restrictinfra``
  :strong:`Default Value`: disabled

  :strong:`Description`: Restrict the use of the infrastructure for enemy units. If this option is enabled,
  the use of roads and rails will be restricted for enemy units.

``revealmap``
  :strong:`Default Value`: empty value / not set

  :strong:`Description`: Reveal the map. Possible values:

  * ``START``: Reveal map at game start. The initial state of the entire map will be known to all players from
    the start of the game, although it may still be fogged (depending on the ``fogofwar`` setting).
  * ``DEAD``: Unfog map for dead players. Dead players can see the entire map, if they are alone in their
    team.

``revolen``
  :strong:`Default Value (Min, Max)`: 5 (1, 20)

  :strong:`Description`: Length of revolution. When changing governments, a period of anarchy will occur.
  Value of this setting, used the way ``revolentype`` setting dictates, defines the length of the anarchy.

``revolentype``
  :strong:`Default Value`: ``RANDOM``

  :strong:`Description`: Way to determine revolution length. Which method is used in determining how long
  period of anarchy lasts when changing government. The actual value is set with ``revolen`` setting. The
  ``quickening`` methods depend on how many times any player has changed to this type of government before, so
  it becomes easier to establish a new system of government if it has been done before. Possible values:

    * ``FIXED``: Fixed to ``revolen`` turns.
    * ``RANDOM``: Randomly 1-'revolen' turns.
    * ``QUICKENING``: First time 'revolen', then always quicker.
    * ``RANDQUICK``: Random, max always quicker.

``savefrequency``
  :strong:`Default Value (Min, Max)`: 15 (2, 1440)

  :strong:`Description`: Minutes per auto-save. How many minutes elapse between automatic game saves. Unlike
  other save types, this save is only meant as backup for computer memory, and it always uses the same name,
  older saves are not kept. This setting only has an effect when the ``autosaves`` setting includes ``TIMER``.

``savename``
  :strong:`Default Value`: "freeciv"

  :strong:`Description`: Definition of the save file name. Within the string the following custom formats are
  allowed:

    * %R = <reason>
    * %S = <suffix>
    * %T = <turn-number>
    * %Y = <game-year>

  Example: ``freeciv-T%04T-Y%+05Y-%R`` returns ``freeciv-T0100-Y00001-manual``

  Be careful to use at least one of ``%T`` and ``%Y``, else newer savegames will overwrite old ones. If none
  of the formats is used ``-T%04T-Y%05Y-%R`` is appended to the value of ``savename`` setting.

``savepalace``
  :strong:`Default Value`: enabled

  :strong:`Description`: Rebuild palace whenever capital is conquered. If this is turned on, when the capital
  is conquered the palace is automatically rebuilt for free in another randomly chosen city. This is
  significant because the technology requirement for building a palace will be ignored. In some rulesets,
  buildings other than the palace are affected by this setting.

``saveturns``
  :strong:`Default Value (Min, Max)`: 1 (1, 200)

  :strong:`Description`: Turns per auto-save. How many turns elapse between automatic game saves. This setting
  only has an effect when the ``autosaves`` setting includes ``NEW TURN``.

``sciencebox``
  :strong:`Default Value (Min, Max)`: 100 (1, 10000)

  :strong:`Description`: Technology cost multiplier percentage. This affects how quickly players can research
  new technology. All tech costs are multiplied by this amount (as a percentage). The base tech costs are
  determined by the ruleset or other game settings.

``scorefile``
  :strong:`Default Value`: "freeciv-score.log"

  :strong:`Description`: Name for the score log file. The default name for the score log file is
  :file:`freeciv-score.log`.

``scorelog``
  :strong:`Default Value`: disabled

  :strong:`Description`: Whether to log player statistics. If this is turned on, player statistics are
  appended to the file defined by the option ``scorefile`` every turn. These statistics can be used to create
  power graphs after the game.

``scoreloglevel``
  :strong:`Default Value`: ``ALL``

  :strong:`Description`: Scorelog level. Whether scores are logged for all players including :term:`AI`'s, or
  only for human players. Possible values:

    * ``ALL``: All players.
    * ``HUMANS``: Human players only.

``separatepoles``
  :strong:`Default Value`: enabled

  :strong:`Description`: Whether the poles are separate continents. If this setting is disabled, the
  continents may attach to poles.

``shieldbox``
  :strong:`Default Value (Min, Max)`: 100 (1, 10000)

  :strong:`Description`: Multiplier percentage for production costs. This affects how quickly units and
  buildings can be produced. The base costs are multiplied by this value (as a percentage).

``singlepole``
  :strong:`Default Value`: disabled

  :strong:`Description`: Whether there is just one pole generated. If this setting is enabled, only one side
  of the map will have a pole. This setting has no effect if the map wraps both directions.

``size``
  :strong:`Default Value (Min, Max)`: 4 (0, 2048)

  :strong:`Description`: Map area (in thousands of tiles). This value is used to determine the map area.
  Size = 4 is a normal map of 4,000 tiles (default). Size = 20 is a huge map of 20,000 tiles. For this option
  to take effect, the "Map size definition" option (``mapsize``) must be set to "Number of tiles"
  (``FULLSIZE``).

``spaceship_travel_time``
  :strong:`Default Value (Min, Max)`: 100 (50, 1000)

  :strong:`Description`: Percentage to multiply spaceship travel time by. This percentage is multiplied onto
  the time it will take for a spaceship to arrive at Alpha Centauri.

``specials``
  :strong:`Default Value (Min, Max)`: 250 (0, 1000)

  :strong:`Description`: Amount of "special" resource tiles for the game. Special resources improve the basic
  terrain type they are on. The server variable's scale is parts per thousand.

``startcity``
  :strong:`Default Value`: disabled

  :strong:`Description`: Whether player starts with a city. If this is set, the game will start with player's
  first city already founded to starting location.

``startpos``
  :strong:`Default Value`: ``DEFAULT``

  :strong:`Description`: The method used to choose where each player's initial units start on the map. For
  scenarios which include pre-set start positions, this setting is ignored. Possible values:

    * ``DEFAULT``: Generator's choice. The start position placement will depend on the map generator chosen.
      See the ``generator`` setting above.
    * ``SINGLE``: One player per continent. One player is placed on each of a set of continents of
      approximately equivalent value (if possible).
    * ``2or3``: Two or three players per continent. Similar to ``SINGLE`` except that two players will be
      placed on each continent, with three on the *best* continent if there is an odd number of players.
    * ``ALL``: All players on a single continent. All players will start on the *best* available continent.
    * ``VARIABLE``: Depending on size of continents. Players will be placed on the *best* available
      continents such that, as far as possible, the number of players on each continent is proportional to its
      value. If the server cannot satisfy the requested setting due to there being too many players for
      continents, it may fall back to one of the others. However, map generators try to create the right
      number of continents for the choice of this ``startpos`` setting and the number of players, so this is
      unlikely to occur.

``startunits``
  :strong:`Default Value`: "ccwwx"

  :strong:`Description`: List of players' initial units. This should be a string of characters, each of which
  specifies a unit role. The first character must be native to at least one "Starter" terrain. The
  case-sensitive characters and their meanings are:

      * c  = City founder (eg., :unit:`Settlers`)
      * w  = Terrain worker (eg., :unit:`Engineers`)
      * x  = Explorer (eg., :unit:`Explorer`)
      * k  = Gameloss (eg., :unit:`Leader`)
      * s  = Diplomat (eg., :unit:`Diplomat`)
      * f  = Ferryboat (eg., :unit:`Trireme`)
      * d  = Ok defense unit (eg., :unit:`Warriors`)
      * D  = Good defense unit (eg., :unit:`Phalanx`)
      * a  = Fast attack unit (eg., :unit:`Horsemen`)
      * A  = Strong attack unit (eg., :unit:`Catapult`)


``steepness``
  :strong:`Default Value (Min, Max)`: 30 (0, 100)

  :strong:`Description`: Amount of hills or mountains on the map. Small values give flat maps, while higher
  values give a steeper map with more hills and mountains.

``team_pooled_research``
    :strong:`Default Value`: enabled

  :strong:`Description`: If this setting is turned on, then the team mates will share the science research.
  Else, every player of the team will have to make its own.

``teamplacement``
  :strong:`Default Value`: ``CLOSEST``

  :strong:`Description`: Method used for placement of team mates. After start positions have been generated
  thanks to the ``startpos`` setting, this setting controls how the start positions will be assigned to the
  different players of the same team. Possible Values:

    * ``DISABLED``: The start positions will be randomly assigned to players, regardless of teams.
    * ``CLOSEST``: As close as possible. Players will be placed as close as possible, regardless of
      continents.
    * ``CONTINENT``: On the same continent. If possible, place all players of the same team onto the same
      island/continent.
    * ``HORIZONTAL``: Horizontal placement. Players of the same team will be placed horizontally.
    * ``VERTICAL``: Vertical placement. Players of the same team will be placed vertically.

``techleak``
  :strong:`Default Value (Min, Max)`: 100 (0, 300)

  :strong:`Description`: The rate of the tech leakage. As other nations learn new technologies, other players
  that have not learned the same technology advance will have the number of bulbs reduced.

``techlevel``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Number of initial techs per player. At the beginning of the game, each player is
  given this many technologies. The technologies chosen are random for each player. Depending on the value of
  ``tech_cost_style`` in the ruleset, a big value for ``techlevel`` can make the next techs really expensive.

``techlossforgiveness``
  :strong:`Default Value (Min, Max)`: -1 (-1, 200)

  :strong:`Description`: Research point (bulbs) debt threshold for losing a tech. When you have negative
  research points, and your shortfall is greater than this percentage of the cost of your current research,
  you forget a technology you already knew. The special value -1 prevents loss of technology regardless of
  research points.

``techlossrestore``
  :strong:`Default Value (Min, Max)`: 50 (-1, 100)

  :strong:`Description`: Research points (bulbs) restored after losing a tech. When you lose a technology due
  to a negative research balance (see ``techlossforgiveness``), this percentage of its research cost is
  credited to your research balance (this may not be sufficient to make it positive). The special value -1
  means that your research balance is always restored to zero, regardless of your previous shortfall.

``techlost_donor``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Chance to lose a technology while giving it to another player. The chance that your
  civilization will lose a technology if you teach it to someone else by treaty, or if it is stolen from you.

``techlost_recv``
  :strong:`Default Value (Min, Max)`: 0 (0, 100)

  :strong:`Description`: Chance to lose a technology while receiving it from another player. The chance that
  learning a technology by treaty or theft will fail.

``techpenalty``
  :strong:`Default Value (Min, Max)`: 100 (0, 100)

  :strong:`Description`: Percentage penalty when changing technology research. If you change your current
  research technology, and you have positive research points (bulbs), you lose this percentage of those
  research points. This does not apply when you have just gained a technology this turn.

``temperature``
  :strong:`Default Value (Min, Max)`: 50 (0, 100)

  :strong:`Description`: Average temperature of the planet. Small values will give a cold map, while larger
  values will give a warmer map.

    * 100 means a very dry and hot planet with no polar arctic zones, only tropical and dry zones.
    * 70 means a hot planet with little polar ice.
    * 50 means a temperate planet with normal polar, cold, temperate, and tropical zones; a desert zone
      overlaps tropical and temperate zones.
    * 30 means a cold planet with small tropical zones.
    * 0 means a very cold planet with large polar zones and no tropics.

``threaded_save``
  :strong:`Default Value`: disabled

  :strong:`Description`: Whether to do saving in separate thread. If this is turned on, compressing and saving
  the actual file containing the game situation takes place in the background while game otherwise continues.
  This way users are not required to wait for the save to finish.

``tilesperplayer``
  :strong:`Default Value (Min, Max)`: 100 (1, 1000)

  :strong:`Description`: Number of (land) tiles per player. This value is used to determine the map
  dimensions. It calculates the map size at game start based on the number of players and the value of
  the setting ``landmass``. For this option to take effect, the "Map size definition" option (``mapsize``)
  must be set to "Tiles per player" (``PLAYER``).

``timeaddenemymove``
  :strong:`Default Value (Min, Max)`: 0 (0, 8639999)

  :strong:`Description`: Timeout at least :math:`n` seconds when enemy moved. Any time a unit moves while in
  sight of an enemy player, the remaining timeout is increased to this value. This setting helps with
  :term:`RTS`.

``timeout``
  :strong:`Default Value (Min, Max)`: 0 (-1, 8639999)

  :strong:`Description`: Maximum seconds per turn. If all players have not hit :guilabel:`Turn Done` before
  this time is up, then the turn ends automatically. Zero means there is no timeout. In servers compiled with
  debugging, a timeout of -1 sets the autogame test mode. Only connections with hack level access may set the
  timeout to fewer than 30 seconds. Use this with the command ``timeoutincrease`` to have a dynamic timer. The
  first turn is treated as a special case and is controlled by the ``first_timeout`` setting.

``tinyisles``
  :strong:`Default Value`: disabled

  :strong:`Description`: Presence of 1x1 islands. This setting controls whether the map generator is allowed
  to make islands of one only tile size.

``topology``
  :strong:`Default Value`: Wrap East-West and Isometric (``WRAPX|ISO``)

  :strong:`Description`: Freeciv21 maps are always two-dimensional. They may wrap at the north-south and
  east-west directions to form a flat map, a cylinder, or a torus (donut). Individual tiles may be rectangular
  or hexagonal, with either an overhead ("classic") or isometric alignment. To play with a particular
  topology, clients will need a matching tileset.

  Possible values (option can take any number of these):

    * ``WRAPX``: Wrap East-West
    * ``WRAPY``: Wrap North-South
    * ``ISO``: Isometric
    * ``HEX``: Hexagonal

``trade_revenue_style``
  :strong:`Default Value`: ``CLASSIC``

  :strong:`Description`: The formula for the trade a city receives from a trade route. Possible values:

    * ``CLASSIC``: Revenues depend on distance and trade with multipliers for overseas and international
      routes.
    * ``SIMPLE``: Revenues are proportional to the average trade of the two cities.

``trademindist``
  :strong:`Default Value (Min, Max)`: 9 (1, 999)

  :strong:`Description`: Minimum distance (tiles) for trade routes. In order for two cities in the same
  civilization to establish a trade route, they must be at least this far apart on the map. For square grids,
  the distance is calculated as *Manhattan distance*, that is, the sum of the displacements along the
  :math:`x` and :math:`y` directions. For hexagonal tiles, the distance is calculated as *Absolute distance*,
  that is, the sum of the absolute value between the :math:`x` and :math:`y` directions.

``tradeworldrelpct``
  :strong:`Default Value (Min, Max)`: 50 (0, 100)

  :strong:`Description`: How largely trade distance is relative to world size. When determining trade between
  cities, the distance factor can be partly or fully relative to world size. This setting determines how big
  percentage of the bonus calculation is relative to world size, and how much only absolute distance matters.

``trading_city``
  :strong:`Default Value`: enabled

  :strong:`Description`: City trading via treaty. If turned off, trading cities in the diplomacy dialog is not
  allowed.

``trading_gold``
  :strong:`Default Value`: enabled

  :strong:`Description`: Gold trading via treaty. If turned off, trading gold in the diplomacy dialog is not
  allowed.

``trading_tech``
  :strong:`Default Value`: enabled

  :strong:`Description`: Technology trading via treaty. If turned off, trading technologies in the diplomacy
  dialog is not allowed.

``traitdistribution``
  :strong:`Default Value`: ``FIXED``

  :strong:`Description`: :term:`AI` trait distribution method. Possible values:

    * ``FIXED``: Fixed
    * ``EVEN``: Even

``turnblock``
  :strong:`Default Value`: enabled

  :strong:`Description`: Turn-blocking game play mode. If this is turned on, the game turn is not advanced
  until all players have finished their turn, including disconnected players.

``unitwaittime``
  :strong:`Default Value (Min, Max)`: 0 (0, 8639999)

  :strong:`Description`: Minimum time between unit actions over turn change. This setting gives the minimum
  amount of time in seconds between unit moves and other significant actions (such as building cities) after a
  turn change occurs. For example, if this setting is set to 20 and a unit moves 5 seconds before the turn
  change, it will not be able to move or act in the next turn for at least 15 seconds. This value is limited
  to a maximum value of two-thirds of ``timeout``. ``unitwaittime`` (:term:`UWT`) is a tool to help reduce
  :term:`RTS` around :term:`TC` in Longturn games.

``unitwaittime_extended``
  :strong:`Default Value`: disabled

  :strong:`Description`: ``unitwaittime`` also applies to newly-built and captured/bribed units. If set,
  newly-built units are subject to ``unitwaittime`` so that the moment the city production was last touched
  counts as their last *action*. Also, getting captured/bribed counts as action for the victim.

``unitwaittime_style``
  :strong:`Default Value`: empty value/not set

  :strong:`Description`: This setting affects ``unitwaittime`` and effectively unused as it only has one
  option to set:

    * ``ACTIVITIES``: Units moved less than ``unitwaittime`` seconds from turn change will not complete
      activities such as pillaging and building roads during turn change, but during the next turn when their
      wait expires.

``unreachableprotects``
  :strong:`Default Value`: enabled

  :strong:`Description`: Does unreachable unit protect reachable ones. This option controls whether tiles with
  both unreachable and reachable units can be attacked. If disabled, any tile with reachable units can be
  attacked. If enabled, tiles with an unreachable unit in them cannot be attacked. Some units in some rulesets
  may override this, never protecting reachable units on their tile.

``victories``
  :strong:`Default Value`: ``SPACERACE|ALLIED``

  :strong:`Description`: What kinds of victories are possible for the game. This setting controls how a game
  can be won. One can always win by conquering the entire planet, but other victory conditions can be enabled
  or disabled:

    * ``SPACERACE``: Spaceship is built and travels to Alpha Centauri.
    * ``ALLIED``: After defeating enemies, all remaining players are allied.
    * ``CULTURE``: Player meets ruleset defined cultural domination criteria.

``wetness``
  :strong:`Default Value (Min, Max)`: 50 (0, 100)

  :strong:`Description`: Amount of water on the landmasses. Small values mean lots of dry, desert-like land.
  Higher values give a wetter map with more swamps, jungles, and rivers.

``xsize``
  :strong:`Default Value (Min, Max)`: 64 (16, 128000)

  :strong:`Description`: Map width in tiles. Defines the map width. For this option to take effect, the
  "Map size definition" option (``mapsize``) must be set to "Width and height" (``XYSIZE``).

``ysize``
  :strong:`Default Value (Min, Max)`: 64 (16, 128000)

  :strong:`Description`: Map height in tiles. Defines the map height. For this option to take effect, the
  "Map size definition" option (``mapsize``) must be set to "Width and height" (``XYSIZE``).
