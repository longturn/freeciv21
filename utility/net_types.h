// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/* This header contained some upper level types related to networking. It is
 * now reduced to some definitions related to LAN scans that don't have a
 * direct Qt equivalent. */

#pragma once

// Which protocol will be used for LAN announcements
enum announce_type { ANNOUNCE_NONE, ANNOUNCE_IPV4, ANNOUNCE_IPV6 };

#define ANNOUNCE_DEFAULT ANNOUNCE_IPV4
