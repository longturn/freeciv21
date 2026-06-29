// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

struct connection;

// generated
#include <packets_gen.h>

// utility
#include "log.h"
#include "shared.h" // MAX_LEN_ADDR

// common
#include "fc_types.h"
#include "player.h"
#include "traderoutes.h"

// std
#include <array>
#include <memory>

// Qt
#include <QtContainerFwd> // QVector<QString>

using packet_capabilities_type = std::uint32_t;

class packet_handler {
public:
  virtual ~packet_handler() = default;
  virtual void *receive(struct connection *pconn) = 0;
  virtual int send(struct connection *pconn, const void *packet,
                   bool force_to_send) = 0;
};

using packet_handlers =
    std::array<std::unique_ptr<packet_handler>, PACKET_LAST>;

void *get_packet_from_connection(struct connection *pc,
                                 enum packet_type *ptype);

void remove_packet_from_buffer(struct socket_packet_buffer *buffer);

void send_attribute_block(const struct player *pplayer,
                          struct connection *pconn);
void generic_handle_player_attribute_chunk(
    struct player *pplayer,
    const struct packet_player_attribute_chunk *chunk);
void packet_handlers_fill_initial(packet_handlers &handlers);
void packet_handlers_fill_capability(packet_handlers &handlers,
                                     packet_capabilities_type capability);
const char *packet_name(enum packet_type type);
bool packet_has_game_info_flag(enum packet_type type);

void packet_header_init(struct packet_header *packet_header);
void post_send_packet_server_join_reply(
    struct connection *pconn, const struct packet_server_join_reply *packet);
void post_receive_packet_server_join_reply(
    struct connection *pconn, const struct packet_server_join_reply *packet);

void pre_send_packet_player_attribute_chunk(
    struct connection *pc, struct packet_player_attribute_chunk *packet);

packet_handlers packet_handlers_initial();
packet_handlers packet_handlers_get(packet_capabilities_type capability);

void packets_deinit();

// We need to cache pc->packet_header because it gets changed *while*
// serializing the packet.
#define SEND_PACKET_START(packet_type)                                      \
  QByteArray dout;                                                          \
  const auto packet_header = pc->packet_header;

#define SEND_PACKET_END(packet_type)                                        \
  {                                                                         \
    auto size = dout.size()                                                 \
                + data_type_size((enum data_type) packet_header.length)     \
                + data_type_size((enum data_type) packet_header.type);      \
    fc_assert(size <= MAX_LEN_PACKET);                                      \
                                                                            \
    QByteArray header;                                                      \
    dio_put_type_raw(header, (enum data_type) packet_header.length, size);  \
    dio_put_type_raw(header, (enum data_type) packet_header.type,           \
                     packet_type);                                          \
    return send_packet_data(pc, header + dout, packet_type);                \
  }

#define RECEIVE_PACKET_START(packet_type, result)                           \
  QByteArrayView din(                                                       \
      pc->buffer->data,                                                     \
      data_type_size((enum data_type) pc->packet_header.length));           \
  struct packet_type packet_buf, *result = &packet_buf;                     \
                                                                            \
  {                                                                         \
    int size;                                                               \
                                                                            \
    dio_get_type_raw(din, (enum data_type) pc->packet_header.length, size); \
    din = QByteArrayView(pc->buffer->data, MIN(size, pc->buffer->ndata));   \
  }                                                                         \
  din.slice(data_type_size((enum data_type) pc->packet_header.length)       \
            + data_type_size((enum data_type) pc->packet_header.type));

#define RECEIVE_PACKET_END(result)                                          \
  if (!packet_check(din, pc)) {                                             \
    return nullptr;                                                         \
  }                                                                         \
  remove_packet_from_buffer(pc->buffer);                                    \
  result = new std::remove_reference<decltype(*result)>::type;              \
  *result = packet_buf;                                                     \
  return result;

#define RECEIVE_PACKET_FIELD_ERROR(field, ...)                              \
  log_packet("Error on field '" #field "'" __VA_ARGS__);                    \
  return nullptr

int send_packet_data(struct connection *pc, QByteArrayView data,
                     enum packet_type packet_type);
bool packet_check(QByteArrayView din, struct connection *pc);

void packet_strvec_compute(char str[MAX_LEN_PACKET],
                           QVector<QString> *qstrvec);
QVector<QString> *packet_strvec_extract(const char *str);
void qstrvec_from_str(QVector<QString> *, char separator, const char *str);
