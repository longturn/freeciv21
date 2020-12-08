
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
    Tatu Rissanen <tatu.rissanen@hut.fi>
    Jeff Mallatt <jjm@codewell.com> (miscellaneous)
    Eleazar (buoy)
    Michael Johnson <justaguest> (nuke explosion)
    The Square Cow (inaccessible terrain)
    GriffonSpade
	Hans Lemurson (Remade most terrain)

"

[file]
gfx = "roundSquare/tiles"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

; Grassland, and whether terrain to north, south, east, west
; is more grassland:

  0,  2, "t.l0.grassland1"
  0,  3, "t.l0.inaccessible1"

  0,  1, "t.l1.grassland1"
  0,  1, "t.l1.hills1"
  0,  1, "t.l1.forest1"
  0,  1, "t.l1.mountains1"
  0,  1, "t.l1.desert1"
  0,  1, "t.l1.jungle1"
  0,  1, "t.l1.plains1"
  0,  1, "t.l1.swamp1"
  0,  1, "t.l1.tundra1"

  0,  1, "t.l2.grassland1"
  0,  1, "t.l2.hills1"
  0,  1, "t.l2.forest1"
  0,  1, "t.l2.mountains1"
  0,  1, "t.l2.desert1"
  0,  1, "t.l2.arctic1"
  0,  1, "t.l2.jungle1"
  0,  1, "t.l2.plains1"
  0,  1, "t.l2.swamp1"
  0,  1, "t.l2.tundra1"


; For hills, forest and mountains don't currently have a full set,
; re-use values but provide for future expansion; current sets
; effectively ignore N/S terrain.

; Hills, and whether terrain to north, south, east, west
; is more hills.

  0,  4, "t.l0.hills_n0e0s0w0",  ; not-hills E and W
         "t.l0.hills_n0e0s1w0",
         "t.l0.hills_n1e0s0w0",
         "t.l0.hills_n1e0s1w0"
  0,  4, "t.l0.hills_n0e1s0w0",  ; hills E ; was 0,5 but I'm using only one hill graphic now ;[HL]
         "t.l0.hills_n0e1s1w0",
         "t.l0.hills_n1e1s0w0",
         "t.l0.hills_n1e1s1w0"
  0,  4, "t.l0.hills_n0e1s0w1",  ; hills E and W ; was 0,6
         "t.l0.hills_n0e1s1w1",
         "t.l0.hills_n1e1s0w1",
         "t.l0.hills_n1e1s1w1"
  0,  4, "t.l0.hills_n0e0s0w1",  ; hills W ; was 0,7
         "t.l0.hills_n0e0s1w1",
         "t.l0.hills_n1e0s0w1",
         "t.l0.hills_n1e0s1w1"

; Forest, and whether terrain to north, south, east, west
; is more forest.

  0,  8, "t.l0.forest_n0e0s0w0",  ; not-forest E and W		//will use this tile as the only forest Graphic ;[HL]
         "t.l0.forest_n0e0s1w0",
         "t.l0.forest_n1e0s0w0",
         "t.l0.forest_n1e0s1w0"
  0,  8, "t.l0.forest_n0e1s0w0",  ; forest E			//was 0,9
         "t.l0.forest_n0e1s1w0",
         "t.l0.forest_n1e1s0w0",
         "t.l0.forest_n1e1s1w0"
  0,  8, "t.l0.forest_n0e1s0w1",  ; forest E and W		//was 0,10
         "t.l0.forest_n0e1s1w1",
         "t.l0.forest_n1e1s0w1",
         "t.l0.forest_n1e1s1w1"
  0,  8, "t.l0.forest_n0e0s0w1",  ; forest W			//was 0,11
         "t.l0.forest_n0e0s1w1",
         "t.l0.forest_n1e0s0w1",
         "t.l0.forest_n1e0s1w1"

; Mountains, and whether terrain to north, south, east, west
; is more mountains.

  0, 12, "t.l0.mountains_n0e0s0w0",  ; not-mountains E and W	//will use this tile as the only hills graphic ;[HL]
         "t.l0.mountains_n0e0s1w0",
         "t.l0.mountains_n1e0s0w0",
         "t.l0.mountains_n1e0s1w0"
  0, 12, "t.l0.mountains_n0e1s0w0",  ; mountains E 			//was 0,13
         "t.l0.mountains_n0e1s1w0",
         "t.l0.mountains_n1e1s0w0",
         "t.l0.mountains_n1e1s1w0"
  0, 12, "t.l0.mountains_n0e1s0w1",  ; mountains E and W	//was 0,14
         "t.l0.mountains_n0e1s1w1",
         "t.l0.mountains_n1e1s0w1",
         "t.l0.mountains_n1e1s1w1"
  0, 12, "t.l0.mountains_n0e0s0w1",  ; mountains W			//was 0,15
         "t.l0.mountains_n0e0s1w1",
         "t.l0.mountains_n1e0s0w1",
         "t.l0.mountains_n1e0s1w1"

; Desert, and whether terrain to north, south, east, west
; is more desert:

  1,  0, "t.l0.desert_n1e1s1w1"
  1,  1, "t.l0.desert_n0e1s1w1"
  1,  2, "t.l0.desert_n1e0s1w1"
  1,  3, "t.l0.desert_n0e0s1w1"
  1,  4, "t.l0.desert_n1e1s0w1"
  1,  5, "t.l0.desert_n0e1s0w1"
  1,  6, "t.l0.desert_n1e0s0w1"
  1,  7, "t.l0.desert_n0e0s0w1"
  1,  8, "t.l0.desert_n1e1s1w0"
  1,  9, "t.l0.desert_n0e1s1w0"
  1, 10, "t.l0.desert_n1e0s1w0"
  1, 11, "t.l0.desert_n0e0s1w0"
  1, 12, "t.l0.desert_n1e1s0w0"
  1, 13, "t.l0.desert_n0e1s0w0"
  1, 14, "t.l0.desert_n1e0s0w0"
  1, 15, "t.l0.desert_n0e0s0w0"

; Arctic, and whether terrain to north, south, east, west
; is more arctic:

  6,  0, "t.l0.arctic_n1e1s1w1"
  6,  1, "t.l0.arctic_n0e1s1w1"
  6,  2, "t.l0.arctic_n1e0s1w1"
  6,  3, "t.l0.arctic_n0e0s1w1"
  6,  4, "t.l0.arctic_n1e1s0w1"
  6,  5, "t.l0.arctic_n0e1s0w1"
  6,  6, "t.l0.arctic_n1e0s0w1"
  6,  7, "t.l0.arctic_n0e0s0w1"
  6,  8, "t.l0.arctic_n1e1s1w0"
  6,  9, "t.l0.arctic_n0e1s1w0"
  6, 10, "t.l0.arctic_n1e0s1w0"
  6, 11, "t.l0.arctic_n0e0s1w0"
  6, 12, "t.l0.arctic_n1e1s0w0"
  6, 13, "t.l0.arctic_n0e1s0w0"
  6, 14, "t.l0.arctic_n1e0s0w0"
  6, 15, "t.l0.arctic_n0e0s0w0"

  2,  0, "t.l1.arctic_n1e1s1w1"
  2,  1, "t.l1.arctic_n0e1s1w1"
  2,  2, "t.l1.arctic_n1e0s1w1"
  2,  3, "t.l1.arctic_n0e0s1w1"
  2,  4, "t.l1.arctic_n1e1s0w1"
  2,  5, "t.l1.arctic_n0e1s0w1"
  2,  6, "t.l1.arctic_n1e0s0w1"
  2,  7, "t.l1.arctic_n0e0s0w1"
  2,  8, "t.l1.arctic_n1e1s1w0"
  2,  9, "t.l1.arctic_n0e1s1w0"
  2, 10, "t.l1.arctic_n1e0s1w0"
  2, 11, "t.l1.arctic_n0e0s1w0"
  2, 12, "t.l1.arctic_n1e1s0w0"
  2, 13, "t.l1.arctic_n0e1s0w0"
  2, 14, "t.l1.arctic_n1e0s0w0"
  2, 15, "t.l1.arctic_n0e0s0w0"

; Jungle, and whether terrain to north, south, east, west
; is more jungle:

  3,  0, "t.l0.jungle_n1e1s1w1"
  3,  1, "t.l0.jungle_n0e1s1w1"
  3,  2, "t.l0.jungle_n1e0s1w1"
  3,  3, "t.l0.jungle_n0e0s1w1"
  3,  4, "t.l0.jungle_n1e1s0w1"
  3,  5, "t.l0.jungle_n0e1s0w1"
  3,  6, "t.l0.jungle_n1e0s0w1"
  3,  7, "t.l0.jungle_n0e0s0w1"
  3,  8, "t.l0.jungle_n1e1s1w0"
  3,  9, "t.l0.jungle_n0e1s1w0"
  3, 10, "t.l0.jungle_n1e0s1w0"
  3, 11, "t.l0.jungle_n0e0s1w0"
  3, 12, "t.l0.jungle_n1e1s0w0"
  3, 13, "t.l0.jungle_n0e1s0w0"
  3, 14, "t.l0.jungle_n1e0s0w0"
  3, 15, "t.l0.jungle_n0e0s0w0"

; Plains, and whether terrain to north, south, east, west
; is more plains:

  4,  0, "t.l0.plains_n1e1s1w1"
  4,  1, "t.l0.plains_n0e1s1w1"
  4,  2, "t.l0.plains_n1e0s1w1"
  4,  3, "t.l0.plains_n0e0s1w1"
  4,  4, "t.l0.plains_n1e1s0w1"
  4,  5, "t.l0.plains_n0e1s0w1"
  4,  6, "t.l0.plains_n1e0s0w1"
  4,  7, "t.l0.plains_n0e0s0w1"
  4,  8, "t.l0.plains_n1e1s1w0"
  4,  9, "t.l0.plains_n0e1s1w0"
  4, 10, "t.l0.plains_n1e0s1w0"
  4, 11, "t.l0.plains_n0e0s1w0"
  4, 12, "t.l0.plains_n1e1s0w0"
  4, 13, "t.l0.plains_n0e1s0w0"
  4, 14, "t.l0.plains_n1e0s0w0"
  4, 15, "t.l0.plains_n0e0s0w0"

; Swamp, and whether terrain to north, south, east, west
; is more swamp:

  5,  0, "t.l0.swamp_n1e1s1w1"
  5,  1, "t.l0.swamp_n0e1s1w1"
  5,  2, "t.l0.swamp_n1e0s1w1"
  5,  3, "t.l0.swamp_n0e0s1w1"
  5,  4, "t.l0.swamp_n1e1s0w1"
  5,  5, "t.l0.swamp_n0e1s0w1"
  5,  6, "t.l0.swamp_n1e0s0w1"
  5,  7, "t.l0.swamp_n0e0s0w1"
  5,  8, "t.l0.swamp_n1e1s1w0"
  5,  9, "t.l0.swamp_n0e1s1w0"
  5, 10, "t.l0.swamp_n1e0s1w0"
  5, 11, "t.l0.swamp_n0e0s1w0"
  5, 12, "t.l0.swamp_n1e1s0w0"
  5, 13, "t.l0.swamp_n0e1s0w0"
  5, 14, "t.l0.swamp_n1e0s0w0"
  5, 15, "t.l0.swamp_n0e0s0w0"

; Tundra, and whether terrain to north, south, east, west
; is more tundra:

  6,  0, "t.l0.tundra_n1e1s1w1"
  6,  1, "t.l0.tundra_n0e1s1w1"
  6,  2, "t.l0.tundra_n1e0s1w1"
  6,  3, "t.l0.tundra_n0e0s1w1"
  6,  4, "t.l0.tundra_n1e1s0w1"
  6,  5, "t.l0.tundra_n0e1s0w1"
  6,  6, "t.l0.tundra_n1e0s0w1"
  6,  7, "t.l0.tundra_n0e0s0w1"
  6,  8, "t.l0.tundra_n1e1s1w0"
  6,  9, "t.l0.tundra_n0e1s1w0"
  6, 10, "t.l0.tundra_n1e0s1w0"
  6, 11, "t.l0.tundra_n0e0s1w0"
  6, 12, "t.l0.tundra_n1e1s0w0"
  6, 13, "t.l0.tundra_n0e1s0w0"
  6, 14, "t.l0.tundra_n1e0s0w0"
  6, 15, "t.l0.tundra_n0e0s0w0"

; Ocean, and whether terrain to north, south, east, west
; is more ocean (else shoreline)

  10,  0, "t.l1.coast_n1e1s1w1"
  10,  1, "t.l1.coast_n0e1s1w1"
  10,  2, "t.l1.coast_n1e0s1w1"
  10,  3, "t.l1.coast_n0e0s1w1"
  10,  4, "t.l1.coast_n1e1s0w1"
  10,  5, "t.l1.coast_n0e1s0w1"
  10,  6, "t.l1.coast_n1e0s0w1"
  10,  7, "t.l1.coast_n0e0s0w1"
  10,  8, "t.l1.coast_n1e1s1w0"
  10,  9, "t.l1.coast_n0e1s1w0"
  10, 10, "t.l1.coast_n1e0s1w0"
  10, 11, "t.l1.coast_n0e0s1w0"
  10, 12, "t.l1.coast_n1e1s0w0"
  10, 13, "t.l1.coast_n0e1s0w0"
  10, 14, "t.l1.coast_n1e0s0w0"
  10, 15, "t.l1.coast_n0e0s0w0"

  10,  0, "t.l1.floor_n1e1s1w1"
  10,  1, "t.l1.floor_n0e1s1w1"
  10,  2, "t.l1.floor_n1e0s1w1"
  10,  3, "t.l1.floor_n0e0s1w1"
  10,  4, "t.l1.floor_n1e1s0w1"
  10,  5, "t.l1.floor_n0e1s0w1"
  10,  6, "t.l1.floor_n1e0s0w1"
  10,  7, "t.l1.floor_n0e0s0w1"
  10,  8, "t.l1.floor_n1e1s1w0"
  10,  9, "t.l1.floor_n0e1s1w0"
  10, 10, "t.l1.floor_n1e0s1w0"
  10, 11, "t.l1.floor_n0e0s1w0"
  10, 12, "t.l1.floor_n1e1s0w0"
  10, 13, "t.l1.floor_n0e1s0w0"
  10, 14, "t.l1.floor_n1e0s0w0"
  10, 15, "t.l1.floor_n0e0s0w0"

  10,  0, "t.l1.lake_n1e1s1w1"
  10,  1, "t.l1.lake_n0e1s1w1"
  10,  2, "t.l1.lake_n1e0s1w1"
  10,  3, "t.l1.lake_n0e0s1w1"
  10,  4, "t.l1.lake_n1e1s0w1"
  10,  5, "t.l1.lake_n0e1s0w1"
  10,  6, "t.l1.lake_n1e0s0w1"
  10,  7, "t.l1.lake_n0e0s0w1"
  10,  8, "t.l1.lake_n1e1s1w0"
  10,  9, "t.l1.lake_n0e1s1w0"
  10, 10, "t.l1.lake_n1e0s1w0"
  10, 11, "t.l1.lake_n0e0s1w0"
  10, 12, "t.l1.lake_n1e1s0w0"
  10, 13, "t.l1.lake_n0e1s0w0"
  10, 14, "t.l1.lake_n1e0s0w0"
  10, 15, "t.l1.lake_n0e0s0w0"

  10,  0, "t.l1.inaccessible_n1e1s1w1"
  10,  1, "t.l1.inaccessible_n0e1s1w1"
  10,  2, "t.l1.inaccessible_n1e0s1w1"
  10,  3, "t.l1.inaccessible_n0e0s1w1"
  10,  4, "t.l1.inaccessible_n1e1s0w1"
  10,  5, "t.l1.inaccessible_n0e1s0w1"
  10,  6, "t.l1.inaccessible_n1e0s0w1"
  10,  7, "t.l1.inaccessible_n0e0s0w1"
  10,  8, "t.l1.inaccessible_n1e1s1w0"
  10,  9, "t.l1.inaccessible_n0e1s1w0"
  10, 10, "t.l1.inaccessible_n1e0s1w0"
  10, 11, "t.l1.inaccessible_n0e0s1w0"
  10, 12, "t.l1.inaccessible_n1e1s0w0"
  10, 13, "t.l1.inaccessible_n0e1s0w0"
  10, 14, "t.l1.inaccessible_n1e0s0w0"
  10, 15, "t.l1.inaccessible_n0e0s0w0"

; Ice Shelves

  11,  0, "t.l2.coast_n1e1s1w1"
  11,  1, "t.l2.coast_n0e1s1w1"
  11,  2, "t.l2.coast_n1e0s1w1"
  11,  3, "t.l2.coast_n0e0s1w1"
  11,  4, "t.l2.coast_n1e1s0w1"
  11,  5, "t.l2.coast_n0e1s0w1"
  11,  6, "t.l2.coast_n1e0s0w1"
  11,  7, "t.l2.coast_n0e0s0w1"
  11,  8, "t.l2.coast_n1e1s1w0"
  11,  9, "t.l2.coast_n0e1s1w0"
  11, 10, "t.l2.coast_n1e0s1w0"
  11, 11, "t.l2.coast_n0e0s1w0"
  11, 12, "t.l2.coast_n1e1s0w0"
  11, 13, "t.l2.coast_n0e1s0w0"
  11, 14, "t.l2.coast_n1e0s0w0"
  11, 15, "t.l2.coast_n0e0s0w0"


  11,  0, "t.l2.floor_n1e1s1w1"
  11,  1, "t.l2.floor_n0e1s1w1"
  11,  2, "t.l2.floor_n1e0s1w1"
  11,  3, "t.l2.floor_n0e0s1w1"
  11,  4, "t.l2.floor_n1e1s0w1"
  11,  5, "t.l2.floor_n0e1s0w1"
  11,  6, "t.l2.floor_n1e0s0w1"
  11,  7, "t.l2.floor_n0e0s0w1"
  11,  8, "t.l2.floor_n1e1s1w0"
  11,  9, "t.l2.floor_n0e1s1w0"
  11, 10, "t.l2.floor_n1e0s1w0"
  11, 11, "t.l2.floor_n0e0s1w0"
  11, 12, "t.l2.floor_n1e1s0w0"
  11, 13, "t.l2.floor_n0e1s0w0"
  11, 14, "t.l2.floor_n1e0s0w0"
  11, 15, "t.l2.floor_n0e0s0w0"


  11,  0, "t.l2.lake_n1e1s1w1"
  11,  1, "t.l2.lake_n0e1s1w1"
  11,  2, "t.l2.lake_n1e0s1w1"
  11,  3, "t.l2.lake_n0e0s1w1"
  11,  4, "t.l2.lake_n1e1s0w1"
  11,  5, "t.l2.lake_n0e1s0w1"
  11,  6, "t.l2.lake_n1e0s0w1"
  11,  7, "t.l2.lake_n0e0s0w1"
  11,  8, "t.l2.lake_n1e1s1w0"
  11,  9, "t.l2.lake_n0e1s1w0"
  11, 10, "t.l2.lake_n1e0s1w0"
  11, 11, "t.l2.lake_n0e0s1w0"
  11, 12, "t.l2.lake_n1e1s0w0"
  11, 13, "t.l2.lake_n0e1s0w0"
  11, 14, "t.l2.lake_n1e0s0w0"
  11, 15, "t.l2.lake_n0e0s0w0"


  11,  0, "t.l2.inaccessible_n1e1s1w1"
  11,  1, "t.l2.inaccessible_n0e1s1w1"
  11,  2, "t.l2.inaccessible_n1e0s1w1"
  11,  3, "t.l2.inaccessible_n0e0s1w1"
  11,  4, "t.l2.inaccessible_n1e1s0w1"
  11,  5, "t.l2.inaccessible_n0e1s0w1"
  11,  6, "t.l2.inaccessible_n1e0s0w1"
  11,  7, "t.l2.inaccessible_n0e0s0w1"
  11,  8, "t.l2.inaccessible_n1e1s1w0"
  11,  9, "t.l2.inaccessible_n0e1s1w0"
  11, 10, "t.l2.inaccessible_n1e0s1w0"
  11, 11, "t.l2.inaccessible_n0e0s1w0"
  11, 12, "t.l2.inaccessible_n1e1s0w0"
  11, 13, "t.l2.inaccessible_n0e1s0w0"
  11, 14, "t.l2.inaccessible_n1e0s0w0"
  11, 15, "t.l2.inaccessible_n0e0s0w0"

; Darkness (unexplored) to north, south, east, west

 12,  0, "mask.tile"
 12,  1, "tx.darkness_n1e0s0w0"
 12,  2, "tx.darkness_n0e1s0w0"
 12,  3, "tx.darkness_n1e1s0w0"
 12,  4, "tx.darkness_n0e0s1w0"
 12,  5, "tx.darkness_n1e0s1w0"
 12,  6, "tx.darkness_n0e1s1w0"
 12,  7, "tx.darkness_n1e1s1w0"
 12,  8, "tx.darkness_n0e0s0w1"
 12,  9, "tx.darkness_n1e0s0w1"
 12, 10, "tx.darkness_n0e1s0w1"
 12, 11, "tx.darkness_n1e1s0w1"
 12, 12, "tx.darkness_n0e0s1w1"
 12, 13, "tx.darkness_n1e0s1w1"
 12, 14, "tx.darkness_n0e1s1w1"
 12, 15, "tx.darkness_n1e1s1w1"
 14, 5, "tx.fog"


; Rivers (as special type), and whether north, south, east, west
; also has river or is ocean:

 13,  0, "road.river_s_n0e0s0w0"
 13,  1, "road.river_s_n1e0s0w0"
 13,  2, "road.river_s_n0e1s0w0"
 13,  3, "road.river_s_n1e1s0w0"
 13,  4, "road.river_s_n0e0s1w0"
 13,  5, "road.river_s_n1e0s1w0"
 13,  6, "road.river_s_n0e1s1w0"
 13,  7, "road.river_s_n1e1s1w0"
 13,  8, "road.river_s_n0e0s0w1"
 13,  9, "road.river_s_n1e0s0w1"
 13, 10, "road.river_s_n0e1s0w1"
 13, 11, "road.river_s_n1e1s0w1"
 13, 12, "road.river_s_n0e0s1w1"
 13, 13, "road.river_s_n1e0s1w1"
 13, 14, "road.river_s_n0e1s1w1"
 13, 15, "road.river_s_n1e1s1w1"

; River outlets, river to north, south, east, west

  14, 0, "road.river_outlet_n"
  14, 1, "road.river_outlet_w"
  14, 2, "road.river_outlet_s"
  14, 3, "road.river_outlet_e"
}

[grid_ocean]
x_top_left = 0
y_top_left = 210
dx = 15
dy = 15

tiles = {"row", "column", "tag"

; coast cell sprites.  See doc/README.graphics

  0,  0, "t.l0.coast_cell_u000"
  0,  0, "t.l0.coast_cell_u001"
  0,  0, "t.l0.coast_cell_u100"
  0,  0, "t.l0.coast_cell_u101"

  0,  1, "t.l0.coast_cell_u010"
  0,  1, "t.l0.coast_cell_u011"
  0,  1, "t.l0.coast_cell_u110"
  0,  1, "t.l0.coast_cell_u111"

  0,  2, "t.l0.coast_cell_r000"
  0,  2, "t.l0.coast_cell_r001"
  0,  2, "t.l0.coast_cell_r100"
  0,  2, "t.l0.coast_cell_r101"

  0,  3, "t.l0.coast_cell_r010"
  0,  3, "t.l0.coast_cell_r011"
  0,  3, "t.l0.coast_cell_r110"
  0,  3, "t.l0.coast_cell_r111"

  0,  4, "t.l0.coast_cell_l000"
  0,  4, "t.l0.coast_cell_l001"
  0,  4, "t.l0.coast_cell_l100"
  0,  4, "t.l0.coast_cell_l101"

  0,  5, "t.l0.coast_cell_l010"
  0,  5, "t.l0.coast_cell_l011"
  0,  5, "t.l0.coast_cell_l110"
  0,  5, "t.l0.coast_cell_l111"

  0,  6, "t.l0.coast_cell_d000"
  0,  6, "t.l0.coast_cell_d001"
  0,  6, "t.l0.coast_cell_d100"
  0,  6, "t.l0.coast_cell_d101"

  0,  7, "t.l0.coast_cell_d010"
  0,  7, "t.l0.coast_cell_d011"
  0,  7, "t.l0.coast_cell_d110"
  0,  7, "t.l0.coast_cell_d111"

 ; Deep Ocean fallback to Ocean tiles
  0,  8, "t.l0.floor_cell_u000"
  0,  8, "t.l0.floor_cell_u001"
  0,  8, "t.l0.floor_cell_u100"
  0,  8, "t.l0.floor_cell_u101"

  0,  9, "t.l0.floor_cell_u010"
  0,  9, "t.l0.floor_cell_u011"
  0,  9, "t.l0.floor_cell_u110"
  0,  9, "t.l0.floor_cell_u111"

  0, 10, "t.l0.floor_cell_r000"
  0, 10, "t.l0.floor_cell_r001"
  0, 10, "t.l0.floor_cell_r100"
  0, 10, "t.l0.floor_cell_r101"

  0, 11, "t.l0.floor_cell_r010"
  0, 11, "t.l0.floor_cell_r011"
  0, 11, "t.l0.floor_cell_r110"
  0, 11, "t.l0.floor_cell_r111"

  0, 12, "t.l0.floor_cell_l000"
  0, 12, "t.l0.floor_cell_l001"
  0, 12, "t.l0.floor_cell_l100"
  0, 12, "t.l0.floor_cell_l101"

  0, 13, "t.l0.floor_cell_l010"
  0, 13, "t.l0.floor_cell_l011"
  0, 13, "t.l0.floor_cell_l110"
  0, 13, "t.l0.floor_cell_l111"

  0, 14, "t.l0.floor_cell_d000"
  0, 14, "t.l0.floor_cell_d001"
  0, 14, "t.l0.floor_cell_d100"
  0, 14, "t.l0.floor_cell_d101"

  0, 15, "t.l0.floor_cell_d010"
  0, 15, "t.l0.floor_cell_d011"
  0, 15, "t.l0.floor_cell_d110"
  0, 15, "t.l0.floor_cell_d111"

; Lake fallback to Ocean tiles
  0, 16, "t.l0.lake_cell_u000"
  0, 16, "t.l0.lake_cell_u001"
  0, 16, "t.l0.lake_cell_u100"
  0, 16, "t.l0.lake_cell_u101"

  0, 17, "t.l0.lake_cell_u010"
  0, 17, "t.l0.lake_cell_u011"
  0, 17, "t.l0.lake_cell_u110"
  0, 17, "t.l0.lake_cell_u111"

  0, 18, "t.l0.lake_cell_r000"
  0, 18, "t.l0.lake_cell_r001"
  0, 18, "t.l0.lake_cell_r100"
  0, 18, "t.l0.lake_cell_r101"

  0, 19, "t.l0.lake_cell_r010"
  0, 19, "t.l0.lake_cell_r011"
  0, 19, "t.l0.lake_cell_r110"
  0, 19, "t.l0.lake_cell_r111"

  0, 20, "t.l0.lake_cell_l000"
  0, 20, "t.l0.lake_cell_l001"
  0, 20, "t.l0.lake_cell_l100"
  0, 20, "t.l0.lake_cell_l101"

  0, 21, "t.l0.lake_cell_l010"
  0, 21, "t.l0.lake_cell_l011"
  0, 21, "t.l0.lake_cell_l110"
  0, 21, "t.l0.lake_cell_l111"

  0, 22, "t.l0.lake_cell_d000"
  0, 22, "t.l0.lake_cell_d001"
  0, 22, "t.l0.lake_cell_d100"
  0, 22, "t.l0.lake_cell_d101"

  0, 23, "t.l0.lake_cell_d010"
  0, 23, "t.l0.lake_cell_d011"
  0, 23, "t.l0.lake_cell_d110"
  0, 23, "t.l0.lake_cell_d111"

; Inaccessible fallback to Ocean tiles
  0, 24, "t.l0.inaccessible_cell_u000"
  0, 24, "t.l0.inaccessible_cell_u001"
  0, 24, "t.l0.inaccessible_cell_u100"
  0, 24, "t.l0.inaccessible_cell_u101"

  0, 25, "t.l0.inaccessible_cell_u010"
  0, 25, "t.l0.inaccessible_cell_u011"
  0, 25, "t.l0.inaccessible_cell_u110"
  0, 25, "t.l0.inaccessible_cell_u111"

  0, 26, "t.l0.inaccessible_cell_r000"
  0, 26, "t.l0.inaccessible_cell_r001"
  0, 26, "t.l0.inaccessible_cell_r100"
  0, 26, "t.l0.inaccessible_cell_r101"

  0, 27, "t.l0.inaccessible_cell_r010"
  0, 27, "t.l0.inaccessible_cell_r011"
  0, 27, "t.l0.inaccessible_cell_r110"
  0, 27, "t.l0.inaccessible_cell_r111"

  0, 28, "t.l0.inaccessible_cell_l000"
  0, 28, "t.l0.inaccessible_cell_l001"
  0, 28, "t.l0.inaccessible_cell_l100"
  0, 28, "t.l0.inaccessible_cell_l101"

  0, 29, "t.l0.inaccessible_cell_l010"
  0, 29, "t.l0.inaccessible_cell_l011"
  0, 29, "t.l0.inaccessible_cell_l110"
  0, 29, "t.l0.inaccessible_cell_l111"

  0, 30, "t.l0.inaccessible_cell_d000"
  0, 30, "t.l0.inaccessible_cell_d001"
  0, 30, "t.l0.inaccessible_cell_d100"
  0, 30, "t.l0.inaccessible_cell_d101"

  0, 31, "t.l0.inaccessible_cell_d010"
  0, 31, "t.l0.inaccessible_cell_d011"
  0, 31, "t.l0.inaccessible_cell_d110"
  0, 31, "t.l0.inaccessible_cell_d111"

}
