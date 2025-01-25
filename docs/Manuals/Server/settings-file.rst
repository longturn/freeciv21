.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Server Settings File
********************

Freeciv21 servers can use a specially formatted plain text :file:`.serv` file. This server settings file
allows you to start a server with consistent settings. This way you can customize the way the server loads at
startup instead of with the defaults from the default ruleset (Classic).

For some examples, refer to the `Longturn games repository <https://github.com/longturn/games>`_. Every
Longturn game has its own directory and within is a :file:`.serv` file for that game. It is recommended to
look at more recent games such as ``LT75`` or later.

For more information on the varying commands you can place in a :file:`.serv` file, you can access help via
the server command prompt:

.. code-block:: sh

    $ freeciv21-server

    ...

    For introductory help, type 'help'.
    > help citymindist


To make use of the :file:`.serv` file, you would start the server with the ``-r`` option, such as:

.. code-block:: sh

    $ freeciv21-server -p 5000 -r mygame.serv


.. tip::
  The Longturn community creates two :file:`.serv` files for each game. The first (top-level) file is used to
  set all of the :doc:`game option parameters <options>` and a second is used to define the players. The
  second file is loaded from the first file via the ``/read <filename>`` server command.

.. tip::
  As noted on other pages, best practice is to use a Bash :file:`.sh` script to standardize all the
  command-line parameters and loading of the settings file to ensure a consistent experience for your players.

For more information on setting up players, refer to :doc:`/Manuals/Advanced/players`.
