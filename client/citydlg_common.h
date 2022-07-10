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

#pragma once

// common
#include "city.h"
#include "fc_types.h"

class QPixmap;
struct worklist;

int get_citydlg_canvas_width();
int get_citydlg_canvas_height();
void generate_citydlg_dimensions();

char *city_production_cost_str(const struct city *pcity);
void get_city_dialog_production(struct city *pcity, char *buffer,
                                size_t buffer_len);

void get_city_dialog_output_text(const struct city *pcity,
                                 Output_type_id otype, char *buffer,
                                 size_t bufsz);
void get_city_dialog_pollution_text(const struct city *pcity, char *buf,
                                    size_t bufsz);
void get_city_dialog_culture_text(const struct city *pcity, char *buf,
                                  size_t bufsz);
void get_city_dialog_illness_text(const struct city *pcity, char *buf,
                                  size_t bufsz);
void get_city_dialog_airlift_text(const struct city *pcity, char *buf,
                                  size_t bufsz);

void get_city_dialog_airlift_value(const struct city *pcity, char *buf,
                                   size_t bufsz);

int get_city_citizen_types(struct city *pcity, enum citizen_feeling index,
                           enum citizen_category *categories);
void city_rotate_specialist(struct city *pcity, int citizen_index);

int city_change_production(struct city *pcity, struct universal *target);
int city_set_worklist(struct city *pcity, const struct worklist *pworklist);

bool city_queue_insert(struct city *pcity, int position,
                       struct universal *target);
bool city_queue_insert_worklist(struct city *pcity, int position,
                                const struct worklist *worklist);
void city_get_queue(struct city *pcity, struct worklist *pqueue);
bool city_set_queue(struct city *pcity, const struct worklist *pqueue);
bool city_can_buy(const struct city *pcity);
int city_sell_improvement(struct city *pcity, Impr_type_id sell_id);
int city_buy_production(struct city *pcity);
int city_change_specialist(struct city *pcity, Specialist_type_id from,
                           Specialist_type_id to);
int city_rename(struct city *pcity, const char *name);
