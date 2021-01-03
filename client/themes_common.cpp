/***********************************************************************
 Freeciv - Copyright (C) 2005 The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

/* utility */
#include "log.h"
#include "shared.h"
#include "support.h"

/* client/include */
#include "themes_g.h"

/* client */
#include "options.h"
#include "themes_common.h"

Q_GLOBAL_STATIC(QVector<QString>, themes_list)
/***************************************************************************
  A theme is a portion of client data, which for following reasons should
  be separated from a tileset:
  - Theme is not only graphic related
  - Theme can be changed independently from tileset
  - Theme implementation is gui specific and most themes can not be shared
    between different guis.
  Theme is recognized by its name.

  Theme is stored in a directory called like the theme. The directory
contains some data files. Each gui defines its own format in the
  get_useable_themes_in_directory() function.
****************************************************************************/

/* A directory containing a list of usable themes */
struct theme_directory {
  /* Path on the filesystem */
  char *path;
  /* Array of theme names */
  char **themes;
  /* Themes array length */
  int num_themes;
};

/* List of all directories with themes */
static int num_directories;
struct theme_directory *directories;

/************************************************************************/ /**
   Initialized themes data
 ****************************************************************************/
void init_themes()
{
  int i;

  /* get GUI-specific theme directories */
  char **gui_directories =
      get_gui_specific_themes_directories(&num_directories);

  directories = new theme_directory[num_directories];

  for (i = 0; i < num_directories; i++) {
    directories[i].path = gui_directories[i];

    /* get useable themes in this directory */
    directories[i].themes = get_useable_themes_in_directory(
        directories[i].path, &(directories[i].num_themes));
  }
  delete[] gui_directories;
}

/************************************************************************/ /**
   Return a static string vector of useable theme names.
 ****************************************************************************/
const QVector<QString> *get_themes_list(const struct option *poption)
{
  if (themes_list->isEmpty()) {
    int i, j, k;

    for (i = 0; i < num_directories; i++) {
      for (j = 0; j < directories[i].num_themes; j++) {
        for (k = 0; k < themes_list->count(); k++) {
          if (strcmp(qUtf8Printable(themes_list->at(k)),
                     directories[i].themes[j])
              == 0) {
            break;
          }
        }
        if (k == themes_list->count()) {
          themes_list->append(directories[i].themes[j]);
        }
      }
    }
  }

  return themes_list;
}

/************************************************************************/ /**
   Loads a theme with the given name. First matching directory will be used.
   If there's no such theme the function returns FALSE.
 ****************************************************************************/
bool load_theme(const char *theme_name)
{
  int i, j;

  for (i = 0; i < num_directories; i++) {
    for (j = 0; j < directories[i].num_themes; j++) {
      if (strcmp(theme_name, directories[i].themes[j]) == 0) {
        gui_load_theme(directories[i].path, directories[i].themes[j]);
        return true;
      }
    }
  }
  return false;
}

/************************************************************************/ /**
   Wrapper for load_theme. It's is used by local options dialog
 ****************************************************************************/
void theme_reread_callback(struct option *poption)
{
  const char *theme_name = option_str_get(poption);

  fc_assert_ret(NULL != theme_name && theme_name[0] != '\0');
  load_theme(theme_name);
}
