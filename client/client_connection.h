// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

// common
#include "connection.h"

/**
 * A connection, as seen from the client.
 */
struct client_connection : public connection {
  /// Increases for every received PACKET_PROCESSING_FINISHED packet.
  int last_processed_request_id_seen;

  /// Holds the id of the request which caused this packet. Can be zero.
  int request_id_of_currently_handled_packet;
};
