
[spec]

; Format and options of this spec file:
options = "+Freeciv-spec-Devel-2019-Jul-03 options"

[info]

; "Karapooze" -- The unit set based on "Cimpletoon" collection of units by YD.

artists = "
    YD
"

[file]
gfx = "cimpletoon/orient_units"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 48
pixel_border = 1

tiles = { "row", "column", "option", "tag"
  0,  0, "cimpletoon", "u.settlers_sw"
  0,  1, "cimpletoon", "u.settlers_w"
  0,  2, "cimpletoon", "u.settlers_nw"
  0,  3, "cimpletoon", "u.settlers_n"
  0,  4, "cimpletoon", "u.settlers_ne"
  0,  5, "cimpletoon", "u.settlers_e"
  0,  6, "cimpletoon", "u.settlers_se"
  0,  7, "cimpletoon", "u.settlers_s"

  1,  0, "cimpletoon", "u.warriors_sw"
  1,  1, "cimpletoon", "u.warriors_w"
  1,  2, "cimpletoon", "u.warriors_nw"
  1,  3, "cimpletoon", "u.warriors_n"
  1,  4, "cimpletoon", "u.warriors_ne"
  1,  5, "cimpletoon", "u.warriors_e"
  1,  6, "cimpletoon", "u.warriors_se"
  1,  7, "cimpletoon", "u.warriors_s"

  2,  0, "cimpletoon", "u.explorer_sw"
  2,  1, "cimpletoon", "u.explorer_w"
  2,  2, "cimpletoon", "u.explorer_nw"
  2,  3, "cimpletoon", "u.explorer_n"
  2,  4, "cimpletoon", "u.explorer_ne"
  2,  5, "cimpletoon", "u.explorer_e"
  2,  6, "cimpletoon", "u.explorer_se"
  2,  7, "cimpletoon", "u.explorer_s"

  3,  0, "cimpletoon", "u.worker_sw"
  3,  1, "cimpletoon", "u.worker_w"
  3,  2, "cimpletoon", "u.worker_nw"
  3,  3, "cimpletoon", "u.worker_n"
  3,  4, "cimpletoon", "u.worker_ne"
  3,  5, "cimpletoon", "u.worker_e"
  3,  6, "cimpletoon", "u.worker_se"
  3,  7, "cimpletoon", "u.worker_s"

  4,  0, "cimpletoon", "u.horsemen_sw"
  4,  1, "cimpletoon", "u.horsemen_w"
  4,  2, "cimpletoon", "u.horsemen_nw"
  4,  3, "cimpletoon", "u.horsemen_n"
  4,  4, "cimpletoon", "u.horsemen_ne"
  4,  5, "cimpletoon", "u.horsemen_e"
  4,  6, "cimpletoon", "u.horsemen_se"
  4,  7, "cimpletoon", "u.horsemen_s"

  5,  0, "cimpletoon", "u.archers_sw"
  5,  1, "cimpletoon", "u.archers_w"
  5,  2, "cimpletoon", "u.archers_nw"
  5,  3, "cimpletoon", "u.archers_n"
  5,  4, "cimpletoon", "u.archers_ne"
  5,  5, "cimpletoon", "u.archers_e"
  5,  6, "cimpletoon", "u.archers_se"
  5,  7, "cimpletoon", "u.archers_s"

  6,  0, "cimpletoon", "u.phalanx_sw"
  6,  1, "cimpletoon", "u.phalanx_w"
  6,  2, "cimpletoon", "u.phalanx_nw"
  6,  3, "cimpletoon", "u.phalanx_n"
  6,  4, "cimpletoon", "u.phalanx_ne"
  6,  5, "cimpletoon", "u.phalanx_e"
  6,  6, "cimpletoon", "u.phalanx_se"
  6,  7, "cimpletoon", "u.phalanx_s"

  7,  0, "cimpletoon", "u.trireme_sw"
  7,  1, "cimpletoon", "u.trireme_w"
  7,  2, "cimpletoon", "u.trireme_nw"
  7,  3, "cimpletoon", "u.trireme_n"
  7,  4, "cimpletoon", "u.trireme_ne"
  7,  5, "cimpletoon", "u.trireme_e"
  7,  6, "cimpletoon", "u.trireme_se"
  7,  7, "cimpletoon", "u.trireme_s"

  8,  0, "cimpletoon", "u.chariot_sw"
  8,  1, "cimpletoon", "u.chariot_w"
  8,  2, "cimpletoon", "u.chariot_nw"
  8,  3, "cimpletoon", "u.chariot_n"
  8,  4, "cimpletoon", "u.chariot_ne"
  8,  5, "cimpletoon", "u.chariot_e"
  8,  6, "cimpletoon", "u.chariot_se"
  8,  7, "cimpletoon", "u.chariot_s"

  9,  0, "cimpletoon", "u.catapult_sw"
  9,  1, "cimpletoon", "u.catapult_w"
  9,  2, "cimpletoon", "u.catapult_nw"
  9,  3, "cimpletoon", "u.catapult_n"
  9,  4, "cimpletoon", "u.catapult_ne"
  9,  5, "cimpletoon", "u.catapult_e"
  9,  6, "cimpletoon", "u.catapult_se"
  9,  7, "cimpletoon", "u.catapult_s"

  10,  0, "cimpletoon", "u.legion_sw"
  10,  1, "cimpletoon", "u.legion_w"
  10,  2, "cimpletoon", "u.legion_nw"
  10,  3, "cimpletoon", "u.legion_n"
  10,  4, "cimpletoon", "u.legion_ne"
  10,  5, "cimpletoon", "u.legion_e"
  10,  6, "cimpletoon", "u.legion_se"
  10,  7, "cimpletoon", "u.legion_s"

  11,  0, "cimpletoon", "u.diplomat_sw"
  11,  1, "cimpletoon", "u.diplomat_w"
  11,  2, "cimpletoon", "u.diplomat_nw"
  11,  3, "cimpletoon", "u.diplomat_n"
  11,  4, "cimpletoon", "u.diplomat_ne"
  11,  5, "cimpletoon", "u.diplomat_e"
  11,  6, "cimpletoon", "u.diplomat_se"
  11,  7, "cimpletoon", "u.diplomat_s"

  12,  0, "cimpletoon", "u.caravan_sw"
  12,  1, "cimpletoon", "u.caravan_w"
  12,  2, "cimpletoon", "u.caravan_nw"
  12,  3, "cimpletoon", "u.caravan_n"
  12,  4, "cimpletoon", "u.caravan_ne"
  12,  5, "cimpletoon", "u.caravan_e"
  12,  6, "cimpletoon", "u.caravan_se"
  12,  7, "cimpletoon", "u.caravan_s"

  13,  0, "cimpletoon", "u.pikemen_sw"
  13,  1, "cimpletoon", "u.pikemen_w"
  13,  2, "cimpletoon", "u.pikemen_nw"
  13,  3, "cimpletoon", "u.pikemen_n"
  13,  4, "cimpletoon", "u.pikemen_ne"
  13,  5, "cimpletoon", "u.pikemen_e"
  13,  6, "cimpletoon", "u.pikemen_se"
  13,  7, "cimpletoon", "u.pikemen_s"

  14,  0, "cimpletoon", "u.knights_sw"
  14,  1, "cimpletoon", "u.knights_w"
  14,  2, "cimpletoon", "u.knights_nw"
  14,  3, "cimpletoon", "u.knights_n"
  14,  4, "cimpletoon", "u.knights_ne"
  14,  5, "cimpletoon", "u.knights_e"
  14,  6, "cimpletoon", "u.knights_se"
  14,  7, "cimpletoon", "u.knights_s"

  15,  0, "cimpletoon", "u.caravel_sw"
  15,  1, "cimpletoon", "u.caravel_w"
  15,  2, "cimpletoon", "u.caravel_nw"
  15,  3, "cimpletoon", "u.caravel_n"
  15,  4, "cimpletoon", "u.caravel_ne"
  15,  5, "cimpletoon", "u.caravel_e"
  15,  6, "cimpletoon", "u.caravel_se"
  15,  7, "cimpletoon", "u.caravel_s"

  16,  0, "cimpletoon", "u.galleon_sw"
  16,  1, "cimpletoon", "u.galleon_w"
  16,  2, "cimpletoon", "u.galleon_nw"
  16,  3, "cimpletoon", "u.galleon_n"
  16,  4, "cimpletoon", "u.galleon_ne"
  16,  5, "cimpletoon", "u.galleon_e"
  16,  6, "cimpletoon", "u.galleon_se"
  16,  7, "cimpletoon", "u.galleon_s"

  17,  0, "cimpletoon", "u.frigate_sw"
  17,  1, "cimpletoon", "u.frigate_w"
  17,  2, "cimpletoon", "u.frigate_nw"
  17,  3, "cimpletoon", "u.frigate_n"
  17,  4, "cimpletoon", "u.frigate_ne"
  17,  5, "cimpletoon", "u.frigate_e"
  17,  6, "cimpletoon", "u.frigate_se"
  17,  7, "cimpletoon", "u.frigate_s"

  18,  0, "cimpletoon", "u.ironclad_sw"
  18,  1, "cimpletoon", "u.ironclad_w"
  18,  2, "cimpletoon", "u.ironclad_nw"
  18,  3, "cimpletoon", "u.ironclad_n"
  18,  4, "cimpletoon", "u.ironclad_ne"
  18,  5, "cimpletoon", "u.ironclad_e"
  18,  6, "cimpletoon", "u.ironclad_se"
  18,  7, "cimpletoon", "u.ironclad_s"

  19,  0, "cimpletoon", "u.musketeers_sw"
  19,  1, "cimpletoon", "u.musketeers_w"
  19,  2, "cimpletoon", "u.musketeers_nw"
  19,  3, "cimpletoon", "u.musketeers_n"
  19,  4, "cimpletoon", "u.musketeers_ne"
  19,  5, "cimpletoon", "u.musketeers_e"
  19,  6, "cimpletoon", "u.musketeers_se"
  19,  7, "cimpletoon", "u.musketeers_s"

  20,  0, "cimpletoon", "u.dragoons_sw"
  20,  1, "cimpletoon", "u.dragoons_w"
  20,  2, "cimpletoon", "u.dragoons_nw"
  20,  3, "cimpletoon", "u.dragoons_n"
  20,  4, "cimpletoon", "u.dragoons_ne"
  20,  5, "cimpletoon", "u.dragoons_e"
  20,  6, "cimpletoon", "u.dragoons_se"
  20,  7, "cimpletoon", "u.dragoons_s"

  21,  0, "cimpletoon", "u.cannon_sw"
  21,  1, "cimpletoon", "u.cannon_w"
  21,  2, "cimpletoon", "u.cannon_nw"
  21,  3, "cimpletoon", "u.cannon_n"
  21,  4, "cimpletoon", "u.cannon_ne"
  21,  5, "cimpletoon", "u.cannon_e"
  21,  6, "cimpletoon", "u.cannon_se"
  21,  7, "cimpletoon", "u.cannon_s"

  22,  0, "cimpletoon", "u.engineers_sw"
  22,  1, "cimpletoon", "u.engineers_w"
  22,  2, "cimpletoon", "u.engineers_nw"
  22,  3, "cimpletoon", "u.engineers_n"
  22,  4, "cimpletoon", "u.engineers_ne"
  22,  5, "cimpletoon", "u.engineers_e"
  22,  6, "cimpletoon", "u.engineers_se"
  22,  7, "cimpletoon", "u.engineers_s"

  23,  0, "cimpletoon", "u.transport_sw"
  23,  1, "cimpletoon", "u.transport_w"
  23,  2, "cimpletoon", "u.transport_nw"
  23,  3, "cimpletoon", "u.transport_n"
  23,  4, "cimpletoon", "u.transport_ne"
  23,  5, "cimpletoon", "u.transport_e"
  23,  6, "cimpletoon", "u.transport_se"
  23,  7, "cimpletoon", "u.transport_s"

  24,  0, "cimpletoon", "u.destroyer_sw"
  24,  1, "cimpletoon", "u.destroyer_w"
  24,  2, "cimpletoon", "u.destroyer_nw"
  24,  3, "cimpletoon", "u.destroyer_n"
  24,  4, "cimpletoon", "u.destroyer_ne"
  24,  5, "cimpletoon", "u.destroyer_e"
  24,  6, "cimpletoon", "u.destroyer_se"
  24,  7, "cimpletoon", "u.destroyer_s"

  25,  0, "cimpletoon", "u.riflemen_sw"
  25,  1, "cimpletoon", "u.riflemen_w"
  25,  2, "cimpletoon", "u.riflemen_nw"
  25,  3, "cimpletoon", "u.riflemen_n"
  25,  4, "cimpletoon", "u.riflemen_ne"
  25,  5, "cimpletoon", "u.riflemen_e"
  25,  6, "cimpletoon", "u.riflemen_se"
  25,  7, "cimpletoon", "u.riflemen_s"

  26,  0, "cimpletoon", "u.cavalry_sw"
  26,  1, "cimpletoon", "u.cavalry_w"
  26,  2, "cimpletoon", "u.cavalry_nw"
  26,  3, "cimpletoon", "u.cavalry_n"
  26,  4, "cimpletoon", "u.cavalry_ne"
  26,  5, "cimpletoon", "u.cavalry_e"
  26,  6, "cimpletoon", "u.cavalry_se"
  26,  7, "cimpletoon", "u.cavalry_s"

  27,  0, "cimpletoon", "u.alpine_troops_sw"
  27,  1, "cimpletoon", "u.alpine_troops_w"
  27,  2, "cimpletoon", "u.alpine_troops_nw"
  27,  3, "cimpletoon", "u.alpine_troops_n"
  27,  4, "cimpletoon", "u.alpine_troops_ne"
  27,  5, "cimpletoon", "u.alpine_troops_e"
  27,  6, "cimpletoon", "u.alpine_troops_se"
  27,  7, "cimpletoon", "u.alpine_troops_s"

  28,  0, "cimpletoon", "u.freight_sw"
  28,  1, "cimpletoon", "u.freight_w"
  28,  2, "cimpletoon", "u.freight_nw"
  28,  3, "cimpletoon", "u.freight_n"
  28,  4, "cimpletoon", "u.freight_ne"
  28,  5, "cimpletoon", "u.freight_e"
  28,  6, "cimpletoon", "u.freight_se"
  28,  7, "cimpletoon", "u.freight_s"

  29,  0, "cimpletoon", "u.spy_sw"
  29,  1, "cimpletoon", "u.spy_w"
  29,  2, "cimpletoon", "u.spy_nw"
  29,  3, "cimpletoon", "u.spy_n"
  29,  4, "cimpletoon", "u.spy_ne"
  29,  5, "cimpletoon", "u.spy_e"
  29,  6, "cimpletoon", "u.spy_se"
  29,  7, "cimpletoon", "u.spy_s"

  30,  0, "cimpletoon", "u.cruiser_sw"
  30,  1, "cimpletoon", "u.cruiser_w"
  30,  2, "cimpletoon", "u.cruiser_nw"
  30,  3, "cimpletoon", "u.cruiser_n"
  30,  4, "cimpletoon", "u.cruiser_ne"
  30,  5, "cimpletoon", "u.cruiser_e"
  30,  6, "cimpletoon", "u.cruiser_se"
  30,  7, "cimpletoon", "u.cruiser_s"

  31,  0, "cimpletoon", "u.battleship_sw"
  31,  1, "cimpletoon", "u.battleship_w"
  31,  2, "cimpletoon", "u.battleship_nw"
  31,  3, "cimpletoon", "u.battleship_n"
  31,  4, "cimpletoon", "u.battleship_ne"
  31,  5, "cimpletoon", "u.battleship_e"
  31,  6, "cimpletoon", "u.battleship_se"
  31,  7, "cimpletoon", "u.battleship_s"

  32,  0, "cimpletoon", "u.submarine_sw"
  32,  1, "cimpletoon", "u.submarine_w"
  32,  2, "cimpletoon", "u.submarine_nw"
  32,  3, "cimpletoon", "u.submarine_n"
  32,  4, "cimpletoon", "u.submarine_ne"
  32,  5, "cimpletoon", "u.submarine_e"
  32,  6, "cimpletoon", "u.submarine_se"
  32,  7, "cimpletoon", "u.submarine_s"

  33,  0, "cimpletoon", "u.marines_sw"
  33,  1, "cimpletoon", "u.marines_w"
  33,  2, "cimpletoon", "u.marines_nw"
  33,  3, "cimpletoon", "u.marines_n"
  33,  4, "cimpletoon", "u.marines_ne"
  33,  5, "cimpletoon", "u.marines_e"
  33,  6, "cimpletoon", "u.marines_se"
  33,  7, "cimpletoon", "u.marines_s"

  34,  0, "cimpletoon", "u.partisan_sw"
  34,  1, "cimpletoon", "u.partisan_w"
  34,  2, "cimpletoon", "u.partisan_nw"
  34,  3, "cimpletoon", "u.partisan_n"
  34,  4, "cimpletoon", "u.partisan_ne"
  34,  5, "cimpletoon", "u.partisan_e"
  34,  6, "cimpletoon", "u.partisan_se"
  34,  7, "cimpletoon", "u.partisan_s"

  35,  0, "cimpletoon", "u.artillery_sw"
  35,  1, "cimpletoon", "u.artillery_w"
  35,  2, "cimpletoon", "u.artillery_nw"
  35,  3, "cimpletoon", "u.artillery_n"
  35,  4, "cimpletoon", "u.artillery_ne"
  35,  5, "cimpletoon", "u.artillery_e"
  35,  6, "cimpletoon", "u.artillery_se"
  35,  7, "cimpletoon", "u.artillery_s"

  36,  0, "cimpletoon", "u.fighter_sw"
  36,  1, "cimpletoon", "u.fighter_w"
  36,  2, "cimpletoon", "u.fighter_nw"
  36,  3, "cimpletoon", "u.fighter_n"
  36,  4, "cimpletoon", "u.fighter_ne"
  36,  5, "cimpletoon", "u.fighter_e"
  36,  6, "cimpletoon", "u.fighter_se"
  36,  7, "cimpletoon", "u.fighter_s"

  37,  0, "cimpletoon", "u.aegis_cruiser_sw"
  37,  1, "cimpletoon", "u.aegis_cruiser_w"
  37,  2, "cimpletoon", "u.aegis_cruiser_nw"
  37,  3, "cimpletoon", "u.aegis_cruiser_n"
  37,  4, "cimpletoon", "u.aegis_cruiser_ne"
  37,  5, "cimpletoon", "u.aegis_cruiser_e"
  37,  6, "cimpletoon", "u.aegis_cruiser_se"
  37,  7, "cimpletoon", "u.aegis_cruiser_s"

  38,  0, "cimpletoon", "u.carrier_sw"
  38,  1, "cimpletoon", "u.carrier_w"
  38,  2, "cimpletoon", "u.carrier_nw"
  38,  3, "cimpletoon", "u.carrier_n"
  38,  4, "cimpletoon", "u.carrier_ne"
  38,  5, "cimpletoon", "u.carrier_e"
  38,  6, "cimpletoon", "u.carrier_se"
  38,  7, "cimpletoon", "u.carrier_s"

  39,  0, "cimpletoon", "u.armor_sw"
  39,  1, "cimpletoon", "u.armor_w"
  39,  2, "cimpletoon", "u.armor_nw"
  39,  3, "cimpletoon", "u.armor_n"
  39,  4, "cimpletoon", "u.armor_ne"
  39,  5, "cimpletoon", "u.armor_e"
  39,  6, "cimpletoon", "u.armor_se"
  39,  7, "cimpletoon", "u.armor_s"

  40,  0, "cimpletoon", "u.mech_inf_sw"
  40,  1, "cimpletoon", "u.mech_inf_w"
  40,  2, "cimpletoon", "u.mech_inf_nw"
  40,  3, "cimpletoon", "u.mech_inf_n"
  40,  4, "cimpletoon", "u.mech_inf_ne"
  40,  5, "cimpletoon", "u.mech_inf_e"
  40,  6, "cimpletoon", "u.mech_inf_se"
  40,  7, "cimpletoon", "u.mech_inf_s"

  41,  0, "cimpletoon", "u.howitzer_sw"
  41,  1, "cimpletoon", "u.howitzer_w"
  41,  2, "cimpletoon", "u.howitzer_nw"
  41,  3, "cimpletoon", "u.howitzer_n"
  41,  4, "cimpletoon", "u.howitzer_ne"
  41,  5, "cimpletoon", "u.howitzer_e"
  41,  6, "cimpletoon", "u.howitzer_se"
  41,  7, "cimpletoon", "u.howitzer_s"

  42,  0, "cimpletoon", "u.paratroopers_sw"
  42,  1, "cimpletoon", "u.paratroopers_w"
  42,  2, "cimpletoon", "u.paratroopers_nw"
  42,  3, "cimpletoon", "u.paratroopers_n"
  42,  4, "cimpletoon", "u.paratroopers_ne"
  42,  5, "cimpletoon", "u.paratroopers_e"
  42,  6, "cimpletoon", "u.paratroopers_se"
  42,  7, "cimpletoon", "u.paratroopers_s"

  43,  0, "cimpletoon", "u.helicopter_sw"
  43,  1, "cimpletoon", "u.helicopter_w"
  43,  2, "cimpletoon", "u.helicopter_nw"
  43,  3, "cimpletoon", "u.helicopter_n"
  43,  4, "cimpletoon", "u.helicopter_ne"
  43,  5, "cimpletoon", "u.helicopter_e"
  43,  6, "cimpletoon", "u.helicopter_se"
  43,  7, "cimpletoon", "u.helicopter_s"

  44,  0, "cimpletoon", "u.bomber_sw"
  44,  1, "cimpletoon", "u.bomber_w"
  44,  2, "cimpletoon", "u.bomber_nw"
  44,  3, "cimpletoon", "u.bomber_n"
  44,  4, "cimpletoon", "u.bomber_ne"
  44,  5, "cimpletoon", "u.bomber_e"
  44,  6, "cimpletoon", "u.bomber_se"
  44,  7, "cimpletoon", "u.bomber_s"

  45,  0, "cimpletoon", "u.awacs_sw"
  45,  1, "cimpletoon", "u.awacs_w"
  45,  2, "cimpletoon", "u.awacs_nw"
  45,  3, "cimpletoon", "u.awacs_n"
  45,  4, "cimpletoon", "u.awacs_ne"
  45,  5, "cimpletoon", "u.awacs_e"
  45,  6, "cimpletoon", "u.awacs_se"
  45,  7, "cimpletoon", "u.awacs_s"

  46,  0, "cimpletoon", "u.nuclear_sw"
  46,  1, "cimpletoon", "u.nuclear_w"
  46,  2, "cimpletoon", "u.nuclear_nw"
  46,  3, "cimpletoon", "u.nuclear_n"
  46,  4, "cimpletoon", "u.nuclear_ne"
  46,  5, "cimpletoon", "u.nuclear_e"
  46,  6, "cimpletoon", "u.nuclear_se"
  46,  7, "cimpletoon", "u.nuclear_s"

  47,  0, "cimpletoon", "u.cruise_missile_sw"
  47,  1, "cimpletoon", "u.cruise_missile_w"
  47,  2, "cimpletoon", "u.cruise_missile_nw"
  47,  3, "cimpletoon", "u.cruise_missile_n"
  47,  4, "cimpletoon", "u.cruise_missile_ne"
  47,  5, "cimpletoon", "u.cruise_missile_e"
  47,  6, "cimpletoon", "u.cruise_missile_se"
  47,  7, "cimpletoon", "u.cruise_missile_s"

  48,  0, "cimpletoon", "u.stealth_bomber_sw"
  48,  1, "cimpletoon", "u.stealth_bomber_w"
  48,  2, "cimpletoon", "u.stealth_bomber_nw"
  48,  3, "cimpletoon", "u.stealth_bomber_n"
  48,  4, "cimpletoon", "u.stealth_bomber_ne"
  48,  5, "cimpletoon", "u.stealth_bomber_e"
  48,  6, "cimpletoon", "u.stealth_bomber_se"
  48,  7, "cimpletoon", "u.stealth_bomber_s"

  49,  0, "cimpletoon", "u.stealth_fighter_sw"
  49,  1, "cimpletoon", "u.stealth_fighter_w"
  49,  2, "cimpletoon", "u.stealth_fighter_nw"
  49,  3, "cimpletoon", "u.stealth_fighter_n"
  49,  4, "cimpletoon", "u.stealth_fighter_ne"
  49,  5, "cimpletoon", "u.stealth_fighter_e"
  49,  6, "cimpletoon", "u.stealth_fighter_se"
  49,  7, "cimpletoon", "u.stealth_fighter_s"

  50,  0, "cimpletoon", "u.leader_sw"
  50,  1, "cimpletoon", "u.leader_w"
  50,  2, "cimpletoon", "u.leader_nw"
  50,  3, "cimpletoon", "u.leader_n"
  50,  4, "cimpletoon", "u.leader_ne"
  50,  5, "cimpletoon", "u.leader_e"
  50,  6, "cimpletoon", "u.leader_se"
  50,  7, "cimpletoon", "u.leader_s"

  51,  0, "cimpletoon", "u.barbarian_leader_sw"
  51,  1, "cimpletoon", "u.barbarian_leader_w"
  51,  2, "cimpletoon", "u.barbarian_leader_nw"
  51,  3, "cimpletoon", "u.barbarian_leader_n"
  51,  4, "cimpletoon", "u.barbarian_leader_ne"
  51,  5, "cimpletoon", "u.barbarian_leader_e"
  51,  6, "cimpletoon", "u.barbarian_leader_se"
  51,  7, "cimpletoon", "u.barbarian_leader_s"

  52,  0, "cimpletoon", "u.fanatics_sw"
  52,  1, "cimpletoon", "u.fanatics_w"
  52,  2, "cimpletoon", "u.fanatics_nw"
  52,  3, "cimpletoon", "u.fanatics_n"
  52,  4, "cimpletoon", "u.fanatics_ne"
  52,  5, "cimpletoon", "u.fanatics_e"
  52,  6, "cimpletoon", "u.fanatics_se"
  52,  7, "cimpletoon", "u.fanatics_s"

  53,  0, "cimpletoon", "u.crusaders_sw"
  53,  1, "cimpletoon", "u.crusaders_w"
  53,  2, "cimpletoon", "u.crusaders_nw"
  53,  3, "cimpletoon", "u.crusaders_n"
  53,  4, "cimpletoon", "u.crusaders_ne"
  53,  5, "cimpletoon", "u.crusaders_e"
  53,  6, "cimpletoon", "u.crusaders_se"
  53,  7, "cimpletoon", "u.crusaders_s"

  54,  0, "cimpletoon", "u.elephants_sw"
  54,  1, "cimpletoon", "u.elephants_w"
  54,  2, "cimpletoon", "u.elephants_nw"
  54,  3, "cimpletoon", "u.elephants_n"
  54,  4, "cimpletoon", "u.elephants_ne"
  54,  5, "cimpletoon", "u.elephants_e"
  54,  6, "cimpletoon", "u.elephants_se"
  54,  7, "cimpletoon", "u.elephants_s"

  55,  0, "cimpletoon", "u.migrants_sw"
  55,  1, "cimpletoon", "u.migrants_w"
  55,  2, "cimpletoon", "u.migrants_nw"
  55,  3, "cimpletoon", "u.migrants_n"
  55,  4, "cimpletoon", "u.migrants_ne"
  55,  5, "cimpletoon", "u.migrants_e"
  55,  6, "cimpletoon", "u.migrants_se"
  55,  7, "cimpletoon", "u.migrants_s"

}
