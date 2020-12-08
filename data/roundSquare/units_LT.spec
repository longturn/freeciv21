[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
	Sketlux (XYZ)
	Hans_Lemurson
"

[file]
gfx = "roundSquare/units_LT"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

  0,  0, "u.longboat" 			;Sketlux
  ;0,  1, "u.caravel"  			;Hans_Lemurson	(this caravel graphic now used in units.png)
  0,  2, "u.flagship_frigate"  	;Hans_Lemurson
  0,  3, "u.barge" 				;unknown
  0,  4, "u.srcaravel"  		;unknown + Hans_Lemurson

  1,  0, "u.tribal_worker"		;Trident + Hans_Lemurson
}

