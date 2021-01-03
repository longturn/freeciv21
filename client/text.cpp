/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <QDateTime>
#include <math.h> /* ceil */
#include <stdarg.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "nation.h"
#include "support.h"

/* common */
#include "calendar.h"
#include "citizens.h"
#include "clientutils.h"
#include "combat.h"
#include "culture.h"
#include "fc_types.h" /* LINE_BREAK */
#include "game.h"
#include "government.h"
#include "map.h"
#include "research.h"
#include "traderoutes.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "control.h"
#include "goto.h"

#include "text.h"

static int get_bulbs_per_turn(int *pours, bool *pteam, int *ptheirs);

/************************************************************************/ /**
   Return a (static) string with a tile's food/prod/trade
 ****************************************************************************/
const QString get_tile_output_text(const struct tile *ptile)
{
  QString str;
  int i;
  char output_text[O_LAST][16];

  for (i = 0; i < O_LAST; i++) {
    int before_penalty = 0;
    int x =
        city_tile_output(NULL, ptile, false, static_cast<Output_type_id>(i));

    if (NULL != client.conn.playing) {
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

/************************************************************************/ /**
   For AIs, fill the buffer with their player name prefixed with "AI". For
   humans, just fill it with their username.
 ****************************************************************************/
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
    /* TRANS: "AI <player name>" */
    fc_snprintf(buf, buflen, _("AI %s"), pplayer->name);
  } else {
    fc_strlcpy(buf, pplayer->username, buflen);
  }
}

/************************************************************************/ /**
   Fill the buffer with the player's nation name (in adjective form) and
   optionally add the player's team name.
 ****************************************************************************/
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
    /* TRANS: "<nation adjective>, team <team name>" */
    fc_snprintf(buf, buflen, _("%s, team %s"),
                nation_adjective_for_player(pplayer),
                team_name_translation(pplayer->team));
  } else {
    fc_strlcpy(buf, nation_adjective_for_player(pplayer), buflen);
  }
}

/************************************************************************/ /**
   Text to popup on a middle-click in the mapview.
 ****************************************************************************/
const QString popup_info_text(struct tile *ptile)
{
  const char *activity_text;
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
  str = str
        + QString(_("Native coordinates: (%1, %2)\n"))
              .arg(QString::number(nat_x), QString::number(nat_y));

  if (client_tile_get_known(ptile) == TILE_UNKNOWN) {
    str += QString(_("Unknown"));
    return str.trimmed();
  }
  str = str
        + QString(_("Terrain: %1")).arg(tile_get_info_text(ptile, true, 0))
        + qendl();
  str = str
        + QString(_("Food/Prod/Trade: %1")).arg(get_tile_output_text(ptile))
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

    if (NULL != client.conn.playing && owner == client.conn.playing) {
      str += QString(_("Our territory")) + qendl();
    } else if (NULL != owner && NULL == client.conn.playing) {
      /* TRANS: "Territory of <username> (<nation + team>)" */
      str +=
          QString(_("Territory of %1 (%2)")).arg(username, nation) + qendl();
    } else if (NULL != owner) {
      struct player_diplstate *ds =
          player_diplstate_get(client.conn.playing, owner);

      if (ds->type == DS_CEASEFIRE) {
        int turns = ds->turns_left;
        /* TRANS: "Territory of <username> (<nation + team>)
         * (<number> turn cease-fire)" */
        str = str
              + QString(PL_("Territory of %1 (%2) (%3 turn cease-fire)",
                            "Territory of %1 (%2) (%3 turn cease-fire)",
                            turns))
                    .arg(username, nation, QString::number(turns))
              + qendl();
      } else if (ds->type == DS_ARMISTICE) {
        int turns = ds->turns_left;
        /* TRANS: "Territory of <username> (<nation + team>)
         * (<number> turn armistice)" */
        str =
            str
            + QString(PL_("Territory of %1 (%2) (%3 turn armistice)",
                          "Territory of %1 (%2) (%3 turn armistice)", turns))
                  .arg(username, nation, QString::number(turns))
            + qendl();
      } else {
        int type = ds->type;
        /* TRANS: "Territory of <username>
         * (<nation + team> | <diplomatic state>)" */
        str = str
              + QString(_("Territory of %1 (%2 | %3)"))
                    .arg(username, nation,
                         diplo_nation_plural_adjectives[type])
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

    if (NULL == client.conn.playing || owner == client.conn.playing) {
      /* TRANS: "City: <city name> | <username> (<nation + team>)" */
      str = str
            + QString(_("City: %1 | %2 (%3)"))
                  .arg(city_name_get(pcity), username, nation)
            + qendl();
    } else {
      struct player_diplstate *ds =
          player_diplstate_get(client_player(), owner);
      if (ds->type == DS_CEASEFIRE) {
        int turns = ds->turns_left;

        /* TRANS:  "City: <city name> | <username>
         * (<nation + team>, <number> turn cease-fire)" */
        str = str
              + QString(PL_("City: %1 | %2 (%3, %4 turn cease-fire)",
                            "City: %1 | %2 (%3, %4 turn cease-fire)", turns))
                    .arg(city_name_get(pcity), username, nation,
                         QString::number(turns))
              + qendl();

      } else if (ds->type == DS_ARMISTICE) {
        int turns = ds->turns_left;

        /* TRANS:  "City: <city name> | <username>
         * (<nation + team>, <number> turn armistice)" */
        str = str
              + QString(PL_("City: %1 | %2 (%3, %4 turn armistice)",
                            "City: %1 | %2 (%3, %4 turn armistice)", turns))
                    .arg(city_name_get(pcity), username, nation,
                         QString::number(turns))
              + qendl();
      } else {
        /* TRANS: "City: <city name> | <username>
         * (<nation + team>, <diplomatic state>)" */
        str = str
              + QString(_("City: %1 | %2 (%3, %4)"))
                    .arg(city_name_get(pcity), username, nation,
                         diplo_city_adjectives[ds->type])
              + qendl();
      }
    }
    if (can_player_see_units_in_city(client_player(), pcity)) {
      int count = unit_list_size(ptile->units);

      if (count > 0) {
        /* TRANS: preserve leading space */
        str = str
              + QString(PL_(" | Occupied with %1 unit.",
                            " | Occupied with %2 units.", count))
                    .arg(QString::number(count));
      } else {
        /* TRANS: preserve leading space */
        str += QString(_(" | Not occupied."));
      }
    } else {
      if (city_is_occupied(pcity)) {
        /* TRANS: preserve leading space */
        str += QString(_(" | Occupied."));
      } else {
        /* TRANS: preserve leading space */
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
      /* TRANS: %s is a list of "and"-separated improvements. */
      str = str
            + QString(_("   with %1.")).arg(strvec_to_and_list(improvements))
            + qendl();
    }

    unit_list_iterate(get_units_in_focus(), pfocus_unit)
    {
      struct city *hcity = game_city_by_number(pfocus_unit->homecity);

      if (utype_can_do_action(unit_type_get(pfocus_unit), ACTION_TRADE_ROUTE)
          && can_cities_trade(hcity, pcity)
          && can_establish_trade_route(hcity, pcity)) {
        /* TRANS: "Trade from Warsaw: 5" */
        str = str
              + QString(_("Trade from %1: %2"))
                    .arg(city_name_get(hcity),
                         QString::number(
                             trade_base_between_cities(hcity, pcity)))
              + qendl();
      }
    }
    unit_list_iterate_end;
  }
  {
    const char *infratext = get_infrastructure_text(ptile->extras);

    if (*infratext != '\0') {
      str += QString(_("Infrastructure: %1")).arg(infratext) + qendl();
    }
  }
  activity_text = concat_tile_activity_text(ptile);
  if (strlen(activity_text) > 0) {
    str += QString(_("Activity: %1")).arg(activity_text) + qendl();
  }
  if (punit && !pcity) {
    struct player *owner = unit_owner(punit);
    const struct unit_type *ptype = unit_type_get(punit);

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    if (!client_player() || owner == client_player()) {
      struct city *hcity = player_city_by_number(owner, punit->homecity);

      /* TRANS: "Unit: <unit type> | <username> (<nation + team>)" */
      str = str
            + QString(_("Unit: %1 | %2 (%3)"))
                  .arg(utype_name_translation(ptype), username, nation)
            + qendl();
      if (game.info.citizen_nationality
          && unit_nationality(punit) != unit_owner(punit)) {
        if (hcity != NULL) {
          /* TRANS: on own line immediately following \n, "from <city> |
           * <nationality> people" */
          str =
              str
              + QString(_("from %1 | %2 people"))
                    .arg(city_name_get(hcity), nation_adjective_for_player(
                                                   unit_nationality(punit)))
              + qendl();
        } else {
          /* TRANS: Nationality of the people comprising a unit, if
           * different from owner. */
          str = str
                + QString(_("%1 people"))
                      .arg(nation_adjective_for_player(
                          unit_nationality(punit)))
                + qendl();
        }
      } else if (hcity != NULL) {
        /* TRANS: on own line immediately following \n, ... <city> */
        str =
            str + QString(_("from %1")).arg(city_name_get(hcity)) + qendl();
      }
    } else if (NULL != owner) {
      struct player_diplstate *ds =
          player_diplstate_get(client_player(), owner);
      if (ds->type == DS_CEASEFIRE) {
        int turns = ds->turns_left;

        /* TRANS:  "Unit: <unit type> | <username> (<nation + team>,
         * <number> turn cease-fire)" */
        str = str
              + QString(PL_("Unit: %1 | %2 (%3, %4 turn cease-fire)",
                            "Unit: %1 | %2 (%3, %4 turn cease-fire)", turns))
                    .arg(utype_name_translation(ptype), username, nation,
                         QString::number(turns))
              + qendl();
      } else if (ds->type == DS_ARMISTICE) {
        int turns = ds->turns_left;

        /* TRANS:  "Unit: <unit type> | <username> (<nation + team>,
         * <number> turn armistice)" */
        str = str
              + QString(PL_("Unit: %1 | %2 (%3, %4 turn armistice)",
                            "Unit: %1 | %2 (%3, %4 turn armistice)", turns))
                    .arg(utype_name_translation(ptype), username, nation,
                         QString::number(turns))
              + qendl();
      } else {
        /* TRANS: "Unit: <unit type> | <username> (<nation + team>,
         * <diplomatic state>)" */
        str = str
              + QString(_("Unit: %1 | %2 (%3, %4)"))
                    .arg(utype_name_translation(ptype), username, nation,
                         diplo_city_adjectives[ds->type])
              + qendl();
      }
    }

    unit_list_iterate(get_units_in_focus(), pfocus_unit)
    {
      int att_chance = FC_INFINITY, def_chance = FC_INFINITY;
      bool found = false;

      unit_list_iterate(ptile->units, tile_unit)
      {
        if (unit_owner(tile_unit) != unit_owner(pfocus_unit)) {
          int att = unit_win_chance(pfocus_unit, tile_unit) * 100;
          int def = (1.0 - unit_win_chance(tile_unit, pfocus_unit)) * 100;

          found = true;

          /* Presumably the best attacker and defender will be used. */
          att_chance = MIN(att, att_chance);
          def_chance = MIN(def, def_chance);
        }
      }
      unit_list_iterate_end;

      if (found) {
        /* TRANS: "Chance to win: A:95% D:46%" */
        str = str
              + QString(_("Chance to win: A:%1% D:%2%"))
                    .arg(QString::number(att_chance),
                         QString::number(def_chance))
              + qendl();
      }
    }
    unit_list_iterate_end;

    /* TRANS: A is attack power, D is defense power, FP is firepower,
     * HP is hitpoints (current and max). */
    str = str
          + QString(_("A:%1 D:%2 FP:%3 HP:%4/%5"))
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
      /* Show bribe cost for own units. */
      str = str
            + QString(_("Probable bribe cost: %1"))
                  .arg(QString::number(unit_bribe_cost(punit, NULL)))
            + qendl();
    } else {
      /* We can only give an (lower) boundary for units of other players. */
      str = str
            + QString(_("Estimated bribe cost: > %1"))
                  .arg(QString::number(
                      unit_bribe_cost(punit, client_player())))
            + qendl();
    }

    if ((NULL == client.conn.playing || owner == client.conn.playing)
        && unit_list_size(ptile->units) >= 2) {
      /* TRANS: "5 more" units on this tile */
      str = str
            + QString(_("  (%1 more)"))
                  .arg(QString::number(unit_list_size(ptile->units) - 1));
    }
  }

  return str.trimmed();
}

#define FAR_CITY_SQUARE_DIST (2 * (6 * 6))
/************************************************************************/ /**
   Returns the text describing the city and its distance.
 ****************************************************************************/
const QString get_nearest_city_text(struct city *pcity, int sq_dist)
{
  if (!pcity) {
    sq_dist = -1;
  }
  QString str =
      QString(
          (sq_dist >= FAR_CITY_SQUARE_DIST)
              /* TRANS: on own line immediately following \n, ... <city> */
              ? _("far from %1")
              : (sq_dist > 0)
                    /* TRANS: on own line immediately following \n, ...
                       <city> */
                    ? _("near %1")
                    : (sq_dist == 0)
                          /* TRANS: on own line immediately following \n, ...
                             <city> */
                          ? _("in %1")
                          : "%1")
          .arg(pcity ? city_name_get(pcity) : "");
  return str.trimmed();
}

/************************************************************************/ /**
   Returns the unit description.
   Used in e.g. city report tooltips.

   FIXME: This function is not re-entrant because it returns a pointer to
   static data.
 ****************************************************************************/
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
    /* TRANS: on own line immediately following \n, ... <city> */
    str += QString(_("from %1")).arg(city_name_get(pcity)) + qendl();
  } else {
    str += qendl();
  }
  if (game.info.citizen_nationality) {
    if (nationality != NULL && owner != nationality) {
      /* TRANS: Nationality of the people comprising a unit, if
       * different from owner. */
      str = str
            + QString(_("%1 people"))
                  .arg(nation_adjective_for_player(nationality))
            + qendl();
    } else {
      str += qendl();
    }
  }
  str = str
        + QStringLiteral("%1").arg(
            get_nearest_city_text(pcity_near, pcity_near_dist))
        + qendl();
#ifdef FREECIV_DEBUG
  str += QString("Unit ID: %1").arg(punit->id);
#endif

  return str.trimmed();
}

/************************************************************************/ /**
   Describe the airlift capacity of a city for the given units (from their
   current positions).
   If pdest is non-NULL, describe its capacity as a destination, otherwise
   describe the capacity of the city the unit's currently in (if any) as a
   source. (If the units in the list are in different cities, this will
   probably not give a useful result in this case.)
   If not all of the listed units can be airlifted, return the description
   for those that can.
   Returns NULL if an airlift is not possible for any of the units.
 ****************************************************************************/
const QString get_airlift_text(const struct unit_list *punits,
                               const struct city *pdest)
{
  QString str;
  bool src = (pdest == NULL);
  enum texttype {
    AL_IMPOSSIBLE,
    AL_UNKNOWN,
    AL_FINITE,
    AL_INFINITE
  } best = AL_IMPOSSIBLE;
  int cur = 0, max = 0;

  unit_list_iterate(punits, punit)
  {
    enum texttype tthis = AL_IMPOSSIBLE;
    enum unit_airlift_result result;

    /* NULL will tell us about the capability of airlifting from source */
    result = test_unit_can_airlift_to(client_player(), punit, pdest);

    switch (result) {
    case AR_NO_MOVES:
    case AR_WRONG_UNITTYPE:
    case AR_OCCUPIED:
    case AR_NOT_IN_CITY:
    case AR_BAD_SRC_CITY:
    case AR_BAD_DST_CITY:
      /* No chance of an airlift. */
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

        fc_assert_ret_val(pcity != NULL, fc_strdup("-"));
        if (!src
            && (game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST)) {
          /* No restrictions on destination (and we can infer this even for
           * other players' cities). */
          tthis = AL_INFINITE;
        } else if (client_player() == city_owner(pcity)) {
          /* A city we know about. */
          int this_cur = pcity->airlift, this_max = city_airlift_max(pcity);

          if (this_max <= 0) {
            /* City known not to be airlift-capable. */
            tthis = AL_IMPOSSIBLE;
          } else {
            if (src
                && (game.info.airlifting_style & AIRLIFTING_UNLIMITED_SRC)) {
              /* Unlimited capacity. */
              tthis = AL_INFINITE;
            } else {
              /* Limited capacity (possibly zero right now). */
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
          /* Unknown capacity. */
          tthis = AL_UNKNOWN;
        }
      }
      break;
    }

    /* Now take the most optimistic view. */
    best = MAX(best, tthis);
  }
  unit_list_iterate_end;

  switch (best) {
  case AL_IMPOSSIBLE:
    return NULL;
  case AL_UNKNOWN:
    str = QLatin1String("?");
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

/************************************************************************/ /**
   Return total expected bulbs.
 ****************************************************************************/
static int get_bulbs_per_turn(int *pours, bool *pteam, int *ptheirs)
{
  const struct research *presearch;
  int ours = 0, theirs = 0;
  bool team = false;

  if (!client_has_player()) {
    return 0;
  }
  presearch = research_get(client_player());

  /* Sum up science */
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

/************************************************************************/ /**
   Return turns until research complete. -1 for never.
 ****************************************************************************/
static int turns_to_research_done(const struct research *presearch,
                                  int per_turn)
{
  if (per_turn > 0) {
    return ceil((double) (presearch->client.researching_cost
                          - presearch->bulbs_researched)
                / per_turn);
  } else {
    return -1;
  }
}

/************************************************************************/ /**
   Return turns per advance (based on currently researched advance).
   -1 for no progress.
 ****************************************************************************/
static int turns_per_advance(const struct research *presearch, int per_turn)
{
  if (per_turn > 0) {
    return MAX(1,
               ceil((double) presearch->client.researching_cost) / per_turn);
  } else {
    return -1;
  }
}

/************************************************************************/ /**
   Return turns until an advance is lost due to tech upkeep.
   -1 if we're not on the way to losing an advance.
 ****************************************************************************/
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

    return ceil((double) bulbs_to_loss / -per_turn);
  }
}

/************************************************************************/ /**
   Returns the text to display in the science dialog.
 ****************************************************************************/
const QString science_dialog_text(void)
{
  bool team;
  int ours, theirs, perturn, upkeep;
  QString str, ourbuf, theirbuf;
  struct research *research;

  perturn = get_bulbs_per_turn(&ours, &team, &theirs);

  research = research_get(client_player());
  upkeep = client_player()->client.tech_upkeep;

  if (NULL == client.conn.playing
      || (ours == 0 && theirs == 0 && upkeep == 0)) {
    return _("Progress: no research");
  }

  if (A_UNSET == research->researching) {
    str = _("Progress: no research");
  } else {
    int turns;

    if ((turns = turns_per_advance(research, perturn)) >= 0) {
      str = str
            + QString(PL_("Progress: %1 turn/advance",
                          "Progress: %1 turns/advance", turns))
                  .arg(turns);
    } else if ((turns = turns_to_tech_loss(research, perturn)) >= 0) {
      /* FIXME: turns to next loss is not a good predictor of turns to
       * following loss, due to techloss_restore etc. But it'll do. */
      str = str
            + QString(PL_("Progress: %1 turn/advance loss",
                          "Progress: %1 turns/advance loss", turns))
                  .arg(turns);
    } else {
      /* no forward progress -- no research, or tech loss disallowed */
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
    /* perturn is defined as: (bulbs produced) - upkeep */
    str = str
          + QString(_("Bulbs produced per turn: %1"))
                .arg(QString::number(perturn + upkeep))
          + qendl();
    /* TRANS: keep leading space; appended to "Bulbs produced per turn: %d"
     */
    str = str
          + QString(_(" (needed for technology upkeep: %1)"))
                .arg(QString::number(upkeep));
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Get the short science-target text.  This is usually shown directly in
   the progress bar.

      5/28 - 3 turns

   The "percent" value, if given, will be set to the completion percentage
   of the research target (actually it's a [0,1] scale not a percent).
 ****************************************************************************/
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
    int perturn = get_bulbs_per_turn(NULL, NULL, NULL);
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
      /* no forward progress -- no research, or tech loss disallowed */
      str = QString(_("%1/%2 (never)"))
                .arg(QString::number(done), QString::number(total));
    }
    if (percent) {
      *percent = (double) done / (double) total;
      *percent = CLIP(0.0, *percent, 1.0);
    }
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Set the science-goal-label text as if we're researching the given goal.
 ****************************************************************************/
const QString get_science_goal_text(Tech_type_id goal)
{
  const struct research *research = research_get(client_player());
  int steps = research_goal_unknown_techs(research, goal);
  int bulbs_needed = research_goal_bulbs_required(research, goal);
  int turns;
  int perturn = get_bulbs_per_turn(NULL, NULL, NULL);
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

/************************************************************************/ /**
   Return the text for the label on the info panel.  (This is traditionally
   shown to the left of the mapview.)

   Clicking on this text should bring up the get_info_label_text_popup text.
 ****************************************************************************/
const QString get_info_label_text(bool moreinfo)
{
  QString str;

  if (NULL != client.conn.playing) {
    str = QString(_("Population: %1"))
              .arg(population_to_text(civ_population(client.conn.playing)))
          + qendl();
  }
  str = str
        + QString(_("Year: %1 (T%2)"))
              .arg(calendar_text(), QString::number(game.info.turn))
        + qendl();

  if (NULL != client.conn.playing) {
    str = str
          + QString(_("Gold: %1 (%2)"))
                .arg(QString::number(client.conn.playing->economic.gold),
                     QString::number(
                         player_get_expected_income(client.conn.playing)))
          + qendl();
    str = str
          + QString(_("Tax: %1 Lux: %2 Sci: %3"))
                .arg(QString::number(client.conn.playing->economic.tax),
                     QString::number(client.conn.playing->economic.luxury),
                     QString::number(client.conn.playing->economic.science))
          + qendl();
  }
  if (game.info.phase_mode == PMT_PLAYERS_ALTERNATE) {
    if (game.info.phase < 0 || game.info.phase >= player_count()) {
      str += QString(_("Moving: Nobody")) + qendl();
    } else {
      str = str
            + QString(_("Moving: %1"))
                  .arg(player_name(player_by_number(game.info.phase)))
            + qendl();
    }
  } else if (game.info.phase_mode == PMT_TEAMS_ALTERNATE) {
    if (game.info.phase < 0 || game.info.phase >= team_count()) {
      str += QString(_("Moving: Nobody")) + qendl();
    } else {
      str =
          str
          + QString(_("Moving: %1"))
                .arg(team_name_translation(team_by_number(game.info.phase)))
          + qendl();
    }
  }

  if (moreinfo) {
    str += QString(_("(Click for more info)")) + qendl();
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Return the text for the popup label on the info panel.  (This is
   traditionally done as a popup whenever the regular info text is clicked
   on.)
 ****************************************************************************/
const QString get_info_label_text_popup(void)
{
  QString str;

  if (NULL != client.conn.playing) {
    str = QString(_("%1 People"))
              .arg(population_to_text(civ_population(client.conn.playing)))
          + qendl();
  }
  str += QString(_("Year: %1")).arg(calendar_text()) + qendl();
  str +=
      QString(_("Turn: %1")).arg(QString::number(game.info.turn)) + qendl();

  if (NULL != client.conn.playing) {
    const struct research *presearch = research_get(client_player());
    int perturn = get_bulbs_per_turn(NULL, NULL, NULL);
    int upkeep = client_player()->client.tech_upkeep;

    str = str
          + QString(_("Gold: %1"))
                .arg(QString::number(client.conn.playing->economic.gold))
          + qendl();
    str = str
          + QString(_("Net Income: %1"))
                .arg(QString::number(
                    player_get_expected_income(client.conn.playing)))
          + qendl();
    /* TRANS: Gold, luxury, and science rates are in percentage values. */
    str = str
          + QString(_("Tax rates: Gold:%1% Luxury:%2% Science:%3%"))
                .arg(QString::number(client.conn.playing->economic.tax),
                     QString::number(client.conn.playing->economic.luxury),
                     QString::number(client.conn.playing->economic.science))
          + qendl();

    str = str
          + QString(_("Researching %1: %2"))
                .arg(research_advance_name_translation(
                         presearch, presearch->researching),
                     get_science_target_text(NULL))
          + qendl();
    /* perturn is defined as: (bulbs produced) - upkeep */
    if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
      str = str
            + QString(_("Bulbs per turn: %1 - %2 = %3"))
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
      str = str
            + QString(_("Culture: %1 (%2/turn)"))
                  .arg(QString::number(client.conn.playing->client.culture),
                       QString::number(history_perturn))
            + qendl();
    }
  }

  /* See also get_global_warming_tooltip and get_nuclear_winter_tooltip. */

  if (game.info.global_warming) {
    int chance, rate;
    global_warming_scaled(&chance, &rate, 100);
    str = str
          + QString(_("Global warming chance: %1% (%2%/turn)"))
                .arg(QString::number(chance), QString::number(rate))
          + qendl();
  } else {
    str += QString(_("Global warming deactivated.")) + qendl();
  }

  if (game.info.nuclear_winter) {
    int chance, rate;
    nuclear_winter_scaled(&chance, &rate, 100);
    str = str
          + QString(_("Nuclear winter chance: %1% (%2%/turn)"))
                .arg(QString::number(chance), QString::number(rate))
          + qendl();
  } else {
    str += QString(_("Nuclear winter deactivated.")) + qendl();
  }

  if (NULL != client.conn.playing) {
    str = str
          + QString(_("Government: %1"))
                .arg(government_name_for_player(client.conn.playing))
          + qendl();
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Return the title text for the unit info shown in the info panel.

   FIXME: this should be renamed.
 ****************************************************************************/
const QString get_unit_info_label_text1(struct unit_list *punits)
{
  QString str;

  if (punits) {
    int count = unit_list_size(punits);

    if (count == 1) {
      str = unit_name_translation(unit_list_get(punits, 0));
    } else {
      str = QString(PL_("%1 unit", "%1 units", count))
                .arg(QString::number(count));
    }
  }
  return str.trimmed();
}

/************************************************************************/ /**
   Return the text body for the unit info shown in the info panel.

   FIXME: this should be renamed.
 ****************************************************************************/
const QString get_unit_info_label_text2(struct unit_list *punits,
                                        int linebreaks)
{
  int count;
  QString str;

  if (!punits) {
    return QLatin1String("");
  }

  count = unit_list_size(punits);

  /* This text should always have the same number of lines if
   * 'linebreaks' has no flags at all. Otherwise the GUI widgets may be
   * confused and try to resize themselves. If caller asks for
   * conditional 'linebreaks', it should take care of these problems
   * itself. */

  /* Line 1. Goto or activity text. */
  if (count > 0 && hover_state != HOVER_NONE) {
    int min, max;

    if (!goto_get_turns(&min, &max)) {
      /* TRANS: Impossible to reach goto target tile */
      str = QStringLiteral("%1").arg(Q_("?goto:Unreachable")) + qendl();
    } else if (min == max) {
      str = QString(_("Turns to target: %1")).arg(QString::number(max))
            + qendl();
    } else {
      str = QString(_("Turns to target: %1 to %2"))
                .arg(QString::number(min), QString::number(max))
            + qendl();
    }
  } else if (count == 1) {
    str = QStringLiteral("%1").arg(
              unit_activity_text(unit_list_get(punits, 0)))
          + qendl();
  } else if (count > 1) {
    str = QString(PL_("%1 unit selected", "%1 units selected", count))
              .arg(QString::number(count))
          + qendl();
  } else {
    str = QString(_("No units selected.")) + qendl();
  }

  /* Lines 2, 3, 4, and possible 5 vary. */
  if (count == 1) {
    struct unit *punit = unit_list_get(punits, 0);
    struct player *owner = unit_owner(punit);
    struct city *pcity = player_city_by_number(owner, punit->homecity);

    str = str
          + QStringLiteral("%1").arg(
              tile_get_info_text(unit_tile(punit), true, linebreaks))
          + qendl();
    {
      const char *infratext =
          get_infrastructure_text(unit_tile(punit)->extras);

      if (*infratext != '\0') {
        str += QStringLiteral("%1").arg(infratext) + qendl();
      } else {
        str += QStringLiteral(" \n");
      }
    }
    if (pcity) {
      str += QStringLiteral("%1").arg(city_name_get(pcity)) + qendl();
    } else {
      str += QStringLiteral(" \n");
    }

    if (game.info.citizen_nationality) {
      struct player *nationality = unit_nationality(punit);

      /* Line 5, nationality text */
      if (nationality != NULL && owner != nationality) {
        /* TRANS: Nationality of the people comprising a unit, if
         * different from owner. */
        str = str
              + QString(_("%1 people"))
                    .arg(nation_adjective_for_player(nationality))
              + qendl();
      } else {
        str += QStringLiteral(" \n");
      }
    }

  } else if (count > 1) {
    int mil = 0, nonmil = 0;
    int types_count[U_LAST], i;
    struct unit_type *top[3];

    memset(types_count, 0, sizeof(types_count));
    unit_list_iterate(punits, punit)
    {
      if (unit_has_type_flag(punit, UTYF_CIVILIAN)) {
        nonmil++;
      } else {
        mil++;
      }
      types_count[utype_index(unit_type_get(punit))]++;
    }
    unit_list_iterate_end;

    top[0] = top[1] = top[2] = NULL;
    unit_type_iterate(utype)
    {
      if (!top[2]
          || types_count[utype_index(top[2])]
                 < types_count[utype_index(utype)]) {
        top[2] = utype;

        if (!top[1]
            || types_count[utype_index(top[1])]
                   < types_count[utype_index(top[2])]) {
          top[2] = top[1];
          top[1] = utype;

          if (!top[0]
              || types_count[utype_index(top[0])]
                     < types_count[utype_index(utype)]) {
            top[1] = top[0];
            top[0] = utype;
          }
        }
      }
    }
    unit_type_iterate_end;

    for (i = 0; i < 2; i++) {
      if (top[i] && types_count[utype_index(top[i])] > 0) {
        if (utype_has_flag(top[i], UTYF_CIVILIAN)) {
          nonmil -= types_count[utype_index(top[i])];
        } else {
          mil -= types_count[utype_index(top[i])];
        }
        str = str
              + QStringLiteral("%1: %2").arg(
                  QString::number(types_count[utype_index(top[i])]),
                  utype_name_translation(top[i]))
              + qendl();
      } else {
        str += QStringLiteral(" \n");
      }
    }

    if (top[2] && types_count[utype_index(top[2])] > 0
        && types_count[utype_index(top[2])] == nonmil + mil) {
      str = str
            + QStringLiteral("%1: %2").arg(
                QString::number(types_count[utype_index(top[2])]),
                utype_name_translation(top[2]))
            + qendl();
    } else if (nonmil > 0 && mil > 0) {
      str = str
            + QString(_("Others: %1 civil; %2 military"))
                  .arg(QString::number(nonmil), QString::number(mil))
            + qendl();
    } else if (nonmil > 0) {
      str += QString(_("Others: %1 civilian")).arg(QString::number(nonmil))
             + qendl();
    } else if (mil > 0) {
      str += QString(_("Others: %1 military")).arg(QString::number(mil))
             + qendl();
    } else {
      str += QStringLiteral(" \n");
    }

    if (game.info.citizen_nationality) {
      str += QStringLiteral(" \n");
    }
  } else {
    str += QStringLiteral(" \n");
    str += QStringLiteral(" \n");
    str += QStringLiteral(" \n");

    if (game.info.citizen_nationality) {
      str += QStringLiteral(" \n");
    }
  }

  /* Line 5/6. Debug text. */
#ifdef FREECIV_DEBUG
  if (count == 1) {
    str += QString("(Unit ID %1)")
               .arg(QString::number(unit_list_get(punits, 0)->id))
           + qendl();
  } else {
    str += QString(" \n");
  }
#endif /* FREECIV_DEBUG */

  return str.trimmed();
}

/************************************************************************/ /**
   Return text about upgrading these unit lists.

   Returns TRUE iff any units can be upgraded.
 ****************************************************************************/
bool get_units_upgrade_info(char *buf, size_t bufsz,
                            struct unit_list *punits)
{
  if (unit_list_size(punits) == 0) {
    fc_snprintf(buf, bufsz, _("No units to upgrade!"));
    return false;
  } else if (unit_list_size(punits) == 1) {
    return (UU_OK == unit_upgrade_info(unit_list_front(punits), buf, bufsz));
  } else {
    int upgrade_cost = 0;
    int num_upgraded = 0;
    int min_upgrade_cost = FC_INFINITY;

    unit_list_iterate(punits, punit)
    {
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
    unit_list_iterate_end;
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

/************************************************************************/ /**
   Return text about disbanding these units.

   Returns TRUE iff any units can be disbanded.
 ****************************************************************************/
bool get_units_disband_info(char *buf, size_t bufsz,
                            struct unit_list *punits)
{
  if (unit_list_size(punits) == 0) {
    fc_snprintf(buf, bufsz, _("No units to disband!"));
    return false;
  } else if (unit_list_size(punits) == 1) {
    if (!unit_can_do_action(unit_list_front(punits), ACTION_DISBAND_UNIT)) {
      fc_snprintf(buf, bufsz, _("%s refuses to disband!"),
                  unit_name_translation(unit_list_front(punits)));
      return false;
    } else {
      /* TRANS: %s is a unit type */
      fc_snprintf(buf, bufsz, _("Disband %s?"),
                  unit_name_translation(unit_list_front(punits)));
      return true;
    }
  } else {
    int count = 0;
    unit_list_iterate(punits, punit)
    {
      if (unit_can_do_action(punit, ACTION_DISBAND_UNIT)) {
        count++;
      }
    }
    unit_list_iterate_end;
    if (count == 0) {
      fc_snprintf(buf, bufsz, _("None of these units may be disbanded."));
      return false;
    } else {
      /* TRANS: %d is never 0 or 1 */
      fc_snprintf(buf, bufsz,
                  PL_("Disband %d unit?", "Disband %d units?", count),
                  count);
      return true;
    }
  }
}

/************************************************************************/ /**
   Get a tooltip text for the info panel research indicator.  See
   client_research_sprite().
 ****************************************************************************/
const QString get_bulb_tooltip(void)
{
  QString str = _("Shows your progress in "
                  "researching the current technology.")
                + qendl();

  if (NULL != client.conn.playing) {
    struct research *research = research_get(client_player());

    if (research->researching == A_UNSET) {
      str += _("No research target.");
    } else {
      int turns;
      int perturn = get_bulbs_per_turn(NULL, NULL, NULL);
      QString buf1, buf2;

      if ((turns = turns_to_research_done(research, perturn)) >= 0) {
        buf1 = QString(PL_("%1 turn", "%1 turns", turns))
                   .arg(QString::number(turns));
      } else if ((turns = turns_to_tech_loss(research, perturn)) >= 0) {
        buf1 = QString(PL_("%1 turn to loss", "%1 turns to loss", turns))
                   .arg(QString::number(turns));
      } else {
        if (perturn < 0) {
          buf1 = QString(_("Decreasing"));
        } else {
          buf1 = QString(_("No progress"));
        }
      }
      /* TRANS: <perturn> bulbs/turn */
      buf2 = QString(PL_("%1 bulb/turn", "%1 bulbs/turn", perturn))
                 .arg(QString::number(perturn));
      /* TRANS: <tech>: <amount>/<total bulbs> */
      str = str
            + QString(_("%1: %2/%3 (%4, %5)."))
                  .arg(research_advance_name_translation(
                           research, research->researching),
                       QString::number(research->bulbs_researched),
                       QString::number(research->client.researching_cost),
                       buf1, buf2)
            + qendl();
    }
  }
  return str.trimmed();
}

/************************************************************************/ /**
   Get a tooltip text for the info panel global warning indicator.  See also
   client_warming_sprite().
 ****************************************************************************/
const QString get_global_warming_tooltip(void)
{
  QString str;

  if (!game.info.global_warming) {
    str = _("Global warming deactivated.") + qendl();
  } else {
    int chance, rate;
    global_warming_scaled(&chance, &rate, 100);
    str = _("Shows the progress of global warming:");
    str += QString(_("Pollution rate: %1%")).arg(QString::number(rate))
           + qendl();
    str = str
          + QString(_("Chance of catastrophic warming each turn: %1%"))
                .arg(QString::number(chance))
          + qendl();
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Get a tooltip text for the info panel nuclear winter indicator.  See also
   client_cooling_sprite().
 ****************************************************************************/
const QString get_nuclear_winter_tooltip(void)
{
  QString str;

  if (!game.info.nuclear_winter) {
    str = _("Nuclear winter deactivated.") + qendl();
  } else {
    int chance, rate;
    nuclear_winter_scaled(&chance, &rate, 100);
    str = _("Shows the progress of nuclear winter:") + qendl();
    str +=
        QString(_("Fallout rate: %1%")).arg(QString::number(rate)) + qendl();
    str += str
           + QString(_("Chance of catastrophic winter each turn: %1%"))
                 .arg(QString::number(chance))
           + qendl();
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Get a tooltip text for the info panel government indicator.  See also
   government_by_number(...)->sprite.
 ****************************************************************************/
const QString get_government_tooltip(void)
{
  QString str;

  str = _("Shows your current government:") + qendl();

  if (NULL != client.conn.playing) {
    str = str
          + QStringLiteral("%1").arg(
              government_name_for_player(client.conn.playing))
          + qendl();
  }
  return str.trimmed();
}

/************************************************************************/ /**
   Returns a description of the given spaceship.  If there is no spaceship
   (pship is NULL) then text with dummy values is returned.
 ****************************************************************************/
const QString get_spaceship_descr(struct player_spaceship *pship)
{
  struct player_spaceship ship;
  QString str;

  if (!pship) {
    pship = &ship;
    memset(&ship, 0, sizeof(ship));
  }
  /* TRANS: spaceship text; should have constant width. */
  str += QString(_("Population:      %1"))
             .arg(QString::number(pship->population))
         + qendl();
  /* TRANS: spaceship text; should have constant width. */
  str = str
        + QString(_("Support:         %1 %"))
              .arg(QString::number((int) (pship->support_rate * 100.0)))
        + qendl();
  /* TRANS: spaceship text; should have constant width. */
  str = str
        + QString(_("Energy:          %1 %"))
              .arg(QString::number((int) (pship->energy_rate * 100.0)))
        + qendl();
  /* TRANS: spaceship text; should have constant width. */
  str = str
        + QString(PL_("Mass:            %1 ton", "Mass:            %1 tons",
                      pship->mass))
              .arg(QString::number(pship->mass))
        + qendl();
  if (pship->propulsion > 0) {
    /* TRANS: spaceship text; should have constant width. */
    str = str
          + QString(_("Travel time:     %1 years"))
                .arg(QString::number(
                    (float) (0.1 * ((int) (pship->travel_time * 10.0)))))
          + qendl();
  } else {
    /* TRANS: spaceship text; should have constant width. */
    str = str + QStringLiteral("%1").arg(_("Travel time:        N/A     "))
          + qendl();
  }
  /* TRANS: spaceship text; should have constant width. */
  str = str
        + QString(_("Success prob.:   %1 %"))
              .arg(QString::number((int) (pship->success_rate * 100.0)))
        + qendl();
  /* TRANS: spaceship text; should have constant width. */
  str = str
        + QString(_("Year of arrival: %1"))
              .arg((pship->state == SSHIP_LAUNCHED) ? textyear(
                       (int) (pship->launch_year + (int) pship->travel_time))
                                                    : "-   ")
        + qendl();

  return str.trimmed();
}

/************************************************************************/ /**
   Get the text showing the timeout.  This is generally disaplyed on the info
   panel.
 ****************************************************************************/
const QString get_timeout_label_text(void)
{
  QString str;

  if (is_waiting_turn_change() && game.tinfo.last_turn_change_time >= 1.5) {
    double wt = get_seconds_to_new_turn();

    if (wt < 0.01) {
      str = Q_("?timeout:wait");
    } else {
      str = QStringLiteral("%1: %2").arg(Q_("?timeout:eta"),
                                         format_duration(wt));
    }
  } else {
    if (current_turn_timeout() <= 0) {
      str = QStringLiteral("%1").arg(Q_("?timeout:off"));
    } else {
      str = QStringLiteral("%1").arg(
          format_duration(get_seconds_to_turndone()));
    }
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Format a duration, in seconds, so it comes up in minutes or hours if
   that would be more meaningful.

   (7 characters, maximum.  Enough for, e.g., "99h 59m".)
 ****************************************************************************/
const QString format_duration(int duration)
{
  QString str;

  if (duration < 0) {
    duration = 0;
  }
  if (duration < 60) {
    str += QString(Q_("?seconds:%1s")).arg(duration, 2);
  } else if (duration < 3600) { /* < 60 minutes */
    str = str
          + QString(Q_("?mins/secs:%1m %2s"))
                .arg(duration / 60, 2)
                .arg(duration % 60, 2);
  } else if (duration < 360000) { /* < 100 hours */
    str += QString(Q_("?hrs/mns:%1h %2m"))
               .arg(duration / 3600, 2)
               .arg((duration / 60) % 60, 2);
  } else if (duration < 8640000) { /* < 100 days */
    str += QString(Q_("?dys/hrs:%1d %2dh"))
               .arg(duration / 86400, 2)
               .arg((duration / 3600) % 24, 2);
  } else {
    str += QStringLiteral("%1").arg(Q_("?duration:overflow"));
  }
  // Show time if there is more than 1hour left
  if (duration > 3600) {
    QDateTime time = QDateTime::currentDateTime();
    QDateTime tc_time = time.addSecs(duration);
    QString day_now = QLocale::system().toString(time, "ddd ");
    QString day_tc = QLocale::system().toString(tc_time, "ddd ");

    str += QStringLiteral("\n") + ((day_now != day_tc) ? day_tc : "")
           + tc_time.toString("hh:mm");
  }
  return str.trimmed();
}

/************************************************************************/ /**
   Return text giving the ping time for the player.  This is generally used
   used in the playerdlg.  This should only be used in playerdlg_common.c.
 ****************************************************************************/
QString get_ping_time_text(const struct player *pplayer)
{
  QString str;

  conn_list_iterate(pplayer->connections, pconn)
  {
    if (!pconn->observer
        /* Certainly not needed, but safer. */
        && 0 == strcmp(pconn->username, pplayer->username)) {
      if (pconn->ping_time != -1) {
        double ping_time_in_ms = 1000 * pconn->ping_time;
        str += QString(_("%1.%2 ms"))
                   .arg(QString::number((int) ping_time_in_ms), 6)
                   .arg(QString::number(((int) (ping_time_in_ms * 100.0))
                                        % 100),
                        2);
      }
      break;
    }
  }
  conn_list_iterate_end;

  return str.trimmed();
}

/************************************************************************/ /**
   Return text giving the score of the player. This should only be used
   in playerdlg_common.c.
 ****************************************************************************/
QString get_score_text(const struct player *pplayer)
{
  QString str;

  if (pplayer->score.game > 0 || NULL == client.conn.playing
      || pplayer == client.conn.playing) {
    str = QStringLiteral("%1").arg(QString::number(pplayer->score.game));
  } else {
    str = QLatin1String("?");
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Get the title for a "report".  This may include the city, economy,
   military, trade, player, etc., reports.  Some clients may generate the
   text themselves to get a better GUI layout.
 ****************************************************************************/
const QString get_report_title(const char *report_name)
{
  QString str;
  const struct player *pplayer = client_player();

  str = QString(report_name) + qendl();

  if (pplayer != NULL) {
    char buf[4 * MAX_LEN_NAME];
    /* TRANS: <nation adjective> <government name>.
     * E.g. "Polish Republic". */
    str = str
          + QString(Q_("?nationgovernment:%1 %2"))
                .arg(nation_adjective_for_player(pplayer),
                     government_name_for_player(pplayer))
          + qendl();
    /* TRANS: Just appending 2 strings, using the correct localized
     * syntax. */
    str = str
          + QString(_("%1 - %2"))
                .arg(ruler_title_for_player(pplayer, buf, sizeof(buf)),
                     calendar_text())
          + qendl();
  } else {
    /* TRANS: "Observer - 1985 AD" */
    str += QString(_("Observer - %1")).arg(calendar_text()) + qendl();
  }
  return str.trimmed();
}

/**********************************************************************/ /**
   Returns custom part of the action selection dialog button text for the
   specified action (given that the action is possible).
 **************************************************************************/
const QString get_act_sel_action_custom_text(struct action *paction,
                                             const struct act_prob prob,
                                             const struct unit *actor_unit,
                                             const struct city *target_city)
{
  struct city *actor_homecity = unit_home(actor_unit);
  QString custom;
  if (!action_prob_possible(prob)) {
    /* No info since impossible. */
    return NULL;
  }

  fc_assert_ret_val(
      (action_get_target_kind(paction) != ATK_CITY || target_city != NULL),
      NULL);

  if (action_has_result(paction, ACTRES_TRADE_ROUTE)) {
    int revenue = get_caravan_enter_city_trade_bonus(
        actor_homecity, target_city, actor_unit->carrying, true);

    custom = QString(
                 /* TRANS: Estimated one time bonus and recurring revenue for
                  * the Establish Trade _Route action. */
                 _("%1 one time bonus + %2 trade"))
                 .arg(QString::number(revenue),
                      QString::number(trade_base_between_cities(
                          actor_homecity, target_city)));
  } else if (action_has_result(paction, ACTRES_MARKETPLACE)) {
    int revenue = get_caravan_enter_city_trade_bonus(
        actor_homecity, target_city, actor_unit->carrying, false);

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
    /* No info to add. */
    return NULL;
  }

  return custom;
}

/**********************************************************************/ /**
   Get information about starting the action in the current situation.
   Suitable for a tool tip for the button that starts it.
   @return an explanation of a tool tip button suitable for a tool tip
 **************************************************************************/
const QString act_sel_action_tool_tip(const struct action *paction,
                                      const struct act_prob prob)
{
  Q_UNUSED(paction)
  return action_prob_explain(prob);
}

/************************************************************************/ /**
   Describing buildings that affect happiness.
 ****************************************************************************/
QString text_happiness_buildings(const struct city *pcity)
{
  struct effect_list *plist = effect_list_new();
  QString effects;
  QString str;

  get_city_bonus_effects(plist, pcity, NULL, EFT_MAKE_CONTENT);
  if (0 < effect_list_size(plist)) {
    effects = get_effect_list_req_text(plist);
    str = QString(_("Buildings: %1.")).arg(effects);
  } else {
    str = _("Buildings: None.");
  }
  effect_list_destroy(plist);

  return str.trimmed();
}

/************************************************************************/ /**
   Describing nationality effects that affect happiness.
 ****************************************************************************/
const QString text_happiness_nationality(const struct city *pcity)
{
  QString str;
  int enemies = 0;

  str = _("Nationality: ") + qendl();

  if (game.info.citizen_nationality) {
    if (get_city_bonus(pcity, EFT_ENEMY_CITIZEN_UNHAPPY_PCT) > 0) {
      struct player *owner = city_owner(pcity);

      citizens_foreign_iterate(pcity, pslot, nationality)
      {
        if (pplayers_at_war(owner, player_slot_get_player(pslot))) {
          enemies += nationality;
        }
      }
      citizens_foreign_iterate_end;

      if (enemies > 0) {
        str = str
              + QString(PL_("%1 enemy nationalist", "%1 enemy nationalists",
                            enemies))
                    .arg(QString::number(enemies));
      }
    }

    if (enemies == 0) {
      str += QString(_("None."));
    }
  } else {
    str += QString(_("Disabled."));
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Describing wonders that affect happiness.
 ****************************************************************************/
QString text_happiness_wonders(const struct city *pcity)
{
  struct effect_list *plist = effect_list_new();
  QString str;

  get_city_bonus_effects(plist, pcity, NULL, EFT_MAKE_HAPPY);
  get_city_bonus_effects(plist, pcity, NULL, EFT_FORCE_CONTENT);
  get_city_bonus_effects(plist, pcity, NULL, EFT_NO_UNHAPPY);
  if (0 < effect_list_size(plist)) {
    QString effects;

    effects = get_effect_list_req_text(plist);
    str = QString(_("Wonders: %1.")).arg(effects);
  } else {
    str = _("Wonders: None.");
  }
  effect_list_destroy(plist);

  return str.trimmed();
}

/************************************************************************/ /**
   Describing city factors that affect happiness.
 ****************************************************************************/
const QString text_happiness_cities(const struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  int cities = city_list_size(pplayer->cities);
  int content = get_player_bonus(pplayer, EFT_CITY_UNHAPPY_SIZE);
  int basis = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE);
  int step = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_STEP);
  QString str;

  if (basis + step <= 0) {
    /* Special case where penalty is disabled; see
     * player_content_citizens(). */
    str = str
          + QString(PL_("Cities: %1 total, but no penalty for empire size.",
                        "Cities: %1 total, but no penalty for empire size.",
                        cities))
                .arg(QString::number(cities))
          + qendl();
    /* TRANS: %d is number of citizens */
    str = str
          + QString(
                PL_("%1 content per city.", "%1 content per city.", content))
                .arg(QString::number(content))
          + qendl();
  } else {
    /* Can have up to and including 'basis' cities without penalty */
    int excess = MAX(cities - basis, 0);
    int penalty;
    int unhappy, angry;
    int last, next;

    if (excess > 0) {
      if (step > 0) {
        penalty = 1 + (excess - 1) / step;
      } else {
        penalty = 1;
      }
    } else {
      penalty = 0;
    }

    unhappy = MIN(penalty, content);
    angry = game.info.angrycitizen ? MAX(penalty - content, 0) : 0;
    if (penalty >= 1) {
      /* 'last' is when last actual malcontent appeared, will saturate
       * if no angry citizens */
      last = basis + (unhappy + angry - 1) * step;
      if (!game.info.angrycitizen && unhappy == content) {
        /* Maxed out unhappy citizens, so no more penalties */
        next = 0;
      } else {
        /* Angry citizens can continue appearing indefinitely */
        next = last + step;
      }
    } else {
      last = 0;
      next = basis;
    }
    /* TRANS: sentence fragment, will have text appended */
    str = str
          + QString(PL_("Cities: %1 total:", "Cities: %1 total:", cities))
                .arg(QString::number(cities))
          + qendl();
    if (excess > 0) {
      /* TRANS: appended to "Cities: %d total:"; preserve leading
       * space. Pluralized in "nearest threshold of %d cities". */
      str =
          str
          + QString(PL_(" %1 over nearest threshold of %2 city.",
                        " %1 over nearest threshold of %2 cities.", last))
                .arg(QString::number(cities - last), QString::number(last));
      /* TRANS: Number of content [citizen(s)] ... */
      str = str
            + QString(PL_("%1 content before penalty.",
                          "%1 content before penalty.", content))
                  .arg(QString::number(content))
            + qendl();
      str += QString(PL_("%1 additional unhappy citizen.",
                         "%1 additional unhappy citizens.", unhappy))
                 .arg(QString::number(unhappy))
             + qendl();
      if (angry > 0) {
        str =
            str
            + QString(PL_("%1 angry citizen.", "%1 angry citizens.", angry))
                  .arg(QString::number(angry))
            + qendl();
      }
    } else {
      /* TRANS: appended to "Cities: %d total:"; preserve leading space. */
      str = str
            + QString(PL_(" not more than %1, so no empire size penalty.",
                          " not more than %1, so no empire size penalty.",
                          next))
                  .arg(QString::number(next));
      str = str
            + QString(PL_("%1 content per city.", "%1 content per city.",
                          content))
                  .arg(QString::number(content))
            + qendl();
    }
    if (next >= cities && penalty < content) {
      str = str
            + QString(PL_("With %1 more city, another citizen will become "
                          "unhappy.",
                          "With %1 more cities, another citizen will become "
                          "unhappy.",
                          next + 1 - cities))
                  .arg(QString::number(next + 1 - cities))
            + qendl();
    } else if (next >= cities) {
      /* We maxed out the number of unhappy citizens, but they can get
       * angry instead. */
      fc_assert(game.info.angrycitizen);
      str = str
            + QString(PL_("With %1 more city, another citizen will become "
                          "angry.",
                          "With %1 more cities, another citizen will become "
                          "angry.",
                          next + 1 - cities))
                  .arg(QString::number(next + 1 - cities))
            + qendl();
    } else {
      /* Either no Empire_Size_Step, or we maxed out on unhappy citizens
       * and ruleset doesn't allow angry ones. */
      str = str
            + QString(_("More cities will not cause more unhappy citizens."))
            + qendl();
    }
  }

  return str.trimmed();
}

/************************************************************************/ /**
   Describing units that affect happiness.
 ****************************************************************************/
const QString text_happiness_units(const struct city *pcity)
{
  int mlmax = get_city_bonus(pcity, EFT_MARTIAL_LAW_MAX);
  int uhcfac = get_city_bonus(pcity, EFT_UNHAPPY_FACTOR);
  QString str;

  if (mlmax > 0) {
    int mleach = get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH);
    if (mlmax == 100) {
      str = QStringLiteral("%1").arg(_("Unlimited martial law in effect."))
            + qendl();
    } else {
      str = str
            + QString(PL_("%1 military unit may impose martial law.",
                          "Up to %1 military units may impose martial "
                          "law.",
                          mlmax))
                  .arg(QString::number(mlmax))
            + qendl();
    }
    str = str
          + QString(PL_("Each military unit makes %1 "
                        "unhappy citizen content.",
                        "Each military unit makes %1 "
                        "unhappy citizens content.",
                        mleach))
                .arg(QString::number(mleach))
          + qendl();
  } else if (uhcfac > 0) {
    str = str
          + QString(_("Military units in the field may cause unhappiness. "))
          + qendl();
  } else {
    str += QString(_("Military units have no happiness effect. ")) + qendl();
  }
  return str.trimmed();
}

/************************************************************************/ /**
   Describing luxuries that affect happiness.
 ****************************************************************************/
const QString text_happiness_luxuries(const struct city *pcity)
{
  return QString(_("Luxury: %1 total."))
             .arg(QString::number(pcity->prod[O_LUXURY]))
         + qendl();
}
