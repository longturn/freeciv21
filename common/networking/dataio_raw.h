// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "support.h"

// std
#include <cstddef> // size_t

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

void dio_input_init(struct data_in *dout, const void *src, size_t src_size);
void dio_input_rewind(struct data_in *din);
size_t dio_input_remaining(struct data_in *din);
bool dio_input_skip(struct data_in *din, size_t size);

size_t data_type_size(enum data_type type);

// gets
bool dio_get_type_raw(struct data_in *din, enum data_type type, int *dest)
    fc__attribute((nonnull(3)));

bool dio_get_uint8_raw(struct data_in *din, int *dest)
    fc__attribute((nonnull(2)));
bool dio_get_uint16_raw(struct data_in *din, int *dest)
    fc__attribute((nonnull(2)));
bool dio_get_uint32_raw(struct data_in *din, int *dest)
    fc__attribute((nonnull(2)));

bool dio_get_sint8_raw(struct data_in *din, int *dest)
    fc__attribute((nonnull(2)));
bool dio_get_sint16_raw(struct data_in *din, int *dest)
    fc__attribute((nonnull(2)));
bool dio_get_sint32_raw(struct data_in *din, int *dest)
    fc__attribute((nonnull(2)));

bool dio_get_bool8_raw(struct data_in *din, bool *dest)
    fc__attribute((nonnull(2)));
bool dio_get_bool32_raw(struct data_in *din, bool *dest)
    fc__attribute((nonnull(2)));
bool dio_get_ufloat_raw(struct data_in *din, float *dest, int float_factor)
    fc__attribute((nonnull(2)));
bool dio_get_sfloat_raw(struct data_in *din, float *dest, int float_factor)
    fc__attribute((nonnull(2)));
bool dio_get_memory_raw(struct data_in *din, void *dest, size_t dest_size)
    fc__attribute((nonnull(2)));
bool dio_get_string_raw(struct data_in *din, char *dest,
                        size_t max_dest_size) fc__attribute((nonnull(2)));
bool dio_get_cm_parameter_raw(struct data_in *din,
                              struct cm_parameter *param)
    fc__attribute((nonnull(2)));
bool dio_get_worklist_raw(struct data_in *din, struct worklist *pwl)
    fc__attribute((nonnull(2)));
bool dio_get_unit_order_raw(struct data_in *din, struct unit_order *order)
    fc__attribute((nonnull(2)));
bool dio_get_requirement_raw(struct data_in *din, struct requirement *preq)
    fc__attribute((nonnull(2)));
bool dio_get_action_probability_raw(struct data_in *din,
                                    struct act_prob *aprob)
    fc__attribute((nonnull(2)));

// Should be a function but we need some macro magic.
#define DIO_BV_GET(pdin, bv)                                                \
  dio_get_memory_raw((pdin), (bv).vec, sizeof((bv).vec))

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

void dio_put_bool8_raw(struct raw_data_out *dout, bool value);
void dio_put_bool32_raw(struct raw_data_out *dout, bool value);
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
  dio_put_memory_raw((pdout), (bv).vec, sizeof((bv).vec))

#define DIO_PUT(f, d, ...) dio_put_##f##_raw(d, ##__VA_ARGS__)
