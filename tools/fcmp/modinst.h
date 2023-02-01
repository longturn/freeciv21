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

#include <QString>
#include <QUrl>

struct fcmp_params {
  QUrl list_url;
  QString inst_prefix;
  QString autoinstall;
};

#define MODPACK_LIST_URL                                                    \
  "https://raw.githubusercontent.com/longturn/modpacks/main/index.json"
#define DEFAULT_URL_START                                                   \
  "https://raw.githubusercontent.com/longturn/modpacks/main/mods/"

#define SPECENUM_NAME modpack_type
#define SPECENUM_VALUE0 MPT_RULESET
#define SPECENUM_VALUE0NAME N_("Ruleset")
#define SPECENUM_VALUE1 MPT_TILESET
#define SPECENUM_VALUE1NAME N_("Tileset")
#define SPECENUM_VALUE2 MPT_MODPACK
#define SPECENUM_VALUE2NAME N_("Modpack")
#define SPECENUM_VALUE3 MPT_SCENARIO
#define SPECENUM_VALUE3NAME N_("Scenario")
#define SPECENUM_VALUE4 MPT_SOUNDSET
#define SPECENUM_VALUE4NAME N_("Soundset")
#define SPECENUM_VALUE5 MPT_MUSICSET
#define SPECENUM_VALUE5NAME N_("Musicset")
#define SPECENUM_VALUE6 MPT_MODPACK_GROUP
#define SPECENUM_VALUE6NAME N_("Group")
#include "specenum_gen.h"

void fcmp_init();
void fcmp_deinit();

void load_install_info_lists(struct fcmp_params *fcmp);
