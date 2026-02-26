// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

// common
#include "dataio_raw.h"

// utility
#include "bitvector.h"

// std
#include <stdexcept>
#include <string_view>

// Qt
#include <QByteArrayView>

namespace freeciv {

template <int fields> class delta_get {
  bit_vector<fields> m_fields = {{0}};
  int m_seen = 0;
  QByteArrayView m_data;

public:
  // The header should have been removed from data
  delta_get(QByteArrayView data) : m_data(data)
  {
    if (!dio_get(m_data, m_fields)) {
      throw std::runtime_error("<delta header>");
    }
  }

  template <class T> void key(const std::string_view name, int &key)
  {
    int_field(name, key);
    // Fetch old values...
  }

  template <class T> void int_field(const std::string_view name, int &out)
  {
    if (BV_ISSET(m_fields, m_seen++)) {
      if (!dio_get<T>(m_data, out)) {
        throw std::runtime_error(std::string(name));
      }
    }
  }

  template <class T> void field(const std::string_view name, T &out)
  {
    if (BV_ISSET(m_fields, m_seen++)) {
      if (!dio_get(m_data, out)) {
        throw std::runtime_error(std::string(name));
      }
    }
  }

  template <> void field(const std::string_view name, bool &out)
  {
    Q_UNUSED(name);
    return BV_ISSET(m_fields, m_seen++);
  }

  template <class T>
  void int_field(const std::string_view name, T out[], int size)
  {
    if (BV_ISSET(m_fields, m_seen++)) {
      for (int i = 0; i < size; ++i) {
        if (!dio_get<T>(m_data, out)) {
          throw std::runtime_error(std::string(name));
        }
      }
    }
  }

  template <class T>
  void field(const std::string_view name, T out[], int size)
  {
    if (BV_ISSET(m_fields, m_seen++)) {
      for (int i = 0; i < size; ++i) {
        if (!dio_get(m_data, out)) {
          throw std::runtime_error(std::string(name));
        }
      }
    }
  }
};

struct field_counter {
  int count = 0;
  constexpr void field() { count++; }
};

template <class Packet> constexpr void serialize(Packet &packet)
{
  packet.field();
  packet.field();
}

constexpr int count_fields()
{
  field_counter cf;
  serialize(cf);
  return cf.count;
}

std::array<int, count_fields()> a;

} // namespace freeciv
