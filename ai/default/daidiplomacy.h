/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

#include "fc_types.h"

#include "ai.h" // incident_type

struct Treaty;
struct Clause;

void dai_diplomacy_begin_new_phase(struct ai_type *ait,
                                   struct player *pplayer);
void dai_diplomacy_actions(struct ai_type *ait, struct player *pplayer);

void dai_treaty_evaluate(struct ai_type *ait, struct player *pplayer,
                         struct player *aplayer, struct Treaty *ptreaty);
void dai_treaty_accepted(struct ai_type *ait, struct player *pplayer,
                         struct player *aplayer, struct Treaty *ptreaty);

void dai_incident(struct ai_type *ait, enum incident_type type,
                  enum casus_belli_range scope, const struct action *paction,
                  struct player *receiver, struct player *violator,
                  struct player *victim);

bool dai_on_war_footing(struct ai_type *ait, struct player *pplayer);

void dai_diplomacy_first_contact(struct ai_type *ait, struct player *pplayer,
                                 struct player *aplayer);
