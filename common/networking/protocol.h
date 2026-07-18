// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Louis Moureaux <m_louis30@yahoo.com>

#pragma once

/**
 * This file contains the implementation of parts of the binary protocol,
 * e.g., delta.
 */

// common
#include "packets.h"

// std
#include <optional>
#include <unordered_map>

/**
 * Base packet handler class for packets using the delta protocol. It keeps
 * track of the last sent and received packets. This allows sending only the
 * changed information, reducing network traffic.
 */
template <class T> class packet_delta_handler : public packet_handler {
protected:
  std::optional<T> last_received, last_sent;
  QBitArray fields;

public:
  packet_delta_handler(int field_count) : fields(field_count) {}
  virtual ~packet_delta_handler() = default;

  void reset() override
  {
    last_received = std::nullopt;
    last_sent = std::nullopt;
  }

  void reset(int key) override
  {
    Q_UNUSED(key);
    reset();
  }
};

/**
 * Equivalent to \ref packet_delta_handler, but stores packets based on a key
 * (e.g., the tile or unit id).
 */
template <class T> class packet_delta_key_handler : public packet_handler {
protected:
  std::unordered_map<int, T> receive_map, send_map;
  QBitArray fields;

public:
  packet_delta_key_handler(int field_count) : fields(field_count) {}
  virtual ~packet_delta_key_handler() = default;

  void reset() override
  {
    receive_map.clear();
    send_map.clear();
  }

  void reset(int key) override
  {
    receive_map.erase(key);
    send_map.erase(key);
  }
};
