// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/*
 * The DataIO module provides a system independent (endianess and
 * sizeof(int) independent) way to write and read data. It supports
 * multiple datas which are collected in a buffer. It provides
 * recognition of error cases like "not enough space" or "not enough
 * data".
 */

// self
#include "dataio_raw.h"

// utility
#include "log.h"

// common
#include "cm.h"
#include "fc_types.h"
#include "requirements.h"
#include "unit.h"
#include "worklist.h"

// Qt
#include <QByteArrayAlgorithms> // qstrlen, qstrdup, qstrncpy
#include <QtEndian>
#include <QtLogging> // qDebug, qWarning, qCricital, etc

// std
#include <cmath>
#include <cstdint>
#include <cstring>

static bool get_conv(char *dst, size_t ndst, const char *src, size_t nsrc);

static DIO_PUT_CONV_FUN put_conv_callback = nullptr;
static DIO_GET_CONV_FUN get_conv_callback = get_conv;

/**
   Sets string conversion callback to be used when putting text.
 */
void dio_set_put_conv_callback(DIO_PUT_CONV_FUN fun)
{
  put_conv_callback = fun;
}

/**
  Returns FALSE if the destination isn't large enough or the source was
  bad. This is default get_conv_callback.
 */
static bool get_conv(char *dst, size_t ndst, const char *src, size_t nsrc)
{
  size_t len = nsrc; // length to copy, not including null
  bool ret = true;

  if (ndst > 0 && len >= ndst) {
    ret = false;
    len = ndst - 1;
  }

  memcpy(dst, src, len);
  dst[len] = '\0';

  return ret;
}

/**
   Sets string conversion callback to use when getting text.
 */
void dio_set_get_conv_callback(DIO_GET_CONV_FUN fun)
{
  get_conv_callback = fun;
}

/**
   Call the get_conv callback.
 */
bool dataio_get_conv_callback(char *dst, size_t ndst, const char *src,
                              size_t nsrc)
{
  return get_conv_callback(dst, ndst, src, nsrc);
}

/**
   Returns TRUE iff the input contains size unread bytes.
 */
static bool enough_data(QByteArrayView &din, size_t size)
{
  return din.size() >= size;
}

/**
   Return the number of unread bytes.
 */
size_t dio_input_remaining(QByteArrayView &din) { return din.size(); }

/**
   Return the size of the data_type in bytes.
 */
size_t data_type_size(enum data_type type)
{
  switch (type) {
  case DIOT_UINT8:
  case DIOT_SINT8:
    return 1;
  case DIOT_UINT16:
  case DIOT_SINT16:
    return 2;
  case DIOT_UINT32:
  case DIOT_SINT32:
    return 4;
  case DIOT_LAST:
    break;
  }

  fc_assert_msg(false, "data_type %d not handled.", type);
  return 0;
}

/**
    Skips 'n' bytes.
 */
bool dio_input_skip(QByteArrayView &din, size_t size)
{
  if (enough_data(din, size)) {
    din.slice(size);
    return true;
  } else {
    return false;
  }
}

/**
   Receive value using 'size' bits to dest.
 */
bool dio_get_type_raw(QByteArrayView &din, enum data_type type, int &dest)
{
  switch (type) {
  case DIOT_UINT8:
    return dio_get<std::uint8_t>(din, dest);
  case DIOT_UINT16:
    return dio_get<std::uint16_t>(din, dest);
  case DIOT_UINT32:
    return dio_get<std::uint32_t>(din, dest);
  case DIOT_SINT8:
    return dio_get<std::int8_t>(din, dest);
  case DIOT_SINT16:
    return dio_get<std::int16_t>(din, dest);
  case DIOT_SINT32:
    return dio_get<std::int32_t>(din, dest);
  case DIOT_LAST:
    break;
  }

  fc_assert_msg(false, "data_type %d not handled.", type);
  return false;
}

/**
   Insert value using 'size' bits. May overflow.
 */
void dio_put_type_raw(QByteArray &dout, enum data_type type, int value)
{
  switch (type) {
  case DIOT_UINT8:
    dio_put<std::uint8_t>(dout, value);
    return;
  case DIOT_UINT16:
    dio_put<std::uint16_t>(dout, value);
    return;
  case DIOT_UINT32:
    dio_put<std::uint32_t>(dout, value);
    return;
  case DIOT_SINT8:
    dio_put<std::int8_t>(dout, value);
    return;
  case DIOT_SINT16:
    dio_put<std::int16_t>(dout, value);
    return;
  case DIOT_SINT32:
    dio_put<std::int32_t>(dout, value);
    return;
  case DIOT_LAST:
    break;
  }

  fc_assert_msg(false, "data_type %d not handled.", type);
}

/**
   Take string. Conversion callback is used.
 */
bool dio_get(QByteArrayView &din, char *dest, size_t max_dest_size)
{
  size_t offset, remaining;

  fc_assert(max_dest_size > 0);

  if (!enough_data(din, 1)) {
    log_packet("Got a bad string");
    return false;
  }

  remaining = dio_input_remaining(din);
  const char *c = din.data();

  // avoid using qstrlen (or qstrcpy) on an (unsigned char*)  --dwp
  for (offset = 0; offset < remaining && c[offset] != '\0'; offset++) {
    // nothing
  }

  if (offset >= remaining) {
    log_packet("Got a too short string");
    return false;
  }

  if (!(*get_conv_callback)(dest, max_dest_size, c, offset)) {
    log_packet("Got a bad encoded string");
    return false;
  }

  din.slice(offset + 1);
  return true;
}

/**
   Insert nullptr-terminated string. Conversion callback is used if set.
 */
void dio_put(QByteArray &dout, const char *value, size_t size)
{
  Q_UNUSED(size);

  if (put_conv_callback) {
    size_t length;
    char *buffer;

    if ((buffer = (*put_conv_callback)(value, &length))) {
      dio_put(dout, reinterpret_cast<const std::byte *>(buffer), length + 1);
      delete[] buffer;
    }
  } else {
    dio_put(dout, reinterpret_cast<const std::byte *>(value),
            qstrlen(value) + 1);
  }
}

/**
 * Take memory. No conversion callback. The destination is assumed to be
 * large enough.
 */
bool dio_get(QByteArrayView &din, std::byte *dest, size_t size)
{
  fc_assert(size > 0);

  if (!enough_data(din, size)) {
    log_packet("Got bad memory");
    return false;
  }

  memcpy(dest, din.data(), size);

  din.slice(size);
  return true;
}

/*
   Insert block directly from memory.
 */
void dio_put(QByteArray &dout, const std::byte *value, std::size_t size)
{
  dout.append(reinterpret_cast<const char *>(value), size);
}

/**
   Get city manager parameters.
 */
bool dio_get(QByteArrayView &din, struct cm_parameter &param)
{
  int i;

  for (i = 0; i < O_LAST; i++) {
    if (!dio_get<std::int16_t>(din, param.minimal_surplus[i])) {
      log_packet("Got a bad cm_parameter");
      return false;
    }
  }

  if (!dio_get(din, param.max_growth) || !dio_get(din, param.require_happy)
      || !dio_get(din, param.allow_disorder)
      || !dio_get(din, param.allow_specialists)) {
    log_packet("Got a bad cm_parameter");
    return false;
  }

  for (i = 0; i < O_LAST; i++) {
    if (!dio_get<std::uint16_t>(din, param.factor[i])) {
      log_packet("Got a bad cm_parameter");
      return false;
    }
  }

  if (!dio_get<std::uint16_t>(din, param.happy_factor)) {
    log_packet("Got a bad cm_parameter");
    return false;
  }

  return true;
}

/**
   Insert cm_parameter struct.
 */
void dio_put(QByteArray &dout, const struct cm_parameter &param)
{
  for (int i = 0; i < O_LAST; i++) {
    dio_put<std::int16_t>(dout, param.minimal_surplus[i]);
  }

  dio_put(dout, param.max_growth);
  dio_put(dout, param.require_happy);
  dio_put(dout, param.allow_disorder);
  dio_put(dout, param.allow_specialists);

  for (int i = 0; i < O_LAST; i++) {
    dio_put<std::uint16_t>(dout, param.factor[i]);
  }

  dio_put<std::uint16_t>(dout, param.happy_factor);
}

/**
   Take unit_order struct and put it in the provided orders.
 */
bool dio_get(QByteArrayView &din, struct unit_order &order)
{
  // These fields are enums
  int iorder, iactivity, idir;

  if (!dio_get<std::uint8_t>(din, iorder)
      || !dio_get<std::uint8_t>(din, iactivity)
      || !dio_get<std::int32_t>(din, order.target)
      || !dio_get<std::int16_t>(din, order.sub_target)
      || !dio_get<std::uint8_t>(din, order.action)
      || !dio_get<std::int8_t>(din, idir)) {
    log_packet("Got a bad unit_order");
    return false;
  }

  order.order = unit_orders(iorder);
  order.activity = unit_activity(iactivity);
  order.dir = direction8(idir);

  return true;
}

/**
   Insert the given unit_order struct/
 */
void dio_put(QByteArray &dout, const struct unit_order &order)
{
  dio_put<std::uint8_t>(dout, order.order);
  dio_put<std::uint8_t>(dout, order.activity);
  dio_put<std::int32_t>(dout, order.target);
  dio_put<std::int16_t>(dout, order.sub_target);
  dio_put<std::uint8_t>(dout, order.action);
  dio_put<std::int8_t>(dout, order.dir);
}

/**
   Take worklist item count and then kind and number for each item, and
   put them to provided worklist.
 */
bool dio_get(QByteArrayView &din, struct worklist &pwl)
{
  int i, length;

  worklist_init(&pwl);

  if (!dio_get<std::uint8_t>(din, length)) {
    log_packet("Got a bad worklist");
    return false;
  }

  for (i = 0; i < length; i++) {
    int identifier;
    int kind;
    struct universal univ;

    if (!dio_get<std::uint8_t>(din, kind)
        || !dio_get<std::uint8_t>(din, identifier)) {
      log_packet("Got a too short worklist");
      return false;
    }

    /*
     * FIXME: the value returned by universal_by_number() should be checked!
     */
    univ = universal_by_number(universals_n(kind), identifier);
    worklist_append(&pwl, &univ);
  }

  return true;
}

/**
   Insert number of worklist items as 8 bit value and then insert
   8 bit kind and 8 bit number for each worklist item.
 */
void dio_put(QByteArray &dout, const struct worklist &pwl)
{
  int length = worklist_length(&pwl);

  dio_put<std::uint8_t>(dout, length);
  for (int i = 0; i < length; i++) {
    const struct universal &pcp = pwl.entries[i];
    dio_put<std::uint8_t>(dout, pcp.kind);
    dio_put<std::uint8_t>(dout, universal_number(&pcp));
  }
}

/**
   De-serialize an action probability.
 */
bool dio_get(QByteArrayView &din, struct act_prob &aprob)
{
  int min, max;

  if (!dio_get<std::uint8_t>(din, min) || !dio_get<std::uint8_t>(din, max)) {
    log_packet("Got a bad action probability");
    return false;
  }

  aprob.min = min;
  aprob.max = max;

  return true;
}

/**
   Serialize an action probability.
 */
void dio_put(QByteArray &dout, const struct act_prob &aprob)
{
  dio_put<std::uint8_t>(dout, aprob.min);
  dio_put<std::uint8_t>(dout, aprob.max);
}

/**
   De-serialize a requirement.
 */
bool dio_get(QByteArrayView &din, struct requirement &preq)
{
  int type, range, value;
  bool survives, present, quiet;

  if (!dio_get<std::uint8_t>(din, type) || !dio_get<std::int32_t>(din, value)
      || !dio_get<std::uint8_t>(din, range) || !dio_get(din, survives)
      || !dio_get(din, present) || !dio_get(din, quiet)) {
    log_packet("Got a bad requirement");
    return false;
  }

  preq = req_from_values(type, range, survives, present, quiet, value);
  if (preq.source.kind == universals_n_invalid()) {
    // Keep bad requirements but make sure we never touch them.
    qWarning() << "The server sent an invalid or unknown requirement.";
    preq.source.kind = VUT_NONE;
  }

  return true;
}

/**
   Serialize a requirement.
 */
void dio_put(QByteArray &dout, const struct requirement &preq)
{
  int type, range, value;
  bool survives, present, quiet;

  req_get_values(&preq, &type, &range, &survives, &present, &quiet, &value);

  dio_put<std::uint8_t>(dout, type);
  dio_put<std::int32_t>(dout, value);
  dio_put<std::uint8_t>(dout, range);
  dio_put(dout, survives);
  dio_put(dout, present);
  dio_put(dout, quiet);
}
