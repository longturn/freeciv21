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

// utility
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "game.h"
#include "nation.h"
#include "player.h"
#include "tech.h"

#include "government.h"

std::vector<government> governments;
/**
   Returns the government that has the given (translated) name.
   Returns nullptr if none match.
 */
struct government *government_by_translated_name(const char *name)
{
  for (auto &gov : governments) {
    if (0 == strcmp(government_name_translation(&gov), name)) {
      return &gov;
    }
  }

  return nullptr;
}

/**
   Returns the government that has the given (untranslated) rule name.
   Returns nullptr if none match.
 */
struct government *government_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  for (auto &gov : governments) {
    if (0 == fc_strcasecmp(government_rule_name(&gov), qname)) {
      return &gov;
    }
  };

  return nullptr;
}

/**
   Return the number of governments.
 */
Government_type_id government_count()
{
  return game.control.government_count;
}

/**
   Return the government index.

   Currently same as government_number(), paired with government_count()
   indicates use as an array index.
 */
Government_type_id government_index(const struct government *pgovern)
{
  fc_assert_ret_val(nullptr != pgovern, 0);
  return pgovern - &governments[0];
}

/**
   Return the government index.
 */
Government_type_id government_number(const struct government *pgovern)
{
  fc_assert_ret_val(nullptr != pgovern, 0);
  return pgovern->item_number;
}

/**
   Return the government with the given index.

   This function returns nullptr for an out-of-range index (some callers
   rely on this).
 */
struct government *government_by_number(const Government_type_id gov)
{
  if (gov < 0 || gov >= game.control.government_count) {
    return nullptr;
  }
  return &governments[gov];
}

/**
   Return the government of a player.
 */
struct government *government_of_player(const struct player *pplayer)
{
  fc_assert_ret_val(nullptr != pplayer, nullptr);
  return pplayer->government;
}

/**
   Return the government of the player who owns the city.
 */
struct government *government_of_city(const struct city *pcity)
{
  fc_assert_ret_val(nullptr != pcity, nullptr);
  return government_of_player(city_owner(pcity));
}

/**
   Return the (untranslated) rule name of the government.
   You don't have to free the return pointer.
 */
const char *government_rule_name(const struct government *pgovern)
{
  fc_assert_ret_val(nullptr != pgovern, nullptr);
  return rule_name_get(&pgovern->name);
}

/**
   Return the (translated) name of the given government.
   You don't have to free the return pointer.
 */
const char *government_name_translation(const struct government *pgovern)
{
  fc_assert_ret_val(nullptr != pgovern, nullptr);

  return name_translation_get(&pgovern->name);
}

/**
   Return the (translated) name of the given government of a player.
   You don't have to free the return pointer.
 */
const char *government_name_for_player(const struct player *pplayer)
{
  return government_name_translation(government_of_player(pplayer));
}

/**
   Can change to government if appropriate tech exists, and one of:
    - no required tech (required is A_NONE)
    - player has required tech
    - we have an appropriate wonder
   Returns FALSE if pplayer is nullptr (used for observers).
 */
bool can_change_to_government(struct player *pplayer,
                              const struct government *gov)
{
  fc_assert_ret_val(nullptr != gov, false);

  if (!pplayer) {
    return false;
  }

  if (get_player_bonus(pplayer, EFT_ANY_GOVERNMENT) > 0) {
    // Note, this may allow govs that are on someone else's "tech tree".
    return true;
  }

  return are_reqs_active(pplayer, nullptr, nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr, nullptr, nullptr,
                         &gov->reqs, RPT_CERTAIN);
}

/**************************************************************************
  Ruler titles.
**************************************************************************/
struct ruler_title {
  const struct nation_type *pnation;
  struct name_translation male;
  struct name_translation female;
};

/**
   Create a new ruler title.
 */
static struct ruler_title *ruler_title_new(const struct nation_type *pnation,
                                           const char *domain,
                                           const char *ruler_male_title,
                                           const char *ruler_female_title)
{
  auto *pruler_title = new ruler_title;

  pruler_title->pnation = pnation;
  name_set(&pruler_title->male, domain, ruler_male_title);
  name_set(&pruler_title->female, domain, ruler_female_title);

  return pruler_title;
}

/**
   Free a ruler title.
 */
static void ruler_title_destroy(struct ruler_title *pruler_title)
{
  delete pruler_title;
}

/**
   Return TRUE if the ruler title is valid.
 */
static bool ruler_title_check(const struct ruler_title *pruler_title)
{
  bool ret = true;

  if (!formats_match(rule_name_get(&pruler_title->male), "%s")) {
    if (nullptr != pruler_title->pnation) {
      qCritical("\"%s\" male ruler title for nation \"%s\" (nb %d) "
                "is not a format. It should match \"%%s\"",
                rule_name_get(&pruler_title->male),
                nation_rule_name(pruler_title->pnation),
                nation_index(pruler_title->pnation));
    } else {
      qCritical("\"%s\" male ruler title is not a format. "
                "It should match \"%%s\"",
                rule_name_get(&pruler_title->male));
    }
    ret = false;
  }

  if (!formats_match(rule_name_get(&pruler_title->female), "%s")) {
    if (nullptr != pruler_title->pnation) {
      qCritical("\"%s\" female ruler title for nation \"%s\" (nb %d) "
                "is not a format. It should match \"%%s\"",
                rule_name_get(&pruler_title->female),
                nation_rule_name(pruler_title->pnation),
                nation_index(pruler_title->pnation));
    } else {
      qCritical("\"%s\" female ruler title is not a format. "
                "It should match \"%%s\"",
                rule_name_get(&pruler_title->female));
    }
    ret = false;
  }

  if (!formats_match(name_translation_get(&pruler_title->male), "%s")) {
    if (nullptr != pruler_title->pnation) {
      qCritical("Translation of \"%s\" male ruler title for nation \"%s\" "
                "(nb %d) is not a format (\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->male),
                nation_rule_name(pruler_title->pnation),
                nation_index(pruler_title->pnation),
                name_translation_get(&pruler_title->male));
    } else {
      qCritical("Translation of \"%s\" male ruler title is not a format "
                "(\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->male),
                name_translation_get(&pruler_title->male));
    }
    ret = false;
  }

  if (!formats_match(name_translation_get(&pruler_title->female), "%s")) {
    if (nullptr != pruler_title->pnation) {
      qCritical("Translation of \"%s\" female ruler title for nation \"%s\" "
                "(nb %d) is not a format (\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->female),
                nation_rule_name(pruler_title->pnation),
                nation_index(pruler_title->pnation),
                name_translation_get(&pruler_title->female));
    } else {
      qCritical("Translation of \"%s\" female ruler title is not a format "
                "(\"%s\"). It should match \"%%s\"",
                rule_name_get(&pruler_title->female),
                name_translation_get(&pruler_title->female));
    }
    ret = false;
  }

  return ret;
}

/**
   Returns all ruler titles for a government type.
 */
QHash<const struct nation_type *, struct ruler_title *> *
government_ruler_titles(const struct government *pgovern)
{
  fc_assert_ret_val(nullptr != pgovern, nullptr);
  return pgovern->ruler_titles;
}

/**
   Add a new ruler title for the nation. Pass nullptr for pnation for
   defining the default title.
 */
struct ruler_title *government_ruler_title_new(
    struct government *pgovern, const struct nation_type *pnation,
    const char *ruler_male_title, const char *ruler_female_title)
{
  const char *domain = nullptr;
  struct ruler_title *pruler_title;

  if (pnation != nullptr) {
    domain = pnation->translation_domain;
  }
  pruler_title =
      ruler_title_new(pnation, domain, ruler_male_title, ruler_female_title);

  if (!ruler_title_check(pruler_title)) {
    ruler_title_destroy(pruler_title);
    return nullptr;
  }

  if (pgovern->ruler_titles->contains(pnation)) {
    if (nullptr != pnation) {
      qCritical("Ruler title for government \"%s\" (nb %d) and "
                "nation \"%s\" (nb %d) was set twice.",
                government_rule_name(pgovern), government_number(pgovern),
                nation_rule_name(pnation), nation_index(pnation));
    } else {
      qCritical("Default ruler title for government \"%s\" (nb %d) "
                "was set twice.",
                government_rule_name(pgovern), government_number(pgovern));
    }
  } else {
    pgovern->ruler_titles->insert(pnation, pruler_title);
  }

  return pruler_title;
}

/**
   Return the nation of the rule title. Returns nullptr if this is default.
 */
const struct nation_type *
ruler_title_nation(const struct ruler_title *pruler_title)
{
  return pruler_title->pnation;
}

/**
   Return the male rule title name.
 */
const char *
ruler_title_male_untranslated_name(const struct ruler_title *pruler_title)
{
  return untranslated_name(&pruler_title->male);
}

/**
   Return the female rule title name.
 */
const char *
ruler_title_female_untranslated_name(const struct ruler_title *pruler_title)
{
  return untranslated_name(&pruler_title->female);
}

/**
   Return the ruler title of the player (translated).
 */
const char *ruler_title_for_player(const struct player *pplayer, char *buf,
                                   size_t buf_len)
{
  const struct government *pgovern = government_of_player(pplayer);
  const struct nation_type *pnation = nation_of_player(pplayer);
  struct ruler_title *pruler_title;

  fc_assert_ret_val(nullptr != buf, nullptr);
  fc_assert_ret_val(0 < buf_len, nullptr);

  // Try specific nation rule title.
  if (!pgovern->ruler_titles->contains(pnation)
      // Try default rule title.
      && !pgovern->ruler_titles->contains(nullptr)) {
    qCritical("Missing title for government \"%s\" (nb %d) "
              "nation \"%s\" (nb %d).",
              government_rule_name(pgovern), government_number(pgovern),
              nation_rule_name(pnation), nation_index(pnation));
    if (pplayer->is_male) {
      fc_snprintf(buf, buf_len, _("Mr. %s"), player_name(pplayer));
    } else {
      fc_snprintf(buf, buf_len, _("Ms. %s"), player_name(pplayer));
    }
  } else {
    pruler_title = pgovern->ruler_titles->value(pnation);
    if (!pruler_title) {
      pruler_title = pgovern->ruler_titles->value(nullptr);
    }
    fc_snprintf(buf, buf_len,
                name_translation_get(pplayer->is_male
                                         ? &pruler_title->male
                                         : &pruler_title->female),
                player_name(pplayer));
  }

  return buf;
}

/**
   Allocate resources associated with the given government.
 */
government::government()
{
  item_number = 0;
  ruler_titles = new QHash<const struct nation_type *, struct ruler_title *>;
  requirement_vector_init(&reqs);
  changed_to_times = 0;
  ruledit_disabled = false;
  ai.better = nullptr;
}

/**
   De-allocate resources associated with the given government.
 */
government::~government()
{
  for (auto *a : ruler_titles->values()) {
    delete a;
  }
  delete ruler_titles;
  delete helptext;
  ruler_titles = nullptr;
  helptext = nullptr;

  requirement_vector_free(&reqs);
}

/**
   Allocate the governments.
 */
void governments_alloc(int num)
{
  fc_assert(0 == governments.size());
  governments.resize(num);
  game.control.government_count = num;

  int i = 0;
  for (auto &gov : governments) {
    gov.item_number = i++;
  }
}

/**
   De-allocate the currently allocated governments.
 */
void governments_free()
{
  governments.clear();
  game.control.government_count = 0;
}

/**
   Is it possible to start a revolution without specifying the target
   government in the current game?
 */
bool untargeted_revolution_allowed()
{
  if (game.info.revolentype == REVOLEN_QUICKENING
      || game.info.revolentype == REVOLEN_RANDQUICK) {
    /* We need to know the target government at the onset of the revolution
     * in order to know how long anarchy will last. */
    return false;
  }
  return true;
}
