// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "log.h"
#include "support.h"

// std
#include <cstddef> // size_t

// Qt
#include <QByteArrayView>
#include <QtEndian>

struct cm_parameter;
struct worklist;
struct unit_order;
struct requirement;
struct act_prob;

struct data_in {
  const void *src;
  size_t src_size, current;
};

struct raw_data_out {
  void *dest;
  size_t dest_size, used, current;
  bool too_short; // set to 1 if try to read past end
};

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

// General functions
void dio_output_init(struct raw_data_out *dout, void *destination,
                     size_t dest_size);
void dio_output_rewind(struct raw_data_out *dout);
size_t dio_output_used(struct raw_data_out *dout);
size_t data_type_size(enum data_type type);

// gets
bool dio_get_type_raw(QByteArrayView &din, enum data_type type, int &dest);

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

bool dio_get_memory_raw(QByteArrayView &din, void *dest, size_t dest_size)
    fc__attribute((nonnull(2)));
bool dio_get_string_raw(QByteArrayView &din, char *dest,
                        size_t max_dest_size) fc__attribute((nonnull(2)));
bool dio_get(QByteArrayView &din, struct cm_parameter &param);
bool dio_get(QByteArrayView &din, struct worklist &pwl);
bool dio_get(QByteArrayView &din, struct unit_order &order);
bool dio_get(QByteArrayView &din, struct requirement &preq);
bool dio_get(QByteArrayView &din, struct act_prob &aprob);

// Should be a function but we need some macro magic.
#define DIO_BV_GET(pdin, bv)                                                \
  dio_get_memory_raw((pdin), (bv).vec.data(), (bv).vec.size())

#define DIO_GET(f, d, ...) dio_get_##f##_raw(d, ##__VA_ARGS__)

// puts
void dio_put_type_raw(struct raw_data_out *dout, enum data_type type,
                      int value);

void dio_put_uint8_raw(struct raw_data_out *dout, int value);
void dio_put_uint16_raw(struct raw_data_out *dout, int value);
void dio_put_uint32_raw(struct raw_data_out *dout, int value);

void dio_put_sint8_raw(struct raw_data_out *dout, int value);
void dio_put_sint16_raw(struct raw_data_out *dout, int value);
void dio_put_sint32_raw(struct raw_data_out *dout, int value);

void dio_put_bool_raw(struct raw_data_out *dout, bool value);
void dio_put_ufloat_raw(struct raw_data_out *dout, float value,
                        int float_factor);
void dio_put_sfloat_raw(struct raw_data_out *dout, float value,
                        int float_factor);

void dio_put_memory_raw(struct raw_data_out *dout, const void *value,
                        size_t size);
void dio_put_string_raw(struct raw_data_out *dout, const char *value);
void dio_put_city_map_raw(struct raw_data_out *dout, const char *value);
void dio_put_cm_parameter_raw(struct raw_data_out *dout,
                              const struct cm_parameter *param);
void dio_put_worklist_raw(struct raw_data_out *dout,
                          const struct worklist *pwl);
void dio_put_unit_order_raw(struct raw_data_out *dout,
                            const struct unit_order *order);
void dio_put_requirement_raw(struct raw_data_out *dout,
                             const struct requirement *preq);
void dio_put_action_probability_raw(struct raw_data_out *dout,
                                    const struct act_prob *aprob);

// Should be a function but we need some macro magic.
#define DIO_BV_PUT(pdout, bv)                                               \
  dio_put_memory_raw((pdout), (bv).vec.data(), (bv).vec.size())

#define DIO_PUT(f, d, ...) dio_put_##f##_raw(d, ##__VA_ARGS__)
