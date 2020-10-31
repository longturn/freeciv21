/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#include "tradecalculation.h"

#include <random>
// common
#include "city.h"
#include "featured_text.h"
#include "traderoutes.h"
// client
#include "chatline_common.h" /* Help me, I want to common */
#include "client_main.h"
// qt
#include "fc_client.h"
#include "mapview.h"
#include "messagewin.h"
#include "page_game.h"

static bool tradecity_rand(const trade_city *t1, const trade_city *t2);

/**********************************************************************/ /**
   Constructor for trade city used to trade calculation
 **************************************************************************/
trade_city::trade_city(struct city *pcity)
{
  city = pcity;
  tile = nullptr;
  trade_num = 0;
  poss_trade_num = 0;
  done = false;
  over_max = false;
}

/**********************************************************************/ /**
   Constructor for trade calculator
 **************************************************************************/
trade_generator::trade_generator() { hover_city = false; }

/**********************************************************************/ /**
   Adds all cities to trade generator
 **************************************************************************/
void trade_generator::add_all_cities()
{
  int i, s;
  struct city *pcity;
  clear_trade_planing();
  s = city_list_size(client.conn.playing->cities);
  if (s == 0) {
    return;
  }
  for (i = 0; i < s; i++) {
    pcity = city_list_get(client.conn.playing->cities, i);
    add_city(pcity);
  }
}

/**********************************************************************/ /**
   Clears genrated routes, virtual cities, cities
 **************************************************************************/
void trade_generator::clear_trade_planing()
{
  for (auto pcity : qAsConst(virtual_cities)) {
    destroy_city_virtual(pcity);
  }
  virtual_cities.clear();
  for (auto tc : qAsConst(cities)) {
    delete tc;
  }
  cities.clear();
  lines.clear();
  queen()->mapview_wdg->repaint();
}

/**********************************************************************/ /**
   Adds single city to trade generator
 **************************************************************************/
void trade_generator::add_city(struct city *pcity)
{
  trade_city *tc = new trade_city(pcity);
  cities.append(tc);
  queen()->infotab->chtwdg->append(
      QString(_("Adding city %1 to trade planning")).arg(tc->city->name));
}

/**********************************************************************/ /**
   Adds/removes tile to trade generator
 **************************************************************************/
void trade_generator::add_tile(struct tile *ptile)
{
  struct city *pcity;
  pcity = tile_city(ptile);

  for (auto tc : qAsConst(cities)) {
    if (pcity != nullptr) {
      if (tc->city == pcity) {
        remove_city(pcity);
        return;
      }
    }
    if (tc->city->tile == ptile) {
      remove_virtual_city(ptile);
      return;
    }
  }

  if (pcity != nullptr) {
    add_city(pcity);
    return;
  }

  pcity = create_city_virtual(client_player(), ptile, "Virtual");
  add_city(pcity);
  virtual_cities.append(pcity);
}

/**********************************************************************/ /**
   Removes single city from trade generator
 **************************************************************************/
void trade_generator::remove_city(struct city *pcity)
{
  for (auto tc : qAsConst(cities)) {
    if (tc->city->tile == pcity->tile) {
      cities.removeAll(tc);
      queen()->infotab->chtwdg->append(
          QString(_("Removing city %1 from trade planning"))
              .arg(tc->city->name));
      return;
    }
  }
}

/**********************************************************************/ /**
   Removes virtual city from trade generator
 **************************************************************************/
void trade_generator::remove_virtual_city(tile *ptile)
{
  for (auto c : qAsConst(virtual_cities)) {
    if (c->tile == ptile) {
      virtual_cities.removeAll(c);
      queen()->infotab->chtwdg->append(
          QString(_("Removing city %1 from trade planning")).arg(c->name));
    }
  }

  for (auto tc : qAsConst(cities)) {
    if (tc->city->tile == ptile) {
      cities.removeAll(tc);
      return;
    }
  }
}

/**********************************************************************/ /**
   Finds trade routes to establish
 **************************************************************************/
void trade_generator::calculate()
{
  int i;
  bool tdone;
  std::random_device rd;
  std::mt19937 g(rd());

  for (i = 0; i < 100; i++) {
    tdone = true;
    std::shuffle(cities.begin(), cities.end(), g);
    lines.clear();
    for (auto tc : qAsConst(cities)) {
      tc->pos_cities.clear();
      tc->new_tr_cities.clear();
      tc->curr_tr_cities.clear();
    }
    for (auto tc : qAsConst(cities)) {
      tc->trade_num = city_num_trade_routes(tc->city);
      tc->poss_trade_num = 0;
      tc->pos_cities.clear();
      tc->new_tr_cities.clear();
      tc->curr_tr_cities.clear();
      tc->done = false;
      for (auto ttc : qAsConst(cities)) {
        if (!have_cities_trade_route(tc->city, ttc->city)
            && can_establish_trade_route(tc->city, ttc->city)) {
          tc->poss_trade_num++;
          tc->pos_cities.append(ttc->city);
        }
        tc->over_max =
            tc->trade_num + tc->poss_trade_num - max_trade_routes(tc->city);
      }
    }

    find_certain_routes();
    discard();
    find_certain_routes();

    for (auto tc : qAsConst(cities)) {
      if (!tc->done) {
        tdone = false;
      }
    }
    if (tdone) {
      break;
    }
  }
  for (auto tc : qAsConst(cities)) {
    if (!tc->done) {
      char text[1024];
      fc_snprintf(text, sizeof(text),
                  PL_("City %s - 1 free trade route.",
                      "City %s - %d free trade routes.",
                      max_trade_routes(tc->city) - tc->trade_num),
                  city_link(tc->city),
                  max_trade_routes(tc->city) - tc->trade_num);
      output_window_append(ftc_client, text);
    }
  }

  queen()->mapview_wdg->repaint();
}

/**********************************************************************/ /**
   Finds highest number of trade routes over maximum for all cities,
   skips given city
 **************************************************************************/
int trade_generator::find_over_max(struct city *pcity = nullptr)
{
  int max = 0;

  for (auto tc : qAsConst(cities)) {
    if (pcity != tc->city) {
      max = qMax(max, tc->over_max);
    }
  }
  return max;
}

/**********************************************************************/ /**
   Finds city with highest trade routes possible
 **************************************************************************/
trade_city *trade_generator::find_most_free()
{
  trade_city *rc = nullptr;
  int max = 0;

  for (auto tc : qAsConst(cities)) {
    if (max < tc->over_max) {
      max = tc->over_max;
      rc = tc;
    }
  }
  return rc;
}

/**********************************************************************/ /**
   Drops all possible trade routes.
 **************************************************************************/
void trade_generator::discard()
{
  trade_city *tc;
  int j = 5;

  for (int i = j; i > -j; i--) {
    while ((tc = find_most_free())) {
      if (!discard_one(tc)) {
        if (!discard_any(tc, i)) {
          break;
        }
      }
    }
  }
}

/**********************************************************************/ /**
   Drops trade routes between given cities
 **************************************************************************/
void trade_generator::discard_trade(trade_city *tc, trade_city *ttc)
{
  tc->pos_cities.removeOne(ttc->city);
  ttc->pos_cities.removeOne(tc->city);
  tc->poss_trade_num--;
  ttc->poss_trade_num--;
  tc->over_max--;
  ttc->over_max--;
  check_if_done(tc, ttc);
}

/**********************************************************************/ /**
   Drops one trade route for given city if possible
 **************************************************************************/
bool trade_generator::discard_one(trade_city *tc)
{
  int best = 0;
  int current_candidate = 0;
  int best_id;
  trade_city *ttc;

  for (int i = cities.size() - 1; i >= 0; i--) {
    ttc = cities.at(i);
    current_candidate = ttc->over_max;
    if (current_candidate > best) {
      best_id = i;
    }
  }
  if (best == 0) {
    return false;
  }

  ttc = cities.at(best_id);
  discard_trade(tc, ttc);
  return true;
}

/**********************************************************************/ /**
   Drops all trade routes for given city
 **************************************************************************/
bool trade_generator::discard_any(trade_city *tc, int freeroutes)
{
  trade_city *ttc;

  for (int i = cities.size() - 1; i >= 0; i--) {
    ttc = cities.at(i);
    if (tc->pos_cities.contains(ttc->city)
        && ttc->pos_cities.contains(tc->city)
        && ttc->over_max > freeroutes) {
      discard_trade(tc, ttc);
      return true;
    }
  }
  return false;
}

/**********************************************************************/ /**
   Helper function ato randomize list
 **************************************************************************/
bool tradecity_rand(const trade_city *t1, const trade_city *t2)
{
  return (qrand() % 2);
}

/**********************************************************************/ /**
   Adds routes for cities which can only have maximum possible trade routes
 **************************************************************************/
void trade_generator::find_certain_routes()
{
  for (auto tc : qAsConst(cities)) {
    if (tc->done || tc->over_max > 0) {
      continue;
    }
    for (auto ttc : qAsConst(cities)) {
      if (ttc->done || ttc->over_max > 0 || tc == ttc || tc->done
          || tc->over_max > 0) {
        continue;
      }
      if (tc->pos_cities.contains(ttc->city)
          && ttc->pos_cities.contains(tc->city)) {
        struct qtiles gilles;
        tc->pos_cities.removeOne(ttc->city);
        ttc->pos_cities.removeOne(tc->city);
        tc->poss_trade_num--;
        ttc->poss_trade_num--;
        tc->new_tr_cities.append(ttc->city);
        ttc->new_tr_cities.append(ttc->city);
        tc->trade_num++;
        ttc->trade_num++;
        tc->over_max--;
        ttc->over_max--;
        check_if_done(tc, ttc);
        gilles.t1 = tc->city->tile;
        gilles.t2 = ttc->city->tile;
        gilles.autocaravan = nullptr;
        lines.append(gilles);
      }
    }
  }
}

/**********************************************************************/ /**
   Marks cities with full trade routes to finish searching
 **************************************************************************/
void trade_generator::check_if_done(trade_city *tc1, trade_city *tc2)
{
  if (tc1->trade_num == max_trade_routes(tc1->city)) {
    tc1->done = true;
  }
  if (tc2->trade_num == max_trade_routes(tc2->city)) {
    tc2->done = true;
  }
}
