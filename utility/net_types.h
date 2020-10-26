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

/* This header contained some upper level types related to networking. It is
 * now reduced to some definitions related to LAN scans that don't have a
 * direct Qt equivalent. */

#ifndef FC__NET_TYPES_H
#define FC__NET_TYPES_H

/* Which protocol will be used for LAN announcements */
enum announce_type { ANNOUNCE_NONE, ANNOUNCE_IPV4, ANNOUNCE_IPV6 };

#define ANNOUNCE_DEFAULT ANNOUNCE_IPV4

#endif /* FC__NET_TYPES_H */
