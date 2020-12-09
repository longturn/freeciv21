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

/* This header contained some upper level types related to networking. It is
 * now reduced to some definitions related to LAN scans that don't have a
 * direct Qt equivalent. */

#pragma once

/* Which protocol will be used for LAN announcements */
enum announce_type { ANNOUNCE_NONE, ANNOUNCE_IPV4, ANNOUNCE_IPV6 };

#define ANNOUNCE_DEFAULT ANNOUNCE_IPV4


