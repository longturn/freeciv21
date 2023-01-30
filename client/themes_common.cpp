/*
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
 */

// utility
#include "log.h"
#include "support.h"

/* client/include */
#include "themes_g.h"

// client
#include "options.h"
#include "themes_common.h"

// gui-qt
#include "qtg_cxxside.h"

Q_GLOBAL_STATIC(QVector<QString>, themes_list)

/**
  \file
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
 */

// A directory containing a list of usable themes
struct theme_directory {
  // Path on the filesystem
  QString path;
  // Array of theme names
  QStringList themes;
};

// List of all directories with themes
static int num_directories;
struct theme_directory *directories;

/**
   Initialized themes data
 */
void init_themes()
{
  int i;

  // get GUI-specific theme directories
  QStringList gui_directories =
      get_gui_specific_themes_directories(&num_directories);

  directories = new theme_directory[num_directories];

  for (i = 0; i < num_directories; i++) {
    directories[i].path = gui_directories.at(i);

    // get useable themes in this directory
    directories[i].themes =
        get_useable_themes_in_directory(directories[i].path);
  }
}

/**
   Return a static string vector of useable theme names.
 */
const QVector<QString> *get_themes_list(const struct option *poption)
{
  if (themes_list->isEmpty()) {
    for (int i = 0; i < num_directories; i++) {
      for (const auto &theme : qAsConst(directories[i].themes)) {
        bool found = false;
        for (int k = 0; k < themes_list->count(); k++) {
          if (themes_list->at(k) == theme) {
            found = true;
            break;
          }
        }
        if (!found) {
          themes_list->append(theme);
        }
      }
    }
  }

  return themes_list;
}

/**
   Loads a theme with the given name. First matching directory will be used.
   If there's no such theme the function returns FALSE.
 */
bool load_theme(const QString &theme_name)
{
  for (int i = 0; i < num_directories; i++) {
    for (const auto &theme : directories[i].themes) {
      if (theme_name == theme) {
        gui_load_theme(directories[i].path, theme);
        return true;
      }
    }
  }
  return false;
}

/**
   Wrapper for load_theme. It's is used by local options dialog
 */
void theme_reread_callback(struct option *poption)
{
  const char *theme_name = option_str_get(poption);

  fc_assert_ret(nullptr != theme_name && theme_name[0] != '\0');
  load_theme(theme_name);
}
