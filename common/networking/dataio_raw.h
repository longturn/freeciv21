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
#include <type_traits>

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
 * The third argument is for compatibility with the form with dest as an \c
 * int.
 */
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
bool dio_get(QByteArrayView &din, T &dest, T = 0)
{
  if (din.size() < sizeof(T)) {
    log_packet("Packet too short: needed %zu bytes, got %lld", sizeof(T),
               din.size());
    return false;
  }

  memcpy(&dest, din.data(), sizeof(T));
  dest = qFromBigEndian(dest);

  din.slice(sizeof(T));
  return true;
}

/**
 * Writes \c value at the end of \c dout.
 * The third argument is for compatibility with the form with dest as an \c
 * int.
 */
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
void dio_put(QByteArray &dout, T value, T = 0)
{
  value = qToBigEndian(value);
  dout.append(reinterpret_cast<char *>(&value), sizeof(T));
}

/**
 * Reads a value from the beginning of \c din and puts it in \c dest.
 * The third argument allows deduction of the template parameter, which
 * specifies the encoding.
 */
template <class T, class = std::enable_if_t<!std::is_same_v<T, int>>>
bool dio_get(QByteArrayView &din, int &dest, T = 0)
{
  T tmp;
  auto ret = dio_get(din, tmp);
  dest = tmp;
  return ret;
}

/**
 * Writes \c value at the end of \c dout.
 * The third argument allows deduction of the template parameter, which
 * specifies the encoding.
 */
template <class T, class = std::enable_if_t<!std::is_same_v<T, int>>>
void dio_put(QByteArray &dout, int value, T = 0)
{
  dio_put(dout, static_cast<T>(value));
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
 * The fourth argument allows deduction of the template parameter, which
 * specifies the encoding.
 */
template <class T>
bool dio_get(QByteArrayView &din, float &dest, int precision, T = 0)
{
  int ival;
  auto ret = dio_get<T>(din, ival);
  dest = static_cast<float>(ival) / precision;
  return ret;
}

/**
 * Writes a float at the end of \c dout.
 * The fourth argument allows deduction of the template parameter, which
 * specifies the encoding.
 */
template <class T>
void dio_put(QByteArray &dout, float value, int precision, T = 0)
{
  dio_put<T>(dout, static_cast<int>(value * precision));
}

/**
 * Reads an enum value from the beginning of \c din and puts it in \c dest.
 * The third argument allows deduction of the template parameter, which
 * specifies the encoding.
 */
template <class Enum, class T,
          class = std::enable_if_t<std::is_enum_v<Enum>>>
bool dio_get(QByteArrayView &din, Enum &dest, T = 0)
{
  int ival;
  auto ret = dio_get<T>(din, ival);
  dest = static_cast<Enum>(ival);
  return ret;
}

/**
 * Writes an enum value at the end of \c dout.
 * The third argument allows deduction of the template parameter, which
 * specifies the encoding.
 */
template <class Enum, class T,
          class = std::enable_if_t<std::is_enum_v<Enum>>>
void dio_put(QByteArray &dout, Enum value, T = 0)
{
  dio_put<T>(dout, static_cast<int>(value));
}

/**
 * Reads an array of values from the beginning of \c din and puts it in \c
 * dest. At most \c size values are written. Additional arguments are passed
 * to \c dio_get for the destination type.
 */
template <class T, std::size_t N, class... Args>
bool dio_get(QByteArrayView &din, std::array<T, N> &dest, std::size_t size,
             Args &&...args)
{
  fc_assert_ret_val_msg(size <= N, false, "received to many values");

  for (std::size_t i = 0; i < size; ++i) {
    if (!dio_get(din, dest[i], std::forward<Args>(args)...)) {
      return false;
    }
  }

  return true;
}

/**
 * Writes an array of values at the end of \c dout.
 * At most \c size values are written. Additional arguments are passed to
 * \c dio_get for the destination type.
 */
template <class T, std::size_t N, class... Args>
void dio_put(QByteArray &dout, const std::array<T, N> &value,
             std::size_t size, Args &&...args)
{
  fc_assert_ret_msg(size <= N, "trying to send too many values");

  for (std::size_t i = 0; i < size; ++i) {
    dio_put(dout, value[i], std::forward<Args>(args)...);
  }
}

/**
 * Used as a marker to enable the array-diff protocol.
 */
struct array_diff {};

/**
 * Reads an array of values from the beginning of \c din and puts it in \c
 * dest, using the array-diff protocol. Additional arguments are passed to
 * \c dio_get for the destination type.
 */
template <class T, std::size_t N, class... Args>
bool dio_get(QByteArrayView &din, std::array<T, N> &dest, array_diff,
             Args &&...args)
{
  // The array-diff format encodes changed elements only. Each element is
  // encoded as a 1-byte index followed by the value. An index of 255 marks
  // the end of the diff.

  // Indices are encoded on one byte, so it wouldn't make sense to receive
  // more than 256 values in an array-diff.
  for (int count = 0; count < 256; ++count) {
    // Get the index in the array.
    std::uint8_t index;
    fc_assert_ret_val_msg(dio_get(din, index), false,
                          "array-diff: incomplete data");

    // An index of 255 represents the end of the diff.
    if (index == 0xff) {
      break;
    }

    // Make sure it fits.
    fc_assert_ret_val_msg(index < dest.size(), false,
                          "array-diff: index out of bounds");

    // Read the value.
    fc_assert_ret_val_msg(
        dio_get(din, dest[index], std::forward<Args>(args)...), false,
        "array-diff: invalid value");
  }

  return true;
}

/**
 * Writes an array of values at the end of \c dout, using the array-diff
 * protocol. The \c ref argument contains the value with respect to which to
 * built the diff.
 * Additional arguments are passed to \c dio_get for the destination type.
 */
template <class T, std::size_t N, class... Args>
void dio_put(QByteArray &dout, const std::array<T, N> &value,
             const std::array<T, N> &ref, Args &&...args)
{
  // See the matching dio_get for a description of the format.

  // The index is an uint8 and 0xff is the stop marker. Thus we can have at
  // most 0xff - 1 elements.
  static_assert(N < 0xff, "Array too long for the array-diff format.");

  // Indices are encoded on one byte, so it wouldn't make sense to receive
  // more than 256 values in an array-diff.
  for (std::uint8_t i = 0; i < value.size(); ++i) {
    if (value[i] != ref[i]) {
      // Put the index
      dio_put(dout, i, std::uint8_t{});
      // And the value
      dio_put(dout, value[i], std::forward<Args>(args)...);
    }
  }

  // End marker.
  dio_put(dout, 0xff, std::uint8_t{});
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

bool dio_get(QByteArrayView &din, char *dest, std::size_t max_dest_size)
    fc__attribute((nonnull(2)));
void dio_put(QByteArray &dout, const char *value, std::size_t = 0)
    fc__attribute((nonnull(2)));

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
