[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
    GriffonSpade
	Hans Lemurson

"

[file]
gfx = "roundSquare/paths"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 30
dy = 30

tiles = { "row", "column", "tag"

  0, 0, "user.attention"	; Variously crosshair/red-square/arrows
; Goto path:

  0, 1, "path.step"            ; turn boundary within path
  0, 2, "path.exhausted_mp"    ; tip of path, no MP left
  0, 3, "path.normal"          ; tip of path with MP remaining
  0, 4, "path.waypoint"

  1, 0, "path.turns_0"
  1, 1, "path.turns_1"
  1, 2, "path.turns_2"
  1, 3, "path.turns_3"
  1, 4, "path.turns_4"
  1, 5, "path.turns_5"
  1, 6, "path.turns_6"
  1, 7, "path.turns_7"
  1, 8, "path.turns_8"
  1, 9, "path.turns_9"

  2, 0, "path.turns_00"
  2, 1, "path.turns_10"
  2, 2, "path.turns_20"
  2, 3, "path.turns_30"
  2, 4, "path.turns_40"
  2, 5, "path.turns_50"
  2, 6, "path.turns_60"
  2, 7, "path.turns_70"
  2, 8, "path.turns_80"
  2, 9, "path.turns_90"

  3, 0, "path.turns_000"
  3, 1, "path.turns_100"
  3, 2, "path.turns_200"
  3, 3, "path.turns_300"
  3, 4, "path.turns_400"
  3, 5, "path.turns_500"
  3, 6, "path.turns_600"
  3, 7, "path.turns_700"
  3, 8, "path.turns_800"
  3, 9, "path.turns_900"

}
