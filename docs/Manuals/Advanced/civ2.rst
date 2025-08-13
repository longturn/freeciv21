.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

.. include:: /global-include.rst

Loading Civilization 2 Files
****************************

Freeciv21 has **limited** and **experimental** support for loading scenario
files designed for the PC game Civilization 2 (hereafter civ2).
This works by reading information from the binary file format used to save
games and scenarios.
The format is not fully understood and only a limited subset the saves can be
loaded.
This is currently not sufficient to play the loaded scenarios, but enables using
them as a base for creating Freeciv21 games.

Civilization 2 scenarios and saves come with one of two extensions: ``.sav`` or
``.scn``.
The underlying format is identical and Freeciv21 can load both.
This is done with the usual :ref:`load command <server-command-load>` or by
selecting the file :ref:`from the user interface <game-manual-load>`.

Currently, Freeciv21 can only load files produced by the Multiplayer Gold
Edition (MGE) of civ2.
Trying to load a file produced by any other version will result in an error.
