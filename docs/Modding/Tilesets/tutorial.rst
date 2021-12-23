Tileset tutorial
****************

The tileset tutorial provides an introduction to the customization of graphics in Freeciv21. It assumes
general knowledge of core game concepts such as units, cities, and terrain. It is also good to have at least
a superficial understanding of how :doc:`rulesets </Modding/Rulesets/overview>` are written before starting
this tutorial, because tilesets specify how to render on screen the objects defined in the ruleset.

Besides a working Freeciv21, the only thing you will need to complete the tutorial is a good text editor ---
graphic assets will be provided when needed. For Windows users, we recommend the excellent `Notepad++`_; on
Linux, a good editor like Gedit, KWrite, or Mousepad usually comes preinstalled. Do not attempt to use Word
or LibreOffice.

The contents of the tutorial are summarized below. Beginners are encouraged to start from the beginning and
read the first chapters in the order they are presented, because they follow a gradual approach and jumping
right in the middle might be overwhelming. More advanced users may jump immediately to the chapter they are
interested in, although we also encourage them to read everything.

:doc:`Tutorial/01_intro`
  This chapter will get you started by creating a new tileset for the tutorial, that you can edit safely.

:doc:`Tutorial/02_units`
  In this chapter, we add graphics for two fictitious units to our tileset.

.. toctree::
  :maxdepth: 1
  :numbered:
  :hidden:

  Tutorial/01_intro.rst
  Tutorial/02_units.rst

.. rubric:: A note on image editing

If you create tilesets, you will likely also want to edit graphics. We are often asked which painting program
we use. We believe that the program that will work best for you depends a lot on your workflow, so we
encourage you to try several of them and make your own opinion. If you don't know where to start, here are a
few free (as in speech) recommendations:

* `Gimp`_ is a generic image editor with a focus on image edition. It also has great drawing capabilities.
* `Krita`_ is a digital painting application. It has a wider selection of brushes and presets than Gimp, and
  also supports vector layers.
* `MyPaint`_ is another program focused on digital painting, famous for its very flexible and powerful brush
  engine.

.. _Notepad++: https://notepad-plus-plus.org/
.. _Gimp: https://gimp.org/
.. _Krita: https://krita.org/
.. _MyPaint: http://mypaint.org/about/
