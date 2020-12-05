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
#ifndef FC__UNITHAND_H
#define FC__UNITHAND_H

/* common */
#include "explanation.h"
#include "unit.h"

#include "hand_gen.h"



bool unit_activity_handling(struct unit *punit,
                            enum unit_activity new_activity);
bool unit_activity_handling_targeted(struct unit *punit,
                                     enum unit_activity new_activity,
                                     struct extra_type **new_target);
void unit_change_homecity_handling(struct unit *punit,
                                   struct city *new_pcity, bool rehome);

bool unit_move_handling(struct unit *punit, struct tile *pdesttile,
                        bool igzoc, bool move_diplomat_city);

void unit_do_action(struct player *pplayer, const int actor_id,
                    const int target_id, const int sub_tgt_id,
                    const char *name, const action_id action_type);

bool unit_perform_action(struct player *pplayer, const int actor_id,
                         const int target_id, const int sub_tgt_id,
                         const char *name, const action_id action_type,
                         const enum action_requester requester);

void illegal_action_msg(struct player *pplayer, const enum event_type event,
                        struct unit *actor, const action_id stopped_action,
                        const struct tile *target_tile,
                        const struct city *target_city,
                        const struct unit *target_unit);

enum ane_kind action_not_enabled_reason(struct unit *punit, action_id act_id,
                                        const struct tile *target_tile,
                                        const struct city *target_city,
                                        const struct unit *target_unit);

bool unit_server_side_agent_set(struct player *pplayer, struct unit *punit,
                                enum server_side_agent agent);


#endif /* FC__UNITHAND_H */
