Freeciv21 Client Manual
***********************

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

The Freeciv21 client (``freeciv21-client``) is the main user interface that allows one to play single-player
and online multi-player games. This manual will mostly track what is in the ``master`` branch of the Freeciv21
GitHub Repository at https://github.com/longturn/freeciv21/tree/master.

This manual was last updated on: 07 August 2022 correlating to the ``master`` branch at b1f1881. The
``master`` branch is post v3.0-beta.3.

Getting started
===============

If you have not installed Freeciv21, then obviously you need to start :doc:`there <../install>`. Precompiled
binaries are available for tagged releases and can be found on the LongTurn GitHub Repository for Freeciv21 at
https://github.com/longturn/freeciv21/releases. The LongTurn community provides binary packages for Debian
based Linux distros (Debian, Ubuntu, Mint, etc.), Windows and MacOS. If you are an Arch Linux user, you can
find Freeciv21 in AUR. If you want to get code that is closer to the ``master`` branch, you will want to
compile the code yourself. The :doc:`installation <../install>` document is mostly for Linux. However, you can
also compile on Windows with the :doc:`MSYS2 <../../Contributing/msys2>` environment or Microsoft :doc:`Visual
Studio <../../Contributing/visual-studio>`.

If you are having trouble, come find the LongTurn Community on Discord at https://discord.gg/98krqGm. A good
place to start is the ``#questions-and-answers`` channel.

Launching the Client
====================

Depending on how you installed Freeciv21 will determine how you launch it. If you installed with one of the
precompiled binary packages, then you should find Freeciv21 in your OS's launcher, Start Menu, etc. If you
compiled the code yourself, then you will go to the location you asked ``--target install`` to place the
files. Double-clicking ``freeciv21-client`` should start it up.

.. figure:: ../../_static/images/gui-elements/start-screen.png
    :height: 400px
    :align: center
    :alt: Freeciv21 Start Screen
    :figclass: align-center

    Figure 1: Start Screen with NightStalker Theme


The following buttons are available on the :guilabel:`Start Screen`:

* :guilabel:`Start new game` -- Start a new single-player game. See `Start new game`_ below.
* :guilabel:`Connect to network game` -- Connect to a LongTurn mutli-player game or one you host yourself. See
  `Connect to network game`_ below.
* :guilabel:`Load saved game` -- Load a previously saved single-player game. See `Load saved game`_ below.
* :guilabel:`Start scenario game` -- Start a single-player scenario game. See `Start scenario game`_ below.
* :guilabel:`Options` -- Set local client options. See `Options`_ below.
* :guilabel:`Quit` -- Quit Freeciv21

.. Note:: Notice that there is not a :guilabel:`Help` button available. This is by design. The in-game help is
  compiled at run-time based on the ruleset you select and other server settings you may set.

Start New Game
--------------


Players List
^^^^^^^^^^^^


Nation
^^^^^^


Rules
^^^^^


Number of Players
^^^^^^^^^^^^^^^^^


AI Skill Level
^^^^^^^^^^^^^^


More Game Options
^^^^^^^^^^^^^^^^^


Client Options
^^^^^^^^^^^^^^


Server output window
^^^^^^^^^^^^^^^^^^^^


Server chat/command line
^^^^^^^^^^^^^^^^^^^^^^^^


Disconnect
^^^^^^^^^^


Pick Nation
^^^^^^^^^^^


Observe
^^^^^^^

Start
^^^^^


Connect to Network Game
-----------------------


Load Saved Game
---------------


Start Scenario Game
-------------------


Options
-------


Main Client Interface
=====================


Menu Bar
--------


Game Menu
^^^^^^^^^


View Menu
^^^^^^^^^


Select Menu
^^^^^^^^^^^


Unit Menu
^^^^^^^^^


Combat Menu
^^^^^^^^^^^


Work Menu
^^^^^^^^^


Multiplayer Menu
^^^^^^^^^^^^^^^^


Civilization Menu
^^^^^^^^^^^^^^^^^


Help Menu
^^^^^^^^^


Top Function Bar
----------------


Map View
^^^^^^^^


Units View
^^^^^^^^^^


Cities View
^^^^^^^^^^^


Nations and Diplomacy View
^^^^^^^^^^^^^^^^^^^^^^^^^^


Research View
^^^^^^^^^^^^^


Economics View
^^^^^^^^^^^^^^


Tax Rates View
^^^^^^^^^^^^^^


National Status View
^^^^^^^^^^^^^^^^^^^^


Messages
^^^^^^^^


Unit Controls
-------------


Mini Map
--------


City Dialog
-----------
