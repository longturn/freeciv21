[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    GriffonSpade
	Hans Lemurson (HP and Veterancy)

"

[file]
gfx = "roundSquare/unit_info"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"
; Unit hit-point bars: approx percent of hp remaining

 0,   0, "unit.hp_0"	;[HL]
 0,	  1, "unit.hp_10"	;[HL]
 0,   2, "unit.hp_20"	;[HL]
 0,   3, "unit.hp_30"	;[HL]
 0,   4, "unit.hp_40"	;[HL]
 0,   5, "unit.hp_50"	;[HL]
 0,   6, "unit.hp_60"	;[HL]
 0,   7, "unit.hp_70"	;[HL]
 0,   8, "unit.hp_80"	;[HL]
 0,   9, "unit.hp_90"	;[HL]
 0,  10, "unit.hp_100"	;[HL]

; Veteran Levels: up to 9 military honors for experienced units

 1, 1, "unit.vet_1"	;[HL]
 1, 2, "unit.vet_2"	;[HL]
 1, 3, "unit.vet_3"	;[HL]
 1, 4, "unit.vet_4"	;[HL]
 1, 5, "unit.vet_5"	;[HL]
 1, 6, "unit.vet_6"	;[HL]
 1, 7, "unit.vet_7"	;[HL]
 1, 8, "unit.vet_8"	;[HL]
 1, 9, "unit.vet_9"


; Unit Misc:

  1,  0, "unit.tired"
  1, 10, "user.attention"	; Variously crosshair/red-square/arrows

  2, 0, "unit.lowfuel"
  2, 1, "unit.fortifying"
  2, 2, "unit.fortified"
  2, 3, "unit.sentry"
  2, 4, "unit.loaded"
  2, 5, "unit.stack"

; Unit activity letters:  (note unit icons have just "u.")

  2, 6, "unit.auto_attack"
  2, 6, "unit.auto_settler"
  2, 7, "unit.connect"
  2, 8, "unit.auto_explore"
  2, 9, "unit.patrol"
  2, 10, "unit.goto"

; Terraforming

  3, 0, "unit.plant" ; used to be "unit.mine"
  3, 1, "unit.irrigate"
  3, 2, "unit.transform"
  3, 3, "unit.pollution"
  3, 4, "unit.fallout"
  3, 5, "unit.pillage"
  3, 6, "unit.convert"

; Road Activities

 4, 0, "unit.road"
 4, 1, "unit.rail"
 4, 2, "unit.maglev"

; Base Building

 4, 4, "unit.buoy"
 4, 5, "unit.airstrip"
 4, 6, "unit.airbase"
 4, 7, "unit.outpost"
 4, 8, "unit.fortress"

}
