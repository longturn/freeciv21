.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Top Function Bar
****************

The :guilabel:`Top Function Bar` is used to get to varying views (pages) in the game without having to us the
main menu, especially the :guilabel:`Civilization` menu.  The :guilabel:`Top Function Bar` is broken up into 9
sections, from left to right.

* `Map View`_
* `Units View`_
* `Cities View`_
* `Nations and Diplomacy View`_
* `Research View`_
* `Economics View`_
* `National Budget View`_
* `National Status View`_
* `Messages`_


.. _game-manual-map-view:

Map View
========

This is your primary playing surface. This is the map where you build your civilization. The button for this
is shown in :numref:`Map View Button` below. :numref:`Game Overview`, has a good example of the
:guilabel:`Map View`.

.. _Map View Button:
.. figure:: /_static/images/gui-elements/top-bar-map.png
  :align: center
  :alt: Freeciv21 map
  :figclass: align-center

  Map View Button


If you hover your mouse over the :guilabel:`Map View` button, a pop up widget will appear and give you
information about your nation. The pop up widget shows: Nationality, Total Population, Year (Turn Number),
Gold (Surplus/Deficit), and National Budget.

To move around the map canvas, you can right-click in the main map area and the canvas will move. The further
from the center of the screen, the faster the map canvas will move per mouse click. You can also use
two-finger gestures on your mouse/trackpad to swipe up, down, left, and right.

One other feature of the :guilabel:`Map View` is the ability to middle-click on a unit (via the
:ref:`Popup Tile Info Shortcut <shortcut-popup-tile-info>`). After a middle-click a popup widget will appear
giving you some information about the tile. :numref:`Unit Information`, gives an example of a :unit:`Howitzer`
on Plains. The popup information widget gives a great deal of information about the coordinates of the tile,
terrain type, and infrastructure improvements made to the tile. If a unit is on the tile, as in our example,
you are also given details about the unit.

.. _Unit Information:
.. figure:: /_static/images/gui-elements/unit-info.png
  :scale: 65%
  :align: center
  :alt: Freeciv21 Unit Information
  :figclass: align-center

  Map View - Unit Information


.. tip::
  The client will give you some basic combat chances if you select one of your units and then use the
  middle-click popup tile info shortcut on the tile of an enemy unit.


.. _game-manual-units-view:

Units View
==========

The :guilabel:`Units View` is a separate page in a table format. When you click the button for it as shown in
:numref:`Units View Button`, the client will switch to a listing of your units.

.. _Units View Button:
.. figure:: /_static/images/gui-elements/top-bar-units.png
  :align: center
  :alt: Freeciv21 units
  :figclass: align-center

  Units View Button


The :guilabel:`Units View` has two tables. For regular games without the ``unitwaittime`` server setting set,
you get something similar to :numref:`Units View` below. If you are playing a game with ``unitwaittime`` set,
then you will see a second table below the table shown in :numref:`Units View`, such as
:numref:`Units View UWT`, that displays the amount of time until the unit can move.

.. _Units View:
.. figure:: /_static/images/gui-elements/units-view.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 Units View
  :figclass: align-center

  Units View


.. _Units View UWT:
.. figure:: /_static/images/gui-elements/units-view-uwt.png
  :align: center
  :scale: 100%
  :alt: Freeciv21 Units View w/ unitwaittime
  :figclass: align-center

  Units View with ``unitwaittime``


The :guilabel:`Find Nearest` button can be used to help you find a specific unit type. Select the row for the
unit type you wish to find on the `Map View`_ and then click the button. The :guilabel:`Units View` page will
close and the closest unit of that type will be selected and centered on the map. To quickly disband every
one of a specific unit type, select the row for the unit type and then click :guilabel:`Disband All`. This
feature is similar to the same button in the `Economics View`_. Lastly, if you have sufficient funds in your
national treasury, you can upgrade all units of a type that are inside your cities by selecting the row for
the unit type you wish to upgrade and click the :guilabel:`Upgrade` button.


.. _game-manual-nations-and-diplomacy-view:

Nations and Diplomacy View
==========================

The :guilabel:`Nations and Diplomacy View` is actually two views accessed from the same place on the
`Top Function Bar`_. When you click the button for it as shown in :numref:`Nations and Diplomacy View Button`,
the client will switch to a list of nations that you are playing against in a table format.

.. _Nations and Diplomacy View Button:
.. figure:: /_static/images/gui-elements/top-bar-nations.png
  :align: center
  :alt: Freeciv21 nations
  :figclass: align-center

  Nations and Diplomacy View Button


:numref:`Nations View` gives a sample of the :guilabel:`Nations View` in the client with all available columns
displayed. If you right-click on the table heading, you will be given a list of column names that you can
enable or disable. If you change anything, then be sure to save the settings from the :guilabel:`Game` menu.
If any players have been killed in the game, the table will show `R.I.P.` next to the name of the player that
has been destroyed.

.. _Nations View:
.. figure:: /_static/images/gui-elements/nations.png
  :align: center
  :scale: 55%
  :alt: Freeciv21 Nations View
  :figclass: align-center

  Nations View


If you have an embassy with a nation you will be able to see much more in the table than if you do not have an
embassy. If you select the row of a nation you have an embassy with, you will be given some interesting
intelligence at the bottom of the page. :numref:`Nations Intelligence` gives an example.

.. _Nations Intelligence:
.. figure:: /_static/images/gui-elements/nations-intel.png
  :align: center
  :scale: 60%
  :alt: Freeciv21 nations intelligence
  :figclass: align-center

  Nations Intelligence


On the left you will see the name of the Nation, The name (username) of the Ruler, the current form of
Government, the Capital city, how much gold they have in their national treasury, the national budget ratios,
research target, and culture score.

.. note::
  The Capital City will show as ``unknown`` if you have not seen the city on the `Map View`_. If it is in the
  unknown or has not been seen by one of your units, then you will not have knowledge of the Capital.

In the center you can see the relationship status of the nation across the game. If you see a half-moon icon
next to a nation, then the nation selected has given shared vision to that nation. In
:numref:`Nations Intelligence` above, you can see that the Aztecs have an alliance with the Iroquois and the
Arabs. The Aztecs also have shared vision with both of these nations.

On the right, you can see a comparison of technological research between your nation and the nation selected.

The :guilabel:`Nations and Diplomacy View` has a few buttons at the upper left. From left to right, they are:
:guilabel:`Meet`, :guilabel:`Cancel Treaty`, :guilabel:`Withdraw Vision`, :guilabel:`Toggle AI Mode`, and
:guilabel:`Active Diplomacy`. This is how you access the :guilabel:`Diplomacy` component of the
:guilabel:`Nations and Diplomacy View`.

Let us talk about the buttons from right to left as :guilabel:`Meet` takes the longest to describe. If you
have any active treaty negotiations ocurring, you can click on the :guilabel:`Active Diplomacy` button to
switch to that page. The :numref:`Nations and Diplomacy View Button` on the top function bar will change to a
flag icon with a red dot to give you a visual reminder that there are open meetings to attend to.

Depending on the command line level you have in the game (default is ``hack`` for single player games), you
may be able to change a player from an AI to a human after a game has started to allow a human player to come
into the game. This is what the :guilabel:`Toggle AI Mode` button does. If the button is greyed out you cannot
change the AI mode in the game.

If you have previously shared vision via a treaty from the :guilabel:`Diplomacy View`, you can revoke it by
clicking on the :guilabel:`Withdraw Vision` button. Sharing vision is similar to you allowing another player
to see all of your territory.

If you have a relationship with a player other than :strong:`War`, you can cancel it with the
:guilabel:`Cancel Treaty` button. Relationship pacts can be changed with the :guilabel:`Diplomacy View`. Most
rulesets support :strong:`Cease Fire`, :strong:`Peace`, and :strong:`Alliance`.

.. note::
  Some forms of government will not allow you to break a :strong:`Peace` or :strong:`Alliance` treaty without
  changing government to Anarchy.

Lastly, clicking :guilabel:`Meet` will bring up a diplomacy screen where you can interact with a player that
you have an embassy with. :numref:`Diplomacy`, shows a sample screen where the parties are agreeing to a cease
fire.

.. _Diplomacy:
.. figure:: /_static/images/gui-elements/diplomacy-meet.png
  :align: center
  :scale: 65%
  :alt: Freeciv21 diplomacy
  :figclass: align-center

  Diplomacy


If you wish to give gold to a player, enter in the amount in the :guilabel:`Gold` box. You can also add
clauses to the treaty by selecting from the :guilabel:`Add Clause` button. Depending on what is enabled in
your game, you can swap sea and world maps, trade cities, give advances, share vision, give an embassy, or
agree to peace or an alliance via a pact. If you are happy with the components of the treaty you can click the
:guilabel:`Accept Treaty` button. The client will change the thumbs-down icon to the right of your nation to a
thumbs-up showing agreement. If you do not want to do anything and definitely do not want to accept the
treaty, then you can click on the :guilabel:`Cancel Meeting` button. This will close the
:guilabel:`Diplomacy View` and return you to the :guilabel:`Nations View`.

.. note::
  The ability to trade maps, cities, or advances is ruleset dependent and may not be enabled for all games.
  The other clauses such as share vision, give an embassy or change the relationship via a pact are enabled at
  all times.

.. tip::
  You do not have to use the :guilabel:`Diplomacy View` to get an embassy with a player. You can always
  build a :unit:`Diplomat` unit and have that unit get an embassy by going to a player's city and "walk" into
  the city. An action dialog will show and you can establish an embassy without asking via diplomacy. See in
  game help for more information on using units to conduct many gameplay features besides simply establishing
  an embassy.

Lastly, you can see in :numref:`Diplomacy` above that there are more than one conversations occurring. Your
foreign state department is busy! If you happen to click out of the :guilabel:`Diplomacy View`, for example by
clicking on the button for the `Map View`_, the button for the :guilabel:`Nations and Diplomacy View` will
slowly pulse, giving you a reminder to come back.

To get back to the :guilabel:`Diplomacy View`, you can click on the :guilabel:`Active Diplomacy` button from
the :guilabel:`Nations View` described earlier to bring it back up.


.. _game-manual-cities-view:

Cities View
===========

The :guilabel:`Cities View` is a separate page in a table format. When you click the button for it as shown in
:numref:`Cities View Button`, the client will switch to a listing of your cities.

.. _Cities View Button:
.. figure:: /_static/images/gui-elements/top-bar-cities.png
  :align: center
  :alt: Freeciv21 Cities
  :figclass: align-center

  Cities View Button


:numref:`Cities` gives an example of the :guilabel:`Cities View` in the client with the default columns
displayed. If you right-click on the table heading, you will be given a list of other columns you may want to
show. If you change anything, then be sure to save the settings from the :guilabel:`Game` menu.

.. _Cities:
.. figure:: /_static/images/gui-elements/cities.png
  :align: center
  :scale: 65%
  :alt: Freeciv21 cities
  :figclass: align-center

  Cities


If you double-click on a city row, the game will switch to the `Map View`_ and open the :doc:`city-dialog` for
that city. When you close the :doc:`city-dialog`, the client will not bring you back to the
:guilabel:`Cities View`. If you right-click on a city's row, a pop-up menu will appear. From this menu you can
change what the city is producing, set a citizen governor, sell a city improvement, conduct an advanced
selection of cities, buy what is currently being produced, and center the city on the map. If you choose to
center the city on the map, the client will close the :guilabel:`Cities View` and open the `Map View`_ and
place the city in the center of the screen.


.. _game-manual-economics-view:

Economics View
==============

The :guilabel:`Economics View` is a separate page set in a table format. When you click the button for it as
shown in :numref:`Economics View Button`, the client will switch to a listing of your nation's economy. A
nation's economy is mostly about city improvement, unit support, and maintenance.

.. _Economics View Button:
.. figure:: /_static/images/gui-elements/top-bar-economy.png
  :align: center
  :alt: Freeciv21 economics
  :figclass: align-center

  Economics View Button


If you hover your mouse over the button, a pop up widget will appear and give you information about your
nation's economy.

:numref:`Economics View` below shows a sample :guilabel:`Economics View`. Notice that you can see city
improvements and units in a table format giving you the number produced, how much gold in upkeep per turn each
consumes, total gold upkeep per turn for all of them, and if any are redundant. A redundant improvement is one
that has been overcome by events; typically by a new technological advancement. You want to sell redundant
items as they are costing you gold and giving nothing back in return. The :guilabel:`Economics View` will not
tell you what city the item is redundant in, you will have to go find that yourself. This is a good use case
for the advanced select option in the `Cities View`_.

The :guilabel:`Economics View` has a few buttons in the upper left: :guilabel:`Disband`, :guilabel:`Sell All`,
and :guilabel:`Sell Redundant`. The :guilabel:`Disband` button will disband all the units of a type that has
been selected in the view. The :guilabel:`Sell All` button does the same for a city improvement that has been
selected in the view. Lastly, the :guilabel:`Sell Redundant` button will only sell redundant city improvements
in those cities for the city improvement that has been selected in the view. The `Messages`_ view will tell
you what was sold where.

.. _Economics View:
.. figure:: /_static/images/gui-elements/economy.png
  :align: center
  :scale: 65%
  :alt: Freeciv21 Economics view
  :figclass: align-center

  Economics View


.. note::
  You can only sell one city improvement at at time per turn, so you might not be able to do all the things
  you want every turn.


.. _game-manual-research-view:

Research View
=============

The :guilabel:`Research View` is a separate page showing the technology research tree. This is the page where
you instruct your scientists which technologies to research. When you click the button for it as shown in
:numref:`Research View Button`, the client will switch to your research tree.

.. _Research View Button:
.. figure:: /_static/images/gui-elements/top-bar-research.png
  :align: center
  :alt: Freeciv21 research
  :figclass: align-center

  Research View Button


:numref:`Research Tree` below shows a sample of a :guilabel:`Research Tree`. In this picture the player has
actually finished all of the available technologies (known as completing the research tree) and is simply
researching "future" technologies.

.. _Research Tree:
.. figure:: /_static/images/gui-elements/research-tree.png
  :align: center
  :scale: 65%
  :alt: Freeciv21 research tree
  :figclass: align-center

  Research Tree


If nothing is being researched, the :guilabel:`Research View` button will pulse to bring attention to it.

On the :guilabel:`Research Tree`, the top left drop down box is where you can pick from a menu of what
technology you want your scientists to concentrate on. The bottom left drop down box is where you can set a
future target. The client will work through the list of technologies as turns progress in order of dependency.
The progress bar on the right will show you how many bulbs you are producing each turn, how many more bulbs
you have left to finish the research target and, if enabled, how many bulbs are being used for technology
upkeep.

If you do not want to use the drop down boxes to pick current and target technologies, you can left-click on
the box for the technology in the :guilabel:`Research Tree` view.

If you hover your mouse over the icons in the :guilabel:`Research Tree`, a pop-up widget will appear giving
you information pulled from the in game help. Only so much information is displayed, so you may be prompted to
go to the in game help for more information.


.. _game-manual-national-budget-view:

National Budget View
====================

The :guilabel:`National Budget View` on the `Top Function Bar`_ shows what percentage of gold, science, and
luxury goods your nation is set at. :numref:`National Budget View Button` shows an example of the
:guilabel:`National Budget View` button.

.. _National Budget View Button:
.. figure:: /_static/images/gui-elements/top-bar-tax-rates.png
  :align: center
  :alt: Freeciv21 National Budget view
  :figclass: align-center

  National Budget View Button


Clicking on the :guilabel:`National Budget View` will bring up the :guilabel:`National Budget` dialog as shown
in :numref:`National Budget Dialog` in the :ref:`Civilization Menu <game-manual-civilization-menu>` section.


.. _game-manual-national-status-view:

National Status View
====================

The :guilabel:`National Status View` on the `Top Function Bar`_ shows various information about your nation
and the world via icons. The four icons from left to right are: Research Progress, Global Warming Chance,
Nuclear Winter Chance, and Government. The Research Progress, Global Warming Chance, Nuclear Winter Chance
icons will change depending on the rate and current status.

.. _National Status View:
.. figure:: /_static/images/gui-elements/top-bar-nation-status.png
  :align: center
  :alt: Freeciv21 national status view
  :figclass: align-center

  National Status View


If you hover your mouse over the :guilabel:`National Status View`, a pop up widget will appear and give you
information about your nation's status. The pop up widget shows: Population, Year, Turn Number, Total Gold,
Net Income, National Budget, Research along with progress, Bulbs per Turn, Culture Score, Global Warming
Change, Nuclear Winter Chance, Current form of Government. Some of this information is a duplicate of what is
shown on the `Map View`_, `National Budget View`_, and `Research View`_. The values for Nuclear Winter and
Global Warming chance give a good indication of what the icon looks like.


.. _game-manual-messages:

Messages
========

The :guilabel:`Messages` button on the `Top Function Bar`_ is used to toggle the message log widget.
:numref:`Messages Button` below shows an example of the :guilabel:`Messages` button.

.. _Messages Button:
.. figure:: /_static/images/gui-elements/top-bar-messages.png
  :align: center
  :alt: Freeciv21 Messages
  :figclass: align-center

  Messages Button


:numref:`Messages Widget` below shows an example of the :guilabel:`Messages` widget. If you double-click on a
message for unit movement, city production and a few other message types the client will take you to the city
or the unit on the `Map View`_.

.. _Messages Widget:
.. figure:: /_static/images/gui-elements/messages.png
  :align: center
  :scale: 65%
  :alt: Freeciv21 Messages widget
  :figclass: align-center

  Messages Widget
