
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03"

[info]

artists = "
    GriffonSpade
    SCL
    Davide Pagnin <nightmare@freeciv.it> (angry citizens) ;copied from small.spec
    Caedo (recolored backgrounds)
"

[file]
gfx = "misc/specialists"

[grid_main]

x_top_left = 0
y_top_left = 0
dx = 15
dy = 20

tiles = { "row", "column", "tag"
; Citizen icons:
  0, 0, "citizen.happy_0"
  0, 1, "citizen.happy_1"
  0, 2, "citizen.content_0"
  0, 3, "citizen.content_1"
  0, 4, "citizen.unhappy_0"
  0, 5, "citizen.unhappy_1"
  0, 6, "citizen.angry_0"
  0, 7, "citizen.angry_1"

; Specialist icons
  1,  0, "specialist.elvis_0"
  1,  1, "specialist.elvis_1"
  1,  0, "specialist.entertainer_0"
  1,  1, "specialist.entertainer_1"
  1,  2, "specialist.scientist_0"
  1,  3, "specialist.scientist_1"
  1,  4, "specialist.taxman_0"
  1,  5, "specialist.taxman_1"
  1,  6, "specialist.worker_0"
  1,  7, "specialist.worker_1"
  1,  8, "specialist.farmer_0"
  1,  9, "specialist.farmer_1"
  1, 10, "specialist.merchant_0"
  1, 11, "specialist.merchant_1"
}
