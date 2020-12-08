[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]
;	Alternate version of Yield-Pip Graphics showing total food instead of calculated surplus.

artists = "
	Hans Lemurson
"

[file]
gfx = "roundSquare/yields_f"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

; Pip graphics representing the Food/Production/Trade of tiles.

 0,  0, "city.t_food_0"
 0,  1, "city.t_food_1"
 0,  2, "city.t_food_2"
 0,  3, "city.t_food_3"
 0,  4, "city.t_food_4"
 0,  5, "city.t_food_5"
 0,  6, "city.t_food_6"
 0,  7, "city.t_food_7"
 0,  8, "city.t_food_8"
 0,  9, "city.t_food_9"

 1,  0, "city.t_shields_0"
 1,  1, "city.t_shields_1"
 1,  2, "city.t_shields_2"
 1,  3, "city.t_shields_3"
 1,  4, "city.t_shields_4"
 1,  5, "city.t_shields_5"
 1,  6, "city.t_shields_6"
 1,  7, "city.t_shields_7"
 1,  8, "city.t_shields_8"
 1,  9, "city.t_shields_9"

 2,  0, "city.t_trade_0"
 2,  1, "city.t_trade_1"
 2,  2, "city.t_trade_2"
 2,  3, "city.t_trade_3"
 2,  4, "city.t_trade_4"
 2,  5, "city.t_trade_5"
 2,  6, "city.t_trade_6"
 2,  7, "city.t_trade_7"
 2,  8, "city.t_trade_8"
 2,  9, "city.t_trade_9"
 } ;end tiles

