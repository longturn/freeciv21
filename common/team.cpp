/**************************************************************************
    Copyright (c) 1996-2020 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdlib>

// utility
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "game.h"
#include "player.h"
#include "team.h"

struct team_slot {
  struct team *team;
  char *defined_name; // Defined by the ruleset.
  char *rule_name;    // Usable untranslated name.
#ifdef FREECIV_ENABLE_NLS
  char *name_translation; // Translated name.
#endif
};

struct team {
  struct player_list *plrlist;
  struct team_slot *slot;
};

static struct {
  struct team_slot *tslots;
  int used_slots;
} team_slots;

/**
   Initialise all team slots.
 */
void team_slots_init()
{
  int i;

  // Init team slots and names.
  team_slots.tslots = new team_slot[team_slot_count()]();
  /* Can't use the defined functions as the needed data will be
   * defined here. */
  for (i = 0; i < team_slot_count(); i++) {
    struct team_slot *tslot = team_slots.tslots + i;

    tslot->team = nullptr;
    tslot->defined_name = nullptr;
    tslot->rule_name = nullptr;
#ifdef FREECIV_ENABLE_NLS
    tslot->name_translation = nullptr;
#endif
  }
  team_slots.used_slots = 0;
}

/**
   Returns TRUE if the team slots have been initialized.
 */
bool team_slots_initialised() { return (team_slots.tslots != nullptr); }

/**
   Remove all team slots.
 */
void team_slots_free()
{
  team_slots_iterate(tslot)
  {
    if (nullptr != tslot->team) {
      team_destroy(tslot->team);
    }
    NFCPP_FREE(tslot->defined_name);
    NFCPP_FREE(tslot->rule_name);

#ifdef FREECIV_ENABLE_NLS
    NFCPP_FREE(tslot->name_translation);
#endif // FREECIV_ENABLE_NLS
  }
  team_slots_iterate_end;
  delete[] team_slots.tslots;
  team_slots.tslots = nullptr;

  team_slots.used_slots = 0;
}

/**
   Returns the total number of team slots (including used slots).
 */
int team_slot_count() { return (MAX_NUM_TEAM_SLOTS); }

/**
   Returns the first team slot.
 */
struct team_slot *team_slot_first() { return team_slots.tslots; }

/**
   Returns the next team slot.
 */
struct team_slot *team_slot_next(struct team_slot *tslot)
{
  tslot++;
  return (tslot < team_slots.tslots + team_slot_count() ? tslot : nullptr);
}

/**
   Returns the index of the team slots.
 */
int team_slot_index(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), 0);
  fc_assert_ret_val(tslot != nullptr, 0);

  return tslot - team_slots.tslots;
}

/**
   Returns the team corresponding to the slot. If the slot is not used, it
   will return nullptr. See also team_slot_is_used().
 */
struct team *team_slot_get_team(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), nullptr);
  fc_assert_ret_val(tslot != nullptr, nullptr);

  return tslot->team;
}

/**
   Returns TRUE is this slot is "used" i.e. corresponds to a
   valid, initialized team that exists in the game.
 */
bool team_slot_is_used(const struct team_slot *tslot)
{
  // No team slot available, if the game is not initialised.
  if (!team_slots_initialised()) {
    return false;
  }

  return nullptr != tslot->team;
}

/**
   Return the possibly unused and uninitialized team slot.
 */
struct team_slot *team_slot_by_number(int team_id)
{
  if (!team_slots_initialised()
      || !(0 <= team_id && team_id < team_slot_count())) {
    return nullptr;
  }

  return team_slots.tslots + team_id;
}

/**
   Does a linear search for a (defined) team name. Returns nullptr when none
   match.
 */
struct team_slot *team_slot_by_rule_name(const char *team_name)
{
  fc_assert_ret_val(team_name != nullptr, nullptr);

  team_slots_iterate(tslot)
  {
    const char *tname = team_slot_rule_name(tslot);

    if (nullptr != tname && 0 == fc_strcasecmp(tname, team_name)) {
      return tslot;
    }
  }
  team_slots_iterate_end;

  return nullptr;
}

/**
   Creates a default name for this team slot.
 */
static inline void team_slot_create_default_name(struct team_slot *tslot)
{
  char buf[MAX_LEN_NAME];

  fc_assert(nullptr == tslot->defined_name);
  fc_assert(nullptr == tslot->rule_name);
#ifdef FREECIV_ENABLE_NLS
  fc_assert(nullptr == tslot->name_translation);
#endif // FREECIV_ENABLE_NLS

  fc_snprintf(buf, sizeof(buf), "Team %d", team_slot_index(tslot) + 1);
  tslot->rule_name = fc_strdup(buf);

#ifdef FREECIV_ENABLE_NLS
  fc_snprintf(buf, sizeof(buf), _("Team %d"), team_slot_index(tslot) + 1);
  tslot->name_translation = fc_strdup(buf);
#endif // FREECIV_ENABLE_NLS

  qDebug("No name defined for team %d! Creating a default name: %s.",
         team_slot_index(tslot), tslot->rule_name);
}

/**
   Returns the name (untranslated) of the slot. Creates a default one if it
   doesn't exist currently.
 */
const char *team_slot_rule_name(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), nullptr);
  fc_assert_ret_val(nullptr != tslot, nullptr);

  if (nullptr == tslot->rule_name) {
    // Get the team slot as changeable (not _const_) struct.
    struct team_slot *changeable =
        team_slot_by_number(team_slot_index(tslot));
    team_slot_create_default_name(changeable);
    return changeable->rule_name;
  }

  return tslot->rule_name;
}

/**
   Returns the name (translated) of the slot. Creates a default one if it
   doesn't exist currently.
 */
const char *team_slot_name_translation(const struct team_slot *tslot)
{
#ifdef FREECIV_ENABLE_NLS
  fc_assert_ret_val(team_slots_initialised(), nullptr);
  fc_assert_ret_val(nullptr != tslot, nullptr);

  if (nullptr == tslot->name_translation) {
    // Get the team slot as changeable (not _const_) struct.
    struct team_slot *changeable =
        team_slot_by_number(team_slot_index(tslot));
    team_slot_create_default_name(changeable);
    return changeable->name_translation;
  }

  return tslot->name_translation;
#else  // FREECIV_ENABLE_NLS
  return team_slot_rule_name(tslot);
#endif // FREECIV_ENABLE_NLS
}

/**
   Returns the name defined in the ruleset for this slot. It may return
   nullptr if the ruleset didn't defined a such name.
 */
const char *team_slot_defined_name(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), nullptr);
  fc_assert_ret_val(nullptr != tslot, nullptr);

  return tslot->defined_name;
}

/**
   Set the name defined in the ruleset for this slot.
 */
void team_slot_set_defined_name(struct team_slot *tslot,
                                const char *team_name)
{
  fc_assert_ret(team_slots_initialised());
  fc_assert_ret(nullptr != tslot);
  fc_assert_ret(nullptr != team_name);

  NFCPP_FREE(tslot->defined_name);
  tslot->defined_name = fc_strdup(team_name);
  NFCPP_FREE(tslot->rule_name);

  tslot->rule_name = fc_strdup(Qn_(team_name));

#ifdef FREECIV_ENABLE_NLS
  NFCPP_FREE(tslot->name_translation);
  tslot->name_translation = fc_strdup(Q_(team_name));
#endif // FREECIV_ENABLE_NLS
}

/**
   Creates a new team for the slot. If slot is nullptr, it will lookup to a
   free slot. If the slot already used, then just return the team.
 */
struct team *team_new(struct team_slot *tslot)
{
  struct team *pteam;

  fc_assert_ret_val(team_slots_initialised(), nullptr);

  if (nullptr == tslot) {
    team_slots_iterate(aslot)
    {
      if (!team_slot_is_used(aslot)) {
        tslot = aslot;
        break;
      }
    }
    team_slots_iterate_end;

    fc_assert_ret_val(nullptr != tslot, nullptr);
  } else if (nullptr != tslot->team) {
    return tslot->team;
  }

  // Now create the team.
  log_debug("Create team for slot %d.", team_slot_index(tslot));
  pteam = new team[1]();
  pteam->slot = tslot;
  tslot->team = pteam;

  // Set default values.
  pteam->plrlist = player_list_new();

  // Increase number of teams.
  team_slots.used_slots++;

  return pteam;
}

/**
   Destroys a team.
 */
void team_destroy(struct team *pteam)
{
  struct team_slot *tslot;

  fc_assert_ret(team_slots_initialised());
  fc_assert_ret(nullptr != pteam);
  fc_assert(0 == player_list_size(pteam->plrlist));

  tslot = pteam->slot;
  fc_assert(tslot->team == pteam);

  player_list_destroy(pteam->plrlist);
  FCPP_FREE(pteam);
  tslot->team = nullptr;
  team_slots.used_slots--;
}

/**
   Return the current number of teams.
 */
int team_count() { return team_slots.used_slots; }

/**
   Return the team index.
 */
int team_index(const struct team *pteam) { return team_number(pteam); }

/**
   Return the team index/number/id.
 */
int team_number(const struct team *pteam)
{
  fc_assert_ret_val(nullptr != pteam, 0);
  return team_slot_index(pteam->slot);
}

/**
   Return struct team pointer for the given team index.
 */
struct team *team_by_number(const int team_id)
{
  const struct team_slot *tslot = team_slot_by_number(team_id);

  return (nullptr != tslot ? team_slot_get_team(tslot) : nullptr);
}

/**
   Returns the name (untranslated) of the team.
 */
const char *team_rule_name(const struct team *pteam)
{
  fc_assert_ret_val(nullptr != pteam, nullptr);

  return team_slot_rule_name(pteam->slot);
}

/**
   Returns the name (translated) of the team.
 */
const char *team_name_translation(const struct team *pteam)
{
  fc_assert_ret_val(nullptr != pteam, nullptr);

  return team_slot_name_translation(pteam->slot);
}

/**
   Set in 'buf' the name of the team 'pteam' in a format like
   "team <team_name>". To avoid to see "team Team 0", it only prints the
   the team number when the name of this team is not defined in the ruleset.
 */
int team_pretty_name(const struct team *pteam, QString &buf)
{
  if (nullptr != pteam) {
    if (nullptr != pteam->slot->defined_name) {
      // TRANS: %s is ruleset-chosen team name (e.g. "Red")
      buf =
          QString(_("team %1")).arg(team_slot_name_translation(pteam->slot));
      return 1;
    } else {
      buf = QString(_("team %1")).arg(QString::number(team_number(pteam)));
      return 1;
    }
  }

  // No need to translate, it's an error.
  buf = QStringLiteral("(null team)");
  return -1;
}

/**
   Returns the member list of the team.
 */
const struct player_list *team_members(const struct team *pteam)
{
  fc_assert_ret_val(nullptr != pteam, nullptr);

  return pteam->plrlist;
}

/**
   Set a player to a team.  Removes the previous team affiliation if
   necessary.
 */
void team_add_player(struct player *pplayer, struct team *pteam)
{
  fc_assert_ret(pplayer != nullptr);

  if (pteam == nullptr) {
    pteam = team_new(nullptr);
  }

  fc_assert_ret(pteam != nullptr);

  if (pteam == pplayer->team) {
    // It is the team of the player.
    return;
  }

  log_debug("Adding player %d/%s to team %s.", player_number(pplayer),
            pplayer->username, team_rule_name(pteam));

  // Remove the player from the old team, if any.
  team_remove_player(pplayer);

  // Put the player on the new team.
  pplayer->team = pteam;
  player_list_append(pteam->plrlist, pplayer);
}

/**
   Remove the player from the team.  This should only be called when deleting
   a player; since every player must always be on a team.

   Note in some very rare cases a player may not be on a team.  It's safe
   to call this function anyway.
 */
void team_remove_player(struct player *pplayer)
{
  struct team *pteam;

  if (pplayer->team) {
    pteam = pplayer->team;

    log_debug("Removing player %d/%s from team %s (%d)",
              player_number(pplayer), player_name(pplayer),
              team_rule_name(pteam), player_list_size(pteam->plrlist));
    player_list_remove(pteam->plrlist, pplayer);

    if (player_list_size(pteam->plrlist) == 0) {
      team_destroy(pteam);
    }
  }
  pplayer->team = nullptr;
}
