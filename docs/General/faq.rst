Frequently Asked Questions (FAQ)
********************************

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

The following page has a listing of frequenty asked questions with answers about Freeciv21.

Gameplay
========

OK, so I installed Freeciv21. How do I play?
--------------------------------------------

Start the client. Depending on your system, you might choose it from a menu, double-click on the 
:file:`freeciv21-client` executable program, or type :file:`freeciv21-client` in a terminal.

Once the client starts, to begin a single-player game, select :guilabel:`Start new game`. Now edit your 
game settings (the defaults should be fine for a beginner-level single-player game) and press the 
:guilabel:`Start` button.

Freeciv21 is a client/server system. But in most cases you don't have to worry about this; the client 
starts a server automatically for you when you start a new game.

Once the game is started you can find information in the :guilabel:`Help` menu. If you've never played a 
Civilization-style game before you may want to look at help on :title-reference:`Strategy and Tactics`.

You can continue to change the game settings through the :menuselection:`Game --> Options --> Server 
Options` menu. Type :literal:`/help` in the chatline (or server command line) to get more information about 
server commands.

Detailed explanations of how to play Freeciv21 are also in the :file:`./doc/README` file distributed with 
the source code, and in the in-game help.

.. todo:: Update reference to README when it is converted to documentation project.

How do I play multiplayer?
--------------------------

You can either join a network game run by someone else, or host your own. You can also join one of the many 
games offered by the longturn.net community.

To join an open network game, choose :guilabel:`Connect to network game` and then :guilabel:`Internet 
servers`. A list of active servers should come up; double-click one to join it. 

To host your own game, we recommend starting a separate server by hand. 

To start the server, enter :file:`freeciv21-server` in a terminal or by double-clicking on the executable. 
This will start up a text-based interface.

If all players are on the same local area network (LAN), they should launch their clients, choose 
:guilabel:`Connect to Network game` and then look in the :guilabel:`Local servers` section. You should see 
the existing server listed; double-click on it to join.

To play over the Internet, players will need to enter the hostname and port into their clients, so the game 
admin will need to tell the other players those details. To join a longturn.net server you start by clicking 
:guilabel:`Connect to Network Game` and then in the bottom-left of the dialog fill in the 
:guilabel:`Connect`, :guilabel:`Port`, and :guilabel:`Username` fields provided by the game admin. Once 
ready, click the :guilabel:`Connect` button at the botton-right, fill in your longturn.net password in the 
:guilabel:`Password` box and you will be added to the game.

.. note:: Hosting an Internet server from a home Internet connection is often problematic, due to 
    firewalling and network address translation (NAT) that can make the server unreachable from the wider 
    Internet. Safely and securely bypassing NAT and firewalls is beyond the scope of this FAQ.

Where is the chatline you are talking about, how do I chat?
-----------------------------------------------------------

The chatline is located at the bottom of the messages window. You can activate and enlarge the chat panel by 
double-clicking on the bottom row of text.

The chatline can be used for normal chatting between players, or for issuing server commands by typing a 
forward-slash :literal:`/` followed by the server command.

See the in-game help on :title-reference:`Chatline` for more detail.

Why can't I attack another player's units?
------------------------------------------

You have to declare war first. See the section for `How do I declare war on another player?`_ below.

.. note:: In some rulesets, you start out at war with all players. In other rulesets, as soon as you 
    make contact with a player, you enter armistise towards peace. At lower skill levels, AI players offer 
    you a cease-fire treaty upon first contact, which if accepted has to be broken before you can attack 
    the player's units or cities. The main thing to remember is you have to be in the war diplomatic state 
    in order to attack an enemy.

How do I declare war on another player?
---------------------------------------

Go to the :guilabel:`Nations` page, select the player row, then click :guilabel:`Cancel Treaty` at the top. 
This drops you from :emphasis:`cease fire`, :emphasis:`armistice`, or :emphasis:`peace` into :emphasis:`war`. 
If you've already signed a permanent :emphasis:`alliance` treaty with the player, you will have to cancel 
treaties several times to get to :emphasis:`war`.

See the in-game help on :title-reference:`Diplomacy` for more detail.

.. note:: The ability to arbitrarily leave :emphasis:`peace` and go to :emphasis:`war` is also heavily 
    dependent on the form of governement your nation is currently ruled by. See the in-game help on
    :title-reference:`Government` for more details.

How do I do diplomatic meetings?
--------------------------------

Go to the :guilabel:`Nations` page, select the player row, then choose :guilabel:`Meet` at the top. Remember 
that you have to either have contact with the player or an embassy established in one of their cities.

How do I trade money with other players?
----------------------------------------

If you want to make a monetary exchange, first initiate a diplomatic meeting as described in the section 
about `How do I do diplomatic meetings?`_ above. In the diplomacy dialog, enter the amount you wish to give in 
the gold input field on your side or the amount you wish to receive in the gold input field on their side. 
With the focus in either input field, press :guilabel:`Enter` to insert the clause into the treaty.

How can I change the way a Freeciv21 game is ended?
---------------------------------------------------

A standard Freeciv21 game ends when only allied players/teams are left alive; when a player's spaceship 
arrives at Alpha Centauri; or when you reach the ending turn - whichever comes first.

For longturn.net multi-player games, the winning conditions are announced before the game begins.

For local games, you can change the default ending turn by changing the endturn setting. You can do this 
through the :menuselection:`Game --> Options --> Remote Server` menu or by typing into the chatline something 
like:

.. code-block:: rst

    /set endturn 300


You can end a running game immediately with:

.. code-block:: rst

    /endgame


For more information, try:

.. code-block:: rst

    /help endgame


If you want to avoid the game ending by space race, or require a single player/team to win, you can change 
the victories setting - again either through the Server Options dialog or through the chatline. For instance 
this changes from the default setting spacerace|allied to disallow allied victory and space race:

.. code-block:: rst

    /set victories ""


You can instead allow space races without them ending the game by instead changing the endspaceship setting.

A single player who defeats all enemies will always win the game -- this conquest victory condition cannot 
be changed.

In rulesets which support it, a cultural domination victory can be enabled, again with the victories setting.

My irrigated grassland produces only 2 food. Is this a bug?
-----------------------------------------------------------

No, it isn't. It's a feature. Your government is probably despotism, which has a -1 output whenever a tile 
produces more than 2 units of food/production/trade. You should change your government (See the in-game help 
on :title-reference:`Government` for more detail) to get rid of this penalty.

This feature is also not 100% affected by the form of government. There are some Small and Great Wonders
in certain rulesets that get rid of the output penalty.

How do I play against computer players?
---------------------------------------

See also the `How do I create teams of AI or human players?`_ section below.

In most cases when you start a single-player game you can change the number of players, and their 
difficulty, directly through the spinbutton. 

.. note:: The number of players here includes human players (an :literal:`aifill` of :literal:`5` adds AI 
    players until the total number of players becomes 5).

If you are playing on a remote server, you'll have to do this manually. Change the :literal:`aifill` server option 
through the :guilabel:`Remote Server` options dialog, or do it on the chatline with something like:

.. code-block:: rst

    /set aifill 30


Difficulty levels are set with the :literal:`/cheating`, :literal:`/hard`, :literal:`/normal`, 
:literal:`/easy`, :literal:`/novice`, and :literal:`/handicapped` commands.

You may also create AI players individually. For instance, to create one hard and one easy AI player, enter:

.. code-block:: rst

    /create ai1
    /hard ai1
    /create ai2
    /easy ai2
    /list


More details are in the :file:`./doc/README` file supplied with Freeciv and the online manual on this site.

.. todo:: Update reference to README when it is converted to documentation project.

Can I build up the palace or throne room as in the commercial Civilization games?
---------------------------------------------------------------------------------

No. This feature is not present in Freeciv21, and will not be until someone draws the graphics for it.

Can I build land over sea/transform ocean to land?
--------------------------------------------------

Yes. You can do that by placing :unit:`engineer` on a :unit:`transport` and going to the ocean tile you want 
to build land on (this must be in a land corner). Click the :unit:`transport` to display a list of the 
transported :unit:`engineers` and activate them. Then give them the order of transforming this tile to 
swamp. This will take a very long time though, so you'd better try with 6 or 8 :unit:`engineers` at a time. 
There must be 3 adjacent land tiles to the ocean tile you are transforming.

Can I change settings or rules to get different types of games?
---------------------------------------------------------------

Of course. Before the game is started, you may change settings through the :guilabel:`Server Options` 
dialog. You may also change these settings or use server commands through the chatline. If you use the 
chatline, use the:

.. code-block:: rst

    /show

command to display the most commonly-changed settings, or

.. code-block:: rst

    /help <setting>


to get help on a particular setting, or

.. code-block:: rst

    /set <setting> <value>


to change a setting to a particular value. After the game begins you may still change some settings, but not 
others.

You can create rulesets or :strong:`modpacks` - alternative sets of units, buildings, and technologies. Several 
different rulesets come with the Freeciv21 distribution, including a civ1 (Civilization 1 compatibility mode), 
and civ2 (Civilization 2 compatibility mode). Use the :literal:`rulesetdir` command to change the 
ruleset (as in :literal:`/rulesetdir civ2`). 

How compatible is Freeciv21 with the commercial Civilization games?
-------------------------------------------------------------------

Freeciv21 was created as a multiplayer version of Civilization |reg| with players moving simultaneously. 
Rules and elements of Civilization II |reg|, and features required for single-player use, such as AI 
players, were added later.

This is why Freeciv21 comes with several game configurations (rulesets): the civ1 and civ2 rulesets implement 
game rules, elements and features that bring it as close as possible to Civilization I and Civilization II 
respectively, while other rulesets such as the default classic ruleset tries to reflect the most popular 
settings among Freeciv21 players. Unimplemented Civilization I and II features are mainly those that would 
have little or no benefit in multiplayer mode, and nobody is working on closing this gap.

Little or no work is being done on implementing features from other similar games, such as SMAC, CTP or 
Civilization III.

So the goal of compatibility is mainly used as a limiting factor in development: when a new feature is added 
to Freeciv21 that makes gameplay different, it is generally implemented in such a way that the 
:emphasis:`traditional` behaviour remains available as an option. However, we're not aiming for absolute 
100% compatibility; in particular, we're not aiming for bug-compatibility.

My opponents seem to be able to play two moves at once!
-------------------------------------------------------

He isn't, it only seems that way. Freeciv21's multiplayer facilities are asynchronous: during a turn, moves 
from connected clients are processed in the order they are received. Server managed movement is executed in 
between turns. This allows human players to surprise their opponents by clever use of goto or quick fingers.

A turn in Longturn lasts 23 hours and it's always possible that he managed to log in twice between your two 
consecutive logins. However, firstly, there is a mechanic that slightly limits this (known as unit wait time), 
and secondly, this can't happen every time because now he has already played his move this turn and now 
needs to wait for the Turn Change to make his next move. So, in the next turn, if you log in before him, now 
it was you who made your move twice. If not, he can't :emphasis:`move twice` until you do.

The primary server setting to mitigate this problem is :literal:`unitwaittime`, which imposes a minimum 
time between moves of a single unit on successive turns.

My opponent's last city is on a 1x1 island so I cannot conquer it, and they won't give up. What can I do?
---------------------------------------------------------------------------------------------------------

It depends on the ruleset, but often researching 'amphibious warfare' will allow you to build a 
:unit:`marine`. Alternatively research 'combined arms' and either move a :unit:`helicopter` or airdrop a 
:unit:`paratrooper` there.

If you can't build :unit:`marines` yet, but you do have :unit:`engineers`, and other land is close-by, you 
can also build a land-bridge to the island (i.e. transform the ocean). If you choose this route, make sure 
that your :unit:`transport` is well defended!

Why are the AI players so hard on 'novice' or 'easy'?
-----------------------------------------------------

Short answer is... You are not expanding fast enough. 

You can also turn off Fog of War. That way, you will see the attacks of the AI. Just type :literal:`/set 
fogofwar disabled` on the chat line before the game starts.

Why are the AI players so easy on 'hard'?
-----------------------------------------

Several reasons. For example, the AI is heavily playtested under and customized to the default ruleset and 
server settings. Although there are several provisions in the code to adapt to changing rules, playing under 
different conditions is quite a handicap for it. Though mostly the AI simply doesn't have a good, all 
encompassing strategy besides :strong:`"eliminate nation x"`. 

To make the game harder, you could try putting some or all of the AI into a team. This will ensure that they 
will waste no time and resources negotiating with each other and spend them trying to eliminate you. They 
will also help each other by trading techs. See the question `How do I create teams of AI or human players?`_.

You can also form more than one AI team by using any of the different predefined teams, or put some AI 
players teamed with you.

What distinguishes AI players from humans? What do the skill levels mean?
-------------------------------------------------------------------------

AI players in Freeciv21 operate in the server, partly before all clients move, partly afterwards. Unlike the 
client, they can in principle observe the full state of the game, including everything about other players, 
although most levels deliberately restrict what they look at to some extent.

All AI players can change production without penalty. Some levels (generally the harder ones) get other 
exceptions from game rules; conversely, easier levels get some penalties, and deliberately play less well in 
some regards.

For more details about how the skill levels differ from each other, see the help for the relevant server 
command (for instance :literal:`/help hard`).

Other than as noted here, the AI players are not known to cheat.

How do I play on a hexagonal grid?
----------------------------------

It is possible to play with hexagonal instead of rectangular tiles. To do this you need to set your topology 
before the game starts; set this with Map topology index from the game settings, or in the chatline:

.. code-block:: rst

    /set topology hex|iso|wrapx


This will cause the client to use an isometric hexagonal tileset when the game starts (go to 
:menuselection:`Game --> Options --> Set local options` to choose a different one from the drop-down; 
hexemplio and isophex are included with the game).

You may also play with overhead hexagonal, in which case you want to set the topology setting to 
:literal:`hex|wrapx`; the hex2t tileset is supplied for this mode.

How do I create teams of AI or human players?
---------------------------------------------

The client has a GUI for setting up teams - just right click on any player and assign them to any team.

You may also use the command-line interface (through the chatline.)

First of all try the :literal:`/list` command. This will show you all players created, including human 
players and AI players (both created automatically by aifill or manually with :literal:`/create`).

Now, you're ready to assign players to teams. To do this you use the team command. For example, if there's 
one human player and you want two more AI players on the same team, you can do to create two AI players and 
put them on the same team you can do:

.. code-block:: rst

    /set aifill 2
    /team AI*2 1
    /team AI*3 1


You may also assign teams for human players, of course. If in doubt use the :literal:`/list` command again; 
it will show you the name of the team each player is on. Make sure you double-check the teams before 
starting the game; you can't change teams after the game has started.

I want more action.
-------------------

In Freeciv21, expansion is everything, even more so than in the single-player commercial Civilization games. 
Some players find it very tedious to build on an empire for hours and hours without even meeting an enemy.

There are various techniques to speed up the game. The best idea is to reduce the time and space allowed for 
expansion as much as possible. One idea for multiplayer mode is to add AI players: they reduce the space per 
player further, and you can toy around with them early on without other humans being aware of it. This only 
works after you can beat the AI, of course.

Another idea is to create starting situations in which the players are already fully developed. There is no 
automated support for this yet, but you can create populated maps with the built-in editor.

Community
=========

Does Freeciv21 violate any rights of the makers of Civilization I or II?
------------------------------------------------------------------------

There have been debates on this in the past and the honest answer seems to be: We don't know.

Freeciv21 doesn't contain any actual material from the commercial Civilization games. (The Freeciv21 
maintainers have always been very strict in ensuring that materials contributed to the Freeciv21 
distribution or Longturn website do not violate anyone's copyright.) The name of Freeciv21 is probably not a 
trademark infringement. The user interface is similar, but with many (deliberate) differences. The game 
itself can be configured to be practically identical to Civilization I or II, so if the rules of a game are 
patentable, and those of the said games are patented, then Freeciv21 may infringe on that patent, but we 
don't believe this to be the case.

Incidentally, there are good reasons to assume that Freeciv21 doesn't harm the sales of any of the 
commercial Civilization games in any way.

Where can I ask questions or send improvements?
-----------------------------------------------

Please ask questions about the game, its installation, or the rest of this site at the Longturn Discord 
Channels at https://discord.gg/98krqGm. The :literal:`#questions-and-answers` channel is a good start.

Patches and bug reports are best reported to the Freeciv21 bug tracking system at 
https://github.com/longturn/freeciv21/issues/new/choose.

Technical Stuff
===============

I've found a bug, what should I do?
-----------------------------------

See the article on `Where can I ask questions or send improvements?`_.

I've started a server but the client cannot find it!
----------------------------------------------------

By default, your server will be available on host :literal:`localhost` (your own machine), port 
:literal:`5556`; these are the default values your client uses when asking which game you want to connect to.

So if you don't get a connection with these values, your server isn't running, or you used :literal:`-p` to 
start it on a different port, or your system's network configuration is broken.

To start your local server, run :file:`freeciv21-server`. Then type :literal:`start` at the
server prompt to begin!

.. code-block:: rst

    username@computername:~/games/freeciv21/bin$ ./freeciv21-server 
    This is the server for Freeciv21 version 3.0.20210721.3-alpha
    You can learn a lot about Freeciv21 at https://longturn.readthedocs.io/en/latest/index.html
    [info] freeciv21-server - Loading rulesets.
    [info] freeciv21-server - AI*1 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*2 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*3 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*4 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - AI*5 has been added as Easy level AI-controlled player (classic).
    [info] freeciv21-server - Now accepting new client connections on port 5556.

    For introductory help, type 'help'.
    > start
    Starting game.


If the server is not running, you will :emphasis:`not` be able to connect to your local server.

If you can't connect to any of the other games listed, a firewall in your organization/ISP is probably 
blocking the connection. You might also need to enable port forwarding on your router.

If you are running a personal firewall, make sure that you allow communication for :file:`freeciv21-server` 
and the :file:`freeciv21-client` to the trusted zone. If you want to allow others to play on your server, 
allow :file:`freeciv21-server` to act as a server on the Internet zone.

How do I restart a saved game?
------------------------------

If for some reason you can't use the start-screen interface for loading a game, you can load one directly 
through the client or server command line. You can start the client, or server, with the :literal:`-f` 
option, for example:

.. code-block:: rst

    freeciv21-server -f freeciv-T0175-Y01250-auto.sav.bz2


Or you can use the :literal:`/load` command inside the server before starting the game.

The server cannot save games!
-----------------------------

In a local game started from the client, the games will be saved into the default Freeciv21 save directory 
(typically :file:`~/.local/share/freeciv21/saves`). If you are running the server from the command line, 
however, any savegames will be stored in the current directory. If the autosaves server setting is set 
appropriately, the server will periodically save the game automatically (which can take a lot of disk space 
in some cases); the frequency is controlled by the :literal:`saveturns` setting. In any case, you should 
check the ownership, permissions, and disk space/quota for the directory or partition you're trying to save 
to.

Where are the save games located by default?
--------------------------------------------

On Unix like systems (e.g. Linux), they will be in :file:`~/.local/share/freeciv21/saves`. On Windows, they 
are typically found in in the :file:`Appdata\\Roaming` User profile directory. For example:

.. code-block:: rst

    C:\Users\MyUserName\AppData\Roaming\freeciv21\saves


You could change this by setting the :literal:`HOME` environment variable, or using the :literal:`--saves` 
command line argument to the server (you would have to run it separately).

How do I find out about the available units, improvements, terrain types, and technologies?
-------------------------------------------------------------------------------------------

There is extensive help on this in the Help menu, but only once the game has been started - this is because 
all of these things are configurable up to that point.

The game comes with an interactive tutorial scenario. To run it, select :guilabel:`Start Scenario Game` from 
the main menu, then load the tutorial scenario.

How do I enable/disable sound support?
--------------------------------------

The client can be started without sound by supplying the commandline arguments :literal:`-P none`. The 
default sound plugin can also be configured in the client settings.

If the client was compiled with sound support, it will be enabled by default. 

Further instructions are in :file:`./doc/README.sound` in the source tarball.

If sound does not work, try:

.. code-block:: rst

    freeciv21-client -d 3 -P SDL -S stdsounds


This will help you get some debug information, which might give a clue why the sound does not work.

What are the system requirements?
---------------------------------

Memory

In a typical game the server takes about 30MB of memory and the client needs about 200MB. These values may 
change with larger maps or tilesets. For a single player game you need to run both the client and the server.

Processor

We recommend at least a 200MHz processor. The server is almost entirely single-threaded, so more cores will 
not help. If you find your game running too slow, these may be the reasons:

Too little memory
  Swapping memory pages on disc (virtual memory) is really slow. Look at the memory requirements above.

Large map
  Larger map doesn't necessary mean a more challenging or enjoyable game. You may try a smaller map.

Many AI players
  Again, having more players doesn't necessary mean a more challenging or enjoyable game.

City Governor (CMA)
  This is a really useful client side agent which helps you to organize our citizens. However, it consumes 
  many CPU cycles.

Maps and compression
  Creating map images and/or the compression of saved games for each turn will slow down new turns. 
  Consider using no compression.

Graphic display
  The client works well on 1024x800 or higher resolutions. On smaller screens you may want to enable 
  the Arrange widgets for small displays option under Interface tab in local options.

Network
  Any modern internet connection will suffice to play Freeciv21. Even mobile hotspots provide enough bandwidth.

Windows
=======

How do I use Freeciv21 under MS Windows?
----------------------------------------

Precompiled binaries can be downloaded from https://github.com/longturn/freeciv21/releases. The native 
Windows packages come as self-extracting installers.

OK, I've downloaded and installed it, how do I run it?
------------------------------------------------------

See the document about :doc:`windows-install`

How do I use a different tileset?
---------------------------------

If the tilesets supplied with Freeciv21 don't do it for you, some popular add-on tilesets are available 
through the :strong:`Freeciv21 Modpack Installer` utility. To install these, just launch the installer from 
the Start menu, and choose the one you want; it should then be automatically downloaded and made available 
for the current user.

If the tileset you want is not available via the modpack installer, you'll have to install it by hand from 
somewhere. To do that is beyond the scope of this FAQ.

How do I use a different ruleset?
---------------------------------

Again, this is easiest if the ruleset is available through the :strong:`Freeciv21 Modpack Installer` utility 
that's shipped with Freeciv21.

If the ruleset you want is not available via the modpack installer, you'll have to install it by hand from 
somewhere. To do that is beyond the scope of this FAQ. 

I opened a ruleset file in Notepad and it is very hard to read
--------------------------------------------------------------

The ruleset files (and other configuration files) are stored with UNIX line endings which Notepad doesn't 
handle correctly. Please use an alternative editor like WordPad, notepad2, or notepad++ instead.

Mac OS X
========

None of the current development team use the Mac OS. We're not building official packages, and don't 
have recent experience.

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
