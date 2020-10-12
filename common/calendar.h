/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__CALENDAR_H
#define FC__CALENDAR_H



struct packet_game_info;
void game_next_year(struct packet_game_info *info);
void game_advance_year(void);

const char *textcalfrag(int frag);
const char *textyear(int year);
const char *calendar_text(void);



#endif /* FC__CALENDAR_H */
