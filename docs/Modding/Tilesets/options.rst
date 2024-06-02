.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

.. role:: unit

Tileset Options
***************

It is often possible to introduce slight variations in a tileset. One may
have, for instance, multiple versions of unit sprites. In such cases, it may
make sense to let the user choose between the two options. This is enabled
by tileset options, which provide a list of parameters that the user can
control under :menuselection:`Game --> Tileset Options` in the
:doc:`main menu </Manuals/Game/menu>`.

Options are always very simple yes/no questions. Tilesets can configure which
questions are asked and, most importantly, load different sprites depending
on the answer.

:since: 3.1

Adding Options
==============

Each option is added as a section in the main ``tilespec`` file of the
tileset. The section title must start with ``option_`` as this is how the
engine recognizes that it defines an option. Each option must be given a name
(which will be used when selecting sprites, see below), a one-line
description for the user, and a default value. The following example can be
used as a starting point:

.. code-block:: ini

  [option_cimpletoon]
  name = "cimpletoon"
  description = _("Use 3D Cimpletoon units")
  default = FALSE

This defines an option called ``cimpletoon`` that will be shown to the user
as "Use 3D Cimpletoon units" and will be disabled when the tileset is used
for the first time.

Sprite Selection
================

When specifying sprites for use in the tileset, option support is enabled by
an additional attribute called ``option``. If the attribute is not empty, it
specifies the name of an option used to control the loading of the sprite,
and the sprite is only loaded when the option is enabled. A simple example
could be as follows:

.. code-block:: py

  tiles = { "row", "column", "option",     "tag"
            0,     0,        "",           "u.settlers"
            0,     1,        "cimpletoon", "u.settlers"
  }

Let us decompose how this works. Regardless of whether the ``cimpletoon``
option is enabled, the sprite at row 0 and column 0 is always specified for
:unit:`Settlers` units. However, when the ``cimpletoon`` is enabled, the
second line overrides the first and the alternative sprite in column 1 is
used. The end result is that the user sees a different sprite depending on
the state of the option.

In the example above, the "main" and "optional" sprites are located in the
same grid. This is not required and they can very well belong to different
files. Remember, however, that the alternative must come *after* the main
sprite in order to override it when the option is enabled.

Tileset options are very flexible and the changes they control can range from
small tweaks involving a couple of sprites to complete overhauls that change
the appearance of the map completely. The main limitation is that map
geometry cannot change, as tiles have the same dimensions regardless of which
options are enabled.
