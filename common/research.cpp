/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/
// utility
#include "fcintl.h"
#include "iterator.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "fc_types.h"
#include "game.h"
#include "name_translation.h"
#include "nation.h"
#include "player.h"
#include "team.h"
#include "tech.h"

#include "research.h"
// FIXME: Dont use extern, move inside some game running class eventually
extern std::vector<research> research_array(MAX_NUM_PLAYER_SLOTS);

struct research_player_iter {
  struct iterator vtable;
  union {
    struct player *pplayer;
    struct player_list_link *plink;
  };
};
#define RESEARCH_PLAYER_ITER(p) ((struct research_player_iter *) p)

static struct name_translation advance_unset_name = NAME_INIT;
static struct name_translation advance_future_name = NAME_INIT;
static struct name_translation advance_unknown_name = NAME_INIT;

Q_GLOBAL_STATIC(QVector<QString>, future_rule_name)
Q_GLOBAL_STATIC(QVector<QString>, future_name_translation);

/**
   Initializes all player research structure.
 */
void researches_init()
{
  // Ensure we have enough space for players or teams.
  fc_assert(research_array.size() >= team_slot_count());
  fc_assert(research_array.size() >= MAX_NUM_PLAYER_SLOTS);

  for (auto &research : research_array) {
    research.tech_goal = A_UNSET;
    research.researching = A_UNSET;
    research.researching_saved = A_UNKNOWN;
    research.future_tech = 0;
    research.inventions[A_NONE].state = TECH_KNOWN;
    advance_index_iterate(A_FIRST, j)
    {
      research.inventions[j].bulbs_researched_saved = 0;
    }
    advance_index_iterate_end;
  }

  game.info.global_advances[A_NONE] = true;

  // Set technology names.
  // TRANS: "None" tech
  name_set(&advance_unset_name, nullptr, N_("?tech:None"));
  name_set(&advance_future_name, nullptr, N_("Future Tech."));
  /* TRANS: "Unknown" advance/technology */
  name_set(&advance_unknown_name, nullptr, N_("(Unknown)"));
}

/**
   Free all resources allocated for the research system
 */
void researches_free()
{
  future_rule_name->clear();
  future_name_translation->clear();
}

/**
   Returns the index of the research in the array.
 */
int research_number(const research *presearch)
{
  fc_assert_ret_val(nullptr != presearch, 0);
  return presearch - &research_array[0];
}

/**
   Returns the research for the given index.
 */
struct research *research_by_number(int number)
{
  fc_assert_ret_val(0 <= number, nullptr);
  fc_assert_ret_val(research_array.size() > number, nullptr);
  return &research_array[number];
}

/**
   Returns the research structure associated with the player.
 */
struct research *research_get(const struct player *pplayer)
{
  if (nullptr == pplayer) {
    // Special case used at client side.
    return nullptr;
  } else if (game.info.team_pooled_research) {
    return &research_array[team_number(pplayer->team)];
  } else {
    return &research_array[player_number(pplayer)];
  }
}

/**
   Returns the name of the research owner: a player name or a team name.
 */
const char *research_rule_name(const struct research *presearch)
{
  if (game.info.team_pooled_research) {
    return team_rule_name(team_by_number(research_number(presearch)));
  } else {
    return player_name(player_by_number(research_number(presearch)));
  }
}

/**
   Returns the name of the research owner: a player name or a team name.
   For most uses you probably want research_pretty_name() instead.
 */
const char *research_name_translation(const struct research *presearch)
{
  if (game.info.team_pooled_research) {
    return team_name_translation(team_by_number(research_number(presearch)));
  } else {
    return player_name(player_by_number(research_number(presearch)));
  }
}

/**
   Set in 'buf' the name of the research owner. It may be either a nation
   plural name, or something like "members of team Red".
 */
int research_pretty_name(const struct research *presearch, char *buf,
                         size_t buf_len)
{
  const struct player *pplayer;

  if (game.info.team_pooled_research) {
    const struct team *pteam = team_by_number(research_number(presearch));

    if (1 != player_list_size(team_members(pteam))) {
      QString buf2;

      team_pretty_name(pteam, buf2);
      /* TRANS: e.g. "members of team 1", or even "members of team Red".
       * Used in many places where a nation plural might be used. */
      return fc_snprintf(buf, buf_len, _("members of %s"),
                         qUtf8Printable(buf2));
    } else {
      pplayer = player_list_front(team_members(pteam));
    }
  } else {
    pplayer = player_by_number(research_number(presearch));
  }

  return fc_strlcpy(buf, nation_plural_for_player(pplayer), buf_len);
}

/**
   Return the name translation for 'tech'. Utility for
   research_advance_rule_name() and research_advance_translated_name().
 */
static inline const struct name_translation *
research_advance_name(Tech_type_id tech)
{
  if (A_UNSET == tech) {
    return &advance_unset_name;
  } else if (A_FUTURE == tech) {
    return &advance_future_name;
  } else if (A_UNKNOWN == tech) {
    return &advance_unknown_name;
  } else {
    const struct advance *padvance = advance_by_number(tech);

    fc_assert_ret_val(nullptr != padvance, nullptr);
    return &padvance->name;
  }
}

/**
   Set a new future tech name in the string vector, and return the string
   duplicate stored inside the vector.
 */
static const char *research_future_set_name(QVector<QString> *psv, int no,
                                            const char *new_name)
{
  if (psv->count() <= no) {
    // Increase the size of the vector if needed.
    psv->resize(no + 1);
  }

  // Set in vector.
  psv->replace(no, new_name);

  // Return duplicate of 'new_name'.
  return qstrdup(qUtf8Printable(psv->at(no)));
}

/**
   Store the rule name of the given tech (including A_FUTURE) in 'buf'.
   'presearch' may be nullptr.
 */
QString research_advance_rule_name(const struct research *presearch,
                                   Tech_type_id tech)
{
  if (A_FUTURE == tech && nullptr != presearch) {
    const int no = presearch->future_tech;

    if (no >= future_rule_name->size()) {
      char buffer[256];

      // NB: 'presearch->future_tech == 0' means "Future Tech. 1".
      fc_snprintf(buffer, sizeof(buffer), "%s %d",
                  rule_name_get(&advance_future_name), no + 1);
      return research_future_set_name(future_rule_name, no, buffer);
    }
    return future_rule_name->at(no);
  }

  return rule_name_get(research_advance_name(tech));
}

/**
   Store the translated name of the given tech (including A_FUTURE) in 'buf'.
   'presearch' may be nullptr.
 */
QString research_advance_name_translation(const struct research *presearch,
                                          Tech_type_id tech)
{
  if (A_FUTURE == tech && nullptr != presearch) {
    const int no = presearch->future_tech;
    QString name;

    if (no < future_name_translation->count()) {
      // FIXME remove check to read outside vector
      name = future_name_translation->at(no);
    }
    if (name.isEmpty()) {
      char buffer[256];

      // NB: 'presearch->future_tech == 0' means "Future Tech. 1".
      fc_snprintf(buffer, sizeof(buffer), _("Future Tech. %d"), no + 1);
      name = research_future_set_name(future_name_translation, no, buffer);
    }

    fc_assert(!name.isEmpty());

    return name;
  }

  return name_translation_get(research_advance_name(tech));
}

/**
   Returns TRUE iff the requirement vector may become active against the
   given target.

   If may become active if all unchangeable requirements are active.
 */
static bool reqs_may_activate(const struct player *target_player,
                              const struct player *other_player,
                              const struct city *target_city,
                              const struct impr_type *target_building,
                              const struct tile *target_tile,
                              const struct unit *target_unit,
                              const struct unit_type *target_unittype,
                              const struct output_type *target_output,
                              const struct specialist *target_specialist,
                              const struct action *target_action,
                              const struct requirement_vector *reqs,
                              const enum req_problem_type prob_type,
                              const enum vision_layer vision_layer)
{
  requirement_vector_iterate(reqs, preq)
  {
    if (is_req_unchanging(preq)
        && !is_req_active(target_player, other_player, target_city,
                          target_building, target_tile, target_unit,
                          target_unittype, target_output, target_specialist,
                          target_action, preq, prob_type, vision_layer)) {
      return false;
    }
  }
  requirement_vector_iterate_end;
  return true;
}

/**
   Evaluates the legality of starting to research this tech according to
   reqs_eval() and the tech's research_reqs. Returns TRUE iff legal.

   The reqs_eval() argument evaluates the requirements. One variant may
   check the current situation while another may check potential future
   situations.

   Helper for research_update().
 */
static bool research_allowed(
    const struct research *presearch, Tech_type_id tech,
    bool (*reqs_eval)(
        const struct player *tplr, const struct player *oplr,
        const struct city *tcity, const struct impr_type *tbld,
        const struct tile *ttile, const struct unit *tunit,
        const struct unit_type *tutype, const struct output_type *top,
        const struct specialist *tspe, const struct action *tact,
        const struct requirement_vector *reqs,
        const enum req_problem_type ptype, const enum vision_layer vlayer))
{
  struct advance *adv;

  adv = valid_advance_by_number(tech);

  if (adv == nullptr) {
    // Not a valid advance.
    return false;
  }

  research_players_iterate(presearch, pplayer)
  {
    if (reqs_eval(pplayer, nullptr, nullptr, nullptr, nullptr, nullptr,
                  nullptr, nullptr, nullptr, nullptr, &(adv->research_reqs),
                  RPT_CERTAIN, V_COUNT)) {
      /* It is enough that one player that shares research is allowed to
       * research it.
       * Reasoning: Imagine a tech with that requires a nation in the
       * player range. If the requirement applies to all players sharing
       * research it will be illegal in research sharing games. To require
       * that the player that fulfills the requirement must order it to be
       * researched creates unnecessary bureaucracy. */
      return true;
    }
  }
  research_players_iterate_end;

  return false;
}

/**
   Returns TRUE iff researching the given tech is allowed according to its
   research_reqs.

   Helper for research_update().
 */
#define research_is_allowed(presearch, tech)                                \
  research_allowed(presearch, tech, are_reqs_active)

/**
   Returns TRUE iff researching the given tech may become allowed according
   to its research_reqs.

   Helper for research_get_reachable_rreqs().
 */
#define research_may_become_allowed(presearch, tech)                        \
  research_allowed(presearch, tech, reqs_may_activate)

/**
   Returns TRUE iff the given tech is ever reachable by the players sharing
   the research as far as research_reqs are concerned.

   Helper for research_get_reachable().
 */
static bool research_get_reachable_rreqs(const struct research *presearch,
                                         Tech_type_id tech)
{
  bv_techs done;
  std::vector<Tech_type_id> techs{tech};
  BV_CLR_ALL(done);
  BV_SET(done, A_NONE);
  BV_SET(done, tech);
  /* Check that all recursive requirements have their research_reqs
   * in order. */
  while (!techs.empty()) { // Iterate till there is no-more techs to check
    auto iter_tech = techs[techs.size() - 1]; // Last Element
    techs.pop_back();                         // Remove Last element
    if (presearch->inventions[iter_tech].state == TECH_KNOWN) {
      /* This tech is already reached. What is required to research it and
       * the techs it depends on is therefore irrelevant. */
      continue;
    }

    if (!research_may_become_allowed(presearch, iter_tech)) {
      /* It will always be illegal to start researching this tech because
       * of unchanging requirements. Since it isn't already known and can't
       * be researched it must be unreachable. */
      return false;
    }

    // Check if required techs are research_reqs reachable.
    for (int req = 0; req < AR_SIZE; req++) {
      Tech_type_id req_tech = advance_required(iter_tech, tech_req(req));

      if (valid_advance_by_number(req_tech) == nullptr) {
        return false;
      } else if (!BV_ISSET(done, req_tech)) {
        techs.push_back(req_tech);
        BV_SET(done, req_tech);
      }
    }
  }

  return true;
}

/**
   Returns TRUE iff the given tech is ever reachable by the players sharing
   the research by checking tech tree limitations.

   Helper for research_update().
 */
static bool research_get_reachable(const struct research *presearch,
                                   Tech_type_id tech)
{
  if (valid_advance_by_number(tech) == nullptr) {
    return false;
  } else {
    advance_root_req_iterate(advance_by_number(tech), proot)
    {
      if (advance_requires(proot, AR_ROOT) == proot) {
        /* This tech requires itself; it can only be reached by special
         * means (init_techs, lua script, ...).
         * If you already know it, you can "reach" it; if not, not. (This
         * case is needed for descendants of this tech.) */
        if (presearch->inventions[advance_number(proot)].state
            != TECH_KNOWN) {
          return false;
        }
      } else {
        for (int req = 0; req < AR_SIZE; req++) {
          if (valid_advance(advance_requires(proot, tech_req(req)))
              == nullptr) {
            return false;
          }
        }
      }
    }
    advance_root_req_iterate_end;
  }

  // Check research reqs reachability.
  return research_get_reachable_rreqs(presearch, tech);
}

/**
   Returns TRUE iff the players sharing 'presearch' already have got the
   knowledge of all root requirement technologies for 'tech' (without which
   it's impossible to gain 'tech').

   Helper for research_update().
 */
static bool research_get_root_reqs_known(const struct research *presearch,
                                         Tech_type_id tech)
{
  advance_root_req_iterate(advance_by_number(tech), proot)
  {
    if (presearch->inventions[advance_number(proot)].state != TECH_KNOWN) {
      return false;
    }
  }
  advance_root_req_iterate_end;

  return true;
}

/**
   Mark as TECH_PREREQS_KNOWN each tech which is available, not known and
   which has all requirements fullfiled.

   Recalculate presearch->num_known_tech_with_flag
   Should always be called after research_invention_set().
 */
void research_update(struct research *presearch)
{
  int techs_researched;

  advance_index_iterate(A_FIRST, i)
  {
    enum tech_state state = presearch->inventions[i].state;
    bool root_reqs_known = true;
    bool reachable = research_get_reachable(presearch, i);

    /* Finding if the root reqs of an unreachable tech isn't redundant.
     * A tech can be unreachable via research but have known root reqs
     * because of unfilfilled research_reqs. Unfulfilled research_reqs
     * doesn't prevent the player from aquiring the tech by other means. */
    root_reqs_known = research_get_root_reqs_known(presearch, i);

    if (reachable) {
      if (state != TECH_KNOWN) {
        // Update state.
        state =
            (root_reqs_known
                     && (presearch->inventions[advance_required(i, AR_ONE)]
                             .state
                         == TECH_KNOWN)
                     && (presearch->inventions[advance_required(i, AR_TWO)]
                             .state
                         == TECH_KNOWN)
                     && research_is_allowed(presearch, i)
                 ? TECH_PREREQS_KNOWN
                 : TECH_UNKNOWN);
      }
    } else {
      fc_assert(state == TECH_UNKNOWN);
    }
    presearch->inventions[i].state = state;
    presearch->inventions[i].reachable = reachable;
    presearch->inventions[i].root_reqs_known = root_reqs_known;

    // Updates required_techs, num_required_techs and bulbs_required.
    BV_CLR_ALL(presearch->inventions[i].required_techs);
    presearch->inventions[i].num_required_techs = 0;
    presearch->inventions[i].bulbs_required = 0;

    if (!reachable || state == TECH_KNOWN) {
      continue;
    }

    techs_researched = presearch->techs_researched;
    advance_req_iterate(valid_advance_by_number(i), preq)
    {
      Tech_type_id j = advance_number(preq);

      if (TECH_KNOWN == research_invention_state(presearch, j)) {
        continue;
      }

      BV_SET(presearch->inventions[i].required_techs, j);
      presearch->inventions[i].num_required_techs++;
      presearch->inventions[i].bulbs_required +=
          research_total_bulbs_required(presearch, j, false);
      /* This is needed to get a correct result for the
       * research_total_bulbs_required() call when
       * game.info.game.info.tech_cost_style is TECH_COST_CIV1CIV2. */
      presearch->techs_researched++;
    }
    advance_req_iterate_end;
    presearch->techs_researched = techs_researched;
  }
  advance_index_iterate_end;

#ifdef FREECIV_DEBUG
  advance_index_iterate(A_FIRST, i)
  {
    QByteArray buf;
    buf.reserve(advance_count() + 1);

    advance_index_iterate(A_NONE, j)
    {
      if (BV_ISSET(presearch->inventions[i].required_techs, j)) {
        buf.insert(j, '1');
      } else {
        buf.insert(j, '0');
      }
    }
    advance_index_iterate_end;
    buf.insert(advance_count(), '\0');

    log_debug("%s: [%3d] %-25s => %s%s%s", research_rule_name(presearch), i,
              advance_rule_name(advance_by_number(i)),
              tech_state_name(research_invention_state(presearch, i)),
              presearch->inventions[i].reachable ? "" : " [unrechable]",
              presearch->inventions[i].root_reqs_known
                  ? ""
                  : " [root reqs aren't known]");
    log_debug("%s: [%3d] %s", research_rule_name(presearch), i,
              qUtf8Printable(buf));
  }
  advance_index_iterate_end;
#endif // FREECIV_DEBUG

  for (int flag = 0; flag <= tech_flag_id_max(); flag++) {
    // Iterate over all possible tech flags (0..max).
    presearch->num_known_tech_with_flag[flag] = 0;

    advance_index_iterate(A_NONE, i)
    {
      if (TECH_KNOWN == research_invention_state(presearch, i)
          && advance_has_flag(i, tech_flag_id(flag))) {
        presearch->num_known_tech_with_flag[flag]++;
      }
    }
    advance_index_iterate_end;
  }
}

/**
   Returns state of the tech for current research.
   This can be: TECH_KNOWN, TECH_UNKNOWN, or TECH_PREREQS_KNOWN
   Should be called with existing techs.

   If 'presearch' is nullptr this checks whether any player knows the tech
   (used by the client).
 */
enum tech_state research_invention_state(const struct research *presearch,
                                         Tech_type_id tech)
{
  fc_assert_ret_val(nullptr != valid_advance_by_number(tech),
                    tech_state(-1));

  if (nullptr != presearch) {
    return presearch->inventions[tech].state;
  } else if (game.info.global_advances[tech]) {
    return TECH_KNOWN;
  } else {
    return TECH_UNKNOWN;
  }
}

/**
   Set research knowledge about tech to given state.
 */
enum tech_state research_invention_set(struct research *presearch,
                                       Tech_type_id tech,
                                       enum tech_state value)
{
  enum tech_state old;

  fc_assert_ret_val(nullptr != valid_advance_by_number(tech),
                    tech_state(-1));

  old = presearch->inventions[tech].state;
  if (old == value) {
    return old;
  }
  presearch->inventions[tech].state = value;

  if (value == TECH_KNOWN) {
    if (!game.info.global_advances[tech]) {
      game.info.global_advances[tech] = true;
      game.info.global_advance_count++;
    }
  }

  return old;
}

/**
   Returns TRUE iff the given tech is ever reachable via research by the
   players sharing the research by checking tech tree limitations.

   'presearch' may be nullptr in which case a simplified result is returned
   (used by the client).
 */
bool research_invention_reachable(const struct research *presearch,
                                  const Tech_type_id tech)
{
  if (valid_advance_by_number(tech) == nullptr) {
    return false;
  } else if (presearch != nullptr) {
    return presearch->inventions[tech].reachable;
  } else {
    researches_iterate(research_iter)
    {
      if (research_iter->inventions[tech].reachable) {
        return true;
      }
    }
    researches_iterate_end;

    return false;
  }
}

/**
   Returns TRUE iff the given tech can be given to the players sharing the
   research immediately.

   If allow_holes is TRUE, any tech with known root reqs is ok. If it's
   FALSE, getting the tech must not leave holes to the known techs tree.
 */
bool research_invention_gettable(const struct research *presearch,
                                 const Tech_type_id tech, bool allow_holes)
{
  if (valid_advance_by_number(tech) == nullptr) {
    return false;
  } else if (presearch != nullptr) {
    return (allow_holes
                ? presearch->inventions[tech].root_reqs_known
                : presearch->inventions[tech].state == TECH_PREREQS_KNOWN);
  } else {
    researches_iterate(research_iter)
    {
      if (allow_holes ? research_iter->inventions[tech].root_reqs_known
                      : research_iter->inventions[tech].state
                            == TECH_PREREQS_KNOWN) {
        return true;
      }
    }
    researches_iterate_end;

    return false;
  }
}

/**
   Return the next tech we should research to advance towards our goal.
   Returns A_UNSET if nothing is available or the goal is already known.
 */
Tech_type_id research_goal_step(const struct research *presearch,
                                Tech_type_id goal)
{
  const struct advance *pgoal = valid_advance_by_number(goal);

  if (nullptr == pgoal || !research_invention_reachable(presearch, goal)) {
    return A_UNSET;
  }

  advance_req_iterate(pgoal, preq)
  {
    switch (research_invention_state(presearch, advance_number(preq))) {
    case TECH_PREREQS_KNOWN:
      return advance_number(preq);
    case TECH_KNOWN:
    case TECH_UNKNOWN:
      break;
    };
  }
  advance_req_iterate_end;
  return A_UNSET;
}

/**
   Returns the number of technologies the player need to research to get
   the goal technology. This includes the goal technology. Technologies
   are only counted once.

   'presearch' may be nullptr in which case it will returns the total number
   of technologies needed for reaching the goal.
 */
int research_goal_unknown_techs(const struct research *presearch,
                                Tech_type_id goal)
{
  const struct advance *pgoal = valid_advance_by_number(goal);

  if (nullptr == pgoal) {
    return 0;
  } else if (nullptr != presearch) {
    return presearch->inventions[goal].num_required_techs;
  } else {
    return pgoal->num_reqs;
  }
}

/**
   Function to determine cost (in bulbs) of reaching goal technology.
   These costs _include_ the cost for researching the goal technology
   itself.

   'presearch' may be nullptr in which case it will returns the total number
   of bulbs needed for reaching the goal.
 */
int research_goal_bulbs_required(const struct research *presearch,
                                 Tech_type_id goal)
{
  const struct advance *pgoal = valid_advance_by_number(goal);

  if (nullptr == pgoal) {
    return 0;
  } else if (nullptr != presearch) {
    return presearch->inventions[goal].bulbs_required;
  } else if (game.info.tech_cost_style == TECH_COST_CIV1CIV2) {
    return game.info.base_tech_cost * pgoal->num_reqs * (pgoal->num_reqs + 1)
           / 2;
  } else {
    int bulbs_required = 0;

    advance_req_iterate(pgoal, preq) { bulbs_required += preq->cost; }
    advance_req_iterate_end;
    return bulbs_required;
  }
}

/**
   Returns if the given tech has to be researched to reach the goal. The
   goal itself isn't a requirement of itself.

   'presearch' may be nullptr.
 */
bool research_goal_tech_req(const struct research *presearch,
                            Tech_type_id goal, Tech_type_id tech)
{
  const struct advance *pgoal, *ptech;

  if (tech == goal || nullptr == (pgoal = valid_advance_by_number(goal))
      || nullptr == (ptech = valid_advance_by_number(tech))) {
    return false;
  } else if (nullptr != presearch) {
    return BV_ISSET(presearch->inventions[goal].required_techs, tech);
  } else {
    advance_req_iterate(pgoal, preq)
    {
      if (preq == ptech) {
        return true;
      }
    }
    advance_req_iterate_end;
    return false;
  }
}

/**
   Function to determine cost for technology.  The equation is determined
   from game.info.tech_cost_style and game.info.tech_leakage.

   tech_cost_style:
   TECH_COST_CIV1CIV2: Civ (I|II) style. Every new tech add base_tech_cost to
                       cost of next tech.
   TECH_COST_CLASSIC: Cost of technology is:
                        base_tech_cost * (1 + reqs) * sqrt(1 + reqs) / 2
                      where reqs == number of requirement for tech, counted
                      recursively.
   TECH_COST_CLASSIC_PRESET: Cost are read from tech.ruleset. Missing costs
                             are generated by style "Classic".
   TECH_COST_EXPERIMENTAL: Cost of technology is:
                             base_tech_cost * (reqs^2
                                               / (1 + sqrt(sqrt(reqs + 1)))
                                               - 0.5)
                           where reqs == number of requirement for tech,
                           counted recursively.
   TECH_COST_EXPERIMENTAL_PRESET: Cost are read from tech.ruleset. Missing
                                  costs are generated by style
 "Experimental".

   tech_leakage:
   TECH_LEAKAGE_NONE: No reduction of the technology cost.
   TECH_LEAKAGE_EMBASSIES: Technology cost is reduced depending on the number
                           of players which already know the tech and you
 have an embassy with. TECH_LEAKAGE_PLAYERS: Technology cost is reduced
 depending on the number of all players (human, AI and barbarians) which
 already know the tech. TECH_LEAKAGE_NO_BARBS: Technology cost is reduced
 depending on the number of normal players (human and AI) which already know
                          the tech.

   At the end we multiply by the sciencebox value, as a percentage.  The
   cost can never be less than 1.

   'presearch' may be nullptr in which case a simplified result is returned
   (used by client and manual code).
 */
int research_total_bulbs_required(const struct research *presearch,
                                  Tech_type_id tech, bool loss_value)
{
  enum tech_cost_style tech_cost_style = game.info.tech_cost_style;
  int members;
  double base_cost, total_cost;
  double leak = 0.0;

  if (!loss_value && nullptr != presearch && !is_future_tech(tech)
      && research_invention_state(presearch, tech) == TECH_KNOWN) {
    // A non-future tech which is already known costs nothing.
    return 0;
  }

  if (is_future_tech(tech)) {
    // Future techs use style TECH_COST_CIV1CIV2.
    tech_cost_style = TECH_COST_CIV1CIV2;
  }

  fc_assert_msg(tech_cost_style_is_valid(tech_cost_style),
                "Invalid tech_cost_style %d", tech_cost_style);
  base_cost = 0.0;
  switch (tech_cost_style) {
  case TECH_COST_CIV1CIV2:
    if (nullptr != presearch) {
      base_cost = game.info.base_tech_cost * presearch->techs_researched;
      break;
    }

    fc_assert(presearch != nullptr);
    fc__fallthrough; // No break; Fallback to using preset cost.
  case TECH_COST_CLASSIC:
  case TECH_COST_CLASSIC_PRESET:
  case TECH_COST_EXPERIMENTAL:
  case TECH_COST_EXPERIMENTAL_PRESET:
  case TECH_COST_LINEAR: {
    const struct advance *padvance = valid_advance_by_number(tech);

    if (nullptr != padvance) {
      base_cost = padvance->cost;
    } else {
      fc_assert(nullptr != padvance); // Always fails.
    }
  } break;
  }

  total_cost = 0.0;
  members = 0;
  research_players_iterate(presearch, pplayer)
  {
    members++;
    total_cost +=
        (base_cost * get_player_bonus(pplayer, EFT_TECH_COST_FACTOR));
  }
  research_players_iterate_end;
  if (0 == members) {
    /* There is no more alive players for this research, no need to apply
     * complicated modifiers. */
    return base_cost * static_cast<double>(game.info.sciencebox) / 100.0;
  }
  base_cost = total_cost / members;

  fc_assert_msg(tech_leakage_style_is_valid(game.info.tech_leakage),
                "Invalid tech_leakage %d", game.info.tech_leakage);
  switch (game.info.tech_leakage) {
  case TECH_LEAKAGE_NONE:
    // no change
    break;

  case TECH_LEAKAGE_EMBASSIES: {
    int players = 0, players_with_tech_and_embassy = 0;

    players_iterate_alive(aplayer)
    {
      const struct research *aresearch = research_get(aplayer);

      players++;
      if (aresearch == presearch
          || (A_FUTURE == tech
                  ? aresearch->future_tech <= presearch->future_tech
                  : TECH_KNOWN
                        != research_invention_state(aresearch, tech))) {
        continue;
      }

      research_players_iterate(presearch, pplayer)
      {
        if (player_has_embassy(pplayer, aplayer)) {
          players_with_tech_and_embassy++;
          break;
        }
      }
      research_players_iterate_end;
    }
    players_iterate_alive_end;

    fc_assert_ret_val(0 < players, base_cost);
    fc_assert(players >= players_with_tech_and_embassy);
    leak = base_cost * players_with_tech_and_embassy
           * game.info.tech_leak_pct / players / 100;
  } break;

  case TECH_LEAKAGE_PLAYERS: {
    int players = 0, players_with_tech = 0;

    players_iterate_alive(aplayer)
    {
      players++;
      if (A_FUTURE == tech
              ? research_get(aplayer)->future_tech > presearch->future_tech
              : TECH_KNOWN
                    == research_invention_state(research_get(aplayer),
                                                tech)) {
        players_with_tech++;
      }
    }
    players_iterate_alive_end;

    fc_assert_ret_val(0 < players, base_cost);
    fc_assert(players >= players_with_tech);
    leak = base_cost * players_with_tech * game.info.tech_leak_pct / players
           / 100;
  } break;

  case TECH_LEAKAGE_NO_BARBS: {
    int players = 0, players_with_tech = 0;

    players_iterate_alive(aplayer)
    {
      if (is_barbarian(aplayer)) {
        continue;
      }
      players++;
      if (A_FUTURE == tech
              ? research_get(aplayer)->future_tech > presearch->future_tech
              : TECH_KNOWN
                    == research_invention_state(research_get(aplayer),
                                                tech)) {
        players_with_tech++;
      }
    }
    players_iterate_alive_end;

    fc_assert_ret_val(0 < players, base_cost);
    fc_assert(players >= players_with_tech);
    leak = base_cost * players_with_tech * game.info.tech_leak_pct / players
           / 100;
  } break;
  }

  if (leak > base_cost) {
    base_cost = 0.0;
  } else {
    base_cost -= leak;
  }

  /* Assign a science penalty to the AI at easier skill levels. This code
   * can also be adopted to create an extra-hard AI skill level where the AI
   * gets science benefits. */

  total_cost = 0.0;
  research_players_iterate(presearch, pplayer)
  {
    if (is_ai(pplayer)) {
      fc_assert(0 < pplayer->ai_common.science_cost);
      total_cost += base_cost * pplayer->ai_common.science_cost / 100.0;
    } else {
      total_cost += base_cost;
    }
  }
  research_players_iterate_end;
  base_cost = total_cost / members;

  base_cost *= static_cast<double>(game.info.sciencebox) / 100.0;

  return MAX(base_cost, 1);
}

/**
   Calculate the bulb upkeep needed for all techs of a player. See also
   research_total_bulbs_required().
 */
int player_tech_upkeep(const struct player *pplayer)
{
  const struct research *presearch = research_get(pplayer);
  int f = presearch->future_tech, t = presearch->techs_researched;
  double tech_upkeep = 0.0;
  double total_research_factor;
  int members;

  if (TECH_UPKEEP_NONE == game.info.tech_upkeep_style) {
    return 0;
  }

  total_research_factor = 0.0;
  members = 0;
  research_players_iterate(presearch, contributor)
  {
    total_research_factor +=
        (get_player_bonus(contributor, EFT_TECH_COST_FACTOR)
         + (is_ai(contributor) ? contributor->ai_common.science_cost / 100.0
                               : 1));
    members++;
  }
  research_players_iterate_end;
  if (0 == members) {
    // No player still alive.
    return 0;
  }

  // Upkeep cost for 'normal' techs (t).
  fc_assert_msg(tech_cost_style_is_valid(game.info.tech_cost_style),
                "Invalid tech_cost_style %d", game.info.tech_cost_style);
  switch (game.info.tech_cost_style) {
  case TECH_COST_CIV1CIV2:
    /* sum_1^t x = t * (t + 1) / 2 */
    tech_upkeep += game.info.base_tech_cost * t * (t + 1) / 2;
    break;
  case TECH_COST_CLASSIC:
  case TECH_COST_CLASSIC_PRESET:
  case TECH_COST_EXPERIMENTAL:
  case TECH_COST_EXPERIMENTAL_PRESET:
  case TECH_COST_LINEAR:
    advance_iterate(A_FIRST, padvance)
    {
      if (TECH_KNOWN
          == research_invention_state(presearch, advance_number(padvance))) {
        tech_upkeep += padvance->cost;
      }
    }
    advance_iterate_end;
    if (0 < f) {
      /* Upkeep cost for future techs (f) are calculated using style 0:
       * sum_t^(t+f) x = (f * (2 * t + f + 1) + 2 * t) / 2 */
      tech_upkeep += static_cast<double>(
          game.info.base_tech_cost * (f * (2 * t + f + 1) + 2 * t) / 2);
    }
    break;
  }

  tech_upkeep *= total_research_factor / members;
  tech_upkeep *= static_cast<double>(game.info.sciencebox) / 100.0;
  /* We only want to calculate the upkeep part of one player, not the
   * whole team! */
  tech_upkeep /= members;
  tech_upkeep /= game.info.tech_upkeep_divider;

  switch (game.info.tech_upkeep_style) {
  case TECH_UPKEEP_BASIC:
    tech_upkeep -= get_player_bonus(pplayer, EFT_TECH_UPKEEP_FREE);
    break;
  case TECH_UPKEEP_PER_CITY:
    tech_upkeep -= get_player_bonus(pplayer, EFT_TECH_UPKEEP_FREE);
    tech_upkeep *= city_list_size(pplayer->cities);
    break;
  case TECH_UPKEEP_NONE:
    fc_assert(game.info.tech_upkeep_style != TECH_UPKEEP_NONE);
    tech_upkeep = 0.0;
  }

  if (0.0 > tech_upkeep) {
    tech_upkeep = 0.0;
  }

  log_debug("[%s (%d)] tech upkeep: %d", player_name(pplayer),
            player_number(pplayer), (int) tech_upkeep);
  return static_cast<int>(tech_upkeep);
}

/**
   Returns the real size of the research player iterator.
 */
size_t research_player_iter_sizeof()
{
  return sizeof(struct research_player_iter);
}

/**
   Returns whether the iterator is currently at a valid state.
 */
static inline bool research_player_iter_valid_state(struct iterator *it)
{
  const player *pplayer = static_cast<const player *>(iterator_get(it));

  return (nullptr == pplayer || pplayer->is_alive);
}

/**
   Returns player of the iterator.
 */
static void *research_player_iter_pooled_get(const struct iterator *it)
{
  return player_list_link_data(RESEARCH_PLAYER_ITER(it)->plink);
}

/**
   Returns the next player sharing the research.
 */
static void research_player_iter_pooled_next(struct iterator *it)
{
  struct research_player_iter *rpit = RESEARCH_PLAYER_ITER(it);

  do {
    rpit->plink = player_list_link_next(rpit->plink);
  } while (!research_player_iter_valid_state(it));
}

/**
   Returns whether the iterate is valid.
 */
static bool research_player_iter_pooled_valid(const struct iterator *it)
{
  return nullptr != RESEARCH_PLAYER_ITER(it)->plink;
}

/**
   Returns player of the iterator.
 */
static void *research_player_iter_not_pooled_get(const struct iterator *it)
{
  return RESEARCH_PLAYER_ITER(it)->pplayer;
}

/**
   Invalidate the iterator.
 */
static void research_player_iter_not_pooled_next(struct iterator *it)
{
  RESEARCH_PLAYER_ITER(it)->pplayer = nullptr;
}

/**
   Returns whether the iterate is valid.
 */
static bool research_player_iter_not_pooled_valid(const struct iterator *it)
{
  return nullptr != RESEARCH_PLAYER_ITER(it)->pplayer;
}

/**
   Initializes a research player iterator.
 */
struct iterator *research_player_iter_init(struct research_player_iter *it,
                                           const struct research *presearch)
{
  struct iterator *base = ITERATOR(it);

  if (game.info.team_pooled_research && nullptr != presearch) {
    base->get = research_player_iter_pooled_get;
    base->next = research_player_iter_pooled_next;
    base->valid = research_player_iter_pooled_valid;
    it->plink = player_list_head(
        team_members(team_by_number(research_number(presearch))));
  } else {
    base->get = research_player_iter_not_pooled_get;
    base->next = research_player_iter_not_pooled_next;
    base->valid = research_player_iter_not_pooled_valid;
    it->pplayer =
        (nullptr != presearch ? player_by_number(research_number(presearch))
                              : nullptr);
  }

  // Ensure we have consistent data.
  if (!research_player_iter_valid_state(base)) {
    iterator_next(base);
  }

  return base;
}
