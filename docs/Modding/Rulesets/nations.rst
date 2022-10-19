..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2021 louis94 <m_louis30@yahoo.com>
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Nation Sets and Flags
*********************

This page describes the contents of the nation :file:`.spec` files. This is intended as developer reference,
and for people wanting to create/compile alternative nation files for Freeciv21. A nation consists of a nation
file in the rulesets and a flag in the tilesets.

There are already many nations, but of course some of them are missing. We're quite open to new "nations" even
if they're just part of a larger country, but obscure towns that nobody knows will be rejected. Things like US
states or German Länder are OK. South African provinces are probably not.

The following information is required to add a new nation:

* The name of the citizens (singular and plural), for instance `Indian` and `Indians`.
* A set of "groups" under which the nation will be shown in the client. Usually, you can copy the groups from
  an existing nation.
* A short historical description of the nation, shown in the Help dialog.
* Several (real) leader names, preferably both men and women. Prefer recognized historically important
  leaders, eg Henri IV (https://en.wikipedia.org/wiki/Henry_IV_of_France) and
  Napoléon Bonaparte (https://en.wikipedia.org/wiki/Napoleon), but not
  Jean Casimir-Perier (https://en.wikipedia.org/wiki/Jean_Casimir-Perier).
* Special names for the ruler in certain governments, if applicable. For instance, when in Despotism the
  Egypian leader is called *Pharaoh*.
* A flag, preferably something official (if your nation doesn't have an official flag, think twice before
  including it). Wikipedia has many of those under free licenses.
* A few nations that will be preferred by the server when there's a civil war. For instance, the Conferderate
  are a civil war nation for the Americans.
* City names. List each city only once, even if it changed name in the course of history. Try to use names
  from the same epoch.

Most of the information goes into your nation's :file:`.ruleset` file. There are many examples in the code
repository.

Four variants of the flag are needed:

* A "small" flag, 29x20 with a 1px black border
* A "large" flag, 44x30 with a 1px black border
* A "shield", 14x14 with a 1px black border and a shield shape
* A "large shield", 19x19 with a 1px black border and a shield shape

Information on where the files go is described below.

Local Nation Files
==================

Most supplied rulesets nations can be added locally without need to modify Freeciv21 distribution files. This
section discuss the way the local override files work. Later sections assume that a nation is being added to
main Freeciv21 distribution, even if only to locally modified copy.

Freeciv21 searches data files from several directories in priority order. Local nations overrides mechanism
uses this to include files from user data directory, :file:`~/.local/share/freeciv21/<freeciv21 version>/override/`.
Freeciv21 distribution has empty versions of those files in a lower priority directory. Once user adds the
file, it gets used instead of the empty one.

* :file:`~/.local/share/freeciv21/<version>/override/nation.ruleset`

Ruleset sections for nations that user wants to add. This can of course use :code:`*include` directives so
that individual nations are in separate files. See below sections for the format of the nation rulesets.

* :file:`~/.local/share/freeciv21/<version>/override/flags.spec`
* :file:`~/.local/share/freeciv21/<version>/override/shields.spec`
* :file:`~/.local/share/freeciv21/<version>/override/flags-large.spec`
* :file:`~/.local/share/freeciv21/<version>/override/shields-large.spec`

Spec files for flag graphics to add. See below sections for the format of spec files and graphics files.


How to add a Nation
===================

To add a nation of your own, you should look at the following files:

* :file:`data/nation/<nationname>.ruleset`

This is the new nation, which you will have to create. It may help to copy one of the other nation files over
and edit it. See below for a style guide for nation files.

* The :code:`<nationname>` bit is to be replaced with the nations name (duh). Please don't use whitespaces and
  special characters. Underlines are ok though.
* The name should be the same as the name of the nation inside the ruleset file.
* The file must be encoded in UTF-8.

* :file:`data/default/nationlist.ruleset`

This lists all nation files. Add your nation (:file:`data/nation/<nationname>.ruleset`) to this list.

data/flags/*

  This is the flags directory. You will have to add a flag-file
  (see below) for your nation to work (see below).

* :file:`data/scenarios/*`

You can add starting position for your nation on a scenario map.

Before a nation can be included in the main distribution, the following files will also have to be edited.
Unless you know what you're doing you shouldn't need to worry about this.

* :file:`data/nation/CMakeLists.txt`

Another list of nation files - add your nation (:code:`<nationname>.ruleset`) to this list.

* :file:`translations/nations/POTFILES.in`

Here is yet another list of nations files; again add your nation (:file:`data/nation/<nationname>.ruleset`) to
it.  Nations part of the "core" group go to :file:`translations/freeciv/POTFILES.in` instead.


How to add a Flag
=================

Overview
--------

PNG is the preferred form for graphics, and flags should be made exclusively in SVG.

A new nation needs a new flag. All flags are stored in SVG (Scalable Vector Graphics) format. Sodipodi and
Inkscape are two good SVG editors. If you are creating a real-world nation you can probably find a Free or
public domain flag that can be used. One good place to look is the Open Clip Art Library (OCAL). Remember that
any flags we add must be licenced under the GPL and should be attributed to their original author, so make a
note of where you found the flag, what its licence is, and who made it.

We also welcome improvements to existing flags. Most of our existing flags come from the Sodipodi clipart
collection, and some of them are less than perfect. One common problem is that the colors are wrong. If you
fix a flag for a real nation be sure to cite your source so we can be sure it's accurate. Good sources for
nation flag data are Wikipedia or Flags Of The World.

If you want to improve an imaginary flag, this is also welcome. We recommend you first contact the original
author of the flag (see the flags/credits file) to discuss your ideas for changes.


Flag Guidelines
---------------

Here are a few guidelines for flags:

* Flags should be rectangles, since an outline is added to them automatically.
* Flags often come in multiple aspect ratios. A 3:2 ratio looks best for Freeciv21 and currently every flag
  has this ratio. For a flag that is "supposed" to be 2:1 or 4:3, you can often find a 3:2 version
  as well.


Flag Specifics
--------------

To add a flag you'll have to edit the following files:

* :file:`data/tilesets/flags/<flagname>.svg`

Here is the SVG flag image. This is not used directly by Freeciv21 but is  rendered into PNG files (at various
resolutions for different tilesets). The SVG file is not used in Freeciv21, but all the other steps for adding
flags are the same. The :code:`<flagname>` should either be the name of the country that represents the flag,
or the common name for the actual flag. When in doubt, use the same name as the name of the nation.

* :file:`data/tilesets/flags/<flagname>.png`

* :file:`data/tilesets/flags/<flagname>-shield.png`

These are the flag images that are used by Freeciv21. They are rendered from the SVG file. Once this file has
been created it can be used with older versions of Freeciv21 as well. To run the conversion program you will
need to install Inkscape, ImageMagick, and (optionally) pngquant.

* :file:`data/misc/flags.spec`

This file has a reference to the flag PNG graphic. The "tag" here must match the flag tag you put in the
nation ruleset file (usually :code:`f.<flagname>`) and the "file" should point to the PNG image at
:file:`flags/<flagname>.png`.

* :file:`data/misc/flags-large.spec`

Just like :file:`flags.spec`, but large version of the graphics.

* :file:`data/misc/shields.spec`

Just like :file:`flags.spec`, this file must include a reference to the flag PNG graphic. The only difference
is that the file should point to the "shield" graphic, :file:`flags/<flagname>-shield.png`.

* :file:`data/misc/shields-large.spec`

Just like :file:`shields.spec`, but large version of the graphics.

Contents and Style
==================

What Nations Can Be Added
-------------------------

A nation in Freeciv21 should preferrably be a current independent country or a historical kingdom or realm. A
nation that is currently governed by or the part of a greater political entity, or in other ways lacks
complete independence could in most cases be made a Freeciv nation as well, but must never be listed as
*modern* (see 'Nation grouping' below.)

Copyrighted content may not be added unless full permission is granted by the holder of the copyright. This
rule effectively disallows the inclusion of nations based on most literary works.


Nation Grouping
---------------

Freeciv21 Supports A Classification Of Nations In An Unlimited Number Of Groups And Every Nation Should Be
Assigned To At Least One. We Currently Have Ancient, Medieval, Early Modern, Modern, African, American, Asian,
European, Oceanian And Imaginary Groups. Modern Nations Are Existing And Politically Independent Countries; A
Nation Listed As Ancient, Medieval Or Early Modern Should Have Had An Independent Dynasty Or State In Ancient
(Until 500 Ad), Medieval (500 - 1500) Or Early Modern (1500 - 1800) Times Respectively. Finally, An Imaginary
Nation Is - As The Name Suggests - A Product Of Someone'S Imagination.


Nation Naming
-------------

The default name of the nation should be the name of the people, country, or empire in English adjective form.
For example, the nation of ancient Babylon is called "Babylonian" in Freeciv21. The plural form should be
standard English as well. For example, plural for the Polish nation is "Poles" in Freeciv21. UTF-8 is
permitted in nation names.


Conflicting Nations
-------------------

To specify one or more nations that the AI shouldn't pick for the same
game, use this syntax:

     :code:`conflicts_with="<nationname>", "<nationname>", ...`

You only have to specify this in the nation you're adding, since it works in both directions. Reasons for
conflicting nations could be either that they represent the same people in different eras (ex: Roman -
Italian) or that the two nations have too similar flags that they are easily mixed up in the game (ex: Russian
- Serbian.)


Civil War Nations
-----------------

Specify one or more civil war nations. When a player's capital is captured, that player might suffer a civil
war where his or her nation is divided and a new player created. The nation for this new player is selected
from one of the civil war nations specified in the ruleset. A civil war nation should be linguistically,
geographically and/or historically related to the current nation. A linguistic relation is especially
important, since city names after a nation run out of their own city names, are selected from the civil war
nations' city lists.

Legend
------

A legend is required in a nation ruleset. The legend can be a summarized history of the nation, or just a
piece of trivia. UTF-8 is permitted in legends.

Leaders
-------

A leader should be a historically notable political leader of the nation. Two living persons per nation are
permitted - one of each sex. An ideal leader list should contain between five and ten names. Use the person's
full name to avoid ambiguity. Monarchs should be marked with the appropriate succession number, using Roman
numerals in standard English style (not German e.g. "Otto II."; Hungarian e.g. "IV. Béla"; Danish e.g.
"Valdemar 4." etc.)

Freeciv21 supports any Unicode character, but please keep to Latin letters. When transcribing from a non-Latin
writing system, be consistent about the system of transcription you are using. Also, try to avoid
unnecessarily technical and/or heavily accented systems of transcription. Subject to the above, leaders should
be written in native orthography, e.g. "Karl XII" instead of "Charles XII" for the Swedish king.

For consistency and readability, put only one leader per line. Feel free to provide a hint of the leader's
identity or a brief background in a comment beside any leader: This information might be used in-game at a
later stage.

Leader titles for each government type (including Despotism and Anarchy) may be specified in a separate tag.
UTF-8 is permitted in leader titles.  If the male and female titles are identical in English, give the latter
the :code:`?female:` qualifier. Use a unique title for each government. Ruler titles should be in English,
though exceptions are made for non English titles as long as they are understood outside of their own language
regions and commonly used in non-academic contexts. Titles from the default ruleset may not be used.

Flag
----

You should provide a unique flag for your nation. Using a flag that is already used by another nation in the
game is not acceptable. An alternative flag does not have to be specified.

Style
-----

A nation must specify a default style. With the supplied rulesets each national style has direct relation to
equivalent city style. The available city styles depends on the tileset used. Practically every tileset has
four city styles: "European", "Classical" (Graeco-Roman style), "Asian" (Pagoda style) and "Tropical" (African
or Polynesian style). In Amplio tileset, "Babylonian" and "Celtic" are also available. If the tileset used by
a client does not support a particular city style, a fallback style is used. Selecting a style for your nation
is not that strict. Just try to keep it somewhat "realistic."

Cities
======

As for the list of city names, you should make a clear decision about the type of the nation you add. An
*ancient* or *medieval* nation may list any city that it at some point controlled. However if your nation is
listed as *modern*, its city list must be restricted to cities within the country's current borders.

The reason for this is, we don't want Freeciv21 to be used as a political vehicle for discussions about
borders or independence of particular nations. Another reason is to avoid overlapping with other nations in
the game.

A city should appear in its native form, rather than Anglicized or Graeco-Roman forms. For example, the Danish
capital is "København" rather than "Copenhagen"; and the ancient Persian capital is "Parsa" rather than
"Persepolis."

City names support any Unicode character, but please keep to Latin letters. When transcribing from a non-Latin
writing system, be consistent about the system of transcribation you are using. Also, try to avoid
unnecessarily technical or heavily accented systems of transcribation.

The ordering of cities should take both chronology of founding and overall historical importance into
consideration. Note that a city earlier in the list has a higher chance of being chosen than later cities.

Natural City Names
------------------

Freeciv21 supports "natural" geographic placements of cities. Cities can be labeled as matching or not
matching a particular type of terrain, which will make them more (or less) likely to show up as the "default"
name. The exact format of the list entry is

     :code:`"<cityname> (<label>, <label>, ...)"`

where the cityname is just the name for the city (note that it may not contain quotes or parenthesis), and
each "label" matches (case-insensitive) a terrain type for the city (or "river"), with a preceding ! to negate
it. The terrain list is optional, of course, so the entry can just contain the cityname if desired. A city
name labeled as matching a terrain type will match a particular map location if that map location is on or
adjacent to a tile of the named terrain type; in the case of the "river" label (which is a special case) only
the map location itself is considered. A complex example:

     :code:`"Wilmington (ocean, river, swamp, forest, !hills, !mountains, !desert)"`

will cause the city of Wilmington to match ocean, river, swamp, and forest tiles while rejecting hills,
mountains, and deserts. Although this degree of detail is probably unnecessary to achieve the desired effect,
the system is designed to degrade smoothly so it should work just fine.

.. note::
  A note on scale: it might be tempting to label London as :code:`!ocean`, i.e. not adjacent to an ocean.
  However, on a reasonably-sized Freeciv21 world map, London will be adjacent to the ocean; labeling it
  :code:`!ocean` will tend to give bad results. This is a limitation of the system, and should be taken into
  account when labelling cities.

At this point, it is useful to put one city per line, only. Finally, don't forget to leave a blank line feed
in the end of your nation ruleset.

