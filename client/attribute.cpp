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

#include "attribute.h"

// utility
#include "fcintl.h"
#include "log.h"

// common
#include "dataio_raw.h"
#include "packets.h"

// client
#include "client_main.h"

// Qt
#include <QByteArray>
#include <QHash>

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

typedef QHash<attr_key, QByteArray> attributeHash;
Q_GLOBAL_STATIC(attributeHash, attribute_hash)

/**
   Initializes the attribute module.
 */
void attribute_init() {}

/**
   Frees the attribute module.
 */
void attribute_free() { attribute_hash->clear(); }

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
  std::vector<int> value_lengths;
  value_lengths.resize(entries);

  /*
   * Step 1: loop through all keys and fill value_lengths and calculate
   * the total_length.
   */
  // preamble
  int total_length = 4 + 1 + 4 + 4;
  // body
  total_length += entries * (4 + 4 + 4 + 2 + 2); // value_size + key
  int i = 0;

  for (const auto &pvalue : std::as_const(*hash)) {
    value_lengths[i] = pvalue.size();
    total_length += value_lengths[i];
    i++;
  }

  /*
   * Step 2: prepare the buffer.
   */
  data.clear();

  /*
   * Step 3: fill out the preamble.
   */
  dio_put<std::uint32_t>(data, 0);
  dio_put<std::uint8_t>(data, 2);
  dio_put<std::uint32_t>(data, hash->size());
  dio_put<std::uint32_t>(data, total_length);

  /*
   * Step 4: fill out the body.
   */
  i = 0;

  attributeHash::const_iterator it = hash->constBegin();
  while (it != hash->constEnd()) {
    dio_put<std::uint32_t>(data, value_lengths[i]);

    dio_put<std::uint32_t>(data, it.key().key);
    dio_put<std::uint32_t>(data, it.key().id);
    dio_put<std::int16_t>(data, it.key().x);
    dio_put<std::int16_t>(data, it.key().y);

    dio_put(data, reinterpret_cast<const std::byte *>(it.value().data()),
            value_lengths[i]);
    i++;
    ++it;
  }

  fc_assert_msg(data.size() == total_length,
                "serialize_hash() total_length = %lu, actual = %lu",
                (long unsigned) total_length, (long unsigned) data.size());

  /*
   * Step 5: return.
   */
  qDebug("attribute.cpp serialize_hash() "
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

  hash->clear();

  QByteArrayView din(data);

  fc_assert_ret_val(dio_get<std::uint32_t>(din, dummy), A_SERIAL_FAIL);
  if (dummy != 0) {
    qDebug("attribute.cpp unserialize_hash() preamble, uint32 %lu != 0",
           static_cast<long unsigned>(dummy));
    return A_SERIAL_OLD;
  }
  fc_assert_ret_val(dio_get<std::uint8_t>(din, dummy), A_SERIAL_FAIL);
  if (dummy != 2) {
    qDebug("attribute.cpp unserialize_hash() preamble, "
           "uint8 %lu != 2 version",
           static_cast<long unsigned>(dummy));
    return A_SERIAL_OLD;
  }
  fc_assert_ret_val(dio_get<std::uint32_t>(din, entries), A_SERIAL_FAIL);
  fc_assert_ret_val(dio_get<std::uint32_t>(din, dummy), A_SERIAL_FAIL);
  if (dummy != data.size()) {
    qDebug("attribute.cpp unserialize_hash() preamble, "
           "uint32 %lu != %lu data_length",
           static_cast<long unsigned>(dummy),
           static_cast<long unsigned>(data.size()));
    return A_SERIAL_FAIL;
  }

  qDebug("attribute.cpp unserialize_hash() "
         "uint32 %lu entries, %lu data_length",
         (long unsigned) entries, (long unsigned) data.size());

  for (i = 0; i < entries; i++) {
    attr_key key;
    int value_length;
    if (!dio_get<std::int32_t>(din, value_length)) {
      qDebug("attribute.cpp unserialize_hash() "
             "uint32 value_length dio_input_too_short");
      return A_SERIAL_FAIL;
    }
    qDebug("attribute.cpp unserialize_hash() "
           "uint32 %lu value_length",
           (long unsigned) value_length);

    // next 12 bytes
    if (!dio_get<std::uint32_t>(din, key.key)
        || !dio_get<std::uint32_t>(din, key.id)
        || !dio_get<std::int16_t>(din, key.x)
        || !dio_get<std::int16_t>(din, key.y)) {
      qDebug("attribute.cpp unserialize_hash() "
             "uint32 key dio_input_too_short");
      return A_SERIAL_FAIL;
    }

    if (din.size() != value_length) {
      qDebug("attribute.cpp unserialize_hash() inconsistent size");
      return A_SERIAL_FAIL;
    }

    if (hash->contains(key)) {
      /* There are some untraceable attribute bugs caused by the CMA that
       * can cause this to happen. I think the only safe thing to do is
       * to delete all attributes. Another symptom of the bug is the
       * value_length (above) is set to a random value, which can also
       * cause a bug. */
      hash->clear();
      return A_SERIAL_FAIL;
    }

    hash->insert(key, QByteArray(din));
  }

  if (!din.empty()) {
    /* This is not an error, as old clients sent overlong serialized
     * attributes pre gna bug #21295, and these will be hanging around
     * in savefiles forever. */
    qDebug("attribute.cpp unserialize_hash() "
           "ignored %lu trailing octets",
           (long unsigned) din.size());
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
void attribute_set(int key, int id, int x, int y, QByteArrayView data)
{
  class attr_key akey(key, id, x, y);

  fc_assert_ret(nullptr != attribute_hash);

  if (!data.isEmpty()) {
    attribute_hash->emplace(akey, data);
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
QByteArray attribute_get(int key, int id, int x, int y)
{
  fc_assert_ret_val(nullptr != attribute_hash, {});

  attr_key akey(key, id, x, y);

  if (!attribute_hash->contains(akey)) {
    qDebug("  not found");
    return {};
  }

  return attribute_hash->value(akey);
}

/**
   Set city related attribute
 */
void attr_city_set(enum attr_city what, int city_id, QByteArrayView data)
{
  attribute_set(what, city_id, -1, -1, data);
}

/**
   Get city related attribute
 */
QByteArray attr_city_get(enum attr_city what, int city_id)
{
  return attribute_get(what, city_id, -1, -1);
}
