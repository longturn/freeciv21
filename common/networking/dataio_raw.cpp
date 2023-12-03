/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

/*
 * The DataIO module provides a system independent (endianess and
 * sizeof(int) independent) way to write and read data. It supports
 * multiple datas which are collected in a buffer. It provides
 * recognition of error cases like "not enough space" or "not enough
 * data".
 */

#include <cmath>
#include <cstdint>
#include <cstring>

// Qt
#include <QtEndian>

// utility
#include "support.h"

// common
#include "events.h"
#include "player.h"
#include "requirements.h"
#include "tech.h"
#include "unit.h"
#include "worklist.h"

/* common/aicore */
#include "cm.h"

#include "dataio.h"

static bool get_conv(char *dst, size_t ndst, const char *src, size_t nsrc);

static DIO_PUT_CONV_FUN put_conv_callback = nullptr;
static DIO_GET_CONV_FUN get_conv_callback = get_conv;

// Uncomment to make field range tests to asserts, fatal with -F
// #define FIELD_RANGE_ASSERT

#if defined(FREECIV_TESTMATIC) && !defined(FIELD_RANGE_ASSERT)
#define FIELD_RANGE_ASSERT
#endif

#ifdef FIELD_RANGE_ASSERT
/* This evaluates _test_ twice. If that's a problem,
 * it should evaluate it just once and store result to variable.
 * That would lose verbosity of the assert message. */
#define FIELD_RANGE_TEST(_test_, _action_, _format_, ...)                   \
  fc_assert(!(_test_));                                                     \
  if (_test_) {                                                             \
    _action_ qCritical(_format_, ##__VA_ARGS__);                            \
  }
#else // FIELD_RANGE_ASSERT
#define FIELD_RANGE_TEST(_test_, _action_, _format_, ...)                   \
  if (_test_) {                                                             \
    _action_ qCritical(_format_, ##__VA_ARGS__);                            \
  }
#endif // FIELD_RANGE_ASSERT

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
   Returns TRUE iff the output has size bytes available.
 */
static bool enough_space(struct raw_data_out *dout, size_t size)
{
  if (dout->current + size > dout->dest_size) {
    dout->too_short = true;
    return false;
  } else {
    dout->used = MAX(dout->used, dout->current + size);
    return true;
  }
}

/**
   Returns TRUE iff the input contains size unread bytes.
 */
static bool enough_data(struct data_in *din, size_t size)
{
  return dio_input_remaining(din) >= size;
}

/**
   Initializes the output to the given output buffer and the given
   buffer size.
 */
void dio_output_init(struct raw_data_out *dout, void *destination,
                     size_t dest_size)
{
  dout->dest = destination;
  dout->dest_size = dest_size;
  dout->current = 0;
  dout->used = 0;
  dout->too_short = false;
}

/**
   Return the maximum number of bytes used.
 */
size_t dio_output_used(struct raw_data_out *dout) { return dout->used; }

/**
   Rewinds the stream so that the put-functions start from the
   beginning.
 */
void dio_output_rewind(struct raw_data_out *dout) { dout->current = 0; }

/**
   Initializes the input to the given input buffer and the given
   number of valid input bytes.
 */
void dio_input_init(struct data_in *din, const void *src, size_t src_size)
{
  din->src = src;
  din->src_size = src_size;
  din->current = 0;
}

/**
   Rewinds the stream so that the get-functions start from the
   beginning.
 */
void dio_input_rewind(struct data_in *din) { din->current = 0; }

/**
   Return the number of unread bytes.
 */
size_t dio_input_remaining(struct data_in *din)
{
  return din->src_size - din->current;
}

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
bool dio_input_skip(struct data_in *din, size_t size)
{
  if (enough_data(din, size)) {
    din->current += size;
    return true;
  } else {
    return false;
  }
}

/**
   Insert value using 8 bits. May overflow.
 */
void dio_put_uint8_raw(struct raw_data_out *dout, int value)
{
  uint8_t x = value;
  FC_STATIC_ASSERT(sizeof(x) == 1, uint8_not_1_byte);

  FIELD_RANGE_TEST((int) x != value, ,
                   "Trying to put %d into 8 bits; "
                   "it will result %d at receiving side.",
                   value, (int) x);

  if (enough_space(dout, 1)) {
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), &x, 1);
    dout->current++;
  }
}

/**
   Insert value using 16 bits. May overflow.
 */
void dio_put_uint16_raw(struct raw_data_out *dout, int value)
{
  uint16_t x = qToBigEndian(uint16_t(value));
  FC_STATIC_ASSERT(sizeof(x) == 2, uint16_not_2_bytes);

  FIELD_RANGE_TEST((int) qFromBigEndian(x) != value, ,
                   "Trying to put %d into 16 bits; "
                   "it will result %d at receiving side.",
                   value, (int) qFromBigEndian(x));

  if (enough_space(dout, 2)) {
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), &x, 2);
    dout->current += 2;
  }
}

/**
   Insert value using 32 bits. May overflow.
 */
void dio_put_uint32_raw(struct raw_data_out *dout, int value)
{
  uint32_t x = qToBigEndian(uint32_t(value));
  FC_STATIC_ASSERT(sizeof(x) == 4, uint32_not_4_bytes);

  FIELD_RANGE_TEST((int) qFromBigEndian(x) != value, ,
                   "Trying to put %d into 32 bits; "
                   "it will result %d at receiving side.",
                   value, (int) qFromBigEndian(x));

  if (enough_space(dout, 4)) {
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), &x, 4);
    dout->current += 4;
  }
}

/**
   Insert value using 'size' bits. May overflow.
 */
void dio_put_type_raw(struct raw_data_out *dout, enum data_type type,
                      int value)
{
  switch (type) {
  case DIOT_UINT8:
    dio_put_uint8_raw(dout, value);
    return;
  case DIOT_UINT16:
    dio_put_uint16_raw(dout, value);
    return;
  case DIOT_UINT32:
    dio_put_uint32_raw(dout, value);
    return;
  case DIOT_SINT8:
    dio_put_sint8_raw(dout, value);
    return;
  case DIOT_SINT16:
    dio_put_sint16_raw(dout, value);
    return;
  case DIOT_SINT32:
    dio_put_sint32_raw(dout, value);
    return;
  case DIOT_LAST:
    break;
  }

  fc_assert_msg(false, "data_type %d not handled.", type);
}

/**
   Insert value using 8 bits. May overflow.
 */
void dio_put_sint8_raw(struct raw_data_out *dout, int value)
{
  dio_put_uint8_raw(dout, 0 <= value ? value : value + 0x100);
}

/**
   Insert value using 16 bits. May overflow.
 */
void dio_put_sint16_raw(struct raw_data_out *dout, int value)
{
  dio_put_uint16_raw(dout, 0 <= value ? value : value + 0x10000);
}

/**
   Insert value using 32 bits. May overflow.
 */
void dio_put_sint32_raw(struct raw_data_out *dout, int value)
{
  if (sizeof(int) == 4) {
    dio_put_uint32_raw(dout, value);
  } else {
    dio_put_uint32_raw(dout, (0 <= value ? value : value + 0x100000000));
  }
}

/**
   Insert value 0 or 1 using 8 bits.
 */
void dio_put_bool8_raw(struct raw_data_out *dout, bool value)
{
  FIELD_RANGE_TEST(value != true && value != false, value = (value != false);
                   , "Trying to put a non-boolean: %d", (int) value);

  dio_put_uint8_raw(dout, value ? 1 : 0);
}

/**
   Insert value 0 or 1 using 32 bits.
 */
void dio_put_bool32_raw(struct raw_data_out *dout, bool value)
{
  FIELD_RANGE_TEST(value != true && value != false, value = (value != false);
                   , "Trying to put a non-boolean: %d", (int) value);

  dio_put_uint32_raw(dout, value ? 1 : 0);
}

/**
   Insert a float number, which is multiplied by 'float_factor' before
   being encoded into an uint32.
 */
void dio_put_ufloat_raw(struct raw_data_out *dout, float value,
                        int float_factor)
{
  uint32_t v = value * float_factor;

  FIELD_RANGE_TEST(
      fabsf((float) v / float_factor - value) > 1.1 / float_factor, ,
      "Trying to put %f with factor %d in 32 bits; "
      "it will result %f at receiving side, having error of %f units.",
      value, float_factor, (float) v / float_factor,
      fabsf((float) v / float_factor - value) * float_factor);

  dio_put_uint32_raw(dout, v);
}

/**
   Insert a float number, which is multiplied by 'float_factor' before
   being encoded into a sint32.
 */
void dio_put_sfloat_raw(struct raw_data_out *dout, float value,
                        int float_factor)
{
  int32_t v = value * float_factor;

  FIELD_RANGE_TEST(
      fabsf((float) v / float_factor - value) > 1.1 / float_factor, ,
      "Trying to put %f with factor %d in 32 bits; "
      "it will result %f at receiving side, having error of %f units.",
      value, float_factor, (float) v / float_factor,
      fabsf((float) v / float_factor - value) * float_factor);

  dio_put_sint32_raw(dout, v);
}

/**
   Insert block directly from memory.
 */
void dio_put_memory_raw(struct raw_data_out *dout, const void *value,
                        size_t size)
{
  if (enough_space(dout, size)) {
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), value, size);
    dout->current += size;
  }
}

/**
   Insert nullptr-terminated string. Conversion callback is used if set.
 */
void dio_put_string_raw(struct raw_data_out *dout, const char *value)
{
  if (put_conv_callback) {
    size_t length;
    char *buffer;

    if ((buffer = (*put_conv_callback)(value, &length))) {
      dio_put_memory_raw(dout, buffer, length + 1);
      delete[] buffer;
    }
  } else {
    dio_put_memory_raw(dout, value, qstrlen(value) + 1);
  }
}

/**
   Insert cm_parameter struct.
 */
void dio_put_cm_parameter_raw(struct raw_data_out *dout,
                              const struct cm_parameter *param)
{
  int i;

  for (i = 0; i < O_LAST; i++) {
    dio_put_sint16_raw(dout, param->minimal_surplus[i]);
  }

  dio_put_bool8_raw(dout, param->max_growth);
  dio_put_bool8_raw(dout, param->require_happy);
  dio_put_bool8_raw(dout, param->allow_disorder);
  dio_put_bool8_raw(dout, param->allow_specialists);

  for (i = 0; i < O_LAST; i++) {
    dio_put_uint16_raw(dout, param->factor[i]);
  }

  dio_put_uint16_raw(dout, param->happy_factor);
}

/**
   Insert the given unit_order struct/
 */
void dio_put_unit_order_raw(struct raw_data_out *dout,
                            const struct unit_order *order)
{
  dio_put_uint8_raw(dout, order->order);
  dio_put_uint8_raw(dout, order->activity);
  dio_put_sint32_raw(dout, order->target);
  dio_put_sint16_raw(dout, order->sub_target);
  dio_put_uint8_raw(dout, order->action);
  dio_put_sint8_raw(dout, order->dir);
}

/**
   Insert number of worklist items as 8 bit value and then insert
   8 bit kind and 8 bit number for each worklist item.
 */
void dio_put_worklist_raw(struct raw_data_out *dout,
                          const struct worklist *pwl)
{
  int i, length = worklist_length(pwl);

  dio_put_uint8_raw(dout, length);
  for (i = 0; i < length; i++) {
    const struct universal *pcp = &(pwl->entries[i]);

    dio_put_uint8_raw(dout, pcp->kind);
    dio_put_uint8_raw(dout, universal_number(pcp));
  }
}

/**
  Receive uint8 value to dest.
 */
bool dio_get_uint8_raw(struct data_in *din, int *dest)
{
  uint8_t x;

  FC_STATIC_ASSERT(sizeof(x) == 1, uint8_not_byte);

  if (!enough_data(din, 1)) {
    log_packet("Packet too short to read 1 byte");

    return false;
  }

  memcpy(&x, ADD_TO_POINTER(din->src, din->current), 1);
  *dest = x;
  din->current++;
  return true;
}

/**
  Receive uint16 value to dest.
 */
bool dio_get_uint16_raw(struct data_in *din, int *dest)
{
  uint16_t x;

  FC_STATIC_ASSERT(sizeof(x) == 2, uint16_not_2_bytes);

  if (!enough_data(din, 2)) {
    log_packet("Packet too short to read 2 bytes");

    return false;
  }

  memcpy(&x, ADD_TO_POINTER(din->src, din->current), 2);
  *dest = qFromBigEndian(x);
  din->current += 2;
  return true;
}

/**
  Receive uint32 value to dest.
 */
bool dio_get_uint32_raw(struct data_in *din, int *dest)
{
  uint32_t x;

  FC_STATIC_ASSERT(sizeof(x) == 4, uint32_not_4_bytes);

  if (!enough_data(din, 4)) {
    log_packet("Packet too short to read 4 bytes");

    return false;
  }

  memcpy(&x, ADD_TO_POINTER(din->src, din->current), 4);
  *dest = qFromBigEndian(x);
  din->current += 4;
  return true;
}

/**
   Receive value using 'size' bits to dest.
 */
bool dio_get_type_raw(struct data_in *din, enum data_type type, int *dest)
{
  switch (type) {
  case DIOT_UINT8:
    return dio_get_uint8_raw(din, dest);
  case DIOT_UINT16:
    return dio_get_uint16_raw(din, dest);
  case DIOT_UINT32:
    return dio_get_uint32_raw(din, dest);
  case DIOT_SINT8:
    return dio_get_sint8_raw(din, dest);
  case DIOT_SINT16:
    return dio_get_sint16_raw(din, dest);
  case DIOT_SINT32:
    return dio_get_sint32_raw(din, dest);
  case DIOT_LAST:
    break;
  }

  fc_assert_msg(false, "data_type %d not handled.", type);
  return false;
}

/**
   Take boolean value from 8 bits.
 */
bool dio_get_bool8_raw(struct data_in *din, bool *dest)
{
  int ival;

  if (!dio_get_uint8_raw(din, &ival)) {
    return false;
  }

  if (ival != 0 && ival != 1) {
    log_packet("Got a bad boolean: %d", ival);
    return false;
  }

  *dest = (ival != 0);
  return true;
}

/**
   Take boolean value from 32 bits.
 */
bool dio_get_bool32_raw(struct data_in *din, bool *dest)
{
  int ival;

  if (!dio_get_uint32_raw(din, &ival)) {
    return false;
  }

  if (ival != 0 && ival != 1) {
    log_packet("Got a bad boolean: %d", ival);
    return false;
  }

  *dest = (ival != 0);
  return true;
}

/**
   Get an unsigned float number, which have been multiplied by 'float_factor'
   and encoded into an uint32 by dio_put_ufloat_raw().
 */
bool dio_get_ufloat_raw(struct data_in *din, float *dest, int float_factor)
{
  int ival;

  if (!dio_get_uint32_raw(din, &ival)) {
    return false;
  }

  *dest = static_cast<float>(ival) / float_factor;
  return true;
}

/**
   Get a signed float number, which have been multiplied by 'float_factor'
   and encoded into a sint32 by dio_put_sfloat().
 */
bool dio_get_sfloat_raw(struct data_in *din, float *dest, int float_factor)
{
  int ival;

  if (!dio_get_sint32_raw(din, &ival)) {
    return false;
  }

  *dest = static_cast<float>(ival) / float_factor;
  return true;
}

/**
   Take value from 8 bits.
 */
bool dio_get_sint8_raw(struct data_in *din, int *dest)
{
  int tmp;

  if (!dio_get_uint8_raw(din, &tmp)) {
    return false;
  }

  if (tmp > 0x7f) {
    tmp -= 0x100;
  }
  *dest = tmp;
  return true;
}

/**
   Take value from 16 bits.
 */
bool dio_get_sint16_raw(struct data_in *din, int *dest)
{
  int tmp;

  if (!dio_get_uint16_raw(din, &tmp)) {
    return false;
  }

  if (tmp > 0x7fff) {
    tmp -= 0x10000;
  }
  *dest = tmp;
  return true;
}

/**
   Take value from 32 bits.
 */
bool dio_get_sint32_raw(struct data_in *din, int *dest)
{
  int tmp;

  if (!dio_get_uint32_raw(din, &tmp)) {
    return false;
  }

  if (sizeof(int) != 4) {
    if (tmp > 0x7fffffff) {
      tmp -= 0x100000000;
    }
  }

  *dest = tmp;
  return true;
}

/**
   Take memory block directly.
 */
bool dio_get_memory_raw(struct data_in *din, void *dest, size_t dest_size)
{
  if (!enough_data(din, dest_size)) {
    log_packet("Got too short memory");
    return false;
  }

  memcpy(dest, ADD_TO_POINTER(din->src, din->current), dest_size);
  din->current += dest_size;
  return true;
}

/**
   Take string. Conversion callback is used.
 */
bool dio_get_string_raw(struct data_in *din, char *dest,
                        size_t max_dest_size)
{
  char *c;
  size_t offset, remaining;

  fc_assert(max_dest_size > 0);

  if (!enough_data(din, 1)) {
    log_packet("Got a bad string");
    return false;
  }

  remaining = dio_input_remaining(din);
  c = static_cast<char *>(ADD_TO_POINTER(din->src, din->current));

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

  din->current += offset + 1;
  return true;
}

/**
   Get city manager parameters.
 */
bool dio_get_cm_parameter_raw(struct data_in *din,
                              struct cm_parameter *param)
{
  int i;

  for (i = 0; i < O_LAST; i++) {
    if (!dio_get_sint16_raw(din, &param->minimal_surplus[i])) {
      log_packet("Got a bad cm_parameter");
      return false;
    }
  }

  if (!dio_get_bool8_raw(din, &param->max_growth)
      || !dio_get_bool8_raw(din, &param->require_happy)
      || !dio_get_bool8_raw(din, &param->allow_disorder)
      || !dio_get_bool8_raw(din, &param->allow_specialists)) {
    log_packet("Got a bad cm_parameter");
    return false;
  }

  for (i = 0; i < O_LAST; i++) {
    if (!dio_get_uint16_raw(din, &param->factor[i])) {
      log_packet("Got a bad cm_parameter");
      return false;
    }
  }

  if (!dio_get_uint16_raw(din, &param->happy_factor)) {
    log_packet("Got a bad cm_parameter");
    return false;
  }

  return true;
}

/**
   Take unit_order struct and put it in the provided orders.
 */
bool dio_get_unit_order_raw(struct data_in *din, struct unit_order *order)
{
  // These fields are enums
  int iorder, iactivity, idir;

  if (!dio_get_uint8_raw(din, &iorder) || !dio_get_uint8_raw(din, &iactivity)
      || !dio_get_sint32_raw(din, &order->target)
      || !dio_get_sint16_raw(din, &order->sub_target)
      || !dio_get_uint8_raw(din, &order->action)
      || !dio_get_sint8_raw(din, &idir)) {
    log_packet("Got a bad unit_order");
    return false;
  }

  order->order = unit_orders(iorder);
  order->activity = unit_activity(iactivity);
  order->dir = direction8(idir);

  return true;
}

/**
   Take worklist item count and then kind and number for each item, and
   put them to provided worklist.
 */
bool dio_get_worklist_raw(struct data_in *din, struct worklist *pwl)
{
  int i, length;

  worklist_init(pwl);

  if (!dio_get_uint8_raw(din, &length)) {
    log_packet("Got a bad worklist");
    return false;
  }

  for (i = 0; i < length; i++) {
    int identifier;
    int kind;
    struct universal univ;

    if (!dio_get_uint8_raw(din, &kind)
        || !dio_get_uint8_raw(din, &identifier)) {
      log_packet("Got a too short worklist");
      return false;
    }

    /*
     * FIXME: the value returned by universal_by_number() should be checked!
     */
    univ = universal_by_number(universals_n(kind), identifier);
    worklist_append(pwl, &univ);
  }

  return true;
}

/**
   De-serialize an action probability.
 */
bool dio_get_action_probability_raw(struct data_in *din,
                                    struct act_prob *aprob)
{
  int min, max;

  if (!dio_get_uint8_raw(din, &min) || !dio_get_uint8_raw(din, &max)) {
    log_packet("Got a bad action probability");
    return false;
  }

  aprob->min = min;
  aprob->max = max;

  return true;
}

/**
   Serialize an action probability.
 */
void dio_put_action_probability_raw(struct raw_data_out *dout,
                                    const struct act_prob *aprob)
{
  dio_put_uint8_raw(dout, aprob->min);
  dio_put_uint8_raw(dout, aprob->max);
}

/**
   De-serialize a requirement.
 */
bool dio_get_requirement_raw(struct data_in *din, struct requirement *preq)
{
  int type, range, value;
  bool survives, present, quiet;

  if (!dio_get_uint8_raw(din, &type) || !dio_get_sint32_raw(din, &value)
      || !dio_get_uint8_raw(din, &range)
      || !dio_get_bool8_raw(din, &survives)
      || !dio_get_bool8_raw(din, &present)
      || !dio_get_bool8_raw(din, &quiet)) {
    log_packet("Got a bad requirement");
    return false;
  }

  /*
   * FIXME: the value returned by req_from_values() should be checked!
   */
  *preq = req_from_values(type, range, survives, present, quiet, value);

  return true;
}

/**
   Serialize a requirement.
 */
void dio_put_requirement_raw(struct raw_data_out *dout,
                             const struct requirement *preq)
{
  int type, range, value;
  bool survives, present, quiet;

  req_get_values(preq, &type, &range, &survives, &present, &quiet, &value);

  dio_put_uint8_raw(dout, type);
  dio_put_sint32_raw(dout, value);
  dio_put_uint8_raw(dout, range);
  dio_put_bool8_raw(dout, survives);
  dio_put_bool8_raw(dout, present);
  dio_put_bool8_raw(dout, quiet);
}

/**
  Create a new address of the location of a field inside a packet.
 */
struct plocation *plocation_field_new(char *name)
{
  auto *out = new plocation;

  out->kind = PADR_FIELD;
  out->name = name;
  out->sub_location = nullptr;

  return out;
}

/**
  Create a new address of the location of an array element inside a packet.
 */
struct plocation *plocation_elem_new(int number)
{
  auto *out = new plocation;

  out->kind = PADR_ELEMENT;
  out->number = number;
  out->sub_location = nullptr;

  return out;
}

/**
   Give textual description of the location. This might return address of
   a static buffer next call reuses, so don't expect result to be valid
   over another call to this.
 */
const char *plocation_name(const struct plocation *loc)
{
  static char locname[10];

  if (loc == nullptr) {
    return "No location";
  }

  switch (loc->kind) {
  case PADR_FIELD:
    return loc->name;
  case PADR_ELEMENT:
    fc_snprintf(locname, sizeof(locname), "%d", loc->number);
    return locname;
  }

  return "Illegal location";
}
