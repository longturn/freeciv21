/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2023 Freeciv21 and Freeciv
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

/**
  This module contains various general - mostly highlevel - functions
  used throughout the client.
 */

#include <QBitArray>
#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

// utility
#include "fcintl.h"
#include "log.h"
#include "support.h"

// common
#include "city.h"
#include "diptreaty.h"
#include "featured_text.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "mapimg.h"
#include "packets.h"
#include "research.h"
#include "spaceship.h"
#include "unitlist.h"

/* client/include */
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "dialogs_g.h"
#include "mapview_g.h"

// client
#include "chatline_common.h"
#include "citydlg_common.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "control.h"
#include "messagewin_common.h"
#include "options.h"
#include "packhand.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "tileset/tilespec.h"
#include "views/view_map.h"
#include "views/view_map_common.h"

extern void flush_dirty_overview();
/**
   Remove unit, client end version
 */
void client_remove_unit(struct unit *punit)
{
  struct city *pcity;
  struct tile *ptile = unit_tile(punit);
  int hc = punit->homecity;
  struct unit old_unit = *punit;
  int old = get_num_units_in_focus();
  bool update;

  log_debug("removing unit %d, %s %s (%d %d) hcity %d", punit->id,
            nation_rule_name(nation_of_unit(punit)), unit_rule_name(punit),
            TILE_XY(unit_tile(punit)), hc);

  update = (get_focus_unit_on_tile(unit_tile(punit)) != nullptr);

  // Check transport status.
  unit_transport_unload(punit);
  if (get_transporter_occupancy(punit) > 0) {
    unit_list_iterate(unit_transport_cargo(punit), pcargo)
    {
      /* The server should take care that the unit is on the right terrain.
       */
      unit_transport_unload(pcargo);
    }
    unit_list_iterate_end;
  }

  control_unit_killed(punit);
  game_remove_unit(&wld, punit);
  punit = nullptr;
  if (old > 0 && get_num_units_in_focus() == 0) {
    unit_focus_advance();
  } else if (update) {
    update_unit_pix_label(get_units_in_focus());
    update_unit_info_label(get_units_in_focus());
  }

  pcity = tile_city(ptile);
  if (nullptr != pcity) {
    if (can_player_see_units_in_city(client_player(), pcity)) {
      pcity->client.occupied = (0 < unit_list_size(pcity->tile->units));
      refresh_city_dialog(pcity);
    }

    log_debug("map city %s, %s, (%d %d)", city_name_get(pcity),
              nation_rule_name(nation_of_city(pcity)),
              TILE_XY(city_tile(pcity)));
  }

  if (!client_has_player() || unit_owner(&old_unit) == client_player()) {
    pcity = game_city_by_number(hc);
    if (nullptr != pcity) {
      refresh_city_dialog(pcity);
      log_debug("home city %s, %s, (%d %d)", city_name_get(pcity),
                nation_rule_name(nation_of_city(pcity)),
                TILE_XY(city_tile(pcity)));
    }
  }

  refresh_unit_mapcanvas(&old_unit, ptile, true);
  flush_dirty_overview();
}

/**
   Remove city, client end version.
 */
void client_remove_city(struct city *pcity)
{
  bool effect_update;
  struct tile *ptile = city_tile(pcity);
  struct city old_city = *pcity;

  log_debug("client_remove_city() %d, %s", pcity->id, city_name_get(pcity));

  /* Explicitly remove all improvements, to properly remove any global
     effects and to handle the preservation of "destroyed" effects. */
  effect_update = false;

  city_built_iterate(pcity, pimprove)
  {
    effect_update = true;
    city_remove_improvement(pcity, pimprove);
  }
  city_built_iterate_end;

  if (effect_update) {
    // nothing yet
  }

  if (auto dialog = is_any_city_dialog_open();
      dialog && pcity->id == dialog->id) {
    popdown_city_dialog();
  }
  game_remove_city(&wld, pcity);
  city_report_dialog_update();
  refresh_city_mapcanvas(&old_city, ptile, true);
}

/**
   Return a string indicating one nation's embassy status with another
 */
const char *get_embassy_status(const struct player *me,
                               const struct player *them)
{
  if (!me || !them || me == them || !them->is_alive || !me->is_alive) {
    return "-";
  }
  if (player_has_embassy(me, them)) {
    if (player_has_embassy(them, me)) {
      return Q_("?embassy:Both");
    } else {
      return Q_("?embassy:Yes");
    }
  } else if (player_has_embassy(them, me)) {
    return Q_("?embassy:With Us");
  } else if (player_diplstate_get(me, them)->contact_turns_left > 0
             || player_diplstate_get(them, me)->contact_turns_left > 0) {
    return Q_("?embassy:Contact");
  } else {
    return Q_("?embassy:No Contact");
  }
}

/**
   Return a string indicating one nation's shaed vision status with another
 */
const char *get_vision_status(const struct player *me,
                              const struct player *them)
{
  if (me && them && gives_shared_vision(me, them)) {
    if (gives_shared_vision(them, me)) {
      return Q_("?vision:Both");
    } else {
      return Q_("?vision:To Them");
    }
  } else if (me && them && gives_shared_vision(them, me)) {
    return Q_("?vision:To Us");
  } else {
    return "";
  }
}

/**
   Copy a string that describes the given clause into the return buffer.
 */
void client_diplomacy_clause_string(char *buf, int bufsiz,
                                    struct Clause *pclause)
{
  struct city *pcity;

  switch (pclause->type) {
  case CLAUSE_ADVANCE:
    fc_snprintf(buf, bufsiz, _("The %s give %s"),
                nation_plural_for_player(pclause->from),
                advance_name_translation(advance_by_number(pclause->value)));
    break;
  case CLAUSE_CITY:
    pcity = game_city_by_number(pclause->value);
    if (pcity) {
      fc_snprintf(buf, bufsiz, _("The %s give %s"),
                  nation_plural_for_player(pclause->from),
                  city_name_get(pcity));
    } else {
      fc_snprintf(buf, bufsiz, _("The %s give an unknown city"),
                  nation_plural_for_player(pclause->from));
    }
    break;
  case CLAUSE_GOLD:
    fc_snprintf(
        buf, bufsiz,
        PL_("The %s give %d gold", "The %s give %d gold", pclause->value),
        nation_plural_for_player(pclause->from), pclause->value);
    break;
  case CLAUSE_MAP:
    fc_snprintf(buf, bufsiz, _("The %s give their worldmap"),
                nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_SEAMAP:
    fc_snprintf(buf, bufsiz, _("The %s give their seamap"),
                nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_CEASEFIRE:
    fc_snprintf(buf, bufsiz, _("The parties agree on a cease-fire"));
    break;
  case CLAUSE_PEACE:
    fc_snprintf(buf, bufsiz, _("The parties agree on a peace"));
    break;
  case CLAUSE_ALLIANCE:
    fc_snprintf(buf, bufsiz, _("The parties create an alliance"));
    break;
  case CLAUSE_VISION:
    fc_snprintf(buf, bufsiz, _("The %s give shared vision"),
                nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_EMBASSY:
    fc_snprintf(buf, bufsiz, _("The %s give an embassy"),
                nation_plural_for_player(pclause->from));
    break;
  default:
    fc_assert(false);
    if (bufsiz > 0) {
      *buf = '\0';
    }
    break;
  }
}

/**
   Return global catastrophe chance and rate of change, scaled to some
   maximum (e.g. 100 gives percentages).
   This mirrors the logic in update_environmental_upset().
 */
static void catastrophe_scaled(int *chance, int *rate, int max, int current,
                               int accum, int level)
{
  // 20 from factor in update_environmental_upset()
  int numer = 20 * max;
  int denom = map_num_tiles();

  if (chance) {
    *chance =
        CLIP(0, (int) ((((long) accum * numer) + (denom - 1)) / denom), max);
  }
  if (rate) {
    *rate = DIVIDE(((long) (current - level) * numer) + (denom - 1), denom);
  }
}

/**
   Return global warming chance and rate of change, scaled to max.
 */
void global_warming_scaled(int *chance, int *rate, int max)
{
  return catastrophe_scaled(chance, rate, max, game.info.heating,
                            game.info.globalwarming, game.info.warminglevel);
}

/**
   Return nuclear winter chance and rate of change, scaled to max.
 */
void nuclear_winter_scaled(int *chance, int *rate, int max)
{
  return catastrophe_scaled(chance, rate, max, game.info.cooling,
                            game.info.nuclearwinter, game.info.coolinglevel);
}

/**
   Return the sprite for the research indicator.
 */
const QPixmap *client_research_sprite()
{
  if (nullptr != client.conn.playing && can_client_change_view()) {
    const struct research *presearch = research_get(client_player());
    int idx = 0;

    if (A_UNSET != presearch->researching) {
      idx = (NUM_TILES_PROGRESS * presearch->bulbs_researched
             / (presearch->client.researching_cost + 1));
    }

    /* This clipping can be necessary since we can end up with excess
     * research */
    idx = CLIP(0, idx, NUM_TILES_PROGRESS - 1);
    return get_indicator_sprite(tileset, INDICATOR_BULB, idx);
  } else {
    return get_indicator_sprite(tileset, INDICATOR_BULB, 0);
  }
}

/**
   Return the sprite for the global-warming indicator.
 */
const QPixmap *client_warming_sprite()
{
  int idx;

  if (can_client_change_view()) {
    // Highest sprite kicks in at about 25% risk
    global_warming_scaled(&idx, nullptr, (NUM_TILES_PROGRESS - 1) * 4);
    idx = CLIP(0, idx, NUM_TILES_PROGRESS - 1);
  } else {
    idx = 0;
  }
  return get_indicator_sprite(tileset, INDICATOR_WARMING, idx);
}

/**
   Return the sprite for the global-cooling indicator.
 */
const QPixmap *client_cooling_sprite()
{
  int idx;

  if (can_client_change_view()) {
    // Highest sprite kicks in at about 25% risk
    nuclear_winter_scaled(&idx, nullptr, (NUM_TILES_PROGRESS - 1) * 4);
    idx = CLIP(0, idx, NUM_TILES_PROGRESS - 1);
  } else {
    idx = 0;
  }
  return get_indicator_sprite(tileset, INDICATOR_COOLING, idx);
}

/**
   Return the sprite for the government indicator.
 */
const QPixmap *client_government_sprite()
{
  if (nullptr != client.conn.playing && can_client_change_view()
      && government_count() > 0) {
    struct government *gov = government_of_player(client.conn.playing);

    return get_government_sprite(tileset, gov);
  } else {
    /* HACK: the UNHAPPY citizen is used for the government
     * when we don't know any better. */
    return get_citizen_sprite(tileset, CITIZEN_UNHAPPY, 0, nullptr);
  }
}

/**
   Find something sensible to display. This is used to overwrite the
   intro gfx.
 */
void center_on_something()
{
  struct city *pcity;
  struct unit *punit;

  if (!can_client_change_view()) {
    return;
  }

  if (get_num_units_in_focus() > 0) {
    queen()->mapview_wdg->center_on_tile(unit_tile(head_of_units_in_focus()),
                                         false);
  } else if (client_has_player()
             && nullptr
                    != (pcity = player_primary_capital(client_player()))) {
    // Else focus on the capital.
    queen()->mapview_wdg->center_on_tile(pcity->tile, false);
  } else if (nullptr != client.conn.playing
             && 0 < city_list_size(client.conn.playing->cities)) {
    // Just focus on any city.
    pcity = city_list_get(client.conn.playing->cities, 0);
    fc_assert_ret(pcity != nullptr);
    queen()->mapview_wdg->center_on_tile(pcity->tile, false);
  } else if (nullptr != client.conn.playing
             && 0 < unit_list_size(client.conn.playing->units)) {
    // Just focus on any unit.
    punit = unit_list_get(client.conn.playing->units, 0);
    fc_assert_ret(punit != nullptr);
    queen()->mapview_wdg->center_on_tile(unit_tile(punit), false);
  } else {
    struct tile *ctile =
        native_pos_to_tile(&(wld.map), wld.map.xsize / 2, wld.map.ysize / 2);

    // Just any known tile will do; search near the middle first.
    /* Iterate outward from the center tile.  We have to give a radius that
     * is guaranteed to be larger than the map will be.  Although this is
     * a misuse of map.xsize and map.ysize (which are native dimensions),
     * it should give a sufficiently large radius. */
    iterate_outward(&(wld.map), ctile, wld.map.xsize + wld.map.ysize, ptile)
    {
      if (client_tile_get_known(ptile) != TILE_UNKNOWN) {
        ctile = ptile;
        break;
      }
    }
    iterate_outward_end;

    queen()->mapview_wdg->center_on_tile(ctile, false);
  }
}

/**
   Encode a CID for the target production.
 */
cid cid_encode(struct universal target)
{
  return VUT_UTYPE == target.kind
             ? B_LAST + utype_number(target.value.utype)
             : improvement_number(target.value.building);
}

/**
   Encode a CID for the target unit type.
 */
cid cid_encode_unit(const struct unit_type *punittype)
{
  struct universal target = {.value = {.utype = punittype},
                             .kind = VUT_UTYPE};

  return cid_encode(target);
}

/**
   Encode a CID for the target building.
 */
cid cid_encode_building(const struct impr_type *pimprove)
{
  struct universal target = {.value = {.building = pimprove},
                             .kind = VUT_IMPROVEMENT};

  return cid_encode(target);
}

/**
   Decode the CID into a city_production structure.
 */
struct universal cid_decode(cid id)
{
  struct universal target;

  if (id >= B_LAST) {
    target.kind = VUT_UTYPE;
    target.value.utype = utype_by_number(id - B_LAST);
  } else {
    target.kind = VUT_IMPROVEMENT;
    target.value.building = improvement_by_number(id);
  }

  return target;
}

/**
   Return TRUE if the city supports at least one unit of the given
   production type (returns FALSE if the production is a building).
 */
bool city_unit_supported(const struct city *pcity,
                         const struct universal *target)
{
  if (VUT_UTYPE == target->kind) {
    const struct unit_type *tvtype = target->value.utype;

    unit_list_iterate(pcity->units_supported, punit)
    {
      if (unit_type_get(punit) == tvtype) {
        return true;
      }
    }
    unit_list_iterate_end;
  }
  return false;
}

/**
   Return TRUE if the city has present at least one unit of the given
   production type (returns FALSE if the production is a building).
 */
bool city_unit_present(const struct city *pcity,
                       const struct universal *target)
{
  if (VUT_UTYPE == target->kind) {
    const struct unit_type *tvtype = target->value.utype;

    unit_list_iterate(pcity->tile->units, punit)
    {
      if (unit_type_get(punit) == tvtype) {
        return true;
      }
    }
    unit_list_iterate_end;
  }
  return false;
}

/**
   A TestCityFunc to tell whether the item is a building and is present.
 */
bool city_building_present(const struct city *pcity,
                           const struct universal *target)
{
  return VUT_IMPROVEMENT == target->kind
         && city_has_building(pcity, target->value.building);
}

/**
   Return the numerical "section" of an item.  This is used for sorting.
 */
static int target_get_section(struct universal target)
{
  if (VUT_UTYPE == target.kind) {
    if (utype_has_flag(target.value.utype, UTYF_CIVILIAN)) {
      return 2;
    } else {
      return 3;
    }
  } else {
    if (improvement_has_flag(target.value.building, IF_GOLD)) {
      return 1;
    } else if (is_small_wonder(target.value.building)) {
      return 4;
    } else if (is_great_wonder(target.value.building)) {
      return 5;
    } else {
      return 0;
    }
  }
}

/**
   Helper for name_and_sort_items().
 */
static int fc_cmp(const void *p1, const void *p2)
{
  const struct item *i1 = static_cast<const item *>(p1),
                    *i2 = static_cast<const item *>(p2);
  int s1 = target_get_section(i1->item);
  int s2 = target_get_section(i2->item);

  if (s1 == s2) {
    return fc_strcasecmp(i1->descr, i2->descr);
  }
  return s1 - s2;
}

/**
   Takes an array of compound ids (cids). It will fill out an array of
   struct items and also sort it.

   section 0: normal buildings
   section 1: Capitalization
   section 2: UTYF_CIVILIAN units
   section 3: other units
   section 4: small wonders
   section 5: great wonders
 */
void name_and_sort_items(struct universal *targets, int num_targets,
                         struct item *items, bool show_cost,
                         struct city *pcity)
{
  int i;

  for (i = 0; i < num_targets; i++) {
    struct universal target = targets[i];
    int cost;
    struct item *pitem = &items[i];
    const char *name;

    pitem->item = target;

    if (VUT_UTYPE == target.kind) {
      name = utype_values_translation(target.value.utype);
      cost = utype_build_shield_cost(pcity, target.value.utype);
    } else {
      name = city_improvement_name_translation(pcity, target.value.building);
      if (improvement_has_flag(target.value.building, IF_GOLD)) {
        cost = -1;
      } else {
        if (pcity != nullptr) {
          cost = impr_build_shield_cost(pcity, target.value.building);
        } else {
          cost = MAX(target.value.building->build_cost * game.info.shieldbox
                         / 100,
                     1);
        }
      }
    }

    if (show_cost) {
      if (cost < 0) {
        fc_snprintf(pitem->descr, sizeof(pitem->descr), "%s (XX)", name);
      } else {
        fc_snprintf(pitem->descr, sizeof(pitem->descr), "%s (%d)", name,
                    cost);
      }
    } else {
      (void) fc_strlcpy(pitem->descr, name, sizeof(pitem->descr));
    }
  }

  qsort(items, num_targets, sizeof(struct item), fc_cmp);
}

/**
   Return possible production targets for the current player's cities.

   FIXME: this should probably take a pplayer argument.
 */
int collect_production_targets(struct universal *targets,
                               struct city **selected_cities,
                               int num_selected_cities, bool append_units,
                               bool append_wonders, bool change_prod,
                               TestCityFunc test_func)
{
  cid first = append_units ? B_LAST : 0;
  cid last = (append_units ? utype_count() + B_LAST : improvement_count());
  cid id;
  int items_used = 0;

  for (id = first; id < last; id++) {
    bool append = false;
    struct universal target = cid_decode(id);

    if (!append_units
        && (append_wonders != is_wonder(target.value.building))) {
      continue;
    }

    if (!change_prod) {
      if (client_has_player()) {
        city_list_iterate(client_player()->cities, pcity)
        {
          append |= test_func(pcity, &target);
        }
        city_list_iterate_end;
      } else {
        cities_iterate(pcity) { append |= test_func(pcity, &target); }
        cities_iterate_end;
      }
    } else {
      int i;

      for (i = 0; i < num_selected_cities; i++) {
        append |= test_func(selected_cities[i], &target);
      }
    }

    if (!append) {
      continue;
    }

    targets[items_used] = target;
    items_used++;
  }
  return items_used;
}

/**
   Collect the cids of all targets which can be build by this city or
   in general.
 */
int collect_eventually_buildable_targets(struct universal *targets,
                                         struct city *pcity,
                                         bool advanced_tech)
{
  struct player *pplayer = client_player();
  int cids_used = 0;

  improvement_iterate(pimprove)
  {
    bool can_build;
    bool can_eventually_build;

    if (nullptr != pcity) {
      // Can the city build?
      can_build = can_city_build_improvement_now(pcity, pimprove);
      can_eventually_build =
          can_city_build_improvement_later(pcity, pimprove);
    } else if (nullptr != pplayer) {
      // Can our player build?
      can_build = can_player_build_improvement_now(pplayer, pimprove);
      can_eventually_build =
          can_player_build_improvement_later(pplayer, pimprove);
    } else {
      // Global observer case: can any player build?
      can_build = false;
      players_iterate(aplayer)
      {
        if (can_player_build_improvement_now(aplayer, pimprove)) {
          can_build = true;
          break;
        }
      }
      players_iterate_end;

      can_eventually_build = false;
      players_iterate(aplayer)
      {
        if (can_player_build_improvement_later(aplayer, pimprove)) {
          can_eventually_build = true;
          break;
        }
      }
      players_iterate_end;
    }

    if ((advanced_tech && can_eventually_build)
        || (!advanced_tech && can_build)) {
      targets[cids_used].kind = VUT_IMPROVEMENT;
      targets[cids_used].value.building = pimprove;
      cids_used++;
    }
  }
  improvement_iterate_end;

  unit_type_iterate(punittype)
  {
    bool can_build;
    bool can_eventually_build;

    if (nullptr != pcity) {
      // Can the city build?
      can_build = can_city_build_unit_now(pcity, punittype);
      can_eventually_build = can_city_build_unit_later(pcity, punittype);
    } else if (nullptr != pplayer) {
      // Can our player build?
      can_build = can_player_build_unit_now(pplayer, punittype);
      can_eventually_build = can_player_build_unit_later(pplayer, punittype);
    } else {
      // Global observer case: can any player build?
      can_build = false;
      players_iterate(aplayer)
      {
        if (can_player_build_unit_now(aplayer, punittype)) {
          can_build = true;
          break;
        }
      }
      players_iterate_end;

      can_eventually_build = false;
      players_iterate(aplayer)
      {
        if (can_player_build_unit_later(aplayer, punittype)) {
          can_eventually_build = true;
          break;
        }
      }
      players_iterate_end;
    }

    if ((advanced_tech && can_eventually_build)
        || (!advanced_tech && can_build)) {
      targets[cids_used].kind = VUT_UTYPE;
      targets[cids_used].value.utype = punittype;
      cids_used++;
    }
  }
  unit_type_iterate_end;

  return cids_used;
}

/**
   Collect the cids of all improvements which are built in the given city.
 */
int collect_already_built_targets(struct universal *targets,
                                  struct city *pcity)
{
  int cids_used = 0;

  fc_assert_ret_val(pcity != nullptr, 0);

  city_built_iterate(pcity, pimprove)
  {
    targets[cids_used].kind = VUT_IMPROVEMENT;
    targets[cids_used].value.building = pimprove;
    cids_used++;
  }
  city_built_iterate_end;

  return cids_used;
}

/**
   Handles a chat or event message.
 */
void handle_event(const char *featured_text, struct tile *ptile,
                  enum event_type event, int turn, int phase, int conn_id)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags;
  int where = MW_OUTPUT;        // where to display the message
  bool fallback_needed = false; /* we want fallback if actual 'where' is not
                                 * usable */
  bool shown = false;           // Message displayed somewhere at least

  if (!event_type_is_valid(event)) {
    // Server may have added a new event; leave as MW_OUTPUT
    qDebug("Unknown event type %d!", event);
  } else {
    where = messages_where[event];
  }

  // Get the original text.
  featured_text_to_plain_text(featured_text, plain_text, sizeof(plain_text),
                              &tags, conn_id != -1);

  // Display link marks when an user is pointed us something.
  if (conn_id != -1) {
    text_tag_list_iterate(tags, ptag)
    {
      if (text_tag_type(ptag) == TTT_LINK) {
        link_mark_add_new(text_tag_link_type(ptag), text_tag_link_id(ptag));
      }
    }
    text_tag_list_iterate_end;
  }

  /* Maybe highlight our player and user names if someone is talking
   * about us. */
  if (-1 != conn_id && client.conn.id != conn_id
      && ft_color_requested(gui_options->highlight_our_names)) {
    const char *username = client.conn.username;
    size_t userlen = qstrlen(username);
    const char *playername = ((client_player() && !client_is_observer())
                                  ? player_name(client_player())
                                  : nullptr);
    size_t playerlen = playername ? qstrlen(playername) : 0;
    const char *p;

    if (playername && playername[0] == '\0') {
      playername = nullptr;
    }

    if (username && username[0] == '\0') {
      username = nullptr;
    }

    for (p = plain_text; *p != '\0'; p++) {
      if (nullptr != username && 0 == fc_strncasecmp(p, username, userlen)) {
        struct text_tag *ptag =
            text_tag_new(TTT_COLOR, p - plain_text, p - plain_text + userlen,
                         gui_options->highlight_our_names);

        fc_assert(ptag != nullptr);

        if (ptag != nullptr) {
          // Appends to be sure it will be applied at last.
          text_tag_list_append(tags, ptag);
        }
      } else if (nullptr != playername
                 && 0 == fc_strncasecmp(p, playername, playerlen)) {
        struct text_tag *ptag = text_tag_new(
            TTT_COLOR, p - plain_text, p - plain_text + playerlen,
            gui_options->highlight_our_names);

        fc_assert(ptag != nullptr);

        if (ptag != nullptr) {
          // Appends to be sure it will be applied at last.
          text_tag_list_append(tags, ptag);
        }
      }
    }
  }

  // Popup
  if (BOOL_VAL(where & MW_POPUP)) {
    /* Popups are usually not shown if player is under AI control.
     * Server operator messages are shown always. */
    if (nullptr == client.conn.playing || is_human(client.conn.playing)
        || event == E_MESSAGE_WALL) {
      popup_notify_goto_dialog(_("Message"), plain_text, tags, ptile);
      shown = true;
    } else {
      /* Force to chatline so it will be visible somewhere at least.
       * Messages window may still handle this so chatline is not needed
       * after all. */
      fallback_needed = true;
    }
  }

  // Message window
  if (BOOL_VAL(where & MW_MESSAGES)) {
    // When the game isn't running, the messages dialog isn't present.
    if (C_S_RUNNING <= client_state()) {
      meswin_add(plain_text, tags, ptile, event, turn, phase);
      shown = true;
    } else {
      // Force to chatline instead.
      fallback_needed = true;
    }
  }

  // Chatline
  if (BOOL_VAL(where & MW_OUTPUT) || (fallback_needed && !shown)) {
    output_window_event(plain_text, tags);
  }

  if (turn == game.info.turn) {
    play_sound_for_event(event);
  }

  // Free tags
  text_tag_list_destroy(tags);
}

/**
   Creates a struct packet_generic_message packet and injects it via
   handle_chat_msg.
 */
void create_event(struct tile *ptile, enum event_type event,
                  const struct ft_color color, const char *format, ...)
{
  va_list ap;
  char message[MAX_LEN_MSG];

  va_start(ap, format);
  fc_vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);

  if (ft_color_requested(color)) {
    char colored_text[MAX_LEN_MSG];

    featured_text_apply_tag(message, colored_text, sizeof(colored_text),
                            TTT_COLOR, 0, FT_OFFSET_UNSET, color);
    handle_event(colored_text, ptile, event, game.info.turn, game.info.phase,
                 -1);
  } else {
    handle_event(message, ptile, event, game.info.turn, game.info.phase, -1);
  }
}

/**
   Find city nearest to given unit and optionally return squared city
   distance Parameter sq_dist may be nullptr. Returns nullptr only if no city
   is known. Favors punit owner's cities over other cities if equally
   distant.
 */
struct city *get_nearest_city(const struct unit *punit, int *sq_dist)
{
  struct city *pcity_near = tile_city(unit_tile(punit));
  int pcity_near_dist;

  if (pcity_near) {
    pcity_near_dist = 0;
  } else {
    pcity_near = nullptr;
    pcity_near_dist = -1;
    players_iterate(pplayer)
    {
      city_list_iterate(pplayer->cities, pcity_current)
      {
        int dist = sq_map_distance(pcity_current->tile, unit_tile(punit));
        if (pcity_near_dist == -1 || dist < pcity_near_dist
            || (dist == pcity_near_dist
                && unit_owner(punit) == city_owner(pcity_current))) {
          pcity_near = pcity_current;
          pcity_near_dist = dist;
        }
      }
      city_list_iterate_end;
    }
    players_iterate_end;
  }

  if (sq_dist) {
    *sq_dist = pcity_near_dist;
  }

  return pcity_near;
}

/**
   Called when the "Buy" button is pressed in the city report for every
   selected city. Checks for coinage and sufficient funds or request the
   purchase if everything is ok.
 */
void cityrep_buy(struct city *pcity)
{
  int value;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    create_event(
        pcity->tile, E_BAD_COMMAND, ftc_client, _("You can't buy %s in %s!"),
        improvement_name_translation(pcity->production.value.building),
        city_link(pcity));
    return;
  }
  value = pcity->client.buy_cost;

  if (city_owner(pcity)->economic.gold >= value) {
    city_buy_production(pcity);
  } else {
    // Split into two to allow localization of two pluralisations.
    char buf[MAX_LEN_MSG];
    /* TRANS: %s is a production type; this whole string is a sentence
     * fragment that is only ever included in one other string
     * (search comments for this string to find it) */
    fc_snprintf(buf, ARRAY_SIZE(buf),
                PL_("%s costs %d gold", "%s costs %d gold", value),
                city_production_name_translation(pcity), value);
    create_event(nullptr, E_BAD_COMMAND, ftc_client,
                 /* TRANS: %s is a pre-pluralised sentence fragment:
                  * "%s costs %d gold" */
                 PL_("%s and you only have %d gold.",
                     "%s and you only have %d gold.",
                     city_owner(pcity)->economic.gold),
                 buf, city_owner(pcity)->economic.gold);
  }
}

/**
   Returns TRUE if any of the units can do the connect activity.
 */
bool can_units_do_connect(const std::vector<unit *> &units,
                          enum unit_activity activity,
                          struct extra_type *tgt)
{
  return std::any_of(units.begin(), units.end(), [&](const auto unit) {
    return can_unit_do_connect(unit, activity, tgt);
  });
}

/**
   Initialize the action probability cache. Shouldn't be kept around
   permanently. Its data is quickly outdated.
 */
void client_unit_init_act_prob_cache(struct unit *punit)
{
  // A double init would cause a leak.
  fc_assert_ret(punit->client.act_prob_cache == nullptr);

  punit->client.act_prob_cache = new act_prob[NUM_ACTIONS]();
}

/**
   Set focus status of all player units to FOCUS_AVAIL.
 */
void unit_focus_set_status(struct player *pplayer)
{
  unit_list_iterate(pplayer->units, punit)
  {
    punit->client.focus_status = FOCUS_AVAIL;
  }
  unit_list_iterate_end;
}

/**
   Initialize a player on the client side.
 */
void client_player_init(struct player *pplayer)
{
  vision_layer_iterate(v)
  {
    pplayer->client.tile_vision[v] = new QBitArray();
  }
  vision_layer_iterate_end;
}

/**
   Reset the private maps of all players.
 */
void client_player_maps_reset()
{
  players_iterate(pplayer)
  {
    int new_size;

    if (pplayer == client.conn.playing) {
      new_size = MAP_INDEX_SIZE;
    } else {
      /* We don't need (or have) information about players other
       * than user of the client. Allocate just one bit as that's
       * the minimum bitvector size (cannot allocate 0 bits)*/
      new_size = 1;
    }

    vision_layer_iterate(v)
    {
      pplayer->client.tile_vision[v]->resize(new_size);
    }
    vision_layer_iterate_end;

    pplayer->tile_known->resize(new_size);
  }
  players_iterate_end;
}

/**
   Create a map image definition on the client.
 */
bool mapimg_client_define()
{
  char str[MAX_LEN_MAPDEF];
  char mi_map[MAPIMG_LAYER_COUNT + 1];
  enum mapimg_layer layer;
  int map_pos = 0;

  // Only one definition allowed.
  while (mapimg_count() != 0) {
    mapimg_delete(0);
  }

  // Map image definition: zoom, turns
  fc_snprintf(str, sizeof(str), "zoom=%d:turns=0:format=%s",
              gui_options->mapimg_zoom, gui_options->mapimg_format);

  // Map image definition: show
  if (client_is_global_observer()) {
    cat_snprintf(str, sizeof(str), ":show=all");
    // use all available knowledge
    gui_options->mapimg_layer[MAPIMG_LAYER_KNOWLEDGE] = false;
  } else {
    cat_snprintf(str, sizeof(str), ":show=plrid:plrid=%d",
                 player_index(client.conn.playing));
    // use only player knowledge
    gui_options->mapimg_layer[MAPIMG_LAYER_KNOWLEDGE] = true;
  }

  // Map image definition: map
  for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
       layer = mapimg_layer_next(layer)) {
    if (gui_options->mapimg_layer[layer]) {
      mi_map[map_pos++] = mapimg_layer_name(layer)[0];
    }
  }
  mi_map[map_pos] = '\0';

  if (map_pos == 0) {
    // no value set - use dummy setting
    sz_strlcpy(mi_map, "-");
  }
  cat_snprintf(str, sizeof(str), ":map=%s", mi_map);

  log_debug("client map image definition: %s", str);

  if (!mapimg_define(str, false) || !mapimg_isvalid(0)) {
    /* An error in the definition string or an error validation the string.
     * The error message is available via mapimg_error(). */
    return false;
  }

  return true;
}

/**
   Returns the nation set in use.
 */
struct nation_set *client_current_nation_set()
{
  if (client_state() < C_S_RUNNING) {
    return nullptr;
  }

  struct option *poption = optset_option_by_name(server_optset, "nationset");
  const char *setting_str;

  if (poption == nullptr || option_type(poption) != OT_STRING
      || (setting_str = option_str_get(poption)) == nullptr) {
    setting_str = "";
  }
  return nation_set_by_setting_value(setting_str);
}

/**
   Returns the current AI skill level on the server, if the same level is
   currently used for all current AI players and will be for new ones;
   else return ai_level_invalid() to indicate inconsistency.
 */
enum ai_level server_ai_level()
{
  enum ai_level lvl = static_cast<ai_level>(game.info.skill_level);

  players_iterate(pplayer)
  {
    if (is_ai(pplayer) && pplayer->ai_common.skill_level != lvl) {
      return ai_level_invalid();
    }
  }
  players_iterate_end;

  if (!is_settable_ai_level(lvl)) {
    return ai_level_invalid();
  }

  return lvl;
}
