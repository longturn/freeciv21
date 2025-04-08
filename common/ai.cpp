// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "ai.h"

// generated
#include <fc_config.h>

// utility
#include "fcintl.h"
#include "log.h" // fc_assert
#include "support.h"

// common
#include "player.h"

// Qt
#include <QMessageLogger>

// std
#include <cstring>

static struct ai_type ai_types[FREECIV_AI_MOD_LAST];

static int ai_type_count = 0;

/**
   Returns ai_type of given id.
 */
struct ai_type *get_ai_type(int id)
{
  fc_assert(id >= 0 && id < FREECIV_AI_MOD_LAST);

  return &ai_types[id];
}

/**
   Initializes AI structure.
 */
void init_ai(struct ai_type *ai) { memset(ai, 0, sizeof(*ai)); }

/**
   Returns id of the given ai_type.
 */
int ai_type_number(const struct ai_type *ai)
{
  int ainbr = ai - ai_types;

  fc_assert_ret_val(ainbr >= 0 && ainbr < FREECIV_AI_MOD_LAST, 0);

  return ainbr;
}

/**
   Find ai type with given name.
 */
struct ai_type *ai_type_by_name(const char *search)
{
  ai_type_iterate(ai)
  {
    if (!fc_strcasecmp(ai_name(ai), search)) {
      return ai;
    }
  }
  ai_type_iterate_end;

  return nullptr;
}

/**
   Return next free ai_type
 */
struct ai_type *ai_type_alloc()
{
  if (ai_type_count >= FREECIV_AI_MOD_LAST) {
    qCritical(_("Too many AI modules. Max is %d."), FREECIV_AI_MOD_LAST);

    return nullptr;
  }

  return get_ai_type(ai_type_count++);
}

/**
   Free latest ai_type
 */
void ai_type_dealloc() { ai_type_count--; }

/**
   Return number of ai types
 */
int ai_type_get_count() { return ai_type_count; }

/**
   Return the name of the ai type.
 */
const char *ai_name(const struct ai_type *ai)
{
  fc_assert_ret_val(ai, nullptr);
  return ai->name;
}

/**
   Return usable ai type name, if possible. This is either the name
   given as parameter or some fallback name for it. nullptr is returned if
   no name matches.
 */
const char *ai_type_name_or_fallback(const char *orig_name)
{
  if (orig_name == nullptr) {
    return nullptr;
  }

  if (ai_type_by_name(orig_name) != nullptr) {
    return orig_name;
  }

  auto fb = ai_type_by_name("classic");
  if (fb != nullptr) {
    qWarning(_("Unknown AI type %s, using \"classic\" instead"), orig_name);
    // Get pointer to persistent name of the ai_type
    return ai_name(fb);
  }

  return nullptr;
}
