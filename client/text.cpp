/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include <math.h> // ceil
#include <string.h>

// utility
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "nation.h"
#include "support.h"

// common
#include "calendar.h"
#include "citizens.h"
#include "city.h"
#include "clientutils.h"
#include "combat.h"
#include "culture.h"
#include "effects.h"
#include "fc_types.h" // LINE_BREAK
#include "game.h"
#include "government.h"
#include "map.h"
#include "research.h"
#include "traderoutes.h"
#include "unitlist.h"

// client
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "control.h"
#include "goto.h"

#include "text.h"

int get_bulbs_per_turn(int *pours, bool *pteam, int *ptheirs);

/**
   Return a (static) string with a tile's food/prod/trade
 */
const QString get_tile_output_text(const struct tile *ptile)
{
  QString str;
  int i;
  char output_text[O_LAST][16];

  for (i = 0; i < O_LAST; i++) {
    int before_penalty = 0;
    int x = city_tile_output(nullptr, ptile, false,
                             static_cast<Output_type_id>(i));

    if (nullptr != client.conn.playing) {
      before_penalty = get_player_output_bonus(
          client.conn.playing,
          get_output_type(static_cast<Output_type_id>(i)),
          EFT_OUTPUT_PENALTY_TILE);
    }

    if (before_penalty > 0 && x > before_penalty) {
      fc_snprintf(output_text[i], sizeof(output_text[i]), "%d(-1)", x);
    } else {
      fc_snprintf(output_text[i], sizeof(output_text[i]), "%d", x);
    }
  }

  str = QStringLiteral("%1/%2/%3")
            .arg(output_text[O_FOOD], output_text[O_SHIELD],
                 output_text[O_TRADE]);

  return qUtf8Printable(str);
}

/**
   For AIs, fill the buffer with their player name prefixed with "AI". For
   humans, just fill it with their username.
 */
static inline void get_full_username(char *buf, int buflen,
                                     const struct player *pplayer)
{
  if (!buf || buflen < 1) {
    return;
  }

  if (!pplayer) {
    buf[0] = '\0';
    return;
  }

  if (is_ai(pplayer)) {
    // TRANS: "AI <player name>"
    fc_snprintf(buf, buflen, _("AI %s"), pplayer->name);
  } else {
    fc_strlcpy(buf, pplayer->username, buflen);
  }
}

/**
   Fill the buffer with the player's nation name (in adjective form) and
   optionally add the player's team name.
 */
static inline void get_full_nation(char *buf, int buflen,
                                   const struct player *pplayer)
{
  if (!buf || buflen < 1) {
    return;
  }

  if (!pplayer) {
    buf[0] = '\0';
    return;
  }

  if (pplayer->team) {
    // TRANS: "<nation adjective>, team <team name>"
    fc_snprintf(buf, buflen, _("%s, team %s"),
                nation_adjective_for_player(pplayer),
                team_name_translation(pplayer->team));
  } else {
    fc_strlcpy(buf, nation_adjective_for_player(pplayer), buflen);
  }
}

/**
   Text to popup on a middle-click in the mapview.
 */
const QString popup_info_text(struct tile *ptile)
{
  QString activity_text;
  struct city *pcity = tile_city(ptile);
  struct unit *punit = find_visible_unit(ptile);
  const char *diplo_nation_plural_adjectives[DS_LAST] = {
      "" /* unused, DS_ARMISTICE */, Q_("?nation:Hostile"),
      "" /* unused, DS_CEASEFIRE */, Q_("?nation:Peaceful"),
      Q_("?nation:Friendly"),        Q_("?nation:Mysterious"),
      Q_("?nation:Friendly(team)")};
  const char *diplo_city_adjectives[DS_LAST] = {
      "" /* unused, DS_ARMISTICE */, Q_("?city:Hostile"),
      "" /* unused, DS_CEASEFIRE */, Q_("?city:Peaceful"),
      Q_("?city:Friendly"),          Q_("?city:Mysterious"),
      Q_("?city:Friendly(team)")};
  QString str;
  char username[MAX_LEN_NAME + 32];
  char nation[2 * MAX_LEN_NAME + 32];
  int tile_x, tile_y, nat_x, nat_y;
  bool first;

  index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
  str = QString(_("Location: (%1, %2) [%3]\n"))
            .arg(QString::number(tile_x), QString::number(tile_y),
                 QString::number(tile_continent(ptile)));
  index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
  str += QString(_("Native coordinates: (%1, %2)\n"))
             .arg(QString::number(nat_x), QString::number(nat_y));

  if (client_tile_get_known(ptile) == TILE_UNKNOWN) {
    str += QString(_("Unknown"));
    return str.trimmed();
  }
  str += QString(_("Terrain: %1")).arg(tile_get_info_text(ptile, true, 0))
         + qendl();
  str += QString(_("Food/Prod/Trade: %1")).arg(get_tile_output_text(ptile))
         + qendl();
  first = true;
  extra_type_iterate(pextra)
  {
    if (pextra->category == ECAT_BONUS
        && tile_has_visible_extra(ptile, pextra)) {
      if (!first) {
        str += QStringLiteral(",%1").arg(extra_name_translation(pextra));
      } else {
        str += QStringLiteral("%1").arg(extra_name_translation(pextra))
               + qendl();
        first = false;
      }
    }
  }
  extra_type_iterate_end;
  if (BORDERS_DISABLED != game.info.borders && !pcity) {
    struct player *owner = tile_owner(ptile);

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    if (nullptr != client.conn.playing && owner == client.conn.playing) {
      str += QString(_("Our territory")) + qendl();
    } else if (nullptr != owner && nullptr == client.conn.playing) {
      // TRANS: "Territory of <username> (<nation + team>)"
      str +=
          QString(_("Territory of %1 (%2)")).arg(username, nation) + qendl();
    } else if (nullptr != owner) {
      struct player_diplstate *ds =
          player_diplstate_get(client.conn.playing, owner);

      if (ds->type == DS_CEASEFIRE) {
        int turns = ds->turns_left;
        /* TRANS: "Territory of <username> (<nation + team>)
         * (<number> turn cease-fire)" */
        str +=
            QString(PL_("Territory of %1 (%2) (%3 turn cease-fire)",
                        "Territory of %1 (%2) (%3 turn cease-fire)", turns))
                .arg(username, nation, QString::number(turns))
            + qendl();
      } else if (ds->type == DS_ARMISTICE) {
        int turns = ds->turns_left;
        /* TRANS: "Territory of <username> (<nation + team>)
         * (<number> turn armistice)" */
        str +=
            QString(PL_("Territory of %1 (%2) (%3 turn armistice)",
                        "Territory of %1 (%2) (%3 turn armistice)", turns))
                .arg(username, nation, QString::number(turns))
            + qendl();
      } else {
        int type = ds->type;
        /* TRANS: "Territory of <username>
         * (<nation + team> | <diplomatic state>)" */
        str +=
            QString(_("Territory of %1 (%2 | %3)"))
                .arg(username, nation, diplo_nation_plural_adjectives[type])
            + qendl();
      }
    } else {
      str += QString(_("Unclaimed territory")) + qendl();
    }
  }
  if (pcity) {
    /* Look at city owner, not tile owner (the two should be the same, if
     * borders are in use). */
    struct player *owner = city_owner(pcity);
    QVector<QString> improvements;
    improvements.reserve(improvement_count());

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    if (nullptr == client.conn.playing || owner == client.conn.playing) {
      // TRANS: "City: <city name> | <username> (<nation + team>)"
      str += QString(_("City: %1 | %2 (%3)"))
                 .arg(city_name_get(pcity), username, nation)
             + qendl();
    } else {
      struct player_diplstate *ds =
          player_diplstate_get(client_player(), owner);
      if (ds->type == DS_CEASEFIRE) {
        int turns = ds->turns_left;

        /* TRANS:  "City: <city name> | <username>
         * (<nation + team>, <number> turn cease-fire)" */
        str += QString(PL_("City: %1 | %2 (%3, %4 turn cease-fire)",
                           "City: %1 | %2 (%3, %4 turn cease-fire)", turns))
                   .arg(city_name_get(pcity), username, nation,
                        QString::number(turns))
               + qendl();

      } else if (ds->type == DS_ARMISTICE) {
        int turns = ds->turns_left;

        /* TRANS:  "City: <city name> | <username>
         * (<nation + team>, <number> turn armistice)" */
        str += QString(PL_("City: %1 | %2 (%3, %4 turn armistice)",
                           "City: %1 | %2 (%3, %4 turn armistice)", turns))
                   .arg(city_name_get(pcity), username, nation,
                        QString::number(turns))
               + qendl();
      } else {
        /* TRANS: "City: <city name> | <username>
         * (<nation + team>, <diplomatic state>)" */
        str += QString(_("City: %1 | %2 (%3, %4)"))
                   .arg(city_name_get(pcity), username, nation,
                        diplo_city_adjectives[ds->type])
               + qendl();
      }
    }
    if (can_player_see_units_in_city(client_player(), pcity)) {
      int count = unit_list_size(ptile->units);

      if (count > 0) {
        // TRANS: preserve leading space
        str += QString(PL_(" | Occupied with %1 unit.",
                           " | Occupied with %2 units.", count))
                   .arg(QString::number(count));
      } else {
        // TRANS: preserve leading space
        str += QString(_(" | Not occupied."));
      }
    } else {
      if (city_is_occupied(pcity)) {
        // TRANS: preserve leading space
        str += QString(_(" | Occupied."));
      } else {
        // TRANS: preserve leading space
        str += QString(_(" | Not occupied."));
      }
    }
    improvement_iterate(pimprove)
    {
      if (is_improvement_visible(pimprove)
          && city_has_building(pcity, pimprove)) {
        improvements.append(improvement_name_translation(pimprove));
      }
    }
    improvement_iterate_end;

    if (!improvements.isEmpty()) {
      // TRANS: %s is a list of "and"-separated improvements.
      str += QString(_("   with %1.")).arg(strvec_to_and_list(improvements))
             + qendl();
    }

    for (const auto &pfocus_unit : get_units_in_focus()) {
      struct city *hcity = game_city_by_number(pfocus_unit->homecity);

      if (utype_can_do_action(unit_type_get(pfocus_unit), ACTION_TRADE_ROUTE)
          && can_cities_trade(hcity, pcity)
          && can_establish_trade_route(hcity, pcity)) {
        // TRANS: "Trade from Warsaw: 5"
        str += QString(_("Trade from %1: %2"))
                   .arg(city_name_get(hcity),
                        QString::number(
                            trade_base_between_cities(hcity, pcity)))
               + qendl();
      }
    }
  }
  {
    const char *infratext = get_infrastructure_text(ptile->extras);

    if (*infratext != '\0') {
      str += QString(_("Infrastructure: %1")).arg(infratext) + qendl();
    }
  }
  activity_text = concat_tile_activity_text(ptile);
  if (activity_text.length() > 0) {
    str += QString(_("Activity: %1")).arg(activity_text) + qendl();
  }
  if (punit && !pcity) {
    struct player *owner = unit_owner(punit);
    const struct unit_type *ptype = unit_type_get(punit);

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    time_t dt = time(nullptr) - punit->action_timestamp;
    if (dt < 0 && !can_unit_move_now(punit)) {
      char buf[64];
      format_time_duration(-dt, buf, sizeof(buf));
      str += _("Can move in ") + QString(buf) + qendl();
    }

    auto unit_description = QString();
    if (punit->name.isEmpty()) {
      // TRANS: "Unit: <unit type> #<unit id>
      unit_description = QString(_("%1 #%2"))
                             .arg(utype_name_translation(ptype))
                             .arg(punit->id);
    } else {
      // TRANS: "Unit: <unit type> #<unit id> "<unit name>"
      unit_description = QString(_("%1 #%2 \"%3\""))
                             .arg(utype_name_translation(ptype))
                             .arg(punit->id)
                             .arg(punit->name);
    }

    if (!client_player() || owner == client_player()) {
      struct city *hcity = player_city_by_number(owner, punit->homecity);

      str += QString(_("Unit: %1 | %2 (%3)"))
                 .arg(unit_description)
                 .arg(username)
                 .arg(nation)
             + qendl();

      if (game.info.citizen_nationality
          && unit_nationality(punit) != unit_owner(punit)) {
        if (hcity != nullptr) {
          /* TRANS: on own line immediately following \n, "from <city> |
           * <nationality> people" */
          str +=
              QString(_("from %1 | %2 people"))
                  .arg(city_name_get(hcity),
                       nation_adjective_for_player(unit_nationality(punit)))
              + qendl();
        } else {
          /* TRANS: Nationality of the people comprising a unit, if
           * different from owner. */
          str +=
              QString(_("%1 people"))
                  .arg(nation_adjective_for_player(unit_nationality(punit)))
              + qendl();
        }
      } else if (hcity != nullptr) {
        // TRANS: on own line immediately following \n, ... <city>
        str += QString(_("from %1")).arg(city_name_get(hcity)) + qendl();
      }
    } else if (nullptr != owner) {
      struct player_diplstate *ds =
          player_diplstate_get(client_player(), owner);
      if (ds->type == DS_CEASEFIRE) {
        int turns = ds->turns_left;

        /* TRANS:  "Unit: <unit type> | <username> (<nation + team>,
         * <number> turn cease-fire)" */
        str += QString(PL_("Unit: %1 | %2 (%3, %4 turn cease-fire)",
                           "Unit: %1 | %2 (%3, %4 turn cease-fire)", turns))
                   .arg(unit_description, username, nation,
                        QString::number(turns))
               + qendl();
      } else if (ds->type == DS_ARMISTICE) {
        int turns = ds->turns_left;

        /* TRANS:  "Unit: <unit type> | <username> (<nation + team>,
         * <number> turn armistice)" */
        str += QString(PL_("Unit: %1 | %2 (%3, %4 turn armistice)",
                           "Unit: %1 | %2 (%3, %4 turn armistice)", turns))
                   .arg(unit_description, username, nation,
                        QString::number(turns))
               + qendl();
      } else {
        /* TRANS: "Unit: <unit type> | <username> (<nation + team>,
         * <diplomatic state>)" */
        str += QString(_("Unit: %1 | %2 (%3, %4)"))
                   .arg(unit_description, username, nation,
                        diplo_city_adjectives[ds->type])
               + qendl();
      }
    }

    for (const auto pfocus_unit : get_units_in_focus()) {
      int att_chance = FC_INFINITY, def_chance = FC_INFINITY;
      bool found = false;

      unit_list_iterate(ptile->units, tile_unit)
      {
        if (unit_owner(tile_unit) != unit_owner(pfocus_unit)) {
          int att = unit_win_chance(pfocus_unit, tile_unit) * 100;
          int def = (1.0 - unit_win_chance(tile_unit, pfocus_unit)) * 100;

          found = true;

          // Presumably the best attacker and defender will be used.
          att_chance = MIN(att, att_chance);
          def_chance = MIN(def, def_chance);
        }
      }
      unit_list_iterate_end;

      if (found) {
        // TRANS: "Chance to win: A:95% D:46%"
        str += QString(_("Chance to win: A:%1% D:%2%"))
                   .arg(QString::number(att_chance),
                        QString::number(def_chance))
               + qendl();
      }
    }

    /* TRANS: A is attack power, D is defense power, FP is firepower,
     * HP is hitpoints (current and max). */
    str += QString(_("A:%1 D:%2 FP:%3 HP:%4/%5"))
               .arg(QString::number(ptype->attack_strength),
                    QString::number(ptype->defense_strength),
                    QString::number(ptype->firepower),
                    QString::number(punit->hp), QString::number(ptype->hp));
    {
      const char *veteran_name =
          utype_veteran_name_translation(ptype, punit->veteran);
      if (veteran_name) {
        str += QStringLiteral(" (%1)").arg(veteran_name);
      }
    }
    str += qendl();

    if (unit_owner(punit) == client_player()
        || client_is_global_observer()) {
      // Show bribe cost for own units.
      str += QString(_("Probable bribe cost: %1"))
                 .arg(QString::number(unit_bribe_cost(punit, nullptr)))
             + qendl();
    } else {
      // We can only give an (lower) boundary for units of other players.
      str +=
          QString(_("Estimated bribe cost: > %1"))
              .arg(QString::number(unit_bribe_cost(punit, client_player())))
          + qendl();
    }

    if ((nullptr == client.conn.playing || owner == client.conn.playing)
        && unit_list_size(ptile->units) >= 2) {
      // TRANS: "5 more" units on this tile
      str += QString(_("  (%1 more)"))
                 .arg(QString::number(unit_list_size(ptile->units) - 1));
    }
  }

  return str.trimmed();
}

#define FAR_CITY_SQUARE_DIST (2 * (6 * 6))
/**
   Returns the text describing the city and its distance.
 */
const QString get_nearest_city_text(struct city *pcity, int sq_dist)
{
  if (!pcity) {
    sq_dist = -1;
  }
  QString str =
      QString((sq_dist >= FAR_CITY_SQUARE_DIST)
                  // TRANS: on own line immediately following \n, ... <city>
                  ? _("far from %1")
                  : (sq_dist > 0)
                        /* TRANS: on own line immediately following \n, ...
                           <city> */
                        ? _("near %1")
                        : (sq_dist == 0)
                              /* TRANS: on own line immediately following \n,
                                 ... <city> */
                              ? _("in %1")
                              : "%1")
          .arg(pcity ? city_name_get(pcity) : "");
  return str.trimmed();
}

/**
   Returns the unit description.
   Used in e.g. city report tooltips.

   FIXME: This function is not re-entrant because it returns a pointer to
   static data.
 */
const QString unit_description(struct unit *punit)
{
  int pcity_near_dist;
  struct player *owner = unit_owner(punit);
  struct player *nationality = unit_nationality(punit);
  struct city *pcity = player_city_by_number(owner, punit->homecity);
  struct city *pcity_near = get_nearest_city(punit, &pcity_near_dist);
  const struct unit_type *ptype = unit_type_get(punit);
  const struct player *pplayer = client_player();
  QString str = QStringLiteral("%1").arg(utype_name_translation(ptype));
  const char *veteran_name =
      utype_veteran_name_translation(ptype, punit->veteran);
  if (veteran_name) {
    str += QStringLiteral("( %1)").arg(veteran_name);
  }

  if (pplayer == owner) {
    unit_upkeep_astr(punit, str);
  } else {
    str += qendl();
  }
  unit_activity_astr(punit, str);

  if (pcity) {
    // TRANS: on own line immediately following \n, ... <city>
    str += QString(_("from %1")).arg(city_name_get(pcity)) + qendl();
  } else {
    str += qendl();
  }
  if (game.info.citizen_nationality) {
    if (nationality != nullptr && owner != nationality) {
      /* TRANS: Nationality of the people comprising a unit, if
       * different from owner. */
      str += QString(_("%1 people"))
                 .arg(nation_adjective_for_player(nationality))
             + qendl();
    } else {
      str += qendl();
    }
  }
  str += QStringLiteral("%1").arg(
             get_nearest_city_text(pcity_near, pcity_near_dist))
         + qendl();
#ifdef FREECIV_DEBUG
  str += QStringLiteral("Unit ID: %1").arg(punit->id);
#endif

  return str.trimmed();
}

/**
   Describe the airlift capacity of a city for the given units (from their
   current positions).
   If pdest is non-nullptr, describe its capacity as a destination, otherwise
   describe the capacity of the city the unit's currently in (if any) as a
   source. (If the units in the list are in different cities, this will
   probably not give a useful result in this case.)
   If not all of the listed units can be airlifted, return the description
   for those that can.
   Returns nullptr if an airlift is not possible for any of the units.
 */
const QString get_airlift_text(const std::vector<unit *> &units,
                               const struct city *pdest)
{
  QString str;
  bool src = (pdest == nullptr);
  enum texttype {
    AL_IMPOSSIBLE,
    AL_UNKNOWN,
    AL_FINITE,
    AL_INFINITE
  } best = AL_IMPOSSIBLE;
  int cur = 0, max = 0;

  for (const auto punit : units) {
    enum texttype tthis = AL_IMPOSSIBLE;
    enum unit_airlift_result result;

    // nullptr will tell us about the capability of airlifting from source
    result = test_unit_can_airlift_to(client_player(), punit, pdest);

    switch (result) {
    case AR_NO_MOVES:
    case AR_WRONG_UNITTYPE:
    case AR_OCCUPIED:
    case AR_NOT_IN_CITY:
    case AR_BAD_SRC_CITY:
    case AR_BAD_DST_CITY:
      // No chance of an airlift.
      tthis = AL_IMPOSSIBLE;
      break;
    case AR_OK:
    case AR_OK_SRC_UNKNOWN:
    case AR_OK_DST_UNKNOWN:
    case AR_SRC_NO_FLIGHTS:
    case AR_DST_NO_FLIGHTS:
      /* May or may not be able to airlift now, but there's a chance we could
       * later */
      {
        const struct city *pcity = src ? tile_city(unit_tile(punit)) : pdest;

        fc_assert_ret_val(pcity != nullptr, fc_strdup("-"));
        if (!src
            && (game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST)) {
          /* No restrictions on destination (and we can infer this even for
           * other players' cities). */
          tthis = AL_INFINITE;
        } else if (client_player() == city_owner(pcity)) {
          // A city we know about.
          int this_cur = pcity->airlift, this_max = city_airlift_max(pcity);

          if (this_max <= 0) {
            // City known not to be airlift-capable.
            tthis = AL_IMPOSSIBLE;
          } else {
            if (src
                && (game.info.airlifting_style & AIRLIFTING_UNLIMITED_SRC)) {
              // Unlimited capacity.
              tthis = AL_INFINITE;
            } else {
              // Limited capacity (possibly zero right now).
              tthis = AL_FINITE;
              /* Store the numbers. This whole setup assumes that numeric
               * capacity isn't unit-dependent. */
              if (best == AL_FINITE) {
                fc_assert(cur == this_cur && max == this_max);
              }
              cur = this_cur;
              max = this_max;
            }
          }
        } else {
          // Unknown capacity.
          tthis = AL_UNKNOWN;
        }
      }
      break;
    }

    // Now take the most optimistic view.
    best = MAX(best, tthis);
  }

  switch (best) {
  case AL_IMPOSSIBLE:
    return nullptr;
  case AL_UNKNOWN:
    str = QStringLiteral("?");
    break;
  case AL_FINITE:
    str = QStringLiteral("%1/%2").arg(cur, max);
    break;
  case AL_INFINITE:
    str = _("Yes");
    break;
  }

  return str.trimmed();
}

/**
   Return total expected bulbs.
 */
int get_bulbs_per_turn(int *pours, bool *pteam, int *ptheirs)
{
  const struct research *presearch;
  int ours = 0, theirs = 0;
  bool team = false;

  if (!client_has_player()) {
    return 0;
  }
  presearch = research_get(client_player());

  // Sum up science
  research_players_iterate(presearch, pplayer)
  {
    if (pplayer == client_player()) {
      city_list_iterate(pplayer->cities, pcity)
      {
        ours += pcity->surplus[O_SCIENCE];
      }
      city_list_iterate_end;
    } else {
      team = true;
      theirs -= pplayer->client.tech_upkeep;
    }
  }
  research_players_iterate_end;

  if (team) {
    theirs += presearch->client.total_bulbs_prod - ours;
  }
  ours -= client_player()->client.tech_upkeep;

  if (pours) {
    *pours = ours;
  }
  if (pteam) {
    *pteam = team;
  }
  if (ptheirs) {
    *ptheirs = theirs;
  }
  return ours + theirs;
}

/**
   Return turns until research complete. -1 for never.
 */
int turns_to_research_done(const struct research *presearch, int per_turn)
{
  if (per_turn > 0) {
    return ceil(static_cast<double>(presearch->client.researching_cost
                                    - presearch->bulbs_researched)
                / per_turn);
  } else {
    return -1;
  }
}

/**
   Return turns per advance (based on currently researched advance).
   -1 for no progress.
 */
static int turns_per_advance(const struct research *presearch, int per_turn)
{
  if (per_turn > 0) {
    return MAX(1,
               ceil((double) presearch->client.researching_cost) / per_turn);
  } else {
    return -1;
  }
}

/**
   Return turns until an advance is lost due to tech upkeep.
   -1 if we're not on the way to losing an advance.
 */
static int turns_to_tech_loss(const struct research *presearch, int per_turn)
{
  if (per_turn >= 0 || game.info.techloss_forgiveness == -1) {
    /* With techloss_forgiveness == -1, we'll never lose a tech, just
     * get further into debt. */
    return -1;
  } else {
    int bulbs_to_loss = presearch->bulbs_researched
                        + (presearch->client.researching_cost
                           * game.info.techloss_forgiveness / 100);

    return ceil(static_cast<double>(bulbs_to_loss) / -per_turn);
  }
}

/**
   Returns the text to display in the science dialog.
 */
const QString science_dialog_text()
{
  bool team;
  int ours, theirs, perturn, upkeep;
  QString str, ourbuf, theirbuf;
  struct research *research;

  perturn = get_bulbs_per_turn(&ours, &team, &theirs);

  research = research_get(client_player());
  upkeep = client_player()->client.tech_upkeep;

  if (nullptr == client.conn.playing
      || (ours == 0 && theirs == 0 && upkeep == 0)) {
    return _("Progress: no research");
  }

  if (A_UNSET == research->researching) {
    str = _("Progress: no research");
  } else {
    int turns;

    if ((turns = turns_per_advance(research, perturn)) >= 0) {
      str += QString(PL_("Progress: %1 turn/advance",
                         "Progress: %1 turns/advance", turns))
                 .arg(turns);
    } else if ((turns = turns_to_tech_loss(research, perturn)) >= 0) {
      /* FIXME: turns to next loss is not a good predictor of turns to
       * following loss, due to techloss_restore etc. But it'll do. */
      str += QString(PL_("Progress: %1 turn/advance loss",
                         "Progress: %1 turns/advance loss", turns))
                 .arg(turns);
    } else {
      // no forward progress -- no research, or tech loss disallowed
      if (perturn < 0) {
        str += QString(_("Progress: decreasing!"));
      } else {
        str += QString(_("Progress: none"));
      }
    }
  }
  ourbuf = QString(PL_("%1 bulb/turn", "%1 bulbs/turn", ours)).arg(ours);
  if (team) {
    /* TRANS: This is appended to "%d bulb/turn" text */
    theirbuf = QString(PL_(", %1 bulb/turn from team",
                           ", %1 bulbs/turn from team", theirs))
                   .arg(theirs);
  } else {
    theirbuf = QLatin1String("");
  }
  str += QStringLiteral(" (%1%2)").arg(ourbuf, theirbuf);

  if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
    // perturn is defined as: (bulbs produced) - upkeep
    str += QString(_("Bulbs produced per turn: %1"))
               .arg(QString::number(perturn + upkeep))
           + qendl();
    /* TRANS: keep leading space; appended to "Bulbs produced per turn: %d"
     */
    str += QString(_(" (needed for technology upkeep: %1)"))
               .arg(QString::number(upkeep));
  }

  return str.trimmed();
}

/**
   Get the short science-target text.  This is usually shown directly in
   the progress bar.

      5/28 - 3 turns

   The "percent" value, if given, will be set to the completion percentage
   of the research target (actually it's a [0,1] scale not a percent).
 */
const QString get_science_target_text(double *percent)
{
  struct research *research = research_get(client_player());
  QString str;

  if (!research) {
    return QStringLiteral("-");
  }

  if (research->researching == A_UNSET) {
    str = QString(_("%1/- (never)"))
              .arg(QString::number(research->bulbs_researched));
    if (percent) {
      *percent = 0.0;
    }
  } else {
    int total = research->client.researching_cost;
    int done = research->bulbs_researched;
    int perturn = get_bulbs_per_turn(nullptr, nullptr, nullptr);
    int turns;

    if ((turns = turns_to_research_done(research, perturn)) >= 0) {
      str = QString(PL_("%1/%2 (%3 turn)", "%1/%2 (%3 turns)", turns))
                .arg(QString::number(done), QString::number(total),
                     QString::number(turns));
    } else if ((turns = turns_to_tech_loss(research, perturn)) >= 0) {
      str = QString(PL_("%1/%2 (%3 turn to loss)",
                        "%1/%2 (%3 turns to loss)", turns))
                .arg(QString::number(done), QString::number(total),
                     QString::number(turns));
    } else {
      // no forward progress -- no research, or tech loss disallowed
      str = QString(_("%1/%2 (never)"))
                .arg(QString::number(done), QString::number(total));
    }
    if (percent) {
      *percent = static_cast<double>(done) / static_cast<double>(total);
      *percent = CLIP(0.0, *percent, 1.0);
    }
  }

  return str.trimmed();
}

/**
   Set the science-goal-label text as if we're researching the given goal.
 */
const QString get_science_goal_text(Tech_type_id goal)
{
  const struct research *research = research_get(client_player());
  int steps = research_goal_unknown_techs(research, goal);
  int bulbs_needed = research_goal_bulbs_required(research, goal);
  int turns;
  int perturn = get_bulbs_per_turn(nullptr, nullptr, nullptr);
  QString str, buf1, buf2, buf3;

  if (!research) {
    return QStringLiteral("-");
  }

  if (research_goal_tech_req(research, goal, research->researching)
      || research->researching == goal) {
    bulbs_needed -= research->bulbs_researched;
  }

  buf1 =
      QString(PL_("%1 step", "%1 steps", steps)).arg(QString::number(steps));
  buf2 = QString(PL_("%1 bulb", "%1 bulbs", bulbs_needed))
             .arg(QString::number(bulbs_needed));
  if (perturn > 0) {
    turns = (bulbs_needed + perturn - 1) / perturn;
    buf3 = QString(PL_("%1 turn", "%1 turns", turns))
               .arg(QString::number(turns));
  } else {
    buf3 = QString(_("never"));
  }
  str += QStringLiteral("(%1 - %2 - %3)").arg(buf1, buf2, buf3);

  return str.trimmed();
}

/**
   Return the text for the label on the info panel.  (This is traditionally
   shown to the left of the mapview.)

   Clicking on this text should bring up the get_info_label_text_popup text.
 */
const QString get_info_label_text(bool moreinfo)
{
  QString str;

  if (nullptr != client.conn.playing) {
    str = QString(_("Population: %1"))
              .arg(population_to_text(civ_population(client.conn.playing)))
          + qendl();
  }
  str += QString(_("Year: %1 (T%2)"))
             .arg(calendar_text(), QString::number(game.info.turn))
         + qendl();

  if (nullptr != client.conn.playing) {
    str += QString(_("Gold: %1 (%2)"))
               .arg(QString::number(client.conn.playing->economic.gold),
                    QString::number(
                        player_get_expected_income(client.conn.playing)))
           + qendl();
    str += QString(_("Tax: %1 Lux: %2 Sci: %3"))
               .arg(QString::number(client.conn.playing->economic.tax),
                    QString::number(client.conn.playing->economic.luxury),
                    QString::number(client.conn.playing->economic.science))
           + qendl();
  }
  if (game.info.phase_mode == PMT_PLAYERS_ALTERNATE) {
    if (game.info.phase < 0 || game.info.phase >= player_count()) {
      str += QString(_("Moving: Nobody")) + qendl();
    } else {
      str += QString(_("Moving: %1"))
                 .arg(player_name(player_by_number(game.info.phase)))
             + qendl();
    }
  } else if (game.info.phase_mode == PMT_TEAMS_ALTERNATE) {
    if (game.info.phase < 0 || game.info.phase >= team_count()) {
      str += QString(_("Moving: Nobody")) + qendl();
    } else {
      str += QString(_("Moving: %1"))
                 .arg(team_name_translation(team_by_number(game.info.phase)))
             + qendl();
    }
  }

  if (moreinfo) {
    str += QString(_("(Click for more info)")) + qendl();
  }

  return str.trimmed();
}

/**
   Return the text for the popup label on the info panel.  (This is
   traditionally done as a popup whenever the regular info text is clicked
   on.)
 */
const QString get_info_label_text_popup()
{
  QString str;

  if (nullptr != client.conn.playing) {
    str = QString(_("%1 People"))
              .arg(population_to_text(civ_population(client.conn.playing)))
          + qendl();
  }
  str += QString(_("Year: %1")).arg(calendar_text()) + qendl();
  str +=
      QString(_("Turn: %1")).arg(QString::number(game.info.turn)) + qendl();

  if (nullptr != client.conn.playing) {
    const struct research *presearch = research_get(client_player());
    int perturn = get_bulbs_per_turn(nullptr, nullptr, nullptr);
    int upkeep = client_player()->client.tech_upkeep;

    str += QString(_("Gold: %1"))
               .arg(QString::number(client.conn.playing->economic.gold))
           + qendl();
    str += QString(_("Net Income: %1"))
               .arg(QString::number(
                   player_get_expected_income(client.conn.playing)))
           + qendl();
    // TRANS: Gold, luxury, and science rates are in percentage values.
    str += QString(_("National budget: Gold:%1% Luxury:%2% Science:%3%"))
               .arg(QString::number(client.conn.playing->economic.tax),
                    QString::number(client.conn.playing->economic.luxury),
                    QString::number(client.conn.playing->economic.science))
           + qendl();

    str += QString(_("Researching %1: %2"))
               .arg(research_advance_name_translation(
                        presearch, presearch->researching),
                    get_science_target_text(nullptr))
           + qendl();
    // perturn is defined as: (bulbs produced) - upkeep
    if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
      str += QString(_("Bulbs per turn: %1 - %2 = %3"))
                 .arg(QString::number(perturn + upkeep),
                      QString::number(upkeep), QString::number(perturn))
             + qendl();
    } else {
      fc_assert(upkeep == 0);
      str += QString(_("Bulbs per turn: %1")).arg(QString::number(perturn))
             + qendl();
    }
    {
      int history_perturn = nation_history_gain(client.conn.playing);
      city_list_iterate(client.conn.playing->cities, pcity)
      {
        history_perturn += city_history_gain(pcity);
      }
      city_list_iterate_end;
      str += QString(_("Culture: %1 (%2/turn)"))
                 .arg(QString::number(client.conn.playing->client.culture),
                      QString::number(history_perturn))
             + qendl();
    }
  }

  if (game.info.global_warming) {
    int chance, rate;
    global_warming_scaled(&chance, &rate, 100);
    str += QString(_("Global warming chance: %1% (%2%/turn)"))
               .arg(QString::number(chance), QString::number(rate))
           + qendl();
  } else {
    str += QString(_("Global warming deactivated.")) + qendl();
  }

  if (game.info.nuclear_winter) {
    int chance, rate;
    nuclear_winter_scaled(&chance, &rate, 100);
    str += QString(_("Nuclear winter chance: %1% (%2%/turn)"))
               .arg(QString::number(chance), QString::number(rate))
           + qendl();
  } else {
    str += QString(_("Nuclear winter deactivated.")) + qendl();
  }

  if (nullptr != client.conn.playing) {
    str += QString(_("Government: %1"))
               .arg(government_name_for_player(client.conn.playing))
           + qendl();
  }

  return str.trimmed();
}

/**
   Return text about upgrading these unit lists.

   Returns TRUE iff any units can be upgraded.
 */
bool get_units_upgrade_info(char *buf, size_t bufsz,
                            const std::vector<unit *> &punits)
{
  if (punits.empty()) {
    fc_snprintf(buf, bufsz, _("No units to upgrade!"));
    return false;
  } else if (punits.size() == 1) {
    return (UU_OK == unit_upgrade_info(punits.front(), buf, bufsz));
  } else {
    int upgrade_cost = 0;
    int num_upgraded = 0;
    int min_upgrade_cost = FC_INFINITY;

    for (const auto punit : punits) {
      if (unit_owner(punit) == client_player()
          && UU_OK == unit_upgrade_test(punit, false)) {
        const struct unit_type *from_unittype = unit_type_get(punit);
        const struct unit_type *to_unittype =
            can_upgrade_unittype(client.conn.playing, from_unittype);
        int cost = unit_upgrade_price(unit_owner(punit), from_unittype,
                                      to_unittype);

        num_upgraded++;
        upgrade_cost += cost;
        min_upgrade_cost = MIN(min_upgrade_cost, cost);
      }
    }
    if (num_upgraded == 0) {
      fc_snprintf(buf, bufsz, _("None of these units may be upgraded."));
      return false;
    } else {
      /* This may trigger sometimes if you don't have enough money for
       * a full upgrade.  If you have enough to upgrade at least one, it
       * will do it. */
      /* Construct prompt in several parts to allow separate pluralisation
       * by localizations */
      char tbuf[MAX_LEN_MSG], ubuf[MAX_LEN_MSG];
      fc_snprintf(tbuf, ARRAY_SIZE(tbuf),
                  PL_("Treasury contains %d gold.",
                      "Treasury contains %d gold.",
                      client_player()->economic.gold),
                  client_player()->economic.gold);
      /* TRANS: this whole string is a sentence fragment that is only ever
       * used by including it in another string (search comments for this
       * string to find it) */
      fc_snprintf(ubuf, ARRAY_SIZE(ubuf),
                  PL_("Upgrade %d unit", "Upgrade %d units", num_upgraded),
                  num_upgraded);
      /* TRANS: This is complicated. The first %s is a pre-pluralised
       * sentence fragment "Upgrade %d unit(s)"; the second is pre-pluralised
       * "Treasury contains %d gold." So the whole thing reads
       * "Upgrade 13 units for 1000 gold?\nTreasury contains 2000 gold." */
      fc_snprintf(
          buf, bufsz,
          PL_("%s for %d gold?\n%s", "%s for %d gold?\n%s", upgrade_cost),
          ubuf, upgrade_cost, tbuf);
      return true;
    }
  }
}

/**
   Returns a description of the given spaceship.  If there is no spaceship
   (pship is nullptr) then text with dummy values is returned.
 */
const QString get_spaceship_descr(struct player_spaceship *pship)
{
  struct player_spaceship ship;
  QString str;

  if (!pship) {
    pship = &ship;
    memset(&ship, 0, sizeof(ship));
  }
  // TRANS: spaceship text; should have constant width.
  str += QString(_("Population:      %1"))
             .arg(QString::number(pship->population))
         + qendl();
  // TRANS: spaceship text; should have constant width.
  str += QString(_("Support:         %1 %"))
             .arg(QString::number(
                 static_cast<int>(pship->support_rate * 100.0)))
         + qendl();
  // TRANS: spaceship text; should have constant width.
  str +=
      QString(_("Energy:          %1 %"))
          .arg(QString::number(static_cast<int>(pship->energy_rate * 100.0)))
      + qendl();
  // TRANS: spaceship text; should have constant width.
  str += QString(PL_("Mass:            %1 ton", "Mass:            %1 tons",
                     pship->mass))
             .arg(QString::number(pship->mass))
         + qendl();
  if (pship->propulsion > 0) {
    // TRANS: spaceship text; should have constant width.
    str += QString(_("Travel time:     %1 years"))
               .arg(QString::number(static_cast<float>(
                   0.1 * (static_cast<int>(pship->travel_time * 10.0)))))
           + qendl();
  } else {
    // TRANS: spaceship text; should have constant width.
    str += QStringLiteral("%1").arg(_("Travel time:        N/A     "))
           + qendl();
  }
  // TRANS: spaceship text; should have constant width.
  str += QString(_("Success prob.:   %1 %"))
             .arg(QString::number(
                 static_cast<int>(pship->success_rate * 100.0)))
         + qendl();
  // TRANS: spaceship text; should have constant width.
  str += QString(_("Year of arrival: %1"))
             .arg((pship->state == SSHIP_LAUNCHED)
                      ? textyear((pship->launch_year
                                  + static_cast<int>(pship->travel_time)))
                      : "-   ")
         + qendl();

  return str.trimmed();
}

/**
   Return text giving the score of the player. This should only be used
   in playerdlg_common.c.
 */
QString get_score_text(const struct player *pplayer)
{
  QString str;

  if (pplayer->score.game > 0 || nullptr == client.conn.playing
      || pplayer == client.conn.playing) {
    str = QStringLiteral("%1").arg(QString::number(pplayer->score.game));
  } else {
    str = QStringLiteral("?");
  }

  return str.trimmed();
}

/**
   Returns custom part of the action selection dialog button text for the
   specified action (given that the action is possible).
 */
const QString get_act_sel_action_custom_text(struct action *paction,
                                             const struct act_prob prob,
                                             const struct unit *actor_unit,
                                             const struct city *target_city)
{
  struct city *actor_homecity = unit_home(actor_unit);
  QString custom;
  if (!action_prob_possible(prob)) {
    // No info since impossible.
    return nullptr;
  }

  fc_assert_ret_val((action_get_target_kind(paction) != ATK_CITY
                     || target_city != nullptr),
                    nullptr);

  if (action_has_result(paction, ACTRES_TRADE_ROUTE)) {
    int revenue = get_caravan_enter_city_trade_bonus(
        actor_homecity, target_city, client_player(), actor_unit->carrying,
        true);

    custom = QString(
                 /* TRANS: Estimated one time bonus and recurring revenue for
                  * the Establish Trade _Route action. */
                 _("%1 one time bonus + %2 trade"))
                 .arg(QString::number(revenue),
                      QString::number(trade_base_between_cities(
                          actor_homecity, target_city)));
  } else if (action_has_result(paction, ACTRES_MARKETPLACE)) {
    int revenue = get_caravan_enter_city_trade_bonus(
        actor_homecity, target_city, client_player(), actor_unit->carrying,
        false);

    custom = QString(
                 /* TRANS: Estimated one time bonus for the Enter Marketplace
                  * action. */
                 _("%1 one time bonus"))
                 .arg(QString::number(revenue));
  } else if ((action_has_result(paction, ACTRES_HELP_WONDER)
              || action_has_result(paction, ACTRES_RECYCLE_UNIT))
             && city_owner(target_city) == client.conn.playing) {
    /* Can only give remaining production for domestic and existing
     * cities. */
    int cost = city_production_build_shield_cost(target_city);
    custom = QString(_("%1 remaining"))
                 .arg(QString::number(cost - target_city->shield_stock));
  } else {
    // No info to add.
    return nullptr;
  }

  return custom;
}

/**
   Get information about starting the action in the current situation.
   Suitable for a tool tip for the button that starts it.
   @return an explanation of a tool tip button suitable for a tool tip
 */
const QString act_sel_action_tool_tip(const struct action *paction,
                                      const struct act_prob prob)
{
  Q_UNUSED(paction)
  return action_prob_explain(prob);
}

namespace /* anonymous */ {

/**
 * Builds text intended at explaining the value taken by an effect.
 */
QString text_happiness_effect_details(const city *pcity, effect_type effect)
{
  auto effects = effect_list_new();
  get_city_bonus_effects(effects, pcity, nullptr, effect);

  // TRANS: Precedes a list of active effects, pluralized on its length.
  QString str = PL_(
      "The following contribution is active:",
      "The following contributions are active:", effect_list_size(effects));
  str += QStringLiteral("<ul>");

  char help_text_buffer[MAX_LEN_PACKET];

  effect_list_iterate(effects, peffect)
  {
    str += QStringLiteral("<li>");
    if (requirement_vector_size(&peffect->reqs) == 0) {
      // TRANS: Describes an effect without requirements; %1 is its value
      str += QString(_("%1 by default"))
                 .arg(effect_type_unit_text(peffect->type, peffect->value));
    } else {
      help_text_buffer[0] = '\0';
      get_effect_req_text(peffect, help_text_buffer,
                          sizeof(help_text_buffer));
      // TRANS: Describes an effect; %1 is its value and %2 the requirements
      str += QString(_("%1 from %2"))
                 .arg(effect_type_unit_text(peffect->type, peffect->value))
                 .arg(help_text_buffer);
    }
    str += QStringLiteral("</li>");
  }
  effect_list_iterate_end;
  effect_list_destroy(effects);

  str += QStringLiteral("</ul>");

  return str;
}
} // anonymous namespace

/**
 * Describe buildings that affect happiness (or rather, anything with a
 * Make_Content effect).
 */
QString text_happiness_buildings(const struct city *pcity)
{
  if (const auto effects = get_effects(EFT_MAKE_CONTENT);
      effect_list_size(effects) == 0) {
    return QString(); // Disabled in the ruleset.
  }

  auto str = QStringLiteral("<p>");
  str += _("Infrastructure can have an effect on citizen happiness.");
  str += QStringLiteral(" ");

  int bonus = get_city_bonus(pcity, EFT_MAKE_CONTENT);
  if (bonus <= 0) {
    // TRANS: Comes after "Infrastructure can have an effect on citizen
    //        happiness"
    str += _("This city doesn't receive any such bonus.");
    return str + QStringLiteral("</p>");
  }

  // TRANS: Comes after "Infrastructure can have an effect on citizen
  //        happiness"
  str +=
      QString(
          PL_("In this city, it can make up to <b>%1 citizen<b> content.",
              "In this city, it can make up to <b>%1 citizens<b> content.",
              bonus))
          .arg(bonus);

  // Add a list of active effects
  str += QStringLiteral("</p><p>");
  str += text_happiness_effect_details(pcity, EFT_MAKE_CONTENT);

  return str + QStringLiteral("</ul></p>");
}

/**
 * Describing nationality effects that affect happiness.
 */
QString text_happiness_nationality(const struct city *pcity)
{
  if (auto effects = get_effects(EFT_ENEMY_CITIZEN_UNHAPPY_PCT);
      !game.info.citizen_nationality || effect_list_size(effects) == 0) {
    // Disabled in the ruleset
    return QString();
  }

  auto str = QStringLiteral("<p>");
  str +=
      _("The presence of enemy citizens can create additional unhappiness.");
  str += QStringLiteral(" ");

  int pct = get_city_bonus(pcity, EFT_ENEMY_CITIZEN_UNHAPPY_PCT);
  if (pct == 0) {
    str += _("However, it is not the case in this city.");
    return str + QStringLiteral("</p>");
  }

  // This is not exactly correct, but gives a first idea.
  int num = std::ceil(100. / pct);
  str += QString(PL_("For every %1 citizen of an enemy nation, one citizen "
                     "becomes unhappy.",
                     "For every %1 citizens of an enemy nation, one citizen "
                     "becomes unhappy.",
                     num))
             .arg(num);
  str += QStringLiteral("</p><p>");

  int enemies = 0;
  const auto owner = city_owner(pcity);
  citizens_foreign_iterate(pcity, pslot, nationality)
  {
    if (pplayers_at_war(owner, player_slot_get_player(pslot))) {
      enemies += nationality;
    }
  }
  citizens_foreign_iterate_end;

  if (enemies == 0) {
    str += _("There is <b>no enemy citizen</b> in this city.");
  } else {
    auto unhappy = enemies * pct / 100;
    // TRANS: "There is 1 enemy citizen in this city, resulting in <b>2
    //        additional unhappy citizens.</b>" (first half)
    str +=
        QString(PL_("There is %1 enemy citizen in this city, ",
                    "There are %1 enemy citizens in this city, ", enemies))
            .arg(enemies);
    // TRANS: "There is 1 enemy citizen in this city, resulting in <b>2
    //        additional unhappy citizens.</b>" (second half)
    str += QString(PL_("resulting in <b>%1 additional unhappy citizen.</b>",
                       "resulting in <b>%1 additional unhappy citizens.</b>",
                       unhappy))
               .arg(unhappy);
  }

  return str + QStringLiteral("</p>");
}

/**
   Describing wonders that affect happiness.
 */
QString text_happiness_wonders(const struct city *pcity)
{
  int effects_count = effect_list_size(get_effects(EFT_MAKE_HAPPY))
                      + effect_list_size(get_effects(EFT_NO_UNHAPPY))
                      + effect_list_size(get_effects(EFT_FORCE_CONTENT));
  if (effects_count == 0) {
    // Disabled in the ruleset
    return QString();
  }

  auto str = QStringLiteral("<p>");
  bool wrote_something = false;

  int happy_bonus = get_city_bonus(pcity, EFT_MAKE_HAPPY);
  if (happy_bonus > 0) {
    if (happy_bonus == 1) {
      str += _("Up to one content citizen in this city can be made happy.");
    } else {
      str +=
          QString(
              PL_("Up to %1 unhappy or content citizen in this city can be "
                  "made happy (unhappy citizens count double).",
                  "Up to %1 unhappy or content citizens in this city "
                  "can be made happy (unhappy citizens count double).",
                  happy_bonus))
              .arg(happy_bonus);
    }

    // Add a list of active effects
    str += QStringLiteral("</p><p>");
    str += text_happiness_effect_details(pcity, EFT_MAKE_HAPPY);
    str += QStringLiteral("</p><p>");
    wrote_something = true;
  }

  if (get_city_bonus(pcity, EFT_NO_UNHAPPY) > 0) {
    str += _("No citizens in this city can ever be unhappy or angry. "
             "Any unhappy or angry citizen are automatically made content.");
  } else if (int bonus = get_city_bonus(pcity, EFT_FORCE_CONTENT);
             bonus > 0) {
    str += QStringLiteral("</p><p>");
    str +=
        QString(PL_("Up to %1 unhappy or angry citizen in this city can be "
                    "made content.",
                    "Up to %1 unhappy or angry citizens in this city can be "
                    "made content.",
                    bonus))
            .arg(bonus);

    // Add a list of active effects
    str += QStringLiteral("</p><p>");
    str += text_happiness_effect_details(pcity, EFT_FORCE_CONTENT);
    wrote_something = true;
  }

  if (!wrote_something) {
    // Make sure there's always something printed.
    str += _("Happiness is currently not changed at this stage.");
  }

  return str + QStringLiteral("</p>");
}

namespace /* anonymous */ {

/**
 * Describes the rules of city happiness for the given player.
 */
QString text_happiness_cities_rules(const player *pplayer, int base_content,
                                    int basis, int step,
                                    bool depends_on_empire_size)
{
  auto str = QString();

  bool next_citizens_are_angry = false;
  if (base_content == 0) {
    str += _("All cities start with all citizens unhappy.");
    next_citizens_are_angry = game.info.angrycitizen;
  } else {
    str += QString(PL_("All cities start with %1 content citizen.",
                       "All cities start with %1 content citizens.",
                       base_content))
               .arg(base_content);
  }
  if (depends_on_empire_size) {
    if (basis > 0) {
      str += QStringLiteral(" ");
      if (next_citizens_are_angry) {
        str += QString(PL_("Once you have more than %1 city, a citizen "
                           "becomes angry.",
                           "Once you have more than %1 cities, a citizen "
                           "becomes angry.",
                           basis))
                   .arg(basis);
      } else if (base_content == 1) {
        // TRANS: Comes after "All cities start with 1 content citizen."
        //        "it" is the citizen.
        str +=
            QString(
                PL_("Once you have more than %1 city, it becomes unhappy.",
                    "Once you have more than %1 cities, it becomes unhappy.",
                    basis))
                .arg(basis);
        next_citizens_are_angry = game.info.angrycitizen;
      } else if (base_content > 1) {
        // TRANS: Comes after "All cities start with N content citizens."
        str += QString(PL_("Once you have more than %1 city, a citizen "
                           "becomes unhappy.",
                           "Once you have more than %1 cities, a citizen "
                           "becomes unhappy.",
                           basis))
                   .arg(basis);
      }
    }

    if (step > 0) {
      str += QStringLiteral(" ");
      if (next_citizens_are_angry) {
        str += QString(PL_("Afterwards, for every %1 additional city, a "
                           "citizen becomes angry.",
                           "Afterwards, for every %1 additional cities, a "
                           "citizen becomes angry.",
                           step))
                   .arg(step);
      } else {
        str += QString(PL_("Afterwards, for every %1 additional city, a "
                           "content citizen becomes unhappy.",
                           "Afterwards, for every %1 additional cities, a "
                           "content citizen becomes unhappy.",
                           step))
                   .arg(step);

        if (game.info.angrycitizen) {
          str += QStringLiteral(" ");
          str += _("If there are no more content citizens, an unhappy "
                   "citizen becomes angry instead.");
        }
      }
    }
  }
  return str;
}

/**
 * Describes what the empire size rules imply for an empire of a given size.
 */
QString text_happiness_cities_apply_rules(int cities, int max_content)
{
  if (max_content > 0) {
    // TRANS: Pluralized in "%2 content citizens"
    return QString(PL_("You have %1 cities, resulting in a maximum of %2 "
                       "content citizen.",
                       "You have %1 cities, resulting in a maximum of %2 "
                       "content citizens.",
                       max_content))
        .arg(cities)
        .arg(max_content);
  } else if (max_content == 0) {
    return _("You have %1 cities, thus all citizens are unhappy.");
  } else {
    // TRANS: Pluralized in "%2 angry citizens"
    return QString(PL_("You have %1 cities, resulting in a maximum of "
                       "%2 angry citizen.",
                       "You have %1 cities, resulting in a maximum of "
                       "%2 angry citizens.",
                       max_content))
        .arg(cities)
        .arg(-max_content);
  }
}

/**
 * Describes how many citizens are content after empire size rules.
 */
QString text_happiness_cities_content(int size, int max_content)
{
  if (max_content >= size) {
    // Very good
    return QString(_("In this city of size %1, <b>all citizens are "
                     "content.</b>"))
        .arg(size);
  } else if (max_content > 0) {
    // Good
    // TRANS: Pluralized in "citizens are content"
    return QString(
               PL_("In this city of size %1, <b>%2 citizen is content.</b>",
                   "In this city of size %1, <b>%2 citizens are "
                   "content.</b>",
                   max_content))
        .arg(size)
        .arg(max_content);
  } else if (max_content == 0) {
    // Still ok
    return QString(
               "In this city of size %1, <b>all citizens are unhappy.</b>")
        .arg(size);
  } else if (-max_content < size) {
    // Somewhat bad
    return QString(
               PL_(
                   // TRANS: Pluralized in "citizens are angry"
                   "In this city of size %1, <b>%2 citizen is angry.</b>",
                   "In this city of size %1, <b>%2 citizens are angry.</b>",
                   -max_content))
        .arg(size)
        .arg(-max_content);
  } else {
    // Very bad
    return QString(
               _("In this city of size %1, <b>all citizens are angry.</b>"))
        .arg(size);
  }
}

/**
 * Describes what would happen to a city's citizens if the empire size would
 * grow further.
 */
QString text_happiness_more_cities(int base_content, int basis, int step,
                                   int max_content, int size, int cities)
{
  // Actual content in this city (negative if there are angry citizens
  // instead)
  int content = std::min<int>(max_content, size);
  if (!game.info.angrycitizen && content < 0) {
    content = 0;
  }
  // How many citizens unhappy about the empire size we'll have when the
  // next unhappy appears
  int unhappy_for_next_threshold = base_content - content + 1;
  // How many cities we'll have when the next unhappy appears
  int cities_to_next_threshold =
      basis + (unhappy_for_next_threshold - 1) * step + 1;
  // ...or how many more we need
  cities_to_next_threshold -= cities;

  if (cities_to_next_threshold > 0) {
    if (content > 0) {
      return QString(PL_("With %1 more city, a content citizen would become "
                         "unhappy.",
                         "With %1 more cities, a content citizen would "
                         "become unhappy.",
                         cities_to_next_threshold))
          .arg(QString::number(cities_to_next_threshold));
    } else if (game.info.angrycitizen) {
      // We maxed out the number of unhappy citizens, but they can get
      // angry instead.
      return QString(
                 PL_("With %1 more city, a citizen would become angry.",
                     "With %1 more cities, a citizen would become angry.",
                     cities_to_next_threshold))
          .arg(QString::number(cities_to_next_threshold));
    }
  }
  return _("Having more cities would not create more unhappiness.");
}
} // anonymous namespace

/**
   Describing city factors that affect happiness.
 */
QString text_happiness_cities(const struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  int cities = city_list_size(pplayer->cities);
  int base_content = get_player_bonus(pplayer, EFT_CITY_UNHAPPY_SIZE);
  int basis = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE);
  int step = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_STEP);
  bool depends_on_empire_size = (basis + step > 0);

  while (depends_on_empire_size && basis <= 0) {
    // In this case, we get one unhappy immediately when we build the first
    // city. So it's equivalent to removing one content.
    base_content--;
    if (step > 0) {
      // The first unhappy appears at a different size. Normalize...
      basis += step;
    } else {
      // This was fake! The ruleset author did something strange.
      depends_on_empire_size = false;
      break;
    }
  }

  // First explain the rules -- see player_base_citizen_happiness
  auto str = QStringLiteral("<p>");
  str += text_happiness_cities_rules(pplayer, base_content, basis, step,
                                     depends_on_empire_size);

  // Now add the status of this city.
  str += QStringLiteral("</p><p>");
  auto max_content = player_base_citizen_happiness(city_owner(pcity));
  if (!game.info.angrycitizen) {
    // No angry citizens: max_content can't go negative
    max_content = CLIP(0, max_content, MAX_CITY_SIZE);
  }
  // If the penalty is enabled, explain it.
  if (depends_on_empire_size) {
    str += text_happiness_cities_apply_rules(cities, max_content);
  }

  auto size = city_size_get(pcity);
  str += QStringLiteral(" ");
  str += text_happiness_cities_content(size, max_content);

  // Finally, add something about building more cities.
  if (depends_on_empire_size) {
    str += QStringLiteral(" ");
    str += text_happiness_more_cities(base_content, basis, step, max_content,
                                      size, cities);
  }

  return str + QStringLiteral("</p>");
}

/**
 * Describe units that affect happiness.
 */
QString text_happiness_units(const struct city *pcity)
{
  auto str = QString();

  /*
   * First part: martial law
   */
  int martial_law_max = get_city_bonus(pcity, EFT_MARTIAL_LAW_MAX);
  int martial_law_each = get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH);
  if (martial_law_each > 0 && martial_law_max >= 0) {
    str += QStringLiteral("<p>");
    // The rules
    if (martial_law_max == 0) {
      str += _("Every military unit in the city may impose martial law.");
    } else {
      str +=
          QString(PL_("%1 military unit in the city may impose martial law.",
                      "Up to %1 military units in the city may impose "
                      "martial law.",
                      martial_law_max))
              .arg(martial_law_max);
    }

    str += QStringLiteral(" ");
    str += QString(PL_("Each of them makes %1 unhappy citizen content.",
                       "Each of them makes %1 unhappy citizens content.",
                       martial_law_each))
               .arg(martial_law_each);

    str += QStringLiteral("</p><p>");

    int count = 0;
    unit_list_iterate(pcity->tile->units, punit)
    {
      if ((count < martial_law_max || martial_law_max == 0)
          && is_military_unit(punit)
          && unit_owner(punit) == city_owner(pcity)) {
        count++;
      }
    }
    unit_list_iterate_end;

    str +=
        QString(PL_("%1 military unit in this city imposes the martial law.",
                    "%1 military units in this city impose the martial law.",
                    count))
            .arg(count);
    str += QStringLiteral(" ");
    str += QString(PL_("<b>%1 citizen</b> is made happier as a result.",
                       "<b>%1 citizens</b> are made happier as a result.",
                       count * martial_law_each))
               .arg(count * martial_law_each);

    str += QStringLiteral("</p>");
  }

  /*
   * Second part: military unhappiness
   *
   * Had to say anything here because this is unit dependent (and I don't
   * want to build a list of unit types or so, this is shown elsewhere).
   */
  str += QStringLiteral("<p>");

  auto pplayer = city_owner(pcity);
  int unhappy_factor = get_player_bonus(pplayer, EFT_UNHAPPY_FACTOR);
  if (unhappy_factor == 0) {
    str += _("Military units have no happiness effect.");
    return str + QStringLiteral("</p>");
  }

  str += _("Military units in the field may cause unhappiness.");

  // Special rule: ignore the first N military unhappy
  int made_content = get_city_bonus(pcity, EFT_MAKE_CONTENT_MIL);
  if (made_content > 0) {
    str += QStringLiteral(" ");
    str += QString(PL_("With respect to normal, %1 fewer citizen will be "
                       "unhappy about military units.",
                       "With respect to normal, %1 fewer citizens will be "
                       "unhappy about military units.",
                       made_content))
               .arg(made_content);
  }

  // Special rule: ignore N military unhappy per unit
  // Let's hope ruleset authors don't combine them too often, the sentences
  // are very similar.
  int made_content_each = get_city_bonus(pcity, EFT_MAKE_CONTENT_MIL_PER);
  if (made_content_each > 0) {
    str += QStringLiteral(" ");
    str += QString(PL_("With respect to normal, %1 fewer citizen will be "
                       "unhappy per each military unit.",
                       "With respect to normal, %1 fewer citizens will be "
                       "unhappy per each military unit.",
                       made_content_each))
               .arg(made_content_each);
  }

  // And finally how many unhappy we get in this city
  int count = 0, happy_upkeep = 0;
  unit_list_iterate(pcity->units_supported, punit)
  {
    int dummy = 0;
    if (auto n = city_unit_unhappiness(punit, &dummy); n > 0) {
      count++;
      happy_upkeep += n;
    }
  }
  unit_list_iterate_end;

  str += QStringLiteral("</p><p>");
  if (count > 0) {
    // TRANS: "This city supports %1 agressive military units, resulting in
    //        up to X unhappy citizens." (first part)
    str += QString(PL_("This city supports %1 agressive military unit, ",
                       "This city supports %1 agressive military units, ",
                       count))
               .arg(count);
    // TRANS: "This city supports X agressive military units, resulting in up
    //        to %1 unhappy citizens." (second part)
    str +=
        QString(
            PL_("resulting in up to <b>%1 additional unhappy citizen.</b>",
                "resulting in up to <b>%1 additional unhappy citizens.</b>",
                happy_upkeep))
            .arg(happy_upkeep);
  } else {
    str += _("Currently, military units do not cause additional unhappiness "
             "in this city.");
  }

  return str + QStringLiteral("</p>");
}

/**
 * Describing luxuries that affect happiness.
 */
QString text_happiness_luxuries(const struct city *pcity)
{
  auto str = QStringLiteral("<p>");

  // Explain the rules
  str += QString(_("For each %1 luxury good produced in a city, a citizen "
                   "becomes happier."))
             .arg(game.info.happy_cost);
  str += QStringLiteral(" ");

  // Give the basis for calculation. Shortcut the rest if there's not enough
  // to help a single citizen.
  auto luxuries = pcity->prod[O_LUXURY];
  if (luxuries == 0) {
    str += _("This city generates no luxury goods.");
    return str + QStringLiteral("</p>");
  } else if (luxuries < game.info.happy_cost) {
    str += QString(PL_("This city generates a total of %1 luxury good, not "
                       "enough to have an effect.",
                       "This city generates a total of %1 luxury goods, not "
                       "enough to have an effect.",
                       luxuries))
               .arg(luxuries);
    return str + QStringLiteral("</p>");
  }

  str += QString(PL_("This city generates a total of %1 luxury good.",
                     "This city generates a total of %1 luxury goods.",
                     luxuries))
             .arg(luxuries);

  str += QStringLiteral("</p><p>");

  auto made_happy = pcity->feel[CITIZEN_HAPPY][FEELING_LUXURY]
                    - pcity->feel[CITIZEN_HAPPY][FEELING_BASE];
  auto made_content = pcity->feel[CITIZEN_CONTENT][FEELING_LUXURY]
                      - pcity->feel[CITIZEN_CONTENT][FEELING_BASE];
  auto made_unhappy = pcity->feel[CITIZEN_UNHAPPY][FEELING_LUXURY]
                      - pcity->feel[CITIZEN_UNHAPPY][FEELING_BASE];
  int citizens_affected = std::max(0, made_happy) + std::max(0, made_content)
                          + std::max(0, made_unhappy);
  int upgrades = made_happy * 3 + made_content * 2 + made_unhappy * 1;
  int cost = upgrades * game.info.happy_cost;

  if (cost < luxuries) {
    str +=
        QString(PL_("%1 citizen is made happier using %2 luxury good(s).",
                    "%1 citizens are made happier using %2 luxury good(s).",
                    citizens_affected))
            .arg(citizens_affected)
            .arg(cost);
    if (luxuries - cost > 0) {
      str += QStringLiteral(" ");
      str += QString(PL_("%1 luxury good is not used.",
                         "%1 luxury goods are not used.", luxuries - cost))
                 .arg(luxuries - cost);
    }
  } else {
    str += QString(PL_("All available luxury goods are used to make %1 "
                       "citizen happier.",
                       "All available luxury goods are used to make %1 "
                       "citizens happier.",
                       citizens_affected))
               .arg(citizens_affected);
  }

  return str + QStringLiteral("</p>");
}
