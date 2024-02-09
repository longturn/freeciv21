.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Server Manual
*************

The Freeciv21 Game Server (``freeciv21-server``) is the engine that powers all games that are played. This
manual will mostly track what is in the ``master`` branch of the Freeciv21 GitHub Repository at
https://github.com/longturn/freeciv21/tree/master. You can see the date that this file was last updated at the
bottom of this page in the footer.

.. note::
  This manual is for advanced users of Freeciv21. Most players simply install the game and either play local
  single-player games or join online Longturn multi-player games and do not really worry at all about the
  server. If you are good at Linux administration, interested in hosting your own server, or better
  understand how the Longturn team goes about creating the online games, then read on!

If you are having trouble, come find the Longturn Community on Discord at https://discord.gg/98krqGm. A good
place to start is the ``#questions-and-answers`` or ``#technicalities`` channels.


.. toctree::
  intro.rst
  command-line.rst
  commands.rst
  options.rst
  settings-file.rst
  fcdb.rst
  :maxdepth: 2
