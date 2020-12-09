/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

/* utility */
#include "bitvector.h"
#include "support.h" /* bool type */

/* common */
#include "fc_types.h"

/**********************************************************************
First, some ascii art showing the spaceship and relevant parts,
including numbering of parts:
      2   4   6   8  10  12  14  16  18  20  22  24  26  28
      |   |   |   |   |   |   |   |   |   |   |   |   |   |
   :::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-  s=structural
 2_:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-  C=component
   :::-:::-:::-:::-:::-:::-:::-:::-:::-:::-/C12/  C13 \:::-  M=module
 4_!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-\F6/\  P6  /!!!-
   :::-:::-\++++++/:::-:::-\++++++/:::-[s ][s ][s ]:::-:::-  P=Propulsion
 6_:::-:::-# M8   #:::-:::-# M2   #:::-[24][26][28]:::-:::-  F=Fuel
   :::-:::-# S2   #:::-:::-# S0   #:::-[s ]/C8>/  C9  \:::-
 8_!!!-!!!-/++++++\!!!-!!!-/++++++\!!!-[22]\F4/\  P4  /!!!-  H=Habitation
   :::-:::-[s ][s ][s ][s ][s ][s ][s ][s ]/C4>/  C5  \:::-  L=Life Support
10_:::-:::-[30][20][18][16][14][10][ 6][ 4]\F2/\  P2  /:::-  S=Solar Panels
   :::-/======\/======\/======\/======\[s ][s ][s ]:::-:::-
12_!!!-# M10  ## M6   ## M4   ## M0   #[ 2][ 8][12]!!!-!!!-
   :::-# L3   ## H2   ## L1   ## H0   #[s ]/C0\/  C1  \:::-
14_:::-\======/\======/\======/\======/[ 0]\F0/\  P0  /:::-
   :::-/======\/======\/======\/======\[s ]/C2\/  C3  \:::-
16_!!!-# M9   ## M7   ## M3   ## M1   #[ 1]\F1/\  P1  /!!!-
   :::-# H3   ## L2   ## H1   ## L0   #[s ][s ][s ]:::-:::-
18_:::-\======/\======/\======/\======/[ 3][ 9][13]:::-:::-
   :::-:::-[s ][s ][s ][s ][s ][s ][s ][s ]/C6\/  C7  \:::-
20_!!!-!!!-[31][21][19][17][15][11][ 7][ 5]\F3/\  P3  /!!!-
   :::-:::-\++++++/:::-:::-\++++++/:::-[s ]/C10/  C11 \:::-
22_:::-:::-# M11  #:::-:::-# M5   #:::-[23]\F5/\  P5  /:::-
   :::-:::-# S3   #:::-:::-# S1   #:::-[s ][s ][s ]:::-:::-
24_!!!-!!!-/++++++\!!!-!!!-/++++++\!!!-[25][27][29]!!!-!!!-
   :::-:::-:::-:::-:::-:::-:::-:::-:::-:::-/C14/  C15 \:::-
26_:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-\F7/\  P7  /:::-
   :::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-:::-
28_!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-!!!-

Now, how things work:

Modules and Components: player (if the client is smart enough)
can choose which sort of each to build, but not where they are
built.  That is, first module built can be choice of H,L,S,
but whichever it is, it is H0, L0 or S0.  If you build 4 F and
0 P, they will be F0,F1,F2,F3.

Structural are different: the first s must be s0, but after that
you can in principle build any s which is adjacent (4 ways) to
another s (but in practice the client may make the choice for you).
Because you have to start with s0, this means each s actually
depends on one single other s, so we just note that required s
below, and don't have to calculate adjacency.

Likewise, whether a component or module is "connected" to the
structure depends in each case on just a single structural.
(Actually, F2 and F3 are exceptions, which have a choice
of two (non-dependent) structs to depend on; we choose just
the one which must be there for P2 and P3).

**********************************************************************/

enum spaceship_state {
  SSHIP_NONE,
  SSHIP_STARTED,
  SSHIP_LAUNCHED,
  SSHIP_ARRIVED
};

#define NUM_SS_STRUCTURALS 32 /* Used in the network protocol. */
#define NUM_SS_COMPONENTS 16
#define NUM_SS_MODULES 12

/* Used in the network protocol. */
BV_DEFINE(bv_spaceship_structure, NUM_SS_STRUCTURALS);

struct player_spaceship {
  /* how many of each part built, including any "unplaced": */
  int structurals;
  int components;
  int modules;
  /* which structurals placed: (array of booleans) */
  bv_spaceship_structure structure;
  /* which components and modules placed: (may or may not be connected) */
  int fuel;
  int propulsion;
  int habitation;
  int life_support;
  int solar_panels;
  /* other stuff: */
  enum spaceship_state state;
  int launch_year;
  /* derived quantities: */
  int population;
  int mass;
  double support_rate;
  double energy_rate;
  double success_rate;
  double travel_time;
};

struct sship_part_info {
  int x, y;     /* position of tile centre */
  int required; /* required for struct connection */
};

extern const struct sship_part_info structurals_info[NUM_SS_STRUCTURALS];
extern const struct sship_part_info components_info[NUM_SS_COMPONENTS];
extern const struct sship_part_info modules_info[NUM_SS_MODULES];

void spaceship_init(struct player_spaceship *ship);
int num_spaceship_structurals_placed(const struct player_spaceship *ship);

struct spaceship_component {
  enum spaceship_place_type type;
  int num;
};

bool next_spaceship_component(struct player *pplayer,
                              struct player_spaceship *ship,
                              struct spaceship_component *fill);


