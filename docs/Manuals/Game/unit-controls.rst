.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Unit Controls
*************

When a unit has been selected on the :ref:`Map View <game-manual-map-view>`, a control widget will appear at
the bottom center of the screen. The :guilabel:`Unit Controls` widget will give you some information about the
unit, the terrain it is on and then some buttons corresponding to actions that the unit can take from the
:ref:`Unit Menu <game-manual-unit-menu>`, :ref:`Combat Menu <game-manual-combat-menu>`, or the
:ref:`Work Menu <game-manual-work-menu>` respectively. :numref:`Unit Controls Widget` shows a :unit:`Workers`
that has been selected.

.. _Unit Controls Widget:
.. figure:: /_static/images/gui-elements/unit-controls.png
  :align: center
  :alt: Freeciv21 Unit Controls widget
  :figclass: align-center

  Unit Controls Widget


You can see that the :unit:`Workers` is selected because it has a white selection ring around its base.
Looking at the dialog, in the header, you can see that this unit is ID # 111, has 4 3/9 Move Points (MPs), and
10 of 10 Hit Points (HPs).

From left to right you can see an image of the unit with MPs overlaid, the terrain it is on with
infrastructure improvements shown, and then the actions that this unit can take. In this example the actions
available are: Plant to Forest/River, Build Road, Go to Tile, Sentry, Auto Worker, Wait, and Done.

Depending on the type of unit selected, the available actions will change, but the other information will
remain the same. If you rename a unit (from the :ref:`Unit Menu <game-manual-unit-menu>`), the name will
appear in quotes after the Unit ID value.

If the unit selected is not in your field of vision on the map, then you can click on the icon for the unit on
the left side and the game map will center on the unit for you. As with other widgets in Freeciv21, you can
click+drag the widget to move it by using the plus symbol in the upper left corner.
