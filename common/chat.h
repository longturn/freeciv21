// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

/* Definitions related to interpreting chat messages.
 * Behaviour generally can't be changed at whim because client and
 * server are assumed to agree on these details.
 */

// Special characters in chatlines.

// The character to mark chatlines as server commands
// FIXME this is still hard-coded in a lot of places
#define SERVER_COMMAND_PREFIX '/'
#define CHAT_ALLIES_PREFIX '.'
#define CHAT_DIRECT_PREFIX ':'
