// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "bitvector.h"
#include "log.h"
#include "support.h"

// std
#include <algorithm>
#include <cstddef> // size_t

// Qt
#include <QByteArray>
#include <QByteArrayView>
#include <QtEndian>

struct cm_parameter;
struct worklist;
struct unit_order;
struct requirement;
struct act_prob;

/* Used for dio_<put|get>_type() methods.
 * NB: we only support integer handling currently. */
enum data_type {
  DIOT_UINT8,
  DIOT_UINT16,
  DIOT_UINT32,
  DIOT_SINT8,
  DIOT_SINT16,
  DIOT_SINT32,

  DIOT_LAST
};

// network string conversion
typedef char *(*DIO_PUT_CONV_FUN)(const char *src, size_t *length);
void dio_set_put_conv_callback(DIO_PUT_CONV_FUN fun);

typedef bool (*DIO_GET_CONV_FUN)(char *dst, size_t ndst, const char *src,
                                 size_t nsrc);
void dio_set_get_conv_callback(DIO_GET_CONV_FUN fun);

bool dataio_get_conv_callback(char *dst, size_t ndst, const char *src,
                              size_t nsrc);

// I/O funtions
size_t data_type_size(enum data_type type);
bool dio_get_type_raw(QByteArrayView &din, enum data_type type, int &dest);
void dio_put_type_raw(QByteArray &dout, enum data_type type, int value);

/**
 * Reads a value from the beginning of \c din and puts it in \c dest.
 * The template parameter specifies the encoding.
 */
template <class T> bool dio_get(QByteArrayView &din, int &dest)
{
  if (din.size() < sizeof(T)) {
    log_packet("Packet too short: needed %zu bytes, got %lld", sizeof(T),
               din.size());
    return false;
  }

  T tmp;
  memcpy(&tmp, din.data(), sizeof(T));
  din.slice(sizeof(T));

  dest = qFromBigEndian(tmp);
  return true;
}

/**
 * Writes \c value at the end of \c dout.
 * The template parameter specifies the encoding.
 */
template <class T> void dio_put(QByteArray &dout, int value)
{
  T tmp = value;
  tmp = qToBigEndian(tmp);
  dout.append(reinterpret_cast<char *>(&tmp), sizeof(T));
}

/**
 * Reads a bool from the beginning of \c din and puts it in \c dest.
 */
inline bool dio_get(QByteArrayView &din, bool &dest)
{
  int tmp;
  auto ret = dio_get<std::uint8_t>(din, tmp);
  dest = (tmp != 0);
  return ret;
}

/**
 * Writes a bool at the end of \c dout.
 */
inline void dio_put(QByteArray &dout, bool value)
{
  dio_put<std::uint8_t>(dout, value);
}

/**
 * Reads a float from the beginning of \c din and puts it in \c dest.
 * The template parameter specifies the underlying encoding.
 */
template <class T>
bool dio_get(QByteArrayView &din, float &dest, int precision)
{
  int ival;
  auto ret = dio_get<T>(din, ival);
  dest = static_cast<float>(ival) / precision;
  return ret;
}

/**
 * Writes a float at the end of \c dout.
 * The template parameter specifies the underlying encoding.
 */
template <class T> void dio_put(QByteArray &dout, float value, int precision)
{
  dio_put<T>(dout, static_cast<int>(value * precision));
}

/**
 * Reads a bit vector from the beginning of \c din and puts it in \c dest.
 */
template <unsigned bits>
bool dio_get(QByteArrayView &din, bit_vector<bits> &dest)
{
  fc_assert_ret_val_msg(din.size() >= dest.vec.size(), false,
                        "Not enough data for bit vector: need %zd, got %lld",
                        dest.vec.size(), din.size());

  std::copy(din.begin(), din.begin() + dest.vec.size(), dest.vec.begin());

  din.slice(dest.vec.size());
  return true;
}

/**
 * Writes a bit vector at the end of \c dout.
 */
template <unsigned bits>
void dio_put(QByteArray &dout, const bit_vector<bits> &value)
{
  dout.append(reinterpret_cast<const char *>(value.vec.data()),
              value.vec.size());
}

bool dio_get(QByteArrayView &din, char *dest, size_t max_dest_size)
    fc__attribute((nonnull(2)));
void dio_put(QByteArray &dout, const char *value, size_t size)
    fc__attribute((nonnull(2)));

void dio_put(QByteArray &dout, const char *value)
    fc__attribute((nonnull(2)));
inline void dio_put(QByteArray &dout, const char *value)
{
  dio_put(dout, value, qstrlen(value));
}

bool dio_get(QByteArrayView &din, std::byte *dest, size_t max_dest_size)
    fc__attribute((nonnull(2)));
void dio_put(QByteArray &dout, const std::byte *value, size_t size)
    fc__attribute((nonnull(2)));

bool dio_get(QByteArrayView &din, struct cm_parameter &param);
void dio_put(QByteArray &dout, const struct cm_parameter &param);

bool dio_get(QByteArrayView &din, struct worklist &pwl);
void dio_put(QByteArray &dout, const struct worklist &pwl);

bool dio_get(QByteArrayView &din, struct unit_order &order);
void dio_put(QByteArray &dout, const struct unit_order &order);

bool dio_get(QByteArrayView &din, struct requirement &preq);
void dio_put(QByteArray &dout, const struct requirement &preq);

bool dio_get(QByteArrayView &din, struct act_prob &aprob);
void dio_put(QByteArray &dout, const struct act_prob &aprob);
