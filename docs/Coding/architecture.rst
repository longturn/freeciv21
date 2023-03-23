..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
..  SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

Architecture
************

Freeciv21 uses a client/server model where two programs interact over a network connection. The *client*
(``freeciv21-client``) is the program that you see on the screen. It handles user commands and forwards them
to the second program, that we call the *server* (``freeciv21-server``). The server checks the commands,
computes their result and sends updates back to the client. The client is capable of managing a server for
its own needs, which is used for single player games. It can also connect to a server managed externally,
which is how multiplayer games work.

The Freeciv21 code is old and evolved from a program written in C. It still bears a lot of this history, but
this is being worked on. New code is written in C++, using modern constructs when it makes sense and relying
heavily on the `Qt Framework <https://doc.qt.io>`_ for platform abstraction. This is currently most visible
in the client, but the rest of the code base is slowly being converted as well.

Repository Organization
=======================

The source code is organized in directories at the root of the repository. The client and server share a lot
of code found in the :file:`common` and :file:`utility` folders. The first one contains code to manage the
game state, of which both the client and server have a copy, and the second is home to various lower-level
utilities that do not have an equivalent in Qt or the C standard library. Freeciv21 also ships with a couple
of external dependencies under :file:`dependencies`.

The client code is found in the :file:`client` folder. The server code is located under :file:`server`,
with the exception of the computer (:term:`AI`) players which is under :file:`ai`. The code for other programs
bundled with Freeciv21, such as the :doc:`Modpack Installer </Manuals/modpack-installer>`, is located under
:file:`tools`.

All the assets used by the client and server are grouped under :file:`data`. This includes among
others :doc:`rulesets </Modding/Rulesets/overview>` and :ref:`tilesets <modding-tilesets>`.
Localization files are located under :file:`translations`.

There are a few additional folders that you will touch less often. The table below describes the complete
structure of the repository:

.. _arch-directories:
.. table:: Freeciv21 Code Repository Organization
  :widths: auto
  :align: left

  ==================== ==========
  Folder               Usage
  ==================== ==========
  :file:`ai`           Code for computer opponents.
  :file:`client`       Client code.
  :file:`cmake`        Build system support code.
  :file:`common`       Code dealing with the game state. Shared by the client, server, and tools.
  :file:`data`         Game assets.
  :file:`dependencies` External dependencies not found in package managers.
  :file:`dist`         Files related to distributing Freeciv21 for various operating systems.
  :file:`docs`         This documentation.
  :file:`scripts`      Useful scripts used by the maintainers.
  :file:`server`       Server code.
  :file:`tools`        Small game-related programs.
  :file:`translations` Localization.
  :file:`utility`      Utility classes and functions not found in Qt or other dependencies.
  ==================== ==========

.. note::
    Some folders do not follow this structure. Their contents should eventually be moved.
