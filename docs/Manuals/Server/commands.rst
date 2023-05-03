..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Server Commands
***************

After a server has been started from the :doc:`command-line <command-line>`, an administrator can issue a set
of commands to the server's own command-line. This server command-line is separate from the OS terminal
command-line.

``/start``
  This command starts the game. When starting a new game, it should be used after all human players have
  connected, and :term:`AI` players have been created (if required), and any desired changes to initial server
  options have been made. After ``/start``, each human player will be able to choose their nation, and then
  the game will begin. This command is also required after loading a savegame for the game to recommence. Once
  the game is running this command is no longer available, since it would have no effect.

``/help``
  With no arguments gives some introductory help. With argument "commands" or "options" gives respectively a
  list of all commands or all options. Otherwise the argument is taken as a command name or option name, and
  help is given for that command or option. For options, the help information includes the current and default
  values for that option. The argument may be abbreviated where unambiguous.

``/list colors``
  List the player colors.

.. _server-command-list-connections:

``/list connections``
  Gives a list of connections to the server.

``/list delegations``
  List of all player delegations.

``/list ignored users``
  List of a player's ignore list.

``/list map image definitions``
  List of defined map images.

``/list players``
  The list of the players in the game.

``/list rulesets``
  List of the available rulesets (for ``/read`` command).

``/list scenarios``
  List of the available scenarios.

``/list nationsets``
  List of the available nation sets in this ruleset.

``/list teams``
  List of the teams of players.

``/list votes``
  List of the running votes.

``/quit``
  Quit the game and shutdown the server.

``/cut <connection-name>``
  Cut specified client's connection to the server, removing that client from the game. If the game has not yet
  started that client's player is removed from the game, otherwise there is no effect on the player. Note that
  this command now takes connection names, not player names. See ``/list connections``.

``/explain <option-name>``
  The ``/explain`` command gives a subset of the functionality of ``/help``, and is included for backward
  compatibility. With no arguments it gives a list of options (such as ``/help options``), and with an
  argument it gives help for a particular option (such as ``/help <option-name>``).

``/show all|vital|situational|rare|changed|locked|rulesetdir``
  With no arguments, shows vital server options (or available options, when used by clients). With an option
  name argument, show only the named option, or options with that prefix. With ``all``, it shows all options.
  With ``vital``, ``situational``, or ``rare``, a set of options with this level. With ``changed``, it shows
  only the options which have been modified from the ruleset defaults. While with ``locked`` all settings
  locked by the ruleset will be listed. With ``ruleset``, it will show the current ruleset directory name.
  See :doc:`options`.

``/wall <message>``
  For each connected client, pops up a window showing the message entered.

``/connectmsg <message>``
  Set message to send to clients when they connect. Empty message means that no message is sent.

``/vote yes|no|abstain [vote number]``
  A player with ``basic`` :ref:`level access <user-permissions>` issuing a control level command starts a new
  vote for the command. The ``/vote`` command followed by "yes", "no", or "abstain", and optionally a vote
  number, gives your vote. If you do not add a vote number, your vote applies to the latest vote. You can only
  suggest one vote at a time. The vote will pass immediately if more than half of the voters who have not
  abstained vote for it, or fail immediately if at least half of the voters who have not abstained vote
  against it.

.. note::
  Voting is not a feature that is used very often, but does come in handy.

``/cancelvote <vote number>``
  With no arguments this command removes your own vote. If you have an admin access level, you can cancel any
  vote by vote number, or all votes with the ``all`` argument.

``/debug diplomacy|ferries|tech|city|units|unit|timing|info``
  Print :term:`AI` debug information about given entity and turn continuous debugging output for this entity
  on or off.

  * ``debug diplomacy <player>``
  * ``debug ferries``
  * ``debug tech <player>``
  * ``debug city <x> <y>``
  * ``debug units <x> <y>``
  * ``debug unit <id>``
  * ``debug timing``
  * ``debug info``


.. _set-option-name-value:

``/set <option-name> <value>``
  Set an option on the server. The syntax and legal values depend on the option. See the help for each option.
  Some options are "bitwise", in that they consist of a choice from a set of values. Separate these with ``|``,
  for instance, ``/set topology wrapx|iso``. For these options, use syntax like ``/set topology ""`` to set no
  values. See :doc:`options`.

``/team <player> <team>``
  A team is a group of players that start out allied, with shared vision, embassies, and fight together to
  achieve team victory with averaged individual scores. Each player is always a member of a team (possibly the
  only member). This command changes which team a player is a member of. Use ``""`` if names contain
  whitespace.

``/rulesetdir <directory>``
  Choose new ruleset directory or modpack.

``/metamessage <meta-line>``
  Set user defined metaserver info line. If parameter is omitted, previously set metamessage will be removed.
  For most of the time user defined metamessage will be used instead of automatically generated messages, if
  it is available.

.. note::
  Freeciv21 does not have its own metaserver at this time and the legacy Freeciv metaserver does not support
  Freeciv21. This means, right now, that all commands related to the metaserver are held over from the fork
  from legacy Freeciv until the Longturn community creates a custom metaserver.

``/metapatches <meta-line>``
  Set metaserver patches line. See Note about Freeciv21 metaserver above.

``/metaconnection up|down|persistent|?``
  ``/metaconnection ?`` reports on the status of the connection to the metaserver. ``/metaconnection down`` or
  ``/metac d`` brings the metaserver connection down. ``/metaconnection up`` or ``/metac u`` brings the
  metaserver connection up. ``/metaconnection persistent`` or ``/metac p`` is like 'up', but keeps trying
  after failures. See Note about Freeciv21 metaserver above.

``/metaserver <address>``
  Set address (URL) for metaserver to report to. Same as ``--Metaserver`` on the :doc:`command-line`. See
  Note about Freeciv21 metaserver above.

``/aitoggle <player-name>``
  Toggle :term:`AI` status of player. By default, new players are AI.

``/take <player-name>``
  Only the console and connections with cmdlevel ``hack`` can force other connections to take over a player.
  If you are not one of these, only the ``<player-name>`` argument is allowed. If ``-`` is given for the
  player name and the connection does not already control a player, one is created and assigned to the
  connection. The ``/allowtake`` :ref:`option <server-option-allowtake>` controls which players may be taken
  and in what circumstances.

  For example, if you have cmdlevel ``hack`` and are connected to a server, you can issue
  ``/take <player-name> -`` to take over any player. If you do not have cmdlevel ``hack``, then the
  ``/allowtake`` :ref:`option <server-option-allowtake>` must be properly set as well as a proper
  ``/delegate`` :ref:`command <server-command-delegate>` by the player wishing to delegate is completed first.
  Then a player can use ``/take`` to take the player while the delegation is in place.

``/observe <player-name>``
  Only the console and connections with cmdlevel ``hack`` can force other connections to observe a player. If
  you are not one of these, only the ``<player-name>`` argument is allowed. If the console gives no
  player-name or the connection uses no arguments, then the connection is attached to a global observer. The
  ``/allowtake`` :ref:`option <server-option-allowtake>` controls which players may be observed and in what
  circumstances.

  For example, if you have cmdlevel ``hack`` and are connected to a server, you can issue ``/observe`` with no
  ``<player-name>`` parameter. The server will change your connection to a global observer, able to view all
  nations. A global observer can make no changes and can only see information. If a user with cmdlevel
  ``hack`` issues ``/observer <player-name>``, then they can only observe that particular nation only. To
  restore to original connection, you issue ``/take <player-name>`` for your own username.

``/detach <connection-name>``
  Only the console and connections with cmdlevel ``hack`` can force other connections to detach from a player.

  This rarely used command essentially forces a connected client to disconnect from a server. To see the
  connections, issue a ``/list connections`` command as noted :ref:`above <server-command-list-connections>`.

``/create <player-name> [ai type]``
  With the ``/create`` command a new player with the given name is created. If ``player-name`` is empty, a
  random name will be assigned when the game begins. Until then the player will be known by a name derived
  from its type. The ``ai type`` parameter can be used to select which :term:`AI` module will be used for the
  created player. This requires that the respective module has been loaded or built in to the server. If the
  game has already started, the new player will have no units or cities. Also, if no free player slots are
  available, the slot of a dead player can be reused (removing all record of that player from the running
  game).

``/away``
  Toggles ``away`` mode for your nation. In away mode, the :term:`AI` will govern your nation but make only
  minimal changes.

.. note::
  The term *minimal changes* is not well understood at this time. The server help does not provide more
  details. An enterprising enthusiast could read the :term:`AI` code to determine what the term means and
  provide more details. Any real player is not going to want the AI to run thier nation and will
  :ref:`delegate <server-command-delegate>` instead.

``/handicapped <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Handicapped``, and sets the default level
  for any new AI players to ``Handicapped``. With an argument, sets the skill level for the specified player
  only. This skill level has the same features as ``Novice``, but may suffer additional ruleset-defined
  penalties.

  * Does not build offensive diplomatic units.
  * Gets reduced bonuses from huts.
  * Prefers defensive buildings and avoids close diplomatic relations.
  * Can see through :term:`FOW`.
  * Does not build air units.
  * Has complete map knowledge, including unexplored territory.
  * Naive at diplomacy.
  * Limits growth to match human players.
  * Believes its cities are always under threat.
  * Always offers cease-fire on first contact.
  * Does not bribe worker or city founder units.
  * Has erratic decision-making.
  * Research takes 250% as long as usual.
  * Has reduced appetite for expansion.

``/novice <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Novice``, and sets the default level for any
  new AI players to ``Novice``. With an argument, sets the skill level for the specified player only.

  * Does not build offensive diplomatic units.
  * Gets reduced bonuses from huts.
  * Prefers defensive buildings and avoids close diplomatic relations.
  * Can see through :term:`FOW`.
  * Does not build air units.
  * Has complete map knowledge, including unexplored territory.
  * Naive at diplomacy.
  * Limits growth to match human players.
  * Believes its cities are always under threat.
  * Always offers cease-fire on first contact.
  * Does not bribe worker or city founder units.
  * Has erratic decision-making.
  * Research takes 250% as long as usual.
  * Has reduced appetite for expansion.

``/easy <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Easy``, and sets the default level for any
  new AI players to ``Easy``. With an argument, sets the skill level for the specified player only.

  * Does not build offensive diplomatic units.
  * Gets reduced bonuses from huts.
  * Prefers defensive buildings and avoids close diplomatic relations.
  * Can see through :term:`FOW`.
  * Does not build air units.
  * Has complete map knowledge, including unexplored territory.
  * Naive at diplomacy.
  * Limits growth to match human players.
  * Always offers cease-fire on first contact.
  * Does not bribe worker or city founder units.
  * Can change city production type without penalty.
  * Has erratic decision-making.
  * Has reduced appetite for expansion.

``/normal <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Normal``, and sets the default level for any
  new AI players to ``Normal``. With an argument, sets the skill level for the specified player only.

  * Does not build offensive diplomatic units.
  * Can see through :term:`FOW`.
  * Has complete map knowledge, including unexplored territory.
  * Can skip anarchy during revolution.
  * Always offers cease-fire on first contact.
  * Does not bribe worker or city founder units.
  * Can change city production type without penalty.

``/hard <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Hard``, and sets the default level for any
  new AI players to ``Hard``. With an argument,  sets the skill level for the specified player only.

  * Has no restrictions on national budget.
  * Can target units and cities in unseen or unexplored territory.
  * Knows the location of huts in unexplored territory.
  * Can see through :term:`FOW`.
  * Has complete map knowledge, including unexplored territory.
  * Can skip anarchy during revolution.
  * Can change city production type without penalty.

``/cheating <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Cheating``, and sets the default level for
  any new AI players to ``Cheating``. With an argument, sets the skill level for the specified player only.

  * Can target units and cities in unseen or unexplored territory.
  * Knows the location of huts in unexplored territory.
  * Can see through :term:`FOW`.
  * Has complete map knowledge, including unexplored territory.
  * Can skip anarchy during revolution.
  * Can change city production type without penalty.

``/experimental <player-name>``
  With no arguments, sets all :term:`AI` players to skill level ``Experimental``, and sets the default level
  for any new AI players to ``Experimental``. With an argument, sets the skill level for the specified player
  only. THIS IS ONLY FOR TESTING OF NEW AI FEATURES! For ordinary servers, this level is no different to
  ``Hard``.

  * Has no restrictions on national budget.
  * Can target units and cities in unseen or unexplored territory.
  * Knows the location of huts in unexplored territory.
  * Can see through :term:`FOW`.
  * Has complete map knowledge, including unexplored territory.
  * Can skip anarchy during revolution.
  * Can change city production type without penalty.

``/cmdlevel none|info|basic|ctrl|admin|hack``
  The command access level controls which server commands are available to users via the client chatline. The
  available levels are:

  * ``none``: no commands
  * ``info``: informational or observer commands only
  * ``basic``: commands available to players in the game
  * ``ctrl``: commands that affect the game and users
  * ``admin``: commands that affect server operation
  * ``hack``: *all* commands - dangerous!

  With no arguments, the current command access levels are reported. With a single argument, the level is set
  for all existing connections, and the default is set for future connections. If ``new`` is specified, the
  level is set for newly connecting clients. If ``first come`` is specified, the ``first come`` level is set.
  It will be granted to the first client to connect, or if there are connections already, the first client to
  issue the ``/first`` command. If a connection name is specified, the level is set for that connection only.
  Command access levels do not persist if a client disconnects, because some untrusted person could reconnect
  with the same name. Note that this command now takes connection names, not player names.

``/first``
  If there is none, become the game organizer with increased permissions.

``/timeoutshow``
  Shows information about the timeout for the current turn, for instance how much time is left.

``/timeoutset <time>``
  This command changes the remaining time for the current turn. Passing a value of ``0`` ends the turn
  immediately. The time is specified as hours, minutes, and seconds using the format ``hh:mm:ss`` (minutes and
  hours are optional).

``/timeoutadd <time>``
  This increases the timeout for the current turn, giving players more time to finish their actions. The time
  is specified as hours, minutes, and seconds using the format ``hh:mm:ss`` (minutes and hours are optional).
  Negative values are allowed

``/timeoutincrease <turn> <turninc> <value> <valuemult>``
  Every ``<turn>`` turns, add ``<value>`` to the timeout timer, then add ``<turninc>`` to ``<turn>`` and
  multiply ``<value>`` by ``<valuemult>``. Use this command in concert with the option ``/timeout``.
  Defaults are ``0 0 0 1``.

``/ignore [type=]<pattern>``
  The given pattern will be added to your ignore list. You will not receive any messages from users matching
  this pattern. The type may be either ``user``, ``host``, or ``ip``. The default type (if omitted) is to
  match against the username. The pattern supports unix glob style wildcards, i.e., ``*`` matches zero or more
  character, ``?`` exactly one character, ``[abc]`` exactly one of ``a``, ``b``, or ``c``, etc. To access your
  current ignore list, issue ``/list ignore``.

``/unignore <range>``
  The ignore list entries in the given range will be removed. You will be able to receive messages from the
  respective users. The range argument may be a single number or a pair of numbers separated by a dash ``-``.
  If the first number is omitted, it is assumed to be ``1``. If  the last is omitted, it is assumed to be the
  last valid ignore list index. To access your current ignore list, issue ``/list ignore``.

.. _server-command-playercolor:

``/playercolor <player-name> <color>``
  This command sets the color of a specific player, overriding any color assigned according to the
  ``plrcolormode`` setting. The color is defined using hexadecimal notation (hex) for the combination of Red,
  Green, and Blue color components (RGB), similarly to HTML. For each component, the lowest (darkest) value is
  ``0`` (in hex: ``00``), and the highest value is ``255`` (in hex: ``FF``). The color definition is simply
  the three hex values concatenated together (``RRGGBB``). For example, the following command sets Caesar to
  pure red: ``playercolor Caesar ff0000``. Before the game starts, this command can only be used if the
  ``plrcolormode`` setting is set to ``PLR_SET``. A player's color can be unset again by specifying ``reset``.
  Once the game has started and colors have been assigned, this command changes the player color in any mode;
  ``reset`` cannot be used. To list the player colors, use ``/list colors``.

``/playernation <player-name> [nation] [is-male] [leader] [style]``
  This command sets the nation, leader name, style, and gender of a specific player. The string "random" can
  be used to select a random nation. The gender parameter should be ``1`` for male, ``0`` for female.
  Omitting any of the player settings will reset the player to defaults. This command may not be used once the
  game has started.

``/endgame``
  End the game immediately in a draw.

``/surrender``
  This tells everyone else that you concede the game, and if all but one player (or one team) have conceded
  the game in this way then the game ends.

``/remove <player-name>``
  This *completely* removes a player from the game, including all cities and units etc. Use with care!

``/save <file-name>``
  Save the current game to file ``<file-name>``. If no ``file-name`` argument is given saves to
  ``<auto-save name prefix><year>m.sav[.gz]``. To reload a savegame created by ``/save``, start the server
  with the command-line argument:``--file <filename>`` or ``-f <filename>`` and use the ``/start`` command
  once players have reconnected. See :doc:`command-line`.

``/scensave <file-name>``
  Save the current game to file ``<file-name>`` as a scenario. If no ``file-name`` argument is given saves to
  ``<auto-save name prefix><year>m.sav[.gz]``. To reload a savegame created by ``/scensave``, start the server
  with the command-line argument: ``--file <filename>`` or ``-f <filename>`` and use the ``/start`` command
  once players have reconnected. See :doc:`command-line`.

``/load <file-name>``
  Load a game from ``<file-name>``. Any current data including players, rulesets and server options are lost.

``/read <file-name>``
  Process server commands from file. See :doc:`settings-file`.

``/write <file-name>``
  Write current settings as server commands to file. See :doc:`settings-file`.

``/reset game|ruleset|script|default``
  Reset all settings if it is possible. The following levels are supported:

  * ``game``: using the values defined at the game start.
  * ``ruleset``: using the values defined in the ruleset.
  * ``script``: using default values and rereading the start script.
  * ``default``: using default values.

``/default <option name>``
  Reset the option to its default value. If the default ever changes in a future version, the option's value
  will follow that change.

``/lua cmd <script line>``
  Evaluate a line of Freeciv21 script or a Freeciv script file in the current game. Variations are:

  * ``lua cmd <script line>``
  * ``lua unsafe-cmd <script line>``
  * ``lua file <script file>``
  * ``lua unsafe-file <script file>``

  The unsafe prefix runs the script in an instance separate from the ruleset. This instance does not restrict
  access to Lua functions that can be used to hack the computer running the Freeciv21 server. Access to it is
  therefore limited to the console and connections with cmdlevel ``hack``.

.. _server-command-kick:

``/kick <user>``
  The connection given by the ``user`` argument will be cut from the server and not allowed to reconnect. The
  time the user would not be able to reconnect is controlled by the ``kicktime`` setting.

.. _server-command-delegate:

``/delegate to <username>``
  Delegation allows a user to nominate another user who can temporarily take over control of their player
  while they are away. Variations are:

  * ``/delegate to <username>``: Allow ``<username>`` to ``delegate take`` your player.
  * ``/delegate cancel``: Nominated user can no longer take your player.
  * ``/delegate take <player-name>``: Take control of a player who has been delegated to you. Behaves like
    ``/take``, except that the ``/allowtake`` restrictions are not enforced.
  * ``/delegate restore``: Relinquish control of a delegated player (opposite of ``/delegate take``) and
    restore your previous view, if any. This also happens automatically if the player's owner reconnects.
  * ``/delegate show``: Show who control of your player is currently delegated to, if anyone.

  The ``[player-name]`` argument can only be used by connections with cmdlevel ``admin`` or above to force the
  corresponding change of the delegation status.

``/aicmd <player> <command>``
  Execute a command in the context of the :term:`AI` for the given player.

``/fcdb lua <script>``
  The argument ``reload`` causes the database script file to be re-read after a change, while the argument
  ``lua`` evaluates a line of Lua script in the context of the Lua instance for the database. See :doc:`fcdb`.

``/mapimg define <mapdef>``
  Create image files of the world/player map. Variations are:

  * ``mapimg define <mapdef>``
  * ``mapimg show <id>|all``
  * ``mapimg create <id>|all``
  * ``mapimg delete <id>|all``
  * ``mapimg colortest``

  This command controls the creation of map images. Supported arguments:

  * ``define <mapdef>``: define a map image; returns numeric ``<id>``.
  * ``show <id>|all``: list map image definitions or show a specific one.
  * ``create <id>|all``: manually save image(s) for current map state.
  * ``delete <id>|all``:  delete map image definition(s).
  * ``colortest``: create test image(s) showing all colors.

  Multiple definitions can be active at once. A definition ``<mapdef>`` consists of colon-separated options:

  .. csv-table:: mapdef options
    :header: "Option", "(Default)", "Description"
    :widths: auto
    :align: left

    "format=<[tool|]format>", "(ppm|ppm)", "file format"
    "show=<show>", "(all)", "which players to show"
    "plrname=<name>", "", "player name"
    "plrid=<id>", "", "numeric player id"
    "plrbv=<bit vector>", "", "see example; first char = id 0"
    "turns=<turns>", "(1)", "save image each <turns> turns (0=no autosave, save with create)"
    "zoom=<zoom>", "(2)", "magnification factor (1-5)"
    "map=<map>", "(bcku)", "which map layers to draw"

  .. raw:: html

        <p>&nbsp;</p>

  ``<[tool|]format> =`` use image format ``<format>``, optionally specifying toolkit ``<tool>``. The following
  toolkits and formats are compiled in:

   * ``0``: ``ppm``

  ``<show>`` determines which players are represented and how many images are saved by this definition:

   * ``none``: no players, only terrain.
   * ``each``: one image per player.
   * ``human``: one image per human player.
   * ``all``: all players on a single image.
   * ``plrname``: just the player named with ``plrname``.
   * ``plrid``: just the player specified with ``plrid``.
   * ``plrbv``: one image per player in ``plrbv``.

  ``<map>`` can contain one or more of the following layers:

   * ``a``: show area within borders of specified players.
   * ``b``: show borders of specified players.
   * ``c``: show cities of specified players.
   * ``f``: show fog of war (single-player images only).
   * ``k``: show only player knowledge (single-player images only).
   * ``t``: full display of terrain types.
   * ``u``: show units of specified players.

  Examples of ``<mapdef>``:

   * ``zoom=1:map=tcub:show=all:format=ppm|ppm``
   * ``zoom=2:map=tcub:show=each:format=png``
   * ``zoom=1:map=tcub:show=plrname:plrname=Otto:format=gif``
   * ``zoom=3:map=cu:show=plrbv:plrbv=010011:format=jpg``
   * ``zoom=1:map=t:show=none:format=magick|jpg``

``/rfcstyle``
  Switch server output between 'RFC-style' and normal style.

``/serverid``
  Simply returns the id of the server.
