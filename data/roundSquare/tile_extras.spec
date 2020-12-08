
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    Eleazar (buoy)
    GriffonSpade
	Hans Lemurson (Remade most terrain)

"

[file]
gfx = "roundSquare/tile_extras"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"
; Terrain special resources:

 0,  0, "ts.arctic_ivory"
 0,  1, "ts.tundra_game"
 0,  2, "ts.oasis"
 0,  3, "ts.wheat"
 0,  4, "ts.grassland_resources"
 0,  5, "ts.spice"
 0,  6, "ts.fruit"
 0,  7, "ts.pheasant"
 0,  8, "ts.wine"
 0,  9, "ts.iron"
 0, 10, "ts.whales"
 0, 11, "ts.seals"

 1,  0, "ts.arctic_oil"
 1,  1, "ts.furs"
 1,  2, "ts.oil"
 1,  3, "ts.buffalo"
 1,  4, "ts.river_resources"
 1,  5, "ts.peat"
 1,  6, "ts.gems"
 1,  7, "ts.silk"
 1,  8, "ts.coal"
 1,  9, "ts.gold"
 1, 10, "ts.fish"
 1, 11, "ts.forest_game"


; Terrain Strategic Resources

 2, 0, "ts.aluminum"
 2, 1, "ts.uranium"
 2, 2, "ts.saltpeter"
 2, 3, "ts.elephant"
 2, 4, "ts.horses"

; Terrain improvements and similar:

 3,  0, "tx.farmland"		;[HL]
 3,  1, "tx.irrigation"	;[HL]
 3,  2, "tx.mine"
 3,  3, "tx.oil_mine"
 3,  4, "tx.oil_rig"		;Make sure to add an oil rig that looks nice on water!
 3,  5, "tx.pollution"
 3,  6, "tx.fallout"

; Bases
 4,  0, "tx.village"
 4,  1, "base.airstrip_mg"
 4,  2, "base.airbase_mg"
 4,  3, "base.outpost_mg"
 4,  4, "base.fortress_bg"
 4,  5, "base.buoy_mg"
 4,  6, "base.ruins_mg"

}
