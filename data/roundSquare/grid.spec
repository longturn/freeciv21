
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
    Jason Dorje Short <jdorje@freeciv.org>
	Hans Lemurson
"

[file]
gfx = "roundSquare/grid"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 30
dy = 30
pixel_border = 1

tiles = { "row", "column", "tag"
  0, 0, "grid.main.we"	;[HL]
  1, 0, "grid.main.ns"	;[HL]
  0, 1, "grid.city.we"	;[HL]
  1, 1, "grid.city.ns"	;[HL]
  0, 2, "grid.worked.we"	;[HL]
  1, 2, "grid.worked.ns"	;[HL]
  0, 3, "grid.unavailable"	;[HL]
  1, 3, "grid.nonnative"
  0, 4, "grid.selected.we"
  1, 4, "grid.selected.ns"
  0, 5, "grid.coastline.we"
  1, 5, "grid.coastline.ns"

  2, 0, "grid.borders.n"	;[HL]
  2, 1, "grid.borders.s"	;[HL]
  2, 2, "grid.borders.w"	;[HL]
  2, 3, "grid.borders.e"	;[HL]
}
