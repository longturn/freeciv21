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
#ifndef FC__VALIDITY_H
#define FC__VALIDITY_H



typedef void (*requirers_cb)(const char *msg, void *data);

bool is_tech_needed(struct advance *padv, requirers_cb cb, void *data);
bool is_building_needed(struct impr_type *pimpr, requirers_cb cb,
                        void *data);
bool is_utype_needed(struct unit_type *ptype, requirers_cb cb, void *data);
bool is_good_needed(struct goods_type *pgood, requirers_cb cb, void *data);
bool is_government_needed(struct government *pgov, requirers_cb cb,
                          void *data);
bool is_extra_needed(struct extra_type *pextra, requirers_cb cb, void *data);
bool is_multiplier_needed(struct multiplier *pmul, requirers_cb cb,
                          void *data);
bool is_terrain_needed(struct terrain *pterr, requirers_cb cb, void *data);



#endif /* FC__VALIDITY_H */
