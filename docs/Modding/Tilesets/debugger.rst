Tileset Debugger
================

.. versionadded:: 3.0-alpha6

The Tileset Debugger, accessible from the :guilabel:`Game` menu, lets you
inspect how the map is drawn. This is very helpful when developing a tileset, to
understand why something is rendering incorrectly or to understand how other
tilesets work.

.. note::
  If you have suggestions regarding the contents and functionality of the
  debugger, you're very welcome to let us know on `Github`_ --- you'll get a
  chance to shape it to your needs.

To start using the debugger, click on the :guilabel:`Pick tile` button and then
somewhere on the map. The window will be updated with the list of sprites used
to draw the selected tile:

.. image:: /_static/images/gui-elements/tileset-debugger.png
  :alt: The tileset Debugger with a forest tile picked up.
  :align: center
  :scale: 75%

The list has two levels. Each top level item corresponds to one layer used to
draw the map. When something is drawn for a layer, its image is added next to
its name and the individual sprites are added in the second level. The sprites
at the top of the list are drawn first and are hidden by the ones below.

In the picture above, which uses the `amplio2` tileset, the four layers of a
forest tile are shown, two of which have sprites: ``Background``. a terrain
layer (``Terrain1``), ``Darkness``, and another terrain layer (also listed as
``Terrain1``). The first terrain layer is made of five sprites: one for the base
texture and four that blend it with adjacent tiles. The second terrain layer has
only one sprite, used to draw the trees.

The offsets used to draw the sprites are also shown. The first number
corresponds to the horizontal axis and runs from left to right; the second to
the vertical axis that runs from top to bottom. Depending on the type of layer,
these values may be computed automatically, so they do not necessarily
correspond to a parameters in the ``tilespec`` file.

.. note::
  The Tileset Debugger is still an experimental feature with important
  limitations. In particular, it only shows sprites that correspond to the
  tile as opposed to its corners and edges.

.. _Github: https://github.com/longturn/freeciv21/issues/new?assignees=&labels=Untriaged%2C+enhancement&template=feature_request.md&title=
