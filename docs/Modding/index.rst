Modding
*******

The Modding category is an area for documentation editors to provide tips and other details on modifying
aspects of Freeciv21 such as Rulesets, Musicsets, Soundsets and Tilesets. All of these areas allow for a
large amount of varyability in game play that is not hardcoded in the software. This is one of the
strengths of Freeciv21.

There are five major areas of Modding that are often called "Modpacks" and are written by "Modders". The
sections below describe these major Modding areas.

All of the Modpacks are written in plain text files, except for Scenarios. The plain text files resemble
:literal:`ini` files in format and are known as :literal:`spec` files to Freeciv21. Scenarios are specially
designed saved games that have been edited for a specific purpose. Examples include: Earth Map, Europe 1900,
Americas Map, etc.

Rulesets
========

Rulesets are a collection of :literal:`spec` files that fully define a game's rules. Rulesets are broken down
into 12 files. The :literal:`spec` files for rulesets are defined as follows:

- :literal:`game.serv`
- :literal:`game/buildings.ruleset`
- :literal:`game/cities.ruleset`
- :literal:`game/effects.ruleset`
- :literal:`game/game.ruleset`
- :literal:`game/governments.ruleset`
- :literal:`game/nations.ruleset`
- :literal:`game/script.lua`
- :literal:`game/styles.ruleset`
- :literal:`game/techs.ruleset`
- :literal:`game/terrain.ruleset`
- :literal:`game/units.ruleset`

Have a look at :file:`civ2civ3.serv` and associated files in :file:`/civ2civ3` for an example.

Refer to Ruleset specific documents:

.. toctree::
  Rulesets/overview.rst
  Rulesets/effects.rst
  Rulesets/actions.rst
  Rulesets/achievements.rst
  Rulesets/nations.rst
  :maxdepth: 1

Tilesets
========

Freeciv21 allows full customization of the appearance of the map including terrain, cities, units, buildings,
and a few elements of the user interface. Freeciv21 already ships with a variety of presets that can be
selected from the menu by navigating to :menuselection:`Game --> Load Another Tileset`.

.. figure:: /_static/images/tilesets_demo.png
  :alt: A Freeciv21 map with the ``hexemplio`` and ``isophex`` tilesets.
  :align: center

  The same map with two tilesets: ``hexemplio`` (left) and ``isophex`` (right).

Nearly every aspect of the map rendering can be customized. In practice, this is achieved using a myriad of
small images, called :emphasis:`sprites`, that are assembled together to form the final map. For instance,
the units above are made of up to four sprites drawn on top of each other: the flag, the health bar, the
yellow activity indicator, and finally the image that represents the unit itself. Customization is made
possible thanks to a system of configuration files that specify where to find the sprites and how to assemble
them.

Tilesets are a collection of :literal:`spec` files that fully define the look and feel of the game map, units,
buildings, etc. This is effectively the graphics layer of Freeciv21. A tileset Modder can create a whole new
custom graphics look and feel. The file layout for a tileset can vary depending on how the author wants to
break out the varying layers. It will always start with a top-level :literal:`.tilespec` file and with a
directory of the same name will have :literal:`.png` graphics files and associated :literal:`.spec` files to
explain to Freeciv21 what to do when.

Have a look at :file:`amplio.tilespec` and associated files in :file:`/amplio` for an example. The following
guides document specific aspects of tileset creation:

.. toctree::
  Tilesets/tutorial.rst
  Tilesets/graphics.rst
  Tilesets/debugger.rst
  Tilesets/compatibility.rst
  :maxdepth: 1

Soundsets
=========

Soundsets are a collection of :literal:`spec` files that allow a Modder to add sound files to varying events
that happen inside the game. Events such as founding a city, or attacking a unit can have a sound associated
with them. There is a huge number of events in Freeciv21 that a Modder can attach a sound file to. Soundsets
will start with a top-lvel :literal:`.soundset` file and with a directory of the same name will have
:literal:`.ogg` sound files to play in the client.

Have a look at :file:`stdsounds.soundspec` and associated files in :file:`/stdsounds` for an example.

.. toctree::
  sound.rst
  :maxdepth: 1

Musicsets
=========

Musicsets are a collection of :literal:`spec` files that allow a Modder to add Music files to play as a
soundtrack inside the game. Game music follows the game based on the nation selected and the mood. The mood is
essentially binary: peace or war. Musicsets will start with a top-lvel :literal:`.musicspec` file and with a
directory of the same name will have :literal:`.ogg` sound files to play in the client.

Have a look at :file:`stdmusic.musicspec` and associated files in :file:`/stdmusic` for an example.

.. toctree::
  musicsets.rst
  :maxdepth: 1

Scenarios
=========

Scenarios are custom saved games that a player can load and play. A Modder will use the map editor to create
a map of the scenario and enable/change varying aspects of the game to set up the game scenario.

Have a look at the scenarios shipped with Freeciv21 in :file:`/scenarios` for some examples.

.. toctree::
  scenarios.rst
  :maxdepth: 1

Installer
=========

Refer to a document on how to serve your own modpack set. If you are interested in how to use the modpack
installer, refer to :doc:`../General/modpack-installer`

.. toctree::
  modpack-installer.rst
  :maxdepth: 1

