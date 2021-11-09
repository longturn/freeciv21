Scenarios Overview
******************

Overview
========

Freeciv21 scenario-files are essentially savegames with specific game situation. The savegame format allows
some things that are useful only when the savegame in question is a scenario. There's a [scenario] section
with some parameters applicable to scenario saves only, and possibility to include some lua script for
scripting the scenario. The deciding difference between regular savegames and scenario savegames is that
latter have :code:`is_scenario` set to TRUE in :code:`[scenario]` section. Freeciv21 saves current game
situation as a scenario file, with the full :code:`[scenario]` section, when one saves by the /scensave server
command or uses "Save Scenario" from the client Edit -menu.


Compatibility
=============

Freeciv21 can load savegames saved by older versions. How far back this is supported depends on version.
Scenario files are often not updated to the latest savegame format version, but can be of such older version.
When editing such older format savegames, one needs to be consistent with the format version. One cannot use
features of the newer format in older format savegame even though freeciv server can load both older format
and newer format. This document describes the current format of the Freeciv21 version it comes with. Some
things described here should not be used when editing savegames with older format.


The [scenario] section
======================

Savegame :code:`[scenario]` section can have following fields:

* :code:`is_scenario` : Always TRUE in a scenario savegame

* :code:`name` : Name of the scenario

* :code:`authors` : Text listing authors of the scenario

* :code:`description` : Description of the scenario

* :code:`save_random` : Whether random number state is saved to the scenario

* :code:`players` : Whether player information is saved to the scenario, or is it map-only scenario

* :code:`startpos_nations` : Whether nations should use start positions defined in the scenario map

* :code:`prevent new cities` : Is founding new cities prevented in the scenario, locking it to cities present
  on the map

* :code:`lake_flooding` : Should ocean tiles connecting to a lake tile flood the lake tile

* :code:`handmade` : Set to TRUE when savegame file is manually edited. This has no gameplay effect, but it
  indicates that scenario might have some properties that are not yet supported in the editor. When editing
  such a scenario in the editor, warning is shown that some properties might get lost

* :code:`allow_ai_type_fallback` : Is it ok to fallback to another player AI type if one defined in the
  scenario file is not available when scenario is loaded

* :code:`ruleset_locked` : Is the scenario locked to one specific ruleset. Default is TRUE

* :code:`ruleset_caps` : When scenario is not ruleset_locked, this capability string defines required
  capabilities of compatible rulesets

* :code:`datafile` : Luadata to load. See section 'Luadata' for details


Luadata
=======

Some rulesets have a lua script that can parse special luadata file to adjust their behavior. Such luadata
file can be provided by the ruleset itself, or by the scenario using the ruleset. This is controlled by
'datafile' field in the scenario savegames :code:`[scenario]` section.

* If the field is omitted, ruleset's own luadata is used, if present
* If field has value 'None' (case insensitive), luadata is not used at all, as if it was not present even in
  the ruleset
* Any other value of datafile is considered prefix part of a filename of form :file:`<prefix>.luadata` to use
  as the luadata file. The file must be found from the freeciv's savegame path, usually from the same
  directory where the scenario savegame itself is.
