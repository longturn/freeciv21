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

/* utility */
#include "support.h"

/* server */
#include "ruleset.h"

#define RULESET_COMPAT_CAP "+Freeciv-3.0-ruleset"

struct rscompat_info {
  bool compat_mode;
  rs_conversion_logger log_cb;
  int ver_buildings;
  int ver_cities;
  int ver_effects;
  int ver_game;
  int ver_governments;
  int ver_nations;
  int ver_styles;
  int ver_techs;
  int ver_terrain;
  int ver_units;
};

void rscompat_init_info(struct rscompat_info *info);

int rscompat_check_capabilities(struct section_file *file,
                                const char *filename,
                                struct rscompat_info *info);

bool rscompat_names(struct rscompat_info *info);

void rscompat_postprocess(struct rscompat_info *info);

/* General upgrade functions that should be kept to avoid regressions in
 * corner case handling. */
void rscompat_enablers_add_obligatory_hard_reqs();

/* Functions from ruleset.c made visible to rscompat.c */
struct requirement_vector *lookup_req_list(struct section_file *file,
                                           struct rscompat_info *compat,
                                           const char *sec, const char *sub,
                                           const char *rfor);

/* Functions specific to 3.0 -> 3.1 transition */
bool rscompat_auto_attack_3_1(struct rscompat_info *compat,
                              struct action_auto_perf *auto_perf,
                              size_t psize,
                              enum unit_type_flag_id *protecor_flag);
const char *rscompat_req_type_name_3_1(const char *type, const char *range,
                                       bool survives, bool present,
                                       bool quiet, const char *value);
const char *rscompat_req_name_3_1(const char *type, const char *old_name);
const char *rscompat_utype_flag_name_3_1(struct rscompat_info *info,
                                         const char *old_type);
bool rscompat_old_effect_3_1(const char *type, struct section_file *file,
                             const char *sec_name,
                             struct rscompat_info *compat);
void rscompat_extra_adjust_3_1(struct rscompat_info *compat,
                               struct extra_type *pextra);
bool rscompat_old_slow_invasions_3_1(struct rscompat_info *compat,
                                     bool slow_invasions);
