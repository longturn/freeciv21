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

#include <fc_config.h>

#include <cstdio>
#include <cstring>
#include <math.h>

// utility
#include "bitvector.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "support.h"

// common
#include "achievements.h"
#include "calendar.h"
#include "connection.h"
#include "demographic.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "specialist.h"
#include "team.h"
#include "unitlist.h"
#include "version.h"

// server
#include "plrhand.h"
#include "score.h"
#include "srv_main.h"

#include "report.h"

// data needed for logging civ score
struct plrdata_slot {
  char *name;
};

struct logging_civ_score {
  FILE *fp;
  int last_turn;
  struct plrdata_slot *plrdata;
};

/* Have to be initialized to value less than -1 so it doesn't seem like
 * report was created at
 * the end of previous turn in the beginning to turn 0. */
struct history_report latest_history_report = {-2};

static struct logging_civ_score *score_log = nullptr;

static void plrdata_slot_init(struct plrdata_slot *plrdata,
                              const char *name);
static void plrdata_slot_replace(struct plrdata_slot *plrdata,
                                 const char *name);
static void plrdata_slot_free(struct plrdata_slot *plrdata);

static void page_conn_etype(struct conn_list *dest, const char *caption,
                            const char *headline, const char *lines,
                            enum event_type event);
enum historian_type {
  HISTORIAN_RICHEST = 0,
  HISTORIAN_ADVANCED = 1,
  HISTORIAN_MILITARY = 2,
  HISTORIAN_HAPPIEST = 3,
  HISTORIAN_LARGEST = 4
};

#define HISTORIAN_FIRST HISTORIAN_RICHEST
#define HISTORIAN_LAST HISTORIAN_LARGEST

static const char *historian_message[] = {
    // TRANS: year <name> reports ...
    N_("%s %s reports on the RICHEST Civilizations in the World."),
    // TRANS: year <name> reports ...
    N_("%s %s reports on the most ADVANCED Civilizations in the World."),
    // TRANS: year <name> reports ...
    N_("%s %s reports on the most MILITARIZED Civilizations in the World."),
    // TRANS: year <name> reports ...
    N_("%s %s reports on the HAPPIEST Civilizations in the World."),
    // TRANS: year <name> reports ...
    N_("%s %s reports on the LARGEST Civilizations in the World.")};

static const char *historian_name[] = {
    // TRANS: [year] <name> [reports ...]
    N_("Herodotus"),
    // TRANS: [year] <name> [reports ...]
    N_("Thucydides"),
    // TRANS: [year] <name> [reports ...]
    N_("Pliny the Elder"),
    // TRANS: [year] <name> [reports ...]
    N_("Livy"),
    // TRANS: [year] <name> [reports ...]
    N_("Toynbee"),
    // TRANS: [year] <name> [reports ...]
    N_("Gibbon"),
    // TRANS: [year] <name> [reports ...]
    N_("Ssu-ma Ch'ien"),
    // TRANS: [year] <name> [reports ...]
    N_("Pan Ku")};

static const char scorelog_magic[] = "#FREECIV SCORELOG2 ";

struct player_score_entry {
  const struct player *player;
  int value;
};

struct city_score_entry {
  struct city *city;
  int value;
};

static int get_great_wonders(const struct player *pplayer);
static int get_total_score(const struct player *pplayer);
static int get_league_score(const struct player *pplayer);
static int get_population(const struct player *pplayer);
static int get_landarea(const struct player *pplayer);
static int get_settledarea(const struct player *pplayer);
static int get_research(const struct player *pplayer);
static int get_income(const struct player *pplayer);
static int get_production(const struct player *pplayer);
static int get_economics(const struct player *pplayer);
static int get_agriculture(const struct player *pplayer);
static int get_pollution(const struct player *pplayer);
static int get_mil_service(const struct player *pplayer);
static int get_culture(const struct player *pplayer);
static int get_pop(
    const struct player
        *pplayer); /* this would better be named get_citizenunits or such */
static int get_cities(const struct player *pplayer);
static int get_improvements(const struct player *pplayer);
static int get_all_wonders(const struct player *pplayer);
static int get_mil_units(const struct player *pplayer);
static int get_units_built(const struct player *pplayer);
static int get_units_killed(const struct player *pplayer);
static int get_units_lost(const struct player *pplayer);

static const char *area_to_text(int value);
static const char *percent_to_text(int value);
static const char *production_to_text(int value);
static const char *economics_to_text(int value);
static const char *agriculture_to_text(int value);
static const char *science_to_text(int value);
static const char *income_to_text(int value);
static const char *mil_service_to_text(int value);
static const char *pollution_to_text(int value);
static const char *culture_to_text(int value);
static const char *citizenunits_to_text(int value);
static const char *cities_to_text(int value);
static const char *improvements_to_text(int value);
static const char *wonders_to_text(int value);
static const char *mil_units_to_text(int value);
static const char *score_to_text(int value);

#define GOOD_PLAYER(p) ((p)->is_alive && !is_barbarian(p))

/*
 * Describes a row.
 */
static std::map<char, demographic> rowtable = {
    {'s', demographic(N_("Score"), get_total_score, score_to_text, true)},
    {'z', demographic(N_("League Score"), get_league_score, score_to_text,
                      true)}, // z cuz inverted s. B)
    {'N', demographic(N_("Population"), get_population, population_to_text,
                      true)},
    {'n',
     demographic(N_("Population"), get_pop, citizenunits_to_text, true)},
    {'c', demographic(N_("Cities"), get_cities, cities_to_text, true)},
    {'i', demographic(N_("Improvements"), get_improvements,
                      improvements_to_text, true)},
    {'w',
     demographic(N_("Wonders"), get_all_wonders, wonders_to_text, true)},
    {'A', demographic(N_("Land Area"), get_landarea, area_to_text, true)},
    {'S',
     demographic(N_("Settled Area"), get_settledarea, area_to_text, true)},
    // TRANS: How literate people are.
    {'L', demographic(N_("?ability:Literacy"), get_literacy, percent_to_text,
                      true)},
    {'a', demographic(N_("Agriculture"), get_agriculture,
                      agriculture_to_text, true)},
    {'P', demographic(N_("Production"), get_production, production_to_text,
                      true)},
    {'E',
     demographic(N_("Economics"), get_economics, economics_to_text, true)},
    {'g', demographic(N_("Gold Income"), get_income, income_to_text, true)},
    {'R',
     demographic(N_("Research Speed"), get_research, science_to_text, true)},
    {'M', demographic(N_("Military Service"), get_mil_service,
                      mil_service_to_text, false)},
    {'m', demographic(N_("Military Units"), get_mil_units, mil_units_to_text,
                      true)},
    {'u', demographic(N_("Built Units"), get_units_built, mil_units_to_text,
                      true)},
    {'k', demographic(N_("Killed Units"), get_units_killed,
                      mil_units_to_text, true)},
    {'l',
     demographic(N_("Lost Units"), get_units_lost, mil_units_to_text, true)},
    {'O',
     demographic(N_("Pollution"), get_pollution, pollution_to_text, true)},
    {'C', demographic(N_("Culture"), get_culture, culture_to_text, true)}};

// Demographics columns.
enum dem_flag { DEM_COL_QUANTITY, DEM_COL_RANK, DEM_COL_BEST, DEM_COL_LAST };
BV_DEFINE(bv_cols, DEM_COL_LAST);
static struct dem_col {
  char key;
} coltable[] = {{'q'}, {'r'}, {'b'}}; // Corresponds to dem_flag enum

// prime number of entries makes for better scaling
static const char *ranking[] = {
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Supreme %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Magnificent %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Great %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Glorious %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Excellent %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Eminent %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Distinguished %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Average %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Mediocre %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Ordinary %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Pathetic %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Useless %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Valueless %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Worthless %s"),
    // TRANS: <#>: The <ranking> Poles
    N_("%2d: The Wretched %s"),
};

/**
   Compare two player score entries. Used as callback for qsort.
 */
static int secompare(const void *a, const void *b)
{
  return ((static_cast<const struct player_score_entry *>(b))->value
          - (static_cast<const struct player_score_entry *>(a))->value);
}

/**
   Construct Historian Report
 */
static void historian_generic(struct history_report *report,
                              enum historian_type which_news)
{
  int i, j = 0, rank = 0;
  struct player_score_entry size[player_count()];

  report->turn = game.info.turn;
  players_iterate(pplayer)
  {
    if (GOOD_PLAYER(pplayer)) {
      switch (which_news) {
      case HISTORIAN_RICHEST:
        size[j].value = pplayer->economic.gold;
        break;
      case HISTORIAN_ADVANCED:
        size[j].value =
            pplayer->score.techs + research_get(pplayer)->future_tech;
        break;
      case HISTORIAN_MILITARY:
        size[j].value = pplayer->score.units;
        break;
      case HISTORIAN_HAPPIEST:
        size[j].value = (((pplayer->score.happy - pplayer->score.unhappy
                           - 2 * pplayer->score.angry)
                          * 1000)
                         / (1 + total_player_citizens(pplayer)));
        break;
      case HISTORIAN_LARGEST:
        size[j].value = total_player_citizens(pplayer);
        break;
      }
      size[j].player = pplayer;
      j++;
    } // else the player is dead or barbarian or observer
  }
  players_iterate_end;

  qsort(size, j, sizeof(size[0]), secompare);
  report->body[0] = '\0';
  for (i = 0; i < j; i++) {
    if (i > 0 && size[i].value < size[i - 1].value) {
      // since i < j, only top entry reigns Supreme
      rank = ((i * ARRAY_SIZE(ranking)) / j) + 1;
    }
    if (rank >= ARRAY_SIZE(ranking)) {
      // clamp to final entry
      rank = ARRAY_SIZE(ranking) - 1;
    }
    cat_snprintf(report->body, REPORT_BODYSIZE, _(ranking[rank]), i + 1,
                 nation_plural_for_player(size[i].player));
    fc_strlcat(report->body, "\n", REPORT_BODYSIZE);
  }
  fc_snprintf(report->title, REPORT_TITLESIZE,
              _(historian_message[which_news]), calendar_text(),
              _(historian_name[fc_rand(ARRAY_SIZE(historian_name))]));
}

/**
   Send history report of this turn.
 */
void send_current_history_report(struct conn_list *dest)
{
  // History report is actually constructed at the end of previous turn.
  if (latest_history_report.turn >= game.info.turn - 1) {
    page_conn_etype(dest, _("Historian Publishes!"),
                    latest_history_report.title, latest_history_report.body,
                    E_BROADCAST_REPORT);
  }
}

/**
  Returns the number of wonders the given city has.
 */
static int nr_wonders(struct city *pcity)
{
  int result = 0;

  city_built_iterate(pcity, i)
  {
    if (is_great_wonder(i)) {
      result++;
    }
  }
  city_built_iterate_end;

  return result;
}

/**
   Send report listing the "best" 5 cities in the world.
 */
void report_top_five_cities(struct conn_list *dest)
{
  const int NUM_BEST_CITIES = 5;
  // a wonder equals WONDER_FACTOR citizen
  const int WONDER_FACTOR = 5;
  struct city_score_entry size[NUM_BEST_CITIES];
  int i;
  char buffer[4096];

  for (i = 0; i < NUM_BEST_CITIES; i++) {
    size[i].value = 0;
    size[i].city = nullptr;
  }

  shuffled_players_iterate(pplayer)
  {
    city_list_iterate(pplayer->cities, pcity)
    {
      int value_of_pcity =
          city_size_get(pcity) + nr_wonders(pcity) * WONDER_FACTOR;

      if (value_of_pcity > size[NUM_BEST_CITIES - 1].value) {
        size[NUM_BEST_CITIES - 1].value = value_of_pcity;
        size[NUM_BEST_CITIES - 1].city = pcity;
        qsort(size, NUM_BEST_CITIES, sizeof(size[0]), secompare);
      }
    }
    city_list_iterate_end;
  }
  shuffled_players_iterate_end;

  buffer[0] = '\0';
  for (i = 0; i < NUM_BEST_CITIES; i++) {
    int wonders;

    if (!size[i].city) {
      /*
       * pcity may be nullptr if there are less then NUM_BEST_CITIES in
       * the whole game.
       */
      break;
    }

    if (player_count() > team_count()) {
      // There exists a team with more than one member.
      QString team_name;

      team_pretty_name(city_owner(size[i].city)->team, team_name);
      cat_snprintf(buffer, sizeof(buffer),
                   // TRANS:"The French City of Lyon (team 3) of size 18".
                   _("%2d: The %s City of %s (%s) of size %d, "), i + 1,
                   nation_adjective_for_player(city_owner(size[i].city)),
                   city_name_get(size[i].city), qUtf8Printable(team_name),
                   city_size_get(size[i].city));
    } else {
      cat_snprintf(buffer, sizeof(buffer),
                   _("%2d: The %s City of %s of size %d, "), i + 1,
                   nation_adjective_for_player(city_owner(size[i].city)),
                   city_name_get(size[i].city), city_size_get(size[i].city));
    }

    wonders = nr_wonders(size[i].city);
    if (wonders == 0) {
      cat_snprintf(buffer, sizeof(buffer), _("with no Great Wonders\n"));
    } else {
      cat_snprintf(
          buffer, sizeof(buffer),
          PL_("with %d Great Wonder\n", "with %d Great Wonders\n", wonders),
          wonders);
    }
  }
  page_conn(dest, _("Traveler's Report:"),
            _("The Five Greatest Cities in the World!"), buffer);
}

/**
   Send report listing all built and destroyed wonders, and wonders
   currently being built.
 */
void report_wonders_of_the_world(struct conn_list *dest)
{
  char buffer[4096];

  buffer[0] = '\0';

  improvement_iterate(i)
  {
    if (is_great_wonder(i)) {
      struct city *pcity = city_from_great_wonder(i);

      if (pcity) {
        if (player_count() > team_count()) {
          // There exists a team with more than one member.
          QString team_name;

          team_pretty_name(city_owner(pcity)->team, team_name);
          cat_snprintf(buffer, sizeof(buffer),
                       // TRANS: "Colossus in Rhodes (Greek, team 2)".
                       _("%s in %s (%s, %s)\n"),
                       city_improvement_name_translation(pcity, i),
                       city_name_get(pcity),
                       nation_adjective_for_player(city_owner(pcity)),
                       qUtf8Printable(team_name));
        } else {
          cat_snprintf(buffer, sizeof(buffer), _("%s in %s (%s)\n"),
                       city_improvement_name_translation(pcity, i),
                       city_name_get(pcity),
                       nation_adjective_for_player(city_owner(pcity)));
        }
      } else if (great_wonder_is_destroyed(i)) {
        cat_snprintf(buffer, sizeof(buffer), _("%s has been DESTROYED\n"),
                     improvement_name_translation(i));
      }
    }
  }
  improvement_iterate_end;

  improvement_iterate(i)
  {
    if (is_great_wonder(i)) {
      players_iterate(pplayer)
      {
        city_list_iterate(pplayer->cities, pcity)
        {
          if (VUT_IMPROVEMENT == pcity->production.kind
              && pcity->production.value.building == i) {
            if (player_count() > team_count()) {
              // There exists a team with more than one member.
              QString team_name;

              team_pretty_name(city_owner(pcity)->team, team_name);
              cat_snprintf(buffer, sizeof(buffer),
                           // TRANS: "([...] (Roman, team 4))".
                           _("(building %s in %s (%s, %s))\n"),
                           improvement_name_translation(i),
                           city_name_get(pcity),
                           nation_adjective_for_player(pplayer),
                           qUtf8Printable(team_name));
            } else {
              cat_snprintf(
                  buffer, sizeof(buffer), _("(building %s in %s (%s))\n"),
                  improvement_name_translation(i), city_name_get(pcity),
                  nation_adjective_for_player(pplayer));
            }
          }
        }
        city_list_iterate_end;
      }
      players_iterate_end;
    }
  }
  improvement_iterate_end;

  page_conn(dest, _("Traveler's Report:"), _("Wonders of the World"),
            buffer);
}

/**
 Helper functions which return the value for the given player.
 */

/**
   Population of player
 */
static int get_population(const struct player *pplayer)
{
  return pplayer->score.population;
}

/**
   Number of citizen units of player
 */
static int get_pop(const struct player *pplayer)
{
  return total_player_citizens(pplayer);
}

/**
   Number of citizens of player
 */
static int get_real_pop(const struct player *pplayer)
{
  return 1000 * get_pop(pplayer);
}

/**
   Land area controlled by player
 */
static int get_landarea(const struct player *pplayer)
{
  return pplayer->score.landarea;
}

/**
   Area settled.
 */
static int get_settledarea(const struct player *pplayer)
{
  return pplayer->score.settledarea;
}

/**
   Research speed
 */
static int get_research(const struct player *pplayer)
{
  return pplayer->score.techout;
}

/**
  Gold income
 */
static int get_income(const struct player *pplayer)
{
  return pplayer->score.goldout;
}

/**
   Production of player
 */
static int get_production(const struct player *pplayer)
{
  return pplayer->score.mfg;
}

/**
   BNP of player
 */
static int get_economics(const struct player *pplayer)
{
  return pplayer->score.bnp;
}

/**
  Food output
 */
static int get_agriculture(const struct player *pplayer)
{
  return pplayer->score.food;
}

/**
   Pollution of player
 */
static int get_pollution(const struct player *pplayer)
{
  return pplayer->score.pollution;
}

/**
   Military service length
 */
static int get_mil_service(const struct player *pplayer)
{
  return (pplayer->score.units * 5000) / (10 + civ_population(pplayer));
}

/**
  Military units
 */
static int get_mil_units(const struct player *pplayer)
{
  return pplayer->score.units;
}

/**
   Number of cities
 */
static int get_cities(const struct player *pplayer)
{
  return pplayer->score.cities;
}

/**
  Number of buildings in cities (not wonders)
 */
static int get_improvements(const struct player *pplayer)
{
  return pplayer->score.improvements;
}

/**
  All wonders, including small wonders
 */
static int get_all_wonders(const struct player *pplayer)
{
  return pplayer->score.all_wonders;
}

/**
   Number of techs
 */
static int get_techs(const struct player *pplayer)
{
  return pplayer->score.techs;
}

/**
   Number of military units
 */
static int get_munits(const struct player *pplayer)
{
  int result = 0;

  // count up military units
  unit_list_iterate(pplayer->units, punit)
  {
    if (is_military_unit(punit)) {
      result++;
    }
  }
  unit_list_iterate_end;

  return result;
}

/**
   Number of city building units.
 */
static int get_settlers(const struct player *pplayer)
{
  int result = 0;

  if (!game.scenario.prevent_new_cities) {
    // count up settlers
    unit_list_iterate(pplayer->units, punit)
    {
      if (unit_can_do_action(punit, ACTION_FOUND_CITY)) {
        result++;
      }
    }
    unit_list_iterate_end;
  }

  return result;
}

/**
   Great wonders for wonder score
 */
static int get_great_wonders(const struct player *pplayer)
{
  return pplayer->score.wonders;
}

/**
   Literacy score calculated one way. See also get_literacy() to see
   alternative way.
 */
static int get_literacy2(const struct player *pplayer)
{
  return pplayer->score.literacy;
}

/**
   Spaceship score
 */
static int get_spaceship(const struct player *pplayer)
{
  return pplayer->score.spaceship;
}

/**
   Number of units built
 */
static int get_units_built(const struct player *pplayer)
{
  return pplayer->score.units_built;
}

/**
   Number of units killed
 */
static int get_units_killed(const struct player *pplayer)
{
  return pplayer->score.units_killed;
}

/**
   Number of units lost
 */
static int get_units_lost(const struct player *pplayer)
{
  return pplayer->score.units_lost;
}

/**
   Amount of gold.
 */
static int get_gold(const struct player *pplayer)
{
  return pplayer->economic.gold;
}

/**
   Tax rate
 */
static int get_taxrate(const struct player *pplayer)
{
  return pplayer->economic.tax;
}

/**
   Science rate
 */
static int get_scirate(const struct player *pplayer)
{
  return pplayer->economic.science;
}

/**
   Luxury rate
 */
static int get_luxrate(const struct player *pplayer)
{
  return pplayer->economic.luxury;
}

/**
   Number of rioting cities
 */
static int get_riots(const struct player *pplayer)
{
  int result = 0;

  city_list_iterate(pplayer->cities, pcity)
  {
    if (pcity->anarchy > 0) {
      result++;
    }
  }
  city_list_iterate_end;

  return result;
}

/**
   Number of happy citizens
 */
static int get_happypop(const struct player *pplayer)
{
  return pplayer->score.happy;
}

/**
   Number of content citizens
 */
static int get_contentpop(const struct player *pplayer)
{
  return pplayer->score.content;
}

/**
   Number of unhappy citizens
 */
static int get_unhappypop(const struct player *pplayer)
{
  return pplayer->score.unhappy;
}

/**
   Number of specialists.
 */
static int get_specialists(const struct player *pplayer)
{
  int count = 0;

  specialist_type_iterate(sp) { count += pplayer->score.specialists[sp]; }
  specialist_type_iterate_end;

  return count;
}

/**
   Current government
 */
static int get_gov(const struct player *pplayer)
{
  return static_cast<int>(government_number(government_of_player(pplayer)));
}

/**
   Total corruption
 */
static int get_corruption(const struct player *pplayer)
{
  int result = 0;

  city_list_iterate(pplayer->cities, pcity)
  {
    result += pcity->waste[O_TRADE];
  }
  city_list_iterate_end;

  return result;
}

/**
   Total score
 */
static int get_total_score(const struct player *pplayer)
{
  return pplayer->score.game;
}

/**
  League score
  Score = N_techs^1. 7 + N_units_built +
  3xN_units_killedx[N_units_killed/(N_units_lost+1)]^0.5
 */
static int get_league_score(const struct player *pplayer)
{
  return (int) (pow((double) pplayer->score.techs, 1.7)
                + pplayer->score.units_built
                + (3 * pplayer->score.units_killed
                   * pow((double) pplayer->score.units_killed
                             / (pplayer->score.units_lost + 1),
                         0.5)));
}

/**
   Culture score
 */
static int get_culture(const struct player *pplayer)
{
  return pplayer->score.culture;
}

/**
   Construct string containing value and its unit.
 */
static const char *value_units(int val, const char *uni)
{
  static char buf[64];

  if (fc_snprintf(buf, sizeof(buf), "%s%s", int_to_text(val), uni) == -1) {
    qCritical("String truncated in value_units()!");
  }

  return buf;
}

/**
   Helper functions which transform the given value to a string
   depending on the unit.
 */
static const char *area_to_text(int value)
{
  // TRANS: abbreviation of "square miles"
  return value_units(value, PL_(" sq. mi.", " sq. mi.", value));
}

/**
   Construct string containing value followed by '%'. So value is already
   considered to be in units of 1/100.
 */
static const char *percent_to_text(int value)
{
  return value_units(value, "%");
}

/**
   Construct string containing value followed by unit suitable for
   production stats.
 */
static const char *production_to_text(int value)
{
  int clip = MAX(0, value);
  // TRANS: "M tons" = million tons, so always plural
  return value_units(clip, PL_(" M tons", " M tons", clip));
}

/**
   Construct string containing value followed by unit suitable for
   economics stats.
 */
static const char *economics_to_text(int value)
{
  // TRANS: "M goods" = million goods, so always plural
  return value_units(value, PL_(" M goods", " M goods", value));
}

/**
  Construct string containing value followed by unit suitable for
  agriculture stats.
 */
static const char *agriculture_to_text(int value)
{
  /* TRANS: "M bushels" = million bushels, so always plural */
  // FIXME: value can go negative for food

  return value_units(value, PL_(" M bushels", " M bushels", value));
}

/**
   Construct string containing value followed by unit suitable for
   science stats.
 */
static const char *science_to_text(int value)
{
  return value_units(value, PL_(" bulb", " bulbs", value));
}

/**
Construct string containing value followed by unit suitable for
gold income stats.
 */
static const char *income_to_text(int value)
{
  return value_units(value, PL_(" gold", " gold", value));
}

/**
   Construct string containing value followed by unit suitable for
   military service stats.
 */
static const char *mil_service_to_text(int value)
{
  return value_units(value, PL_(" month", " months", value));
}

/**
   Construct string containing value followed by unit suitable for
   pollution stats.
 */
static const char *pollution_to_text(int value)
{
  return value_units(value, PL_(" ton", " tons", value));
}

/**
   Construct string containing value followed by unit suitable for
   culture stats.
 */
static const char *culture_to_text(int value)
{
  // TRANS: Unit(s) of culture
  return value_units(value, PL_(" point", " points", value));
}

/**
  Construct string containing value followed by unit suitable for
  citizen unit stats.
 */
static const char *citizenunits_to_text(int value)
{
  return value_units(value, PL_(" citizen", " citizens", value));
}

/**
  Construct string containing value followed by unit suitable for
  military unit stats.
 */
static const char *mil_units_to_text(int value)
{
  return value_units(value, PL_(" unit", " units", value));
}

/**
  Construct string containing value followed by unit suitable for
  city stats.
 */
static const char *cities_to_text(int value)
{
  return value_units(value, PL_(" city", " cities", value));
}

/**
  Construct string containing value followed by unit suitable for
  score stats.
 */
static const char *score_to_text(int value)
{
  return value_units(value, PL_(" point", " points", value));
}

/**
  Construct string containing value followed by unit suitable for
  improvement stats.
 */
static const char *improvements_to_text(int value)
{
  return value_units(value, PL_(" improvement", " improvements", value));
}

/**
  Construct string containing value followed by unit suitable for
  wonders stats.
 */
static const char *wonders_to_text(int value)
{
  return value_units(value, PL_(" wonder", " wonders", value));
}

/**
   Construct one demographics line.
 */
static void dem_line_item(char *outptr, size_t out_size,
                          struct player *pplayer, const demographic &demo,
                          bv_cols selcols)
{
  int value = 0;
  if (pplayer
      && (BV_ISSET(selcols, DEM_COL_QUANTITY)
          || BV_ISSET(selcols, DEM_COL_RANK)
          || BV_ISSET(selcols, DEM_COL_BEST))) {
    if (pplayer->score.demographics.count(demo.name()) > 0) {
      value = pplayer->score.demographics[demo.name()];
    } else {
      // Fallback in case it's missing (e.g. save).
      // TRANS: %s can be score, population, economics, etc.
      qWarning(
          _("Updating demographic \"%s\". The value may be different from "
            "what it was at turn change."),
          Q_(demo.name()));
      value = demo.evaluate(pplayer);
    }
  }

  if (nullptr != pplayer && BV_ISSET(selcols, DEM_COL_QUANTITY)) {
    const char *text = demo.text(value);

    cat_snprintf(outptr, out_size, " %s", text);
    cat_snprintf(outptr, out_size, "%*s",
                 18 - static_cast<int>(get_internal_string_length(text)),
                 "");
  }

  if (nullptr != pplayer && BV_ISSET(selcols, DEM_COL_RANK)) {
    int basis = value;
    int place = 1;

    players_iterate(other)
    {
      if (GOOD_PLAYER(other) && demo.compare(basis, demo.evaluate(other))) {
        place++;
      }
    }
    players_iterate_end;

    cat_snprintf(outptr, out_size, _("(ranked %d)"), place);
  }

  if (nullptr == pplayer || BV_ISSET(selcols, DEM_COL_BEST)) {
    struct player *best_player = pplayer;
    int best_value = nullptr != pplayer ? value : 0;

    players_iterate(other)
    {
      if (GOOD_PLAYER(other)) {
        int value = demo.evaluate(other);
        if (!best_player || demo.compare(best_value, value)) {
          best_player = other;
          best_value = value;
        }
      }
    }
    players_iterate_end;

    if (nullptr == pplayer
        || (player_has_embassy(pplayer, best_player)
            && (pplayer != best_player))) {
      cat_snprintf(outptr, out_size, "   %s: %s",
                   nation_plural_for_player(best_player),
                   demo.text(demo.evaluate(best_player)));
    }
  }
}

/**
   Verify that a given demography string is valid.  See
   game.demography. If the string is not valid the index of the _first_
   invalid character is return as 'error'.

   Other settings callback functions are in settings.c, but this one uses
   static values from this file so it's done separately.
 */
bool is_valid_demography(const char *demography, int *error)
{
  int len = qstrlen(demography), i;

  /* We check each character individually to see if it's valid.  This
   * does not check for duplicate entries. */
  for (i = 0; i < len; i++) {
    bool found = false;
    int j;

    // See if the character is a valid column label.
    for (j = 0; j < DEM_COL_LAST; j++) {
      if (demography[i] == coltable[j].key) {
        found = true;
        break;
      }
    }

    if (found) {
      continue;
    }

    // See if the character is a valid row label.
    if (rowtable.count(demography[i]) == 0) {
      if (error != nullptr) {
        (*error) = i;
      }
      // The character is invalid.
      return false;
    }
  }

  // Looks like all characters were valid.
  return true;
}

/**
 * Updates cached demographics values.
 *
 * All values are stored, even the ones that are not enabled. This simplifies
 * the implementation and allows toggling demographics during the game.
 */
void update_demographics(struct player *pplayer)
{
  pplayer->score.demographics.clear();
  for (const auto &[_, demo] : rowtable) {
    pplayer->score.demographics[demo.name()] = demo.evaluate(pplayer);
  }
}

/**
   Send demographics report; what gets reported depends on value of
   demographics server option.
 */
void report_demographics(struct connection *pconn)
{
  char civbuf[1024];
  char buffer[4096];
  bv_cols selcols;
  int numcols = 0;
  struct player *pplayer = pconn->playing;

  BV_CLR_ALL(selcols);
  fc_assert_ret(ARRAY_SIZE(coltable) == DEM_COL_LAST);
  for (int i = 0; i < DEM_COL_LAST; i++) {
    if (strchr(game.server.demography, coltable[i].key)) {
      BV_SET(selcols, i);
      numcols++;
    }
  }

  std::vector<demographic> rows;
  auto demography = QString(game.server.demography);
  for (const auto &[key, demo] : rowtable) {
    if (demography.contains(key)) {
      rows.push_back(demo);
    }
  }

  if ((!pconn->observer && !pplayer) || (pplayer && !pplayer->is_alive)
      || rows.empty() || numcols == 0) {
    page_conn(pconn->self, _("Demographics Report:"),
              _("Sorry, the Demographics report is unavailable."), "");
    return;
  }

  if (pplayer) {
    fc_snprintf(civbuf, sizeof(civbuf), _("%s %s (%s)"),
                nation_adjective_for_player(pplayer),
                government_name_for_player(pplayer), calendar_text());
  } else {
    civbuf[0] = '\0';
  }

  buffer[0] = '\0';
  for (const auto &demo : rows) {
    const char *name = Q_(demo.name());

    cat_snprintf(buffer, sizeof(buffer), "%s", name);
    cat_snprintf(buffer, sizeof(buffer), "%*s",
                 18 - static_cast<int>(get_internal_string_length(name)),
                 "");
    dem_line_item(buffer, sizeof(buffer), pplayer, demo, selcols);
    sz_strlcat(buffer, "\n");
  }

  page_conn(pconn->self, _("Demographics Report:"), civbuf, buffer);
}

/**
   Send achievements list
 */
void report_achievements(struct connection *pconn)
{
  char civbuf[1024];
  char buffer[4096];
  struct player *pplayer = pconn->playing;

  if (pplayer == nullptr) {
    return;
  }

  fc_snprintf(civbuf, sizeof(civbuf), _("%s %s (%s)"),
              nation_adjective_for_player(pplayer),
              government_name_for_player(pplayer), calendar_text());

  buffer[0] = '\0';

  achievements_iterate(pach)
  {
    if (achievement_player_has(pach, pplayer)) {
      cat_snprintf(buffer, sizeof(buffer), "%s\n",
                   achievement_name_translation(pach));
    }
  }
  achievements_iterate_end;

  page_conn(pconn->self, _("Achievements List:"), civbuf, buffer);
}

/**
   Allocate and initialize plrdata slot.
 */
static void plrdata_slot_init(struct plrdata_slot *plrdata, const char *name)
{
  fc_assert_ret(plrdata->name == nullptr);

  plrdata->name = new char[MAX_LEN_NAME]();
  plrdata_slot_replace(plrdata, name);
}

/**
   Replace plrdata slot with new one named according to input parameter.
 */
static void plrdata_slot_replace(struct plrdata_slot *plrdata,
                                 const char *name)
{
  fc_assert_ret(plrdata->name != nullptr);

  fc_strlcpy(plrdata->name, name, MAX_LEN_NAME);
}

/**
   Free resources allocated for plrdata slot.
 */
static void plrdata_slot_free(struct plrdata_slot *plrdata)
{
  delete[] plrdata->name;
  plrdata->name = nullptr;
}

/**
   Reads the whole file denoted by fp. Sets last_turn and id to the
   values contained in the file. Returns the player_names indexed by
   player_no at the end of the log file.

   Returns TRUE iff the file had read successfully.
 */
static bool scan_score_log(char *id)
{
  int line_nr, turn, plr_no, spaces;
  struct plrdata_slot *plrdata;
  char plr_name[120], line[120], *ptr;

  fc_assert_ret_val(score_log != nullptr, false);
  fc_assert_ret_val(score_log->fp != nullptr, false);

  score_log->last_turn = -1;
  id[0] = '\0';

  for (line_nr = 1;; line_nr++) {
    if (!fgets(line, sizeof(line), score_log->fp)) {
      if (feof(score_log->fp) != 0) {
        break;
      }
      qCritical("[%s:-] Can't read scorelog file header!",
                game.server.scorefile);
      return false;
    }

    ptr = strchr(line, '\n');
    if (!ptr) {
      qCritical("[%s:%d] Line too long!", game.server.scorefile, line_nr);
      return false;
    }
    *ptr = '\0';

    if (line_nr == 1) {
      if (strncmp(line, scorelog_magic, qstrlen(scorelog_magic)) != 0) {
        qCritical("[%s:%d] Bad file magic!", game.server.scorefile, line_nr);
        return false;
      }
    }

    if (strncmp(line, "id ", qstrlen("id ")) == 0) {
      if (strlen(id) > 0) {
        qCritical("[%s:%d] Multiple ID entries!", game.server.scorefile,
                  line_nr);
        return false;
      }
      fc_strlcpy(id, line + qstrlen("id "), MAX_LEN_GAME_IDENTIFIER);
      if (strcmp(id, server.game_identifier) != 0) {
        qCritical("[%s:%d] IDs don't match! game='%s' scorelog='%s'",
                  game.server.scorefile, line_nr, server.game_identifier,
                  id);
        return false;
      }
    }

    if (strncmp(line, "turn ", qstrlen("turn ")) == 0) {
      if (sscanf(line + qstrlen("turn "), "%d", &turn) != 1) {
        qCritical("[%s:%d] Bad line (turn)!", game.server.scorefile,
                  line_nr);
        return false;
      }

      fc_assert_ret_val(turn > score_log->last_turn, false);
      score_log->last_turn = turn;
    }

    if (strncmp(line, "addplayer ", qstrlen("addplayer ")) == 0) {
      if (3
          != std::sscanf(line + qstrlen("addplayer "), "%d %d %s", &turn,
                         &plr_no, plr_name)) {
        qCritical("[%s:%d] Bad line (addplayer)!", game.server.scorefile,
                  line_nr);
        return false;
      }

      // Now get the complete player name if there are several parts.
      ptr = line + qstrlen("addplayer ");
      spaces = 0;
      while (*ptr != '\0' && spaces < 2) {
        if (*ptr == ' ') {
          spaces++;
        }
        ptr++;
      }
      fc_snprintf(plr_name, sizeof(plr_name), "%s", ptr);
      log_debug("add player '%s' (from line %d: '%s')", plr_name, line_nr,
                line);

      if (0 > plr_no || plr_no >= MAX_NUM_PLAYER_SLOTS) {
        qCritical("[%s:%d] Invalid player number: %d!",
                  game.server.scorefile, line_nr, plr_no);
        return false;
      }

      plrdata = score_log->plrdata + plr_no;
      if (plrdata->name != nullptr) {
        qCritical("[%s:%d] Two names for one player (id %d)!",
                  game.server.scorefile, line_nr, plr_no);
        return false;
      }

      plrdata_slot_init(plrdata, plr_name);
    }

    if (strncmp(line, "delplayer ", qstrlen("delplayer ")) == 0) {
      if (2
          != sscanf(line + qstrlen("delplayer "), "%d %d", &turn, &plr_no)) {
        qCritical("[%s:%d] Bad line (delplayer)!", game.server.scorefile,
                  line_nr);
        return false;
      }

      if (!(plr_no >= 0 && plr_no < MAX_NUM_PLAYER_SLOTS)) {
        qCritical("[%s:%d] Invalid player number: %d!",
                  game.server.scorefile, line_nr, plr_no);
        return false;
      }

      plrdata = score_log->plrdata + plr_no;
      if (plrdata->name == nullptr) {
        qCritical("[%s:%d] Trying to remove undefined player (id %d)!",
                  game.server.scorefile, line_nr, plr_no);
        return false;
      }

      plrdata_slot_free(plrdata);
    }
  }

  if (score_log->last_turn == -1) {
    qCritical("[%s:-] Scorelog contains no turn!", game.server.scorefile);
    return false;
  }

  if (strlen(id) == 0) {
    qCritical("[%s:-] Scorelog contains no ID!", game.server.scorefile);
    return false;
  }

  if (score_log->last_turn + 1 != game.info.turn) {
    qCritical("[%s:-] Scorelog doesn't match savegame!",
              game.server.scorefile);
    return false;
  }

  return true;
}

/**
   Initialize score logging system
 */
void log_civ_score_init()
{
  if (score_log != nullptr) {
    return;
  }

  score_log = new logging_civ_score[1]();
  score_log->fp = nullptr;
  score_log->last_turn = -1;
  score_log->plrdata = new plrdata_slot[MAX_NUM_PLAYER_SLOTS]();
  player_slots_iterate(pslot)
  {
    struct plrdata_slot *plrdata =
        score_log->plrdata + player_slot_index(pslot);
    plrdata_slot_free(plrdata);
  }
  player_slots_iterate_end;

  latest_history_report.turn = -2;
}

/**
   Free resources allocated for score logging system
 */
void log_civ_score_free()
{
  if (!score_log) {
    // nothing to do
    return;
  }

  if (score_log->fp) {
    fclose(score_log->fp);
    score_log->fp = nullptr;
  }

  if (score_log->plrdata) {
    player_slots_iterate(pslot)
    {
      struct plrdata_slot *plrdata =
          score_log->plrdata + player_slot_index(pslot);
      plrdata_slot_free(plrdata);
    }
    player_slots_iterate_end;
    delete[] score_log->plrdata;
    score_log->plrdata = nullptr;
  }

  delete[] score_log;
  score_log = nullptr;
}

/**
   Create a log file of the civilizations so you can see what was happening.
 */
void log_civ_score_now()
{
  enum { SL_CREATE, SL_APPEND, SL_UNSPEC } oper = SL_UNSPEC;
  char id[MAX_LEN_GAME_IDENTIFIER];
  int i = 0;

  /* Add new tags only at end of this list. Maintaining the order of
   * old tags is critical. */
  static const std::array score_tags{
      demographic("pop", get_pop, citizenunits_to_text, true),
      demographic("bnp", get_economics, economics_to_text, true),
      demographic("mfg", get_production, production_to_text, true),
      demographic("cities", get_cities, cities_to_text, true),
      demographic("techs", get_techs, nullptr, true),
      demographic("munits", get_munits, mil_units_to_text, true),
      demographic("settlers", get_settlers, mil_units_to_text, true),
      demographic("wonders", get_great_wonders, wonders_to_text, true),
      demographic("techout", get_research, science_to_text, true),
      demographic("landarea", get_landarea, area_to_text, true),
      demographic("settledarea", get_settledarea, area_to_text, true),
      demographic("pollution", get_pollution, pollution_to_text, true),
      demographic("literacy", get_literacy2, percent_to_text, true),
      demographic("spaceship", get_spaceship, score_to_text, true),
      demographic("gold", get_gold, income_to_text, true),
      demographic("taxrate", get_taxrate, percent_to_text, true),
      demographic("scirate", get_scirate, percent_to_text, true),
      demographic("luxrate", get_luxrate, percent_to_text, true),
      demographic("riots", get_riots, nullptr, false),
      demographic("happypop", get_happypop, citizenunits_to_text, true),
      demographic("contentpop", get_contentpop, citizenunits_to_text, true),
      demographic("unhappypop", get_unhappypop, citizenunits_to_text, false),
      demographic("specialists", get_specialists, citizenunits_to_text,
                  true),
      demographic("gov", get_gov, nullptr, true /* FIXME */),
      demographic("corruption", get_corruption, economics_to_text, true),
      demographic("score", get_total_score, score_to_text, true),
      demographic("unitsbuilt", get_units_built, mil_units_to_text, true),
      demographic("unitskilled", get_units_killed, mil_units_to_text, true),
      demographic("unitslost", get_units_lost, score_to_text, true),
      demographic("culture", get_culture, culture_to_text, true),
  };

  if (!game.server.scorelog) {
    return;
  }

  if (!score_log) {
    return;
  }

  if (!score_log->fp) {
    if (game.info.year == GAME_START_YEAR) {
      oper = SL_CREATE;
    } else {
      score_log->fp = fc_fopen(game.server.scorefile, "r");
      if (!score_log->fp) {
        oper = SL_CREATE;
      } else {
        if (!scan_score_log(id)) {
          goto log_civ_score_disable;
        }
        oper = SL_APPEND;

        fclose(score_log->fp);
        score_log->fp = nullptr;
      }
    }

    switch (oper) {
    case SL_CREATE:
      score_log->fp = fc_fopen(game.server.scorefile, "w");
      if (!score_log->fp) {
        qCritical("Can't open scorelog file '%s' for creation!",
                  game.server.scorefile);
        goto log_civ_score_disable;
      }
      fprintf(score_log->fp, "%s%s\n", scorelog_magic, freeciv21_version());
      fprintf(score_log->fp, "\n"
                             "# For a specification of the format of this "
                             "see doc/README.scorelog or \n"
                             "# "
                             "<https://raw.githubusercontent.com/freeciv/"
                             "freeciv/master/doc/README.scorelog>.\n"
                             "\n");

      fprintf(score_log->fp, "id %s\n", server.game_identifier);
      for (const auto &demo : score_tags) {
        fprintf(score_log->fp, "tag %d %s\n", i, demo.name());
      }
      break;
    case SL_APPEND:
      score_log->fp = fc_fopen(game.server.scorefile, "a");
      if (!score_log->fp) {
        qCritical("Can't open scorelog file '%s' for appending!",
                  game.server.scorefile);
        goto log_civ_score_disable;
      }
      break;
    default:
      qCritical("[%s] bad operation %d", __FUNCTION__,
                static_cast<int>(oper));
      goto log_civ_score_disable;
    }
  }

  if (game.info.turn > score_log->last_turn) {
    fprintf(score_log->fp, "turn %d %d %s\n", game.info.turn, game.info.year,
            calendar_text());
    score_log->last_turn = game.info.turn;
  }

  player_slots_iterate(pslot)
  {
    struct plrdata_slot *plrdata =
        score_log->plrdata + player_slot_index(pslot);
    if (plrdata->name != nullptr && player_slot_is_used(pslot)) {
      struct player *pplayer = player_slot_get_player(pslot);

      if (!GOOD_PLAYER(pplayer)) {
        fprintf(score_log->fp, "delplayer %d %d\n", game.info.turn - 1,
                player_number(pplayer));
        plrdata_slot_free(plrdata);
      }
    }
  }
  player_slots_iterate_end;

  players_iterate(pplayer)
  {
    struct plrdata_slot *plrdata =
        score_log->plrdata + player_index(pplayer);

    if (plrdata->name == nullptr && GOOD_PLAYER(pplayer)) {
      switch (game.server.scoreloglevel) {
      case SL_HUMANS:
        if (is_ai(pplayer)) {
          break;
        }

        fc__fallthrough; /* No break - continue to actual implementation
                          * in SL_ALL case if reached here */
      case SL_ALL:
        fprintf(score_log->fp, "addplayer %d %d %s\n", game.info.turn,
                player_number(pplayer), player_name(pplayer));
        plrdata_slot_init(plrdata, player_name(pplayer));
      }
    }
  }
  players_iterate_end;

  players_iterate(pplayer)
  {
    struct plrdata_slot *plrdata =
        score_log->plrdata + player_index(pplayer);

    if (GOOD_PLAYER(pplayer)) {
      switch (game.server.scoreloglevel) {
      case SL_HUMANS:
        if (is_ai(pplayer) && plrdata->name == nullptr) {
          // If a human player toggled into AI mode, don't break.
          break;
        }

        fc__fallthrough; /* No break - continue to actual implementation
                          * in SL_ALL case if reached here */
      case SL_ALL:
        if (strcmp(plrdata->name, player_name(pplayer)) != 0) {
          log_debug("player names does not match '%s' != '%s'",
                    plrdata->name, player_name(pplayer));
          fprintf(score_log->fp, "delplayer %d %d\n", game.info.turn - 1,
                  player_number(pplayer));
          fprintf(score_log->fp, "addplayer %d %d %s\n", game.info.turn,
                  player_number(pplayer), player_name(pplayer));
          plrdata_slot_replace(plrdata, player_name(pplayer));
        }
      }
    }
  }
  players_iterate_end;

  for (const auto &demo : score_tags) {
    players_iterate(pplayer)
    {
      if (!GOOD_PLAYER(pplayer)
          || (game.server.scoreloglevel == SL_HUMANS && is_ai(pplayer))) {
        continue;
      }

      fprintf(score_log->fp, "data %d %d %d %d\n", game.info.turn, i,
              player_number(pplayer), demo.evaluate(pplayer));
    }
    players_iterate_end;
  }

  fflush(score_log->fp);

  return;

log_civ_score_disable:

  log_civ_score_free();
}

/**
   Produce random history report if it's time for one.
 */
void make_history_report()
{
  if (player_count() == 1) {
    return;
  }

  if (game.server.scoreturn > game.info.turn) {
    return;
  }

  game.server.scoreturn = (game.info.turn + GAME_DEFAULT_SCORETURN
                           + fc_rand(GAME_DEFAULT_SCORETURN));

  historian_generic(
      &latest_history_report,
      historian_type(game.server.scoreturn % (HISTORIAN_LAST + 1)));
  send_current_history_report(game.est_connections);
}

/**
   Inform clients about player scores and statistics when the game ends.
   Called only from server/srv_main.c srv_scores()
 */
void report_final_scores(struct conn_list *dest)
{
  static const struct {
    const char *name;
    int (*score)(const struct player *);
  } score_categories[] = {
      {N_("Population\n"), get_real_pop},
      // TRANS: "M goods" = million goods
      {N_("Trade\n(M goods)"), get_economics},
      // TRANS: "M tons" = million tons
      {N_("Production\n(M tons)"), get_production},
      {N_("Cities\n"), get_cities},
      {N_("Technologies\n"), get_techs},
      {N_("Military Service\n(months)"), get_mil_service},
      {N_("Wonders\n"), get_great_wonders},
      {N_("Research Speed\n(bulbs)"), get_research},
      // TRANS: "sq. mi." is abbreviation for "square miles"
      {N_("Land Area\n(sq. mi.)"), get_landarea},
      // TRANS: "sq. mi." is abbreviation for "square miles"
      {N_("Settled Area\n(sq. mi.)"), get_settledarea},
      {N_("Literacy\n(%)"), get_literacy},
      {N_("Culture\n"), get_culture},
      {N_("Spaceship\n"), get_spaceship},
      {N_("Built Units\n"), get_units_built},
      {N_("Killed Units\n"), get_units_killed},
      {N_("Unit Losses\n"), get_units_lost},
  };
  const size_t score_categories_num = ARRAY_SIZE(score_categories);

  int i, j;
  struct player_score_entry size[player_count()];
  struct packet_endgame_report packet;

  fc_assert(score_categories_num <= ARRAY_SIZE(packet.category_name));

  if (!dest) {
    dest = game.est_connections;
  }

  packet.category_num = score_categories_num;
  for (j = 0; j < score_categories_num; j++) {
    sz_strlcpy(packet.category_name[j], score_categories[j].name);
  }

  i = 0;
  players_iterate(pplayer)
  {
    if (!is_barbarian(pplayer)) {
      size[i].value = pplayer->score.game;
      size[i].player = pplayer;
      i++;
    }
  }
  players_iterate_end;

  qsort(size, i, sizeof(size[0]), secompare);

  packet.player_num = i;

  lsend_packet_endgame_report(dest, &packet);

  for (i = 0; i < packet.player_num; i++) {
    struct packet_endgame_player ppacket;
    const struct player *pplayer = size[i].player;

    ppacket.category_num = score_categories_num;
    ppacket.player_id = player_number(pplayer);
    ppacket.score = size[i].value;
    for (j = 0; j < score_categories_num; j++) {
      ppacket.category_score[j] = score_categories[j].score(pplayer);
    }

    ppacket.winner = pplayer->is_winner;

    lsend_packet_endgame_player(dest, &ppacket);
  }
}

/**
   This function pops up a non-modal message dialog on the player's desktop
 */
void page_conn(struct conn_list *dest, const char *caption,
               const char *headline, const char *lines)
{
  page_conn_etype(dest, caption, headline, lines, E_REPORT);
}

/**
   This function pops up a non-modal message dialog on the player's desktop

   event == E_REPORT: message should not be ignored by clients watching
                      AI players with ai_popup_windows off. Example:
                      Server Options, Demographics Report, etc.

   event == E_BROADCAST_REPORT: message can safely be ignored by clients
                      watching AI players with ai_popup_windows off. For
                      example: Herodot's report... and similar messages.
 */
static void page_conn_etype(struct conn_list *dest, const char *caption,
                            const char *headline, const char *lines,
                            enum event_type event)
{
  struct packet_page_msg packet;
  int i;
  int len;

  sz_strlcpy(packet.caption, caption);
  sz_strlcpy(packet.headline, headline);
  packet.event = event;
  len = qstrlen(lines);
  if ((len % (MAX_LEN_CONTENT - 1)) == 0) {
    packet.parts = len / (MAX_LEN_CONTENT - 1);
  } else {
    packet.parts = len / (MAX_LEN_CONTENT - 1) + 1;
  }
  packet.len = len;

  lsend_packet_page_msg(dest, &packet);

  for (i = 0; i < packet.parts; i++) {
    struct packet_page_msg_part part;
    int plen;

    plen = MIN(len, (MAX_LEN_CONTENT - 1));
    qstrncpy(part.lines, &(lines[(MAX_LEN_CONTENT - 1) * i]), plen);
    part.lines[plen] = '\0';

    lsend_packet_page_msg_part(dest, &part);

    len -= plen;
  }
}

/**
   Return current history report
 */
struct history_report *history_report_get()
{
  return &latest_history_report;
}
