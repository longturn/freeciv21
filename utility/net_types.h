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

/* This header contains some upper level types related to networking.
 * The idea is that this header can be included without need to special
 * handling of the conflicts of definitions of lower level types that
 * appear in netintf.h */

#ifndef FC__NET_TYPES_H
#define FC__NET_TYPES_H

/* Which protocol will be used for LAN announcements */
enum announce_type { ANNOUNCE_NONE, ANNOUNCE_IPV4, ANNOUNCE_IPV6 };

#define ANNOUNCE_DEFAULT ANNOUNCE_IPV4

enum fc_addr_family { FC_ADDR_IPV4, FC_ADDR_IPV6, FC_ADDR_ANY };

#endif /* FC__NET_TYPES_H */
