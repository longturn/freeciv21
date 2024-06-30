/*       ,%%%%%%%,                 ***************************************
       ,%%/\%%%%/\%,         Copyright (c) 1996-2021 Freeciv21 and Freeciv
      ,%%%\c "" J/%%,        contributors. This file is part of Freeciv21.
      %%%%/ d  b \%%%  Freeciv21 is free software: you can redistribute it
      %%%%    _  |%%% and/or modify it under the terms of the GNU  General
      `%%%%(=_Y_=)%%'    Public License  as published by the Free Software
       `%%%%`\7/%%%'         Foundation, either version 3 of the  License,
         `%%%%%%%'       or (at your option) any later version. You should
            have received  a copy of the GNU  General Public License along
                 with Freeciv21. If not, see https://www.gnu.org/licenses/.
 */

#include <QHash>

// utility
#include "dataio.h"
#include "fcintl.h"
#include "genhash.h" // genhash_val_t
#include "log.h"

// common
#include "packets.h"

// client
#include "client_main.h"

#include "attribute.h"

#define log_attribute log_debug

enum attribute_serial {
  A_SERIAL_FAIL,
  A_SERIAL_OK,
  A_SERIAL_OLD,
};

class attr_key {
public:
  attr_key() = default;
  attr_key(int, int, int, int);
  int key{0}, id{0}, x{0}, y{0};
};

attr_key::attr_key(int k, int i, int x0, int y0)
    : key(k), id(i), x(x0), y(y0)
{
}

inline bool operator==(const attr_key &pkey1, const attr_key &pkey2)
{
  return pkey1.key == pkey2.key && pkey1.id == pkey2.id && pkey1.x == pkey2.x
         && pkey1.y == pkey2.y;
}

inline uint qHash(const attr_key &pkey, uint seed)
{
  return qHash(pkey.id, seed) ^ pkey.x ^ pkey.y ^ pkey.key;
}

typedef QHash<attr_key, void *> attributeHash;
Q_GLOBAL_STATIC(attributeHash, attribute_hash)

/**
   Initializes the attribute module.
 */
void attribute_init() {}

/**
   Frees the attribute module.
 */
void attribute_free()
{
  for (auto *at : qAsConst(*attribute_hash)) {
    ::operator delete(at);
  }
  attribute_hash->clear();
}

/**
   Serialize an attribute hash for network/storage.
 */
static enum attribute_serial serialize_hash(attributeHash *hash,
                                            QByteArray &data)
{
  /*
   * Layout of version 2:
   *
   * struct {
   *   uint32 0;   always != 0 in version 1
   *   uint8 2;
   *   uint32 entries;
   *   uint32 total_size_in_bytes;
   * } preamble;
   *
   * struct {
   *   uint32 value_size;
   *   char key[], char value[];
   * } body[entries];
   */
  const size_t entries = hash->size();
  int total_length;
  std::vector<int> value_lengths;
  value_lengths.resize(entries);
  struct raw_data_out dout;
  int i;

  /*
   * Step 1: loop through all keys and fill value_lengths and calculate
   * the total_length.
   */
  // preamble
  total_length = 4 + 1 + 4 + 4;
  // body
  total_length += entries * (4 + 4 + 4 + 2 + 2); // value_size + key
  i = 0;

  for (auto *pvalue : qAsConst(*hash)) {
    struct data_in din;

    dio_input_init(&din, pvalue, 4);
    fc_assert_ret_val(dio_get_uint32_raw(&din, &value_lengths[i]),
                      A_SERIAL_FAIL);

    total_length += value_lengths[i];
    i++;
  }

  /*
   * Step 2: allocate memory.
   */
  data.resize(total_length);
  dio_output_init(&dout, data.data(), total_length);

  /*
   * Step 3: fill out the preamble.
   */
  dio_put_uint32_raw(&dout, 0);
  dio_put_uint8_raw(&dout, 2);
  dio_put_uint32_raw(&dout, hash->size());
  dio_put_uint32_raw(&dout, total_length);

  /*
   * Step 4: fill out the body.
   */
  i = 0;

  attributeHash::const_iterator it = hash->constBegin();
  while (it != hash->constEnd()) {
    dio_put_uint32_raw(&dout, value_lengths[i]);

    dio_put_uint32_raw(&dout, it.key().key);
    dio_put_uint32_raw(&dout, it.key().id);
    dio_put_sint16_raw(&dout, it.key().x);
    dio_put_sint16_raw(&dout, it.key().y);

    dio_put_memory_raw(&dout, ADD_TO_POINTER(it.value(), 4),
                       value_lengths[i]);
    i++;
    ++it;
  }

  fc_assert(!dout.too_short);
  fc_assert_msg(dio_output_used(&dout) == total_length,
                "serialize_hash() total_length = %lu, actual = %lu",
                (long unsigned) total_length,
                (long unsigned) dio_output_used(&dout));

  /*
   * Step 5: return.
   */
  log_attribute("attribute.cpp serialize_hash() "
                "serialized %lu entries in %lu bytes",
                (long unsigned) entries, (long unsigned) total_length);
  return A_SERIAL_OK;
}

/**
   This data was serialized (above), sent as an opaque data packet to the
   server, stored in a savegame, retrieved from the savegame, sent as an
   opaque data packet back to the client, and now is ready to be restored.
   Check everything!
 */
static enum attribute_serial unserialize_hash(attributeHash *hash,
                                              const QByteArray &data)
{
  int entries, i, dummy;
  struct data_in din;

  hash->clear();

  dio_input_init(&din, data.constData(), data.size());

  fc_assert_ret_val(dio_get_uint32_raw(&din, &dummy), A_SERIAL_FAIL);
  if (dummy != 0) {
    qDebug("attribute.cpp unserialize_hash() preamble, uint32 %lu != 0",
           static_cast<long unsigned>(dummy));
    return A_SERIAL_OLD;
  }
  fc_assert_ret_val(dio_get_uint8_raw(&din, &dummy), A_SERIAL_FAIL);
  if (dummy != 2) {
    qDebug("attribute.cpp unserialize_hash() preamble, "
           "uint8 %lu != 2 version",
           static_cast<long unsigned>(dummy));
    return A_SERIAL_OLD;
  }
  fc_assert_ret_val(dio_get_uint32_raw(&din, &entries), A_SERIAL_FAIL);
  fc_assert_ret_val(dio_get_uint32_raw(&din, &dummy), A_SERIAL_FAIL);
  if (dummy != data.size()) {
    qDebug("attribute.cpp unserialize_hash() preamble, "
           "uint32 %lu != %lu data_length",
           static_cast<long unsigned>(dummy),
           static_cast<long unsigned>(data.size()));
    return A_SERIAL_FAIL;
  }

  log_attribute("attribute.cpp unserialize_hash() "
                "uint32 %lu entries, %lu data_length",
                (long unsigned) entries, (long unsigned) data.size());

  for (i = 0; i < entries; i++) {
    attr_key key;
    void *pvalue;
    int value_length;
    struct raw_data_out dout;

    if (!dio_get_uint32_raw(&din, &value_length)) {
      qDebug("attribute.cpp unserialize_hash() "
             "uint32 value_length dio_input_too_short");
      return A_SERIAL_FAIL;
    }
    log_attribute("attribute.cpp unserialize_hash() "
                  "uint32 %lu value_length",
                  (long unsigned) value_length);

    // next 12 bytes
    if (!dio_get_uint32_raw(&din, &key.key)
        || !dio_get_uint32_raw(&din, &key.id)
        || !dio_get_sint16_raw(&din, &key.x)
        || !dio_get_sint16_raw(&din, &key.y)) {
      qDebug("attribute.cpp unserialize_hash() "
             "uint32 key dio_input_too_short");
      return A_SERIAL_FAIL;
    }
    pvalue = new char[value_length + 4];

    dio_output_init(&dout, pvalue, value_length + 4);
    dio_put_uint32_raw(&dout, value_length);
    if (!dio_get_memory_raw(&din, ADD_TO_POINTER(pvalue, 4), value_length)) {
      qDebug("attribute.cpp unserialize_hash() "
             "memory dio_input_too_short");
      return A_SERIAL_FAIL;
    }

    if (hash->contains(key)) {
      /* There are some untraceable attribute bugs caused by the CMA that
       * can cause this to happen. I think the only safe thing to do is
       * to delete all attributes. Another symptom of the bug is the
       * value_length (above) is set to a random value, which can also
       * cause a bug. */
      ::operator delete[](pvalue);
      hash->clear();
      return A_SERIAL_FAIL;
    }

    hash->insert(key, pvalue);
  }

  if (dio_input_remaining(&din) > 0) {
    /* This is not an error, as old clients sent overlong serialized
     * attributes pre gna bug #21295, and these will be hanging around
     * in savefiles forever. */
    log_attribute("attribute.cpp unserialize_hash() "
                  "ignored %lu trailing octets",
                  (long unsigned) dio_input_remaining(&din));
  }

  return A_SERIAL_OK;
}

/**
   Send current state to the server. Note that the current
   implementation will send all attributes to the server.
 */
void attribute_flush()
{
  struct player *pplayer = client_player();

  if (!pplayer || client_is_observer() || !pplayer->is_alive) {
    return;
  }

  if (0 == attribute_hash->size()) {
    return;
  }

  pplayer->attribute_block.clear();
  serialize_hash(attribute_hash, pplayer->attribute_block);
  send_attribute_block(pplayer, &client.conn);
}

/**
   Recreate the attribute set from the player's
   attribute_block. Shouldn't be used by normal code.
 */
void attribute_restore()
{
  struct player *pplayer = client_player();

  if (!pplayer) {
    return;
  }

  fc_assert_ret(attribute_hash != nullptr);

  switch (unserialize_hash(attribute_hash, pplayer->attribute_block)) {
  case A_SERIAL_FAIL:
    qCritical(_("There has been a CMA error. "
                "Your citizen governor settings may be broken."));
    break;
  case A_SERIAL_OLD:
    qInfo(_("Old attributes detected and removed."));
    break;
  default:
    break;
  };
}

/**
   Low-level function to set an attribute.  If data_length is zero the
   attribute is removed.
 */
void attribute_set(int key, int id, int x, int y, size_t data_length,
                   const void *const data)
{
  class attr_key akey(key, id, x, y);

  log_attribute("attribute_set(key = %d, id = %d, x = %d, y = %d, "
                "data_length = %lu, data = %p)",
                key, id, x, y, (long unsigned) data_length, data);

  fc_assert_ret(nullptr != attribute_hash);

  if (0 != data_length) {
    void *pvalue = new char[data_length + 4];
    struct raw_data_out dout;

    dio_output_init(&dout, pvalue, data_length + 4);
    dio_put_uint32_raw(&dout, data_length);
    dio_put_memory_raw(&dout, data, data_length);

    attribute_hash->insert(akey, pvalue);
  } else {
    attribute_hash->remove(akey);
  }

  // Sync with the server
  attribute_flush();
}

/**
   Low-level function to get an attribute. If data hasn't enough space
   to hold the attribute data isn't set to the attribute. Returns the
   actual size of the attribute. Can be zero if the attribute is
   unset. To get the size of an attribute use
     size = attribute_get(key, id, x, y, 0, nullptr)
 */
size_t attribute_get(int key, int id, int x, int y, size_t max_data_length,
                     void *data)
{
  attr_key akey(key, id, x, y);
  int length;
  struct data_in din;

  log_attribute("attribute_get(key = %d, id = %d, x = %d, y = %d, "
                "max_data_length = %lu, data = %p)",
                key, id, x, y, (long unsigned) max_data_length, data);

  fc_assert_ret_val(nullptr != attribute_hash, 0);

  if (!attribute_hash->contains(akey)) {
    log_attribute("  not found");
    return 0;
  }

  auto *pvalue = attribute_hash->value(akey);

  dio_input_init(&din, pvalue, 0xffffffff);
  fc_assert_ret_val(dio_get_uint32_raw(&din, &length), 0);

  if (length <= max_data_length) {
    dio_get_memory_raw(&din, data, length);
  }

  log_attribute("  found length = %d", length);
  return length;
}

/**
   Set city related attribute
 */
void attr_city_set(enum attr_city what, int city_id, size_t data_length,
                   const void *const data)
{
  attribute_set(what, city_id, -1, -1, data_length, data);
}

/**
   Get city related attribute
 */
size_t attr_city_get(enum attr_city what, int city_id,
                     size_t max_data_length, void *data)
{
  return attribute_get(what, city_id, -1, -1, max_data_length, data);
}
