// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "packets.h"

// generated
#include <packets_gen.h>

// utility
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// commmon
#include "connection.h"
#include "dataio_raw.h"
#include "fc_types.h"
#include "game.h"
#include "player.h"

// Qt
#include <QByteArrayAlgorithms> // qstrlen, qstrdup, qstrncpy
#include <QGlobalStatic>        // Q_GLOBAL_STATIC
#include <QRegularExpression>
#include <QScopedArrayPointer>
#include <QString>
#include <QtContainerFwd>        // QVector<QString>
#include <QtLogging>             // qDebug, qWarning, qCricital, etc
#include <QtPreprocessorSupport> // Q_UNUSED

// std
#include <cstdlib> // EXIT_FAILURE, free, at_quick_exit
#include <cstring> // str*, mem*
#include <utility> // std:move, std::as_const
#include <zconf.h> // uLongf, Bytef
#include <zlib.h>  // Z_*

/*
 * Value for the 16bit size to indicate a jumbo packet
 */
#define JUMBO_SIZE 0xffff

/*
 * All values 0<=size<COMPRESSION_BORDER are uncompressed packets.
 */
#define COMPRESSION_BORDER (16 * 1024 + 1)
#define PACKET_STRVEC_SEPARATOR '\3'
/*
 * All compressed packets this size or greater are sent as a jumbo packet.
 */
#define JUMBO_BORDER (64 * 1024 - COMPRESSION_BORDER - 1)

#define log_compress log_debug
#define log_compress2 log_debug

#define MAX_DECOMPRESSION 400

/*
 * Valid values are 0, 1 and 2. For 2 you have to set generate_stats
 * to 1 in generate_packets.py.
 */
#define PACKET_SIZE_STATISTICS 0

extern "C" const char *const packet_functional_capability;

typedef QHash<QString, struct packet_handlers *> packetsHash;
Q_GLOBAL_STATIC(packetsHash, packet_handlers_hash)

static int stat_size_alone = 0;
static int stat_size_uncompressed = 0;
static int stat_size_compressed = 0;
static int stat_size_no_compression = 0;

/**
   Returns the compression level. Initilialize it if needed.
 */
static inline int get_compression_level()
{
  static int level = -2; // Magic not initialized, see below.

  if (-2 == level) {
    const char *s = getenv("FREECIV_COMPRESSION_LEVEL");

    if (nullptr == s || !str_to_int(s, &level) || -1 > level || 9 < level) {
      level = -1;
    }
  }

  return level;
}

/**
   Send all waiting data. Return TRUE on success.
 */
static bool conn_compression_flush(struct connection *pconn)
{
  int compression_level = get_compression_level();
  uLongf compressed_size = 12 + 1.001 * pconn->compression.queue.size;
  int error;
  QScopedArrayPointer<Bytef> compressed(new Bytef[compressed_size]);
  bool jumbo;
  unsigned long compressed_packet_len;

  error = compress2(compressed.data(), &compressed_size,
                    pconn->compression.queue.p,
                    pconn->compression.queue.size, compression_level);
  fc_assert_ret_val(error == Z_OK, false);

  /* Compression signalling currently assumes a 2-byte packet length; if that
   * changes, the protocol should probably be changed */
  fc_assert_ret_val(
      data_type_size(data_type(pconn->packet_header.length)) == 2, false);

  // Include normal length field in decision
  jumbo = (compressed_size + 2 >= JUMBO_BORDER);

  compressed_packet_len = compressed_size + (jumbo ? 6 : 2);
  if (compressed_packet_len < pconn->compression.queue.size) {
    struct raw_data_out dout;

    log_compress("COMPRESS: compressed %lu bytes to %ld (level %d)",
                 (unsigned long) pconn->compression.queue.size,
                 compressed_size, compression_level);
    stat_size_uncompressed += pconn->compression.queue.size;
    stat_size_compressed += compressed_size;

    if (!jumbo) {
      unsigned char header[2];
      FC_STATIC_ASSERT(COMPRESSION_BORDER > MAX_LEN_PACKET,
                       uncompressed_compressed_packet_len_overlap);

      log_compress("COMPRESS: sending %ld as normal", compressed_size);

      dio_output_init(&dout, header, sizeof(header));
      dio_put_uint16_raw(&dout, 2 + compressed_size + COMPRESSION_BORDER);
      connection_send_data(pconn, header, sizeof(header));
      connection_send_data(pconn, compressed.data(), compressed_size);
    } else {
      unsigned char header[6];
      FC_STATIC_ASSERT(JUMBO_SIZE >= JUMBO_BORDER + COMPRESSION_BORDER,
                       compressed_normal_jumbo_packet_len_overlap);

      log_compress("COMPRESS: sending %ld as jumbo", compressed_size);
      dio_output_init(&dout, header, sizeof(header));
      dio_put_uint16_raw(&dout, JUMBO_SIZE);
      dio_put_uint32_raw(&dout, 6 + compressed_size);
      connection_send_data(pconn, header, sizeof(header));
      connection_send_data(pconn, compressed.data(), compressed_size);
    }
  } else {
    log_compress("COMPRESS: would enlarge %lu bytes to %ld; "
                 "sending uncompressed",
                 (unsigned long) pconn->compression.queue.size,
                 compressed_packet_len);
    connection_send_data(pconn, pconn->compression.queue.p,
                         pconn->compression.queue.size);
    stat_size_no_compression += pconn->compression.queue.size;
  }
  return pconn->used;
}

/**
   Thaw the connection. Then maybe compress the data waiting to send them
   to the connection. Returns TRUE on success. See also
   conn_compression_freeze().
 */
bool conn_compression_thaw(struct connection *pconn)
{
  pconn->compression.frozen_level--;
  fc_assert_action_msg(pconn->compression.frozen_level >= 0,
                       pconn->compression.frozen_level = 0,
                       "Too many calls to conn_compression_thaw on %s!",
                       conn_description(pconn));
  if (0 == pconn->compression.frozen_level) {
    return conn_compression_flush(pconn);
  }
  return pconn->used;
}

/**
   It returns the request id of the outgoing packet (or 0 if is_server()).
 */
int send_packet_data(struct connection *pc, unsigned char *data, int len,
                     enum packet_type packet_type)
{
  // default for the server
  int result = 0;

  log_packet("sending packet type=%s(%d) len=%d to %s",
             packet_name(packet_type), packet_type, len,
             is_server() ? pc->username : "server");

  if (!is_server()) {
    pc->client.last_request_id_used =
        get_next_request_id(pc->client.last_request_id_used);
    result = pc->client.last_request_id_used;
    log_packet("sending request %d", result);
  }

  if (pc->outgoing_packet_notify) {
    pc->outgoing_packet_notify(pc, packet_type, len, result);
  }

  int size = len;
  if (conn_compression_frozen(pc)) {
    size_t old_size;

    /* Keep this a decent amount less than MAX_LEN_BUFFER to avoid the
     * (remote) possibility of trying to dump MAX_LEN_BUFFER to the
     * network in one go */
#define MAX_LEN_COMPRESS_QUEUE (MAX_LEN_BUFFER / 2)
    FC_STATIC_ASSERT(MAX_LEN_COMPRESS_QUEUE < MAX_LEN_BUFFER,
                     compress_queue_maxlen_too_big);

    /* If this packet would cause us to overfill the queue, flush
     * everything that's in there already before queuing this one */
    if (MAX_LEN_COMPRESS_QUEUE
        < byte_vector_size(&pc->compression.queue) + len) {
      log_compress2("COMPRESS: huge queue, forcing to flush (%lu/%lu)",
                    (long unsigned) byte_vector_size(&pc->compression.queue),
                    (long unsigned) MAX_LEN_COMPRESS_QUEUE);
      if (!conn_compression_flush(pc)) {
        return -1;
      }
      byte_vector_reserve(&pc->compression.queue, 0);
    }

    old_size = byte_vector_size(&pc->compression.queue);
    byte_vector_reserve(&pc->compression.queue, old_size + len);
    memcpy(pc->compression.queue.p + old_size, data, len);
    log_compress2("COMPRESS: putting %s into the queue",
                  packet_name(packet_type));
  } else {
    stat_size_alone += size;
    log_compress("COMPRESS: sending %s alone (%d bytes total)",
                 packet_name(packet_type), stat_size_alone);
    connection_send_data(pc, data, len);
  }
  log_compress2("COMPRESS: STATS: alone=%d compression-expand=%d "
                "compression (before/after) = %d/%d",
                stat_size_alone, stat_size_no_compression,
                stat_size_uncompressed, stat_size_compressed);
  log_compress2("COMPRESS: STATS: alone=%d compression-expand=%d "
                "compression (before/after) = %d/%d",
                stat_size_alone, stat_size_no_compression,
                stat_size_uncompressed, stat_size_compressed);

#if PACKET_SIZE_STATISTICS
  {
    static struct {
      int counter;
      int size;
    } packets_stats[PACKET_LAST];
    static int packet_counter = 0;
    static int last_start_turn_seen = -1;
    static bool start_turn_seen = FALSE;

    int size = len;
    bool print = FALSE;
    bool clear = FALSE;

    if (!packet_counter) {
      int i;

      for (i = 0; i < PACKET_LAST; i++) {
        packets_stats[i].counter = 0;
        packets_stats[i].size = 0;
      }
    }

    packets_stats[packet_type].counter++;
    packets_stats[packet_type].size += size;

    packet_counter++;
    if (packet_type == PACKET_BEGIN_TURN
        && last_start_turn_seen != game.info.turn) {
      start_turn_seen = TRUE;
      last_start_turn_seen = game.info.turn;
    }

    if ((packet_type == PACKET_PROCESSING_FINISHED
         || packet_type == PACKET_THAW_CLIENT)
        && start_turn_seen) {
      start_turn_seen = FALSE;
      print = TRUE;
      clear = TRUE;
    }

    if (print) {
      int i, sum = 0;

#if PACKET_SIZE_STATISTICS == 2
      delta_stats_report();
#endif
      printf("Transmitted packets:\n");
      printf("%8s %8s %8s %s", "Packets", "Bytes", "Byt/Pac", "Name\n");

      for (i = 0; i < PACKET_LAST; i++) {
        if (packets_stats[i].counter == 0) {
          continue;
        }
        sum += packets_stats[i].size;
        printf("%8d %8d %8d %s(%i)\n", packets_stats[i].counter,
               packets_stats[i].size,
               packets_stats[i].size / packets_stats[i].counter,
               packet_name(static_cast<enum packet_type>(i)), i);
      }
      printf("turn=%d; transmitted %d bytes in %d packets;average size "
             "per packet %d bytes\n",
             game.info.turn, sum, packet_counter, sum / packet_counter);
      printf("turn=%d; transmitted %d bytes\n", game.info.turn,
             pc->statistics.bytes_send);
    }
    if (clear) {
      int i;

      for (i = 0; i < PACKET_LAST; i++) {
        packets_stats[i].counter = 0;
        packets_stats[i].size = 0;
      }
      packet_counter = 0;
      pc->statistics.bytes_send = 0;
      delta_stats_reset();
    }
  }
#endif // PACKET_SIZE_STATISTICS

  return result;
}

/**
   Read and return a packet from the connection 'pc'. The type of the
   packet is written in 'ptype'. On error, the connection is closed and
   the function returns nullptr.
 */
void *get_packet_from_connection_raw(struct connection *pc,
                                     enum packet_type *ptype)
{
  int len_read;
  int whole_packet_len;
  struct {
    enum packet_type type;
    int itype;
  } utype;
  struct data_in din;
  bool compressed_packet = false;
  int header_size = 0;
  void *data;
  void *(*receive_handler)(struct connection *);

  if (!pc->used) {
    return nullptr; // connection was closed, stop reading
  }

  if (pc->buffer->ndata
      < data_type_size(data_type(pc->packet_header.length))) {
    // Not got enough for a length field yet
    return nullptr;
  }

  dio_input_init(&din, pc->buffer->data, pc->buffer->ndata);
  dio_get_type_raw(&din, data_type(pc->packet_header.length), &len_read);

  // The non-compressed case
  whole_packet_len = len_read;

  /* Compression signalling currently assumes a 2-byte packet length; if that
   * changes, the protocol should probably be changed */
  fc_assert(data_type_size(data_type(pc->packet_header.length)) == 2);
  if (len_read == JUMBO_SIZE) {
    compressed_packet = true;
    header_size = 6;
    if (dio_input_remaining(&din) >= 4) {
      dio_get_uint32_raw(&din, &whole_packet_len);
      log_compress("COMPRESS: got a jumbo packet of size %d",
                   whole_packet_len);
    } else {
      // to return nullptr below
      whole_packet_len = 6;
    }
  } else if (len_read >= COMPRESSION_BORDER) {
    compressed_packet = true;
    header_size = 2;
    whole_packet_len = len_read - COMPRESSION_BORDER;
    log_compress("COMPRESS: got a normal packet of size %d",
                 whole_packet_len);
  }

  if (static_cast<unsigned>(whole_packet_len) > pc->buffer->ndata) {
    return nullptr; // not all data has been read
  }

  if (whole_packet_len < header_size) {
    qDebug("The packet size is reported to be less than header alone. "
           "The connection will be closed now.");
    connection_close(pc, _("illegal packet size"));

    return nullptr;
  }

  if (compressed_packet) {
    uLong compressed_size = whole_packet_len - header_size;
    int decompress_factor = 80;
    unsigned long int decompressed_size =
        decompress_factor * compressed_size;
    int error = Z_DATA_ERROR;
    struct socket_packet_buffer *buffer = pc->buffer;
    void *decompressed = fc_malloc(decompressed_size);

    do {
      error =
          uncompress(static_cast<Bytef *>(decompressed), &decompressed_size,
                     static_cast<const Bytef *>(
                         ADD_TO_POINTER(buffer->data, header_size)),
                     compressed_size);

      if (error == Z_DATA_ERROR) {
        decompress_factor += 50;
        decompressed_size = decompress_factor * compressed_size;
        decompressed = fc_realloc(decompressed, decompressed_size);
      }

      if (error != Z_OK) {
        if (error != Z_DATA_ERROR || decompress_factor > MAX_DECOMPRESSION) {
          qDebug("Uncompressing of the packet stream failed. "
                 "The connection will be closed now.");
          connection_close(pc, _("decoding error"));
          free(decompressed);
          return nullptr;
        }
      }
    } while (error != Z_OK);

    buffer->ndata -= whole_packet_len;
    /*
     * Remove the packet with the compressed data and shift all the
     * remaining data to the front.
     */
    memmove(buffer->data, buffer->data + whole_packet_len, buffer->ndata);

    if (buffer->ndata + decompressed_size > buffer->nsize) {
      buffer->nsize += decompressed_size;
      buffer->data = static_cast<unsigned char *>(
          fc_realloc(buffer->data, buffer->nsize));
    }

    /*
     * Make place for the uncompressed data by moving the remaining
     * data.
     */
    memmove(buffer->data + decompressed_size, buffer->data, buffer->ndata);

    /*
     * Copy the uncompressed data.
     */
    memcpy(buffer->data, decompressed, decompressed_size);

    free(decompressed);

    buffer->ndata += decompressed_size;

    log_compress("COMPRESS: decompressed %ld into %ld", compressed_size,
                 decompressed_size);

    return get_packet_from_connection(pc, ptype);
  }

  /*
   * At this point the packet is a plain uncompressed one. These have
   * to have to be at least the header bytes in size.
   */
  if (whole_packet_len
      < (data_type_size(data_type(pc->packet_header.length))
         + data_type_size(data_type(pc->packet_header.type)))) {
    qDebug("The packet stream is corrupt. The connection "
           "will be closed now.");
    connection_close(pc, _("decoding error"));
    return nullptr;
  }

  dio_get_type_raw(&din, data_type(pc->packet_header.type), &utype.itype);
  utype.type = packet_type(utype.itype);

  if (utype.type >= PACKET_LAST
      || (receive_handler = pc->phs.handlers->receive[utype.type])
             == nullptr) {
    qDebug("Received unsupported packet type %d (%s). The connection "
           "will be closed now.",
           utype.type, packet_name(utype.type));
    connection_close(pc, _("unsupported packet type"));
    return nullptr;
  }

  log_packet("got packet type=(%s)%d len=%d from %s",
             packet_name(utype.type), utype.itype, whole_packet_len,
             is_server() ? pc->username : "server");

  *ptype = utype.type;

  if (pc->incoming_packet_notify) {
    pc->incoming_packet_notify(pc, utype.type, whole_packet_len);
  }

#if PACKET_SIZE_STATISTICS
  {
    static struct {
      int counter;
      int size;
    } packets_stats[PACKET_LAST];
    static int packet_counter = 0;

    int packet_type = utype.itype;
    int size = whole_packet_len;

    if (!packet_counter) {
      int i;

      for (i = 0; i < PACKET_LAST; i++) {
        packets_stats[i].counter = 0;
        packets_stats[i].size = 0;
      }
    }

    packets_stats[packet_type].counter++;
    packets_stats[packet_type].size += size;

    packet_counter++;
    if (packet_counter % 100 == 0) {
      int i, sum = 0;

      printf("Received packets:\n");
      for (i = 0; i < PACKET_LAST; i++) {
        if (packets_stats[i].counter == 0)
          continue;
        sum += packets_stats[i].size;
        printf("  [%-25.25s %3d]: %6d packets; %8d bytes total; "
               "%5d bytes/packet average\n",
               packet_name(static_cast<enum packet_type>(i)), i,
               packets_stats[i].counter, packets_stats[i].size,
               packets_stats[i].size / packets_stats[i].counter);
      }
      printf("received %d bytes in %d packets;average size "
             "per packet %d bytes\n",
             sum, packet_counter, sum / packet_counter);
    }
  }
#endif // PACKET_SIZE_STATISTICS
  data = receive_handler(pc);
  if (!data) {
    connection_close(pc, _("incompatible packet contents"));
    return nullptr;
  } else {
    return data;
  }
}

/**
   Remove the packet from the buffer
 */
void remove_packet_from_buffer(struct socket_packet_buffer *buffer)
{
  struct data_in din;
  int len;

  dio_input_init(&din, buffer->data, buffer->ndata);
  fc_assert_ret(dio_get_uint16_raw(&din, &len));
  memmove(buffer->data, buffer->data + len, buffer->ndata - len);
  buffer->ndata -= len;
  log_debug("remove_packet_from_buffer: remove %d; remaining %lu", len,
            buffer->ndata);
}

/**
   Set the packet header field lengths used for the login protocol,
   before the capability of the connection could be checked.

   NB: These values cannot be changed for backward compatibility reasons.
 */
void packet_header_init(struct packet_header *packet_header)
{
  packet_header->length = DIOT_UINT16;
  packet_header->type = DIOT_UINT8;
}

/**
   Set the packet header field lengths used after the login protocol,
   after the capability of the connection could be checked.
 */
static inline void packet_header_set(struct packet_header *packet_header)
{
  // Ensure we have values initialized in packet_header_init().
  fc_assert(packet_header->length == DIOT_UINT16);
  fc_assert(packet_header->type == DIOT_UINT8);

  packet_header->length = DIOT_UINT16;
  packet_header->type = DIOT_UINT16;
}

/**
   Modify if needed the packet header field lengths.
 */
void post_send_packet_server_join_reply(
    struct connection *pconn, const struct packet_server_join_reply *packet)
{
  if (packet->you_can_join) {
    packet_header_set(&pconn->packet_header);
  }
}

/**
   Modify if needed the packet header field lengths.
 */
void post_receive_packet_server_join_reply(
    struct connection *pconn, const struct packet_server_join_reply *packet)
{
  if (packet->you_can_join) {
    packet_header_set(&pconn->packet_header);
  }
}

/**
   Sanity check packet
 */
bool packet_check(struct data_in *din, struct connection *pc)
{
  size_t rem = dio_input_remaining(din);

  if (rem > 0) {
    int type, len;

    dio_input_rewind(din);
    dio_get_type_raw(din, data_type(pc->packet_header.length), &len);
    dio_get_type_raw(din, data_type(pc->packet_header.type), &type);

    log_packet("received long packet (type %d, len %d, rem %lu) from %s",
               type, len, static_cast<unsigned long>(rem),
               conn_description(pc));
    return false;
  }
  return true;
}

/**
  Updates pplayer->attribute_block according to the given packet.
 */
void generic_handle_player_attribute_chunk(
    struct player *pplayer,
    const struct packet_player_attribute_chunk *chunk)
{
  log_packet("received attribute chunk %u/%u %u",
             static_cast<unsigned int>(chunk->offset),
             static_cast<unsigned int>(chunk->total_length),
             static_cast<unsigned int>(chunk->chunk_length));

  fc_assert_ret(chunk->chunk_length < ATTRIBUTE_CHUNK_SIZE);

  if (chunk->total_length < 0 || chunk->chunk_length < 0
      || chunk->total_length >= MAX_ATTRIBUTE_BLOCK || chunk->offset < 0
      || chunk->offset
             > chunk->total_length // necessary check on 32 bit systems
      || chunk->chunk_length > chunk->total_length
      || chunk->offset + chunk->chunk_length > chunk->total_length
      || (chunk->offset != 0
          && chunk->total_length
                 != pplayer->attribute_block_buffer.size())) {
    // wrong attribute data
    pplayer->attribute_block_buffer.clear();
    qCritical("Received wrong attribute chunk");
    return;
  }
  // first one in a row
  if (chunk->offset == 0) {
    pplayer->attribute_block_buffer.resize(chunk->total_length);
  }
  memcpy(pplayer->attribute_block_buffer.data() + chunk->offset, chunk->data,
         chunk->chunk_length);

  if (chunk->offset + chunk->chunk_length == chunk->total_length) {
    // Received full attribute block
    pplayer->attribute_block = pplayer->attribute_block_buffer;
    pplayer->attribute_block_buffer.clear();
  }
}

/**
  Split the attribute block into chunks and send them over pconn.
 */
void send_attribute_block(const struct player *pplayer,
                          struct connection *pconn)
{
  struct packet_player_attribute_chunk packet;

  if (!pplayer || pplayer->attribute_block.isEmpty()) {
    return;
  }

  fc_assert_ret(pplayer->attribute_block.size() < MAX_ATTRIBUTE_BLOCK);

  auto chunks =
      (pplayer->attribute_block.size() - 1) / ATTRIBUTE_CHUNK_SIZE + 1;
  auto bytes_left = pplayer->attribute_block.size();

  connection_do_buffer(pconn);

  for (int current_chunk = 0; current_chunk < chunks; current_chunk++) {
    int size_of_current_chunk = MIN(bytes_left, ATTRIBUTE_CHUNK_SIZE - 1);

    packet.offset = (ATTRIBUTE_CHUNK_SIZE - 1) * current_chunk;
    packet.total_length = pplayer->attribute_block.size();
    packet.chunk_length = size_of_current_chunk;

    memcpy(packet.data, pplayer->attribute_block.data() + packet.offset,
           packet.chunk_length);
    bytes_left -= packet.chunk_length;

    if (packet.chunk_length < ATTRIBUTE_CHUNK_SIZE - 1) {
      /* Last chunk is not full. Make sure that delta does
       * not use random data. */
      memset(packet.data + packet.chunk_length, 0,
             ATTRIBUTE_CHUNK_SIZE - 1 - packet.chunk_length);
    }

    send_packet_player_attribute_chunk(pconn, &packet);
  }

  connection_do_unbuffer(pconn);
}

/**
   Test and log for sending player attribute_block
 */
void pre_send_packet_player_attribute_chunk(
    struct connection *pc, struct packet_player_attribute_chunk *packet)
{
  Q_UNUSED(pc)
  fc_assert(packet->total_length > 0
            && packet->total_length < MAX_ATTRIBUTE_BLOCK);
  // 500 bytes header, just to be sure
  fc_assert(packet->chunk_length > 0
            && packet->chunk_length < MAX_LEN_PACKET - 500);
  fc_assert(packet->chunk_length <= packet->total_length);
  fc_assert(packet->offset >= 0 && packet->offset < packet->total_length);

  log_packet("sending attribute chunk %d/%d %d", packet->offset,
             packet->total_length, packet->chunk_length);
}

/**
   Destroy the packet handler hash table.
 */
static void packet_handlers_free() {}

/**
   Returns the packet handlers variant with no special capability.
 */
const struct packet_handlers *packet_handlers_initial()
{
  static struct packet_handlers default_handlers;
  static bool initialized = false;

  if (!initialized) {
    memset(&default_handlers, 0, sizeof(default_handlers));
    packet_handlers_fill_initial(&default_handlers);
    initialized = true;
  }

  return &default_handlers;
}

/**
   Returns the packet handlers variant for 'capability'.
 */
const struct packet_handlers *packet_handlers_get(const char *capability)
{
  struct packet_handlers *phandlers;
  char functional_capability[MAX_LEN_CAPSTR] = "";
  QStringList tokens;

  fc_assert(strlen(capability) < sizeof(functional_capability));

  // Get functional network capability string.
  tokens = QString(capability).split(QRegularExpression("[ \t\n,]+"));
  tokens.sort();

  for (const auto &str : std::as_const(tokens)) {
    if (!has_capability(qUtf8Printable(str), packet_functional_capability)) {
      continue;
    }
    if (functional_capability[0] != '\0') {
      sz_strlcat(functional_capability, " ");
    }
    sz_strlcat(functional_capability, qUtf8Printable(str));
  }

  // Lookup handlers for the capabilities or create new handlers.
  if (!packet_handlers_hash->contains(functional_capability)) {
    phandlers = new struct packet_handlers;
    memcpy(phandlers, packet_handlers_initial(), sizeof(*phandlers));
    packet_handlers_fill_capability(phandlers, functional_capability);
    packet_handlers_hash->insert(functional_capability, phandlers);
  }
  phandlers = packet_handlers_hash->value(functional_capability);
  fc_assert(phandlers != nullptr);
  return phandlers;
}

/**
   Call when there is no longer a requirement for protocol processing.
   All connections must have been closed.
 */
void packets_deinit() { packet_handlers_free(); }

void packet_strvec_compute(char str[MAX_LEN_PACKET],
                           QVector<QString> *qstrvec)
{
  if (qstrvec == nullptr) {
    str[0] = '\0';
    return;
  }

  const auto joined =
      QStringList::fromVector(*qstrvec).join(PACKET_STRVEC_SEPARATOR);
  qstrncpy(str, qUtf8Printable(joined), MAX_LEN_PACKET);
}

QVector<QString> *packet_strvec_extract(const char *str)
{
  QVector<QString> *qstrvec = nullptr;
  if ('\0' != str[0]) {
    qstrvec = new QVector<QString>;
    qstrvec_from_str(qstrvec, PACKET_STRVEC_SEPARATOR, str);
  }
  return qstrvec;
}

/**
   Build the string vector from a string until 'str_size' bytes are read.
   Passing -1 for 'str_size' will assume 'str' as the expected format. Note
   it's a bit dangerous.

   This string format is a list of strings separated by 'separator'.

   See also strvec_to_str().
 */
void qstrvec_from_str(QVector<QString> *psv, char separator, const char *str)
{
  const char *p;
  char *new_str;

  psv->clear();
  while ((p = strchr(str, separator))) {
    new_str = new char[p - str + 1];
    memcpy(new_str, str, p - str);
    new_str[p - str] = '\0';
    psv->append(new_str);
    str = p + 1;
    delete[] new_str;
  }
  if ('\0' != *str) {
    psv->append(str);
  }
}
