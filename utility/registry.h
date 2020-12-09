/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once



#include "shared.h"

struct section;

void registry_module_init(void);
void registry_module_close(void);

struct section_file *secfile_new(bool allow_duplicates);
void secfile_destroy(struct section_file *secfile);
struct section_file *secfile_load(const char *filename,
                                  bool allow_duplicates);

void secfile_allow_digital_boolean(struct section_file *secfile,
                                   bool allow_digital_boolean);

const char *secfile_error(void);
const char *section_name(const struct section *psection);

#include "registry_ini.h"




