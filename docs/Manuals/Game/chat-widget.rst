.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Chat Widget
***********

The :guilabel:`chat widget`, also known as the :guilabel:`Server Chat/Command Line`, is a floating bar
typically found either in the upper left or lower left of the :ref:`map view <game-manual-map-view>` when a
game is started. It looks something like this:

.. _Chat Widget Minimized:
.. figure:: /_static/images/gui-elements/chat-widget.png
    :align: center
    :alt: Minimized Chat Widget

    Minimized Chat Widget

You can move the widget by clicking the plus ( + ) symbol on the upper left corner. On the right edge you will
see either two up arrows or two down arrows to maximize or minimize the widget. By default the widget is
minimized. You can resize the widget's width and height when it is maximized. When minimized, the widget's
width will stay the same size as when maximized.

.. _Chat Widget Maximized:
.. figure:: /_static/images/gui-elements/chat-widget-expanded.png
    :align: center
    :alt: Maximized Chat Widget

    Maximized Chat Widget

The :guilabel:`chat widget` is a very powerful tool that you can use to conduct a number of things while
playing a game.

#. Chat with other players. You can chat with an individual player and also with a group of players (allies).

#. Issue :doc:`commands </Manuals/Server/commands>` to the server. Local single-player games have ``hack``
   command level access and can issue any command to the server. Longturn multi-player games have ``info``
   command level access and will only have a limited set of commands. The ``/list``, ``/take``, and
   ``/observe`` commands are common for Longturn multi-player games.

#. Interact with tiles on the map.

#. Set specific :doc:`messages </Manuals/Game/message-options>` to output there. This is great for team games
   where you might want to copy and paste the results of combat or other events to a group chat such as a
   Discord server. The :ref:`messages widget <game-manual-messages>` in the :doc:`/Manuals/Game/top-bar` does
   not natively support copy and paste at this time.

The following documentation discusses the out of box default settings. You can change some of them from the
:doc:`/Manuals/Game/shortcut-options`.

Chat with Players
=================

Human to human communication is a very important aspect of Longturn multi-player games. Leveraging the chat
feature of the :guilabel:`chat widget` is an easy way to have some simple conversations with a player or a
group of players.

Communication is between the ``username`` of a player. You can show that column in the
:ref:`nations view <game-manual-nations-and-diplomacy-view>`.

To initiate a chat with player username ``Johnathan``, enter ``john`` or ``Johnathan`` followed by a
colon ( : ) and then finish your message.

.. code-block:: sh

    Jonathan: I am only exploring, not an act of aggression nor spying!


In the ``john`` scenario, the server will find all players that start with ``john`` and if the name is
ambiguous the server will refuse to send the message. If you want to ensure you are holding a private chat
with ``Johnathan``, then be sure to use the whole username.

If you start a chat with no username and only a colon ( : ), this will send a message to :strong:`all players`.

.. code-block:: sh

    :Yo, heads up! Armageddon is upon you all! buyahahahaha [eol]


Lastly, if you start a chat with a period ( . ), this will send a message to your :strong:`allies` only.

.. code-block:: sh

    .Hey allies, I am going to attack that weak enemy at [tile].


As it pertains to communicating with everyone or your allies, you can also control it by toggling the
bubble/horn icon on the bottom right edge of the maximized :guilabel:`chat widget`. The icon is visible when
playing Longturn multi-player games and not while playing single-player games against the :term:`AI`.
Depending on how you have it set you can ignore using either the colon (everyone) or the period (allies) text
in the chat.

.. _Chat Widget Comm:
.. figure:: /_static/images/gui-elements/chat-widget-bubble.png
    :align: center
    :alt: Chat Widget Group Comms

    Chat Widget Group Comms

If you want to get fancy with the formatting of the text, you can add a few ``html`` styled font changes with
the square brackets ( [ or ] ).

* For :strong:`bold` font use :literal:`[bold]some text[/bold]`. The shortcut code is ``[b]``.

* For :emphasis:`italic` font use :literal:`[italic]some text[/italic]`. The shortcut is ``[i]``.

* For :strike:`strike-through` font use :literal:`[strike]some text[/strike]`. The shortcut is ``[s]``.

* For :underline:`underline` font use :literal:`[underline]some text[/underline]`. The shortcut is ``[u]``.

* To colorize the font, you can add color tags. You have the option of colorizing the foreground (fg) and/or
  the background (bg). Use :literal:`[color fg="blue" bg="yellow"]some text[/color]` to add blue to the
  foreground and yellow to the background. You do not have to colorize both sides, you can only add one side.
  You can use most primary color names. The shortcut is ``[c]``.


Server Commands
===============

There are a lot of :doc:`server commands </Manuals/Server/commands>` available and we will not go over all of
them here. However, there are a few to highlight that are useful during Longturn multi-player games:

* :literal:`/list connections`
* :literal:`/list delegations`
* :literal:`/list players`
* :literal:`/take <player-name>`
* :literal:`/observe <player-name>`

.. tip:: You can also access the :guilabel:`Server Chat/Command Line` on the new game screen by entering
  commands into the text box in the lower left corner. The :literal:`/take <player-name>` command is often
  useful here when starting a new Longturn multi-player game.

  .. _Chat Widget New Game:
  .. figure:: /_static/images/gui-elements/start-new-game.png
      :align: center
      :alt: Start New Game

      Start New Game


Interact with the Game Map
==========================

One of the greatest, and potentially least used, feature of the :guilabel:`chat widget` is interacting with
the game map. You can link to tiles, units, and cities. Similar to the chat section above, we will use a tag of
``[link]`` (shortcut ``[l]``). The good news is you do not have to type all the information. Most of the time,
you can use a :doc:`keyboard + mouse shortcut </Manuals/Game/shortcut-options>`. For all of these options, you
start by clicking your mouse in the :guilabel:`chat widget` text box first and then...

* To link to a city, press ``ctrl-alt + right click`` over a city and a link should show in the widget. For
  example: ``[l tgt="city" id=121 name="Dzithinahndé" /]``.

* To link to a unit, press ``shift-ctrl-alt + right click`` over a unit and a link should show in the widget.
  For example: ``[l tgt="unit" id=106 name="Workers" /]``.

* To link to any tile that is visible to your player, press press ``ctrl-alt + right click`` over a tile and a
  link should show in the widget. For example: ``[l tgt="tile" x=2 y=30 /]``.

.. tip::
  A great way to help you map out where you may want to place cities on the map is to send a message to
  yourself with link(s) to some tiles. A frame box will show over the tile same as when you send a link to a
  player, everyone, or your allies. When you send a message to yourself, the tile markers are only visible to
  you. This is similar to if you send private message to another player, the message and map links are private.


Output Messages
===============

As mentioned above, you can set specific :doc:`messages </Manuals/Game/message-options>` to output to the
:guilabel:`chat widget`. Some good ones to pick are:

* Unit: Attack Failed
* Unit: Attack Succeeded
* Unit: Defender Destroyed
* Unit: Defender Survived
