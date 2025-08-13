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

.. warning::
  Experimental support means that the game may misbehave (or outright crash)
  when encountering an unexpected feature.
  When reporting such cases, please always include the file you were trying to
  load.

Civilization 2 scenarios and saves come with one of two extensions: ``.sav`` or
``.scn``.
The underlying format is identical and Freeciv21 can load both.
This is done with the usual :ref:`load command <server-command-load>` or by
selecting the file :ref:`from the user interface <game-manual-load>`.

Currently, Freeciv21 can only load files produced by the Multiplayer Gold
Edition (MGE) of civ2.
Trying to load a file produced by any other version will result in an error.
Freeciv21 also cannot parse civ2 map files (``.mp``).

Supported Features
------------------

Currently, Freeciv21 mainly loads ruleset-independent features:

* The username, color, and gender of all alive players.
* AI status. AI level is assigned based on the civ2 difficulty level.
* Treasury and tax rates.
* Diplomatic relations between players (except for the *None* diplomatic status,
  which is translated to *War*, and *Vendetta* which is ignored).
* Embassies.
* The map: terrain, rivers, improvements and player knowledge thereof.
* Cities and citizen assignment on the map.

Known Limitations
-----------------

The main limitation is that Freeciv21 cannot understand changes to the civ2
rules.
The games are loaded with an unmodified ``civ2`` ruleset.
Information from ``RULES.TXT`` and ``EVENTS.TXT`` is disregarded.
This means that terrains may have the wrong yields and also prevents a
meaningful loading of nations, units, buildings, techs, or specialists.
While the format of the above files is well documented and could easily be
parsed, doing so will require some rethinking of the way Freeciv21 handles game
rules (in particular regarding saves).

Another important issue is that Freeciv21 does not load scenario sprites.
The map may look strange as a result.
For now, we suggest creating a modified version of the ``isotrident`` tileset
for each scenario.
``Isotrident`` has the same geometry as the original civ2 sprites and terrain
sprites follow a similar layout.

Other known limitations include:

* The game rules in civ2 depend on the difficulty level, in particular regarding
  unhappiness. Freeciv21 always targets the deity (hardest) setting.
* If the map has an odd number of rows, the last row is skipped. This is
  required because Freeciv21 only supports an even number of rows.
* Tile special resources are generated again the Freeciv21 way. The information
  stored by civ2 is not sufficient to restore them faithfully. Freeciv21 ignores
  the "no resources" flag.
* Base ownership is not assigned.
* Map wrapping is not restored.
* Trade routes are missing. Commodities are not supported by Freeciv21's
  ``civ2`` ruleset.
* Cities' food, shields, improvements, and original owners are not restored.
* Freeciv21 supports more than 255 cities. This may affect scenarios that rely
  on this limit to prevent the creating of new cities.
* The name of the Barbarian nation appears incorrectly. It is not stored in the
  save.
* All players start in Despotism.
* Some cities may generate warnings. This is caused by civ2 saving what appears
  to be inconsistent data about citizen assignment to tiles around the cities,
  and is harmless (but you may want to check the cities).
* Many game settings that could be mapped to Freeciv21 server settings are not.
  Example of this are victory settings and the Barbarian activity modifier.
