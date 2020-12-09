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
#ifndef FC__CULTURE_H
#define FC__CULTURE_H


struct city;
struct player;

int city_culture(const struct city *pcity);
int city_history_gain(const struct city *pcity);

int player_culture(const struct player *plr);
int nation_history_gain(const struct player *pplayer);


#endif /* FC__CULTURE_H */
