.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


City Dialog
***********

Getting to know the :guilabel:`City Dialog` is a major aspect of playing Freeciv21. As a player you will spend
a great amount of time using this dialog box. The :guilabel:`City Dialog` is accessed by clicking on a city in
the :ref:`Map View <game-manual-map-view>` or by double-clicking a city from the table in the
:ref:`Cities View <game-manual-cities-view>`. :numref:`City Dialog Overview` shows a sample of the
:guilabel:`City Dialog`. The dialog box is broken up into 5 major segments: city information (top center),
production and citizen governor tabs (left), city citizen tile output (center), general, and citizens tabs
(right) and present units (bottom center).

.. _City Dialog Overview:
.. figure:: /_static/images/gui-elements/city-dialog.png
  :align: center
  :scale: 50%
  :alt: Freeciv21 city dialog
  :figclass: align-center

  City Dialog


Let us start at the top center as highlighted in :numref:`City Dialog Top Center`. In this segment of the
dialog box is some general information about the city. The name of the city is in the header at the top
center. If you click on the city name, a dialog box will appear and allow you to rename the city to something
else. On the left and right sides is an arrow that when clicked will move you to the previous and next cities
in the city array. In the center, the dialog will show how large the city is. Each rectangle icon is
equivalent to 1 citizen. There is also a large X button on the right. If clicked the :guilabel:`City Dialog`
will close and return you to the :ref:`Map View <game-manual-map-view>`. At the bottom center of this segment
is information about the city's status. From left to right you will see net food, net production, net gold,
net science, net trade, and turns to grow to the next city size. If you hover your mouse over any of these
icons, the client will show you detailed information on the calculation for the net value displayed.

.. _City Dialog Top Center:
.. figure:: /_static/images/gui-elements/city-dialog-top-center.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog top center
  :figclass: align-center

  City Dialog - Top Center


.. note::
  The city array will change over time as you gain and lose cities. It generally follows a left to right, top
  to bottom pattern on the map.

Moving to the center left segment highlighted in :numref:`City Dialog Production`. You will see two tabs:
:guilabel:`Production` and :guilabel:`Governor`. :numref:`City Dialog Production` shows the information that
you can find on the :guilabel:`Production` tab. You can see how much gold it will cost to buy the current item
being produced. If you click on the :guilabel:`Buy` button, a confirmation dialog will appear. In
:numref:`City Dialog Production`'s example a :improvement:`Barracks` is being constructed for a total cost of
30 shields (production). The city has produced net 18 of 30 needed shields and at its current rate of net +5
production will take 3 turns to complete. The player for this example has also added multiple items to the
work list. When the :improvement:`Barracks` is finished, the city will start production on
:unit:`Pikemen`. At this point, the city needs 12 shields to finish the :improvement:`Barracks`. At +5
shields per turn the city will produce a total of 15 shields in the same 3 turns. This means that the surplus
shields will transfer to the :unit:`Pikemen` when the :improvement:`Barracks` is complete. If the
production rate did not have extra shields left over, then no shields would go towards the :unit:`Pikemen`,
when the :improvement:`Barracks` is complete.

.. _City Dialog Production:
.. figure:: /_static/images/gui-elements/city-dialog-prod.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog production
  :figclass: align-center

  City Dialog - Production


At the bottom of the segment are four buttons: :guilabel:`Add`, :guilabel:`Move Up`, :guilabel:`Move Down`,
and :guilabel:`Delete`. Clicking on :guilabel:`Add`, will open a pop up menu showing all of the items that the
city can produce. Clicking on the item will add it to the bottom of the work list above. If you wish to move
this new item or another item in the work list up, click (select) it from the work list and click the
:guilabel:`Move Up` button. Same action for the :guilabel:`Move Down` button. To remove the item from the work
list, select an item in the work list and click on :guilabel:`Delete`. You can also double-click on an item in
the work list and it will be removed from the work list.


City Production Work Lists
==========================

One more feature of the :guilabel:`Production` tab is the work list editor. You can save work lists for later
use. To get started, populate the work list with things you want to build. This can be a collection of city
improvements, units, and wonders. Once you have the list configured the way you like it, right-click on the
work list canvas and a pop up menu will show. Start by selecting :guilabel:`Save Worklist`. A dialog box will
pop up allowing you to give the list a name. If you clear out the work list on the :guilabel:`Production` tab
and then right-click on the work list canvas, you can pick :guilabel:`Insert Worklist` or :guilabel:`Change
Worklist` from the sub-menu. :guilabel:`Insert Worklist` will add the items in the saved work list to the main
work list. :guilabel:`Change Worklist` will clear what is in the main work list and replace it with the saved
work list.

Moving to the center, we can see the city citizen tile output segment and the city's full vision radius
highlighted over the main map as shown in :numref:`City Dialog City Center`. This example shows a size 4 city,
which means 4 tiles can be managed by the citizens. Each citizen can be assigned to work one tile, extracting
food, production and trade from it (the numbers shown are in the same order). In addition, the city tile is
always worked for free. In this example, there is an irrigated grassland (``3/0/1``), a non-irrigated
grassland (``2/0/0``), a non-irrigated river grassland (``2/0/1``), and a mined hills with roads and wine
(``1/3/4``) tile being managed by a citizen. You also see the city center is on a forest river tile and gives
``1/3/1`` output. You can click on the city center and the client will automatically pick the best tiles for
net food to aid city growth. You can also click on a tile to remove the citizen from the tile and then click
another tile to have the citizen manage another tile. This is commonly referred to as city micromanagement. If
you remove a citizen from managing a tile, take a look at the top center segment. The citizens icon bar will
show one entertainer specialist. If you want to change the entertainer to a scientist or a taxman, you can
click on it in the icon bar to change.

.. _City Dialog City Center:
.. figure:: /_static/images/gui-elements/city-dialog-center.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog city center
  :figclass: align-center

  City Dialog - City Center


Moving right, we can see the :guilabel:`General` tab on panel as highlighted in :numref:`City Dialog General`.
This tab shows information similar to what is shown on the top center segment, along with units and city
improvements that the city has produced and is supporting. Hovering your mouse over many of the items at the
top of the :guilabel:`General` tab will show detailed calculation on how the net value is calculated. Hovering
your mouse over the citizens value will give you information on the happiness of the city's citizens.

.. _City Dialog General:
.. figure:: /_static/images/gui-elements/city-dialog-general.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog general
  :figclass: align-center

  City Dialog - General


Moving to the bottom center, you will see a list of the units that are present in the city, as highlighted in
:numref:`City Dialog Present Units`. There can be units present in a city that the city is not supporting, or
from your allies. If you right-click on a unit, a pop up box with action commands you can give to the unit
will be displayed. If you double-click on a unit, the :guilabel:`City Dialog` will close and the unit will be
selected. See :doc:`unit-controls` for more information on what you can do with units. If you wish to select
more than one unit to give a command to, you can do that by holding the ``ctrl`` key on your keyboard and then
left-clicking on the units you want to select. When finished, right-click on one of the selected units and
pick the option you want. This is a great way to activate a collection of units all at once. Depending on the
number of units in the city, this widget will expand left and right to the width of the screen.

.. _City Dialog Present Units:
.. figure:: /_static/images/gui-elements/city-dialog-units.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog units
  :figclass: align-center

  City Dialog - Present Units


The :guilabel:`Governor` tab, as highlighted in :numref:`City Dialog Governor`, gives information on the
Citizen Governor for this city. For more information on how to use the Citizen Governor refer to
:doc:`../../Playing/cma`.

.. _City Dialog Governor:
.. figure:: /_static/images/gui-elements/city-dialog-governor.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog governor
  :figclass: align-center

  City Dialog - Governor


On the right side is the :guilabel:`Citizens` tab, as highlighted in :numref:`City Dialog Citizens`. The
:guilabel:`Citizens` tab shows you information about the happiness and nationality of the citizens. Happiness
is broken down into segments: Cities (Empire Size), Luxury Goods, Buildings, Nationality, Units,
and Wonders.

.. _City Dialog Citizens:
.. figure:: /_static/images/gui-elements/city-dialog-citizens.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog citizens
  :figclass: align-center

  City Dialog - Citizens


Overall happiness of the citizens in your cities depends heavily on all of these factors. Certain forms of
government have varying degrees of empire size penalties and as your empire grows you will have to deal with
the negative consequences of managing a large empire and the unhappiness it creates. The amount of luxury
goods you are producing as part of your :ref:`National Budget View <game-manual-national-budget-view>` will
aid this problem. Certain city improvements can improve happiness, as well as units in the city (martial law),
along with wonders. If units are in the field in battle against your enemies, they can cause unhappiness.
Hovering your mouse over the appropriate row will give you more information about it.

:numref:`City Dialog Citizens Nationality` shows what it looks like when you have mixed nationality in your
cities. Mixed nationality can cause unhappiness and occurs when you conquer an opponent's city.

.. _City Dialog Citizens Nationality:
.. figure:: /_static/images/gui-elements/city-dialog-citizens-nationality.png
  :align: center
  :scale: 75%
  :alt: Freeciv21 city dialog citizens nationality
  :figclass: align-center

  City Dialog - Citizens Nationality
