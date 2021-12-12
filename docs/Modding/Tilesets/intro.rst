Tileset tutorial
****************

copy an existing tileset... amplio...

A new unit
==========

One of the most common changes done when developing custom rules (as described in the
:doc:`/Modding/Rulesets/overview`) is to add new unit types to the game. It is important to note that
Freeciv21 has a priori no knowledge of which unit types will exist, and much less of what they should look
like. Some collaboration between the ruleset and the tileset is thus needed to provide this information.
This is achieved using :emphasis:`graphic tags` in the rules; for instance, one could find the following
in the definition of a new unit type, the Alien:

.. code-block:: ini

  [unit_trebuchet]
  name          = _("?unit:Alien")
  graphic       = "u.alien"
  graphic_alt   = "u.explorer"
  ...

The ``graphic`` directive above specifies the name that the unit has from the tileset point of view,
``u.alien``. The second directive, ``graphic_alt``, gives another name that can be used in case the
tileset doesn't support the first one. In this case, it is set to ``u.explorer``, which the author of the
rules expects to be more widely available than ``u.alien`` --- the Explorer is one of the units used in
the default rulesets. It is also not completely wrong, since in these rules the aliens are quite peaceful. To
add support for the Alien unit to our tileset, we thus need to provide the graphics for ``u.alien``. For
this, we will use the sprites below:

.. figure:: /_static/images/tileset-tutorial/aliens.png
  :alt: A sprite sheet with two alien units, one green and one red.
  :align: center

  Aliens. Aren't they cute?

Notice that a single image contains several sprites arranged in a grid. This makes it easier to send your
units to friends, but is in no way mandatory: after all, a grid with a single cell is still a grid. One can
adjust the width of the green lines to your liking or remove them, or even use several grids in a single
image. All it takes is adjusting the configuration accordingly.
