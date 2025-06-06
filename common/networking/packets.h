// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

struct connection;
struct data_in;

// generated
#include <packets_gen.h>

// utility
#include "log.h"
#include "shared.h" // MAX_LEN_ADDR

// common
#include "connection.h"
#include "fc_types.h"
#include "player.h"
#include "traderoutes.h"

// Qt
#include <QtContainerFwd> // QVector<QString>

struct packet_handlers {
  union {
    int (*no_packet)(struct connection *pconn);
    int (*packet)(struct connection *pconn, const void *packet);
    int (*force_to_send)(struct connection *pconn, const void *packet,
                         bool force_to_send);
  } send[PACKET_LAST];
  void *(*receive[PACKET_LAST])(struct connection *pconn);
};

void *get_packet_from_connection_raw(struct connection *pc,
                                     enum packet_type *ptype);

#define get_packet_from_connection(pc, ptype)                               \
  get_packet_from_connection_raw(pc, ptype)

void remove_packet_from_buffer(struct socket_packet_buffer *buffer);

void send_attribute_block(const struct player *pplayer,
                          struct connection *pconn);
void generic_handle_player_attribute_chunk(
    struct player *pplayer,
    const struct packet_player_attribute_chunk *chunk);
void packet_handlers_fill_initial(struct packet_handlers *phandlers);
void packet_handlers_fill_capability(struct packet_handlers *phandlers,
                                     const char *capability);
const char *packet_name(enum packet_type type);
bool packet_has_game_info_flag(enum packet_type type);

void packet_header_init(struct packet_header *packet_header);
void post_send_packet_server_join_reply(
    struct connection *pconn, const struct packet_server_join_reply *packet);
void post_receive_packet_server_join_reply(
    struct connection *pconn, const struct packet_server_join_reply *packet);

void pre_send_packet_player_attribute_chunk(
    struct connection *pc, struct packet_player_attribute_chunk *packet);

const struct packet_handlers *packet_handlers_initial();
const struct packet_handlers *packet_handlers_get(const char *capability);

void packets_deinit();

#define SEND_PACKET_START(packet_type)                                      \
  unsigned char buffer[MAX_LEN_PACKET];                                     \
  struct raw_data_out dout;                                                 \
                                                                            \
  dio_output_init(&dout, buffer, sizeof(buffer));                           \
  dio_put_type_raw(&dout, (enum data_type) pc->packet_header.length, 0);    \
  dio_put_type_raw(&dout, (enum data_type) pc->packet_header.type,          \
                   packet_type);

#define SEND_PACKET_END(packet_type)                                        \
  {                                                                         \
    size_t size = dio_output_used(&dout);                                   \
                                                                            \
    dio_output_rewind(&dout);                                               \
    dio_put_type_raw(&dout, (enum data_type) pc->packet_header.length,      \
                     size);                                                 \
    fc_assert(!dout.too_short);                                             \
    return send_packet_data(pc, buffer, size, packet_type);                 \
  }

#define RECEIVE_PACKET_START(packet_type, result)                           \
  struct data_in din;                                                       \
  struct packet_type packet_buf, *result = &packet_buf;                     \
                                                                            \
  dio_input_init(                                                           \
      &din, pc->buffer->data,                                               \
      data_type_size((enum data_type) pc->packet_header.length));           \
  {                                                                         \
    int size;                                                               \
                                                                            \
    dio_get_type_raw(&din, (enum data_type) pc->packet_header.length,       \
                     &size);                                                \
    dio_input_init(&din, pc->buffer->data, MIN(size, pc->buffer->ndata));   \
  }                                                                         \
  dio_input_skip(                                                           \
      &din, (data_type_size((enum data_type) pc->packet_header.length)      \
             + data_type_size((enum data_type) pc->packet_header.type)));

#define RECEIVE_PACKET_END(result)                                          \
  if (!packet_check(&din, pc)) {                                            \
    return nullptr;                                                         \
  }                                                                         \
  remove_packet_from_buffer(pc->buffer);                                    \
  result = new std::remove_reference<decltype(*result)>::type;              \
  *result = packet_buf;                                                     \
  return result;

#define RECEIVE_PACKET_FIELD_ERROR(field, ...)                              \
  log_packet("Error on field '" #field "'" __VA_ARGS__);                    \
  return nullptr

int send_packet_data(struct connection *pc, unsigned char *data, int len,
                     enum packet_type packet_type);
bool packet_check(struct data_in *din, struct connection *pc);

void packet_strvec_compute(char str[MAX_LEN_PACKET],
                           QVector<QString> *qstrvec);
QVector<QString> *packet_strvec_extract(const char *str);
void qstrvec_from_str(QVector<QString> *, char separator, const char *str);
