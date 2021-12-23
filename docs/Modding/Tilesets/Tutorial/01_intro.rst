First steps
***********

When customizing graphics, the easiest is to start from an existing tileset and modify it. You
should :emphasis:`never` modify directly the tilesets shipped by Freeciv21: first because if you
break them, you have no "good" reference to compare to, and second because any update of Freeciv21
will overwrite your changes. Therefore, we will start this tutorial by duplicating the ``amplio2``
tileset into a new tileset called, very imaginatively, ``tutorial``. In order to find the files,
it is helpful to start a game and open :menuselection:`Help --> About Freeciv21` from the menu.
You will find there the list of directories in which Freeciv21 looks for its data files: one of
them will contain a file called ``amplio2.tilespec``.

.. note::
  On Windows, the file explorer may not show the complete name of files by default. Because of
  this, ``amplio2.tilespec`` might appear as a file called ``amplio2`` with the description
  saying "TILESPEC file". This can be changed in Windows Explorer by checking
  :menuselection:`View --> Show/hide --> File name extensions`. In the rest of this
  documentation, the complete name will always be used.

Once you have found the ``amplio2.tilespec`` file, copy it to another of the Freeciv21 folders (we
suggest using a folder inside your home directory) and rename it to ``tutorial.tilespec``. That's
it: your new tileset can now be selected from the menu (you may need to reopen the settings
window). For now, it is identical to the original ``amplio2``; we will address this in a moment.

The ``.tilespec`` file contains some metadata that should be updated. To do this, open it in your
text editor (see the :doc:`introduction </Modding/Tilesets/tutorial>` for suggestions). Then, find
the following line:

.. code-block:: ini

  name = "Amplio2"

Clearly, the name of the tileset should be changed to ``Tutorial``. Near this line, you will also
find a short description of the tileset:

.. code-block:: ini

  summary = _("Large isometric tileset.")

The underscore and parentheses around the quotes indicate that this value can be translated, and
you should leave them in place. However, the description itself could be improved: change it, for
instance, to "Tileset from the Tileset Tutorial". Once done, you can save the file and reload the
tileset in Freeciv21; the default keystroke is :kbd:`Control-Shift-F5`.

.. note::
  You may find that syntax highlighting gives strange results with Freeciv21 files: when this happens,
  configuring your editor to use syntax highlighting for INI configuration files usually helps.

You may have noticed that we didn't need to copy any image file in order to create the new tileset.
Indeed, if you scroll down in ``tutorial.tilespec``, you will find that it references files from
``amplio2``:

.. code-block:: py

  files =
    "amplio2/terrain1.spec",
    "amplio2/maglev.spec",
    "amplio2/terrain2.spec",
    ...

The ability to reference files from another tileset is very useful to organize files. This is how,
for instance, nation flags are imported: there is a central list that every tileset references. In
the :doc:`next chapter <02_units>` of this tutorial, you will learn how to new sprites.
