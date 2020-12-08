
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
     Freim <...>
     Hans Lemurson
"
; Graphics designed to be small yet still recognizable.

[file]
gfx = "roundSquare/citybar"

[grid_big]

x_top_left = 1
y_top_left = 1
pixel_border = 2
dx = 8
dy = 8

tiles = { "row", "column", "tag"

  0,  0, "citybar.shields"	;[HL]
  0,  1, "citybar.food"		;[HL]
  0,  2, "citybar.trade"	;[HL]

}

[grid_star]

x_top_left = 1
y_top_left = 11
pixel_border = 2
dx = 8
dy = 12

tiles = { "row", "column", "tag"

  0,  0, "citybar.occupied"		;[HL]
  0,  1, "citybar.occupancy_0"	;[HL]
  0,  2, "citybar.occupancy_1"	;[HL]
  0,  3, "citybar.occupancy_2"	;[HL]
  0,  4, "citybar.occupancy_3"	;[HL]

}

[grid_bg]
x_top_left = 1
y_top_left = 41
pixel_border = 1
dx = 50
dy = 25

tiles = { "row", "column", "tag"

  0,  0, "citybar.background"

}
