Musicsets Overview
******************

A Musicset is a soundtrack for Freeciv21 and is often documented in a single :literal:`.musicspec` file with
references to the media files in a separate sub-directory.

The soundtrack is broken up into time periods following the technology tree and is split by two moods: peace
and combat. Peace is generally defined when you are "farming" and growing your civilization with no fighting
going on at all. Any kind of fighting, inlcuding defense of your cities will trigger combat and the music mood
will change. After 16 turns of no combat the music will return to peace.

At the beginning of the game, you select the style of music you want to hear by picking the city style of your
civilization. The current choices are:

* European
* Classical
* Tropical
* Asian
* Babylonian
* Celtic

The city style is automatically selected for you when you pick your nation, but you can change it if you like.

The discovery of :strong:`University` changes the music for all city styles for the Renaissance Age. All further
disoveries noted below will change the music for all city styles/nations.

The discovery of :strong:`Railroad` brings in the Industrial Age.

The discovery of :strong:`Automobile` brings in the Electric Age.

The discovery of :strong:`Rocketry brings` in the Modern Age.

The discovery of :strong:`Superconductors` brings in the Post-Modern Age soundtrack. This is the final
soundtrack of the game.

This is all documented in the :file:`sytles.ruleset` file in any ruleset. The ruleset file also establishes
the look and feel of the cities as they grow and technologies are learned.


Spec File Layout
================

The :literal:`.musicspec` file is quite simple. Here is a sample:

.. code-block:: ini

  [musicspec]
  ; Format and options of this spec file:
  options = "+Freeciv-2.6-musicset"

  [info]
  ; Add information related to the musicset here, such as a list of artists

  [files]
  music_menu = "MusicDirectory/MenuMusic.ogg"
  music_victory = "MusicDirectory/VictoryMusic.ogg"
  music_defeat = "MusicDirectory/DefeatMusic.ogg"

  ;Ancient Times - Bronze Age / Selection of European City Style
  music_european_peace_X = "MusicDirectory/EuropeanPeaceSong0.ogg"
  music_european_combat_X = "MusicDirectory/EuropeanCombatSong0.ogg"
  ;
  ;Ancient Times - Bronze Age / Selection of Classical City Stlye
  music_classical_peace_X = "MusicDirectory/ClassicalPeaceSong0.ogg"
  music_classical_combat_X = "MusicDirectory/ClassicalCombatSong0.ogg"
  ;
  ;Ancient Times - Bronze Age / Selection of Tropical City Stlye
  music_tropical_peace_X = "MusicDirectory/TropicalPeaceSong0.ogg"
  music_tropical_combat_X = "MusicDirectory/TropicalCombatSong0.ogg"
  ;
  ;Ancient Times - Bronze Age / Selection of Asian City Style
  music_asian_peace_X = "MusicDirectory/AsianPeaceSong0.ogg"
  music_asian_combat_X = "MusicDirectory/AsianCombatSong0.ogg"
  ;
  ;Ancient Times - Bronze Age / Selection of Babylonian City Stlye
  music_babylonian_peace_X = "MusicDirectory/BabylonianPeaceSong0.ogg"
  music_babylonian_combat_X = "MusicDirectory/BabylonianCombatSong0.ogg"
  ;
  ;Ancient Times - Bronze Age / Selection of Celtic City Stlye
  music_celtic_peace_X = "MusicDirectory/CelticPeaceSong0.ogg"
  music_celtic_combat_X = "MusicDirectory/CelticPeaceSong0.ogg"
  ;
  ;Discovery of University brings in Renaissance Age
  music_renaissance_peace_X = "MusicDirectory/RenaissancePeaceSong0.ogg"
  music_renaissance_combat_X = "MusicDirectory/RenaissanceCombatSong0.ogg"
  ;
  ;Discovery of Railroad brings in the Industrial Age - Much more combat here
  music_industrial_peace_X = "MusicDirectory/IndustrialPeaceSong0.ogg"
  music_industrial_combat_X = "MusicDirectory/IndustrialCombatSong0.ogg"
  ;
  ;Discovery of Automobile brings in the Electric Age - At ths time, pretty much all combat
  music_electricage_peace_X = "MusicDirectory/ElectricPeaceSong0.ogg"
  music_electricage_combat_X = "MusicDirectory/ElectricCombatSong0.ogg"
  ;
  ;Discovery of Rocketry brings in the Modern Age
  music_modern_peace_X = "MusicDirectory/ModernPeaceSong0.ogg"
  music_modern_combat_X = "MusicDirectory/ModernCombatSong0.ogg"
  ;
  ;Discovery of Superconductors brings in the Post-Modern Age
  music_postmodern_peace_X = "MusicDirectory/PostModernPeaceSong0.ogg"
  music_postmodern_combat_X = "MusicDirectory/PostModernCombatSong0.ogg"


The value of X is a number from zero ( 0 ) up. For more than one song of each type, add a row and give the
option another number +1 from the last.  You can reuse song files in different areas.
