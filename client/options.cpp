/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2023 Freeciv21 and Freeciv
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

#include <fc_config.h>

#include <cstring>
#include <qglobal.h>
#include <sys/stat.h>

// Qt
#include <QDir>
#include <QHash>
#include <QUrl>

// utility
#include "deprecations.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "registry_ini.h"
#include "shared.h"
#include "support.h"

// generated
#include "fc_version.h"

// common
#include "events.h"
#include "version.h"

/* client/include */
#include "gui_main_g.h"
#include "menu_g.h"
#include "optiondlg_g.h"
#include "repodlgs_g.h"
#include "voteinfo_bar_g.h"

// client
#include "audio/audio.h"
#include "chatline_common.h"
#include "citybar.h"
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "global_worklist.h"
#include "governor.h"
#include "mapctrl_common.h"
#include "music.h"
#include "options.h"
#include "overview_common.h"
#include "packhand_gen.h"
#include "qtg_cxxside.h"
#include "themes_common.h"
#include "tileset/tilespec.h"
#include "views/view_cities_data.h"
#include "views/view_map_common.h"
#include "views/view_nations_data.h"

const char *const TILESET_OPTIONS_PREFIX = "tileset_";

typedef QHash<QString, QString> optionsHash;
typedef QHash<QString, intptr_t> dialOptionsHash;

client_options *gui_options = nullptr;

/* Set to TRUE after the first call to options_init(), to avoid the usage
 * of non-initialized datas when calling the changed callback. */
static bool options_fully_initialized = false;

/* See #2237
static const QVector<QString> *
get_mapimg_format_list(const struct option *poption);
*/

/**
  Option set structure.
 */
struct option_set {
  struct option *(*option_by_number)(int);
  struct option *(*option_first)();

  int (*category_number)();
  const char *(*category_name)(int);
};

/**
   Returns the option corresponding of the number in this option set.
 */
struct option *optset_option_by_number(const struct option_set *poptset,
                                       int id)
{
  fc_assert_ret_val(nullptr != poptset, nullptr);

  return poptset->option_by_number(id);
}

/**
   Returns the option corresponding of the name in this option set.
 */
struct option *optset_option_by_name(const struct option_set *poptset,
                                     const char *name)
{
  fc_assert_ret_val(nullptr != poptset, nullptr);

  options_iterate(poptset, poption)
  {
    if (0 == strcmp(option_name(poption), name)) {
      return poption;
    }
  }
  options_iterate_end;
  return nullptr;
}

/**
   Returns the first option of this option set.
 */
struct option *optset_option_first(const struct option_set *poptset)
{
  fc_assert_ret_val(nullptr != poptset, nullptr);

  return poptset->option_first();
}

/**
   Returns the name (translated) of the category of this option set.
 */
const char *optset_category_name(const struct option_set *poptset,
                                 int category)
{
  fc_assert_ret_val(nullptr != poptset, nullptr);

  return poptset->category_name(category);
}

struct option_common_vtable {
  int (*number)(const struct option *);
  const char *(*name)(const struct option *);
  const char *(*description)(const struct option *);
  const char *(*help_text)(const struct option *);
  int (*category)(const struct option *);
  bool (*is_changeable)(const struct option *);
  struct option *(*next)(const struct option *);
};

struct option_bool_vtable {
  bool (*get)(const struct option *);
  bool (*def)(const struct option *);
  bool (*set)(struct option *, bool);
};

struct option_int_vtable {
  int (*get)(const struct option *);
  int (*def)(const struct option *);
  int (*minimum)(const struct option *);
  int (*maximum)(const struct option *);
  bool (*set)(struct option *, int);
};
// Specific string accessors (OT_STRING == type).
struct option_str_vtable {
  const char *(*get)(const struct option *);
  const char *(*def)(const struct option *);
  const QVector<QString> *(*values)(const struct option *);
  bool (*set)(struct option *, const char *);
};
// Specific enum accessors (OT_ENUM == type).
struct option_enum_vtable {
  int (*get)(const struct option *);
  int (*def)(const struct option *);
  const QVector<QString> *(*values)(const struct option *);
  bool (*set)(struct option *, int);
  int (*cmp)(const char *, const char *);
};
// Specific bitwise accessors (OT_BITWISE == type).
struct option_bitwise_vtable {
  unsigned (*get)(const struct option *);
  unsigned (*def)(const struct option *);
  const QVector<QString> *(*values)(const struct option *);
  bool (*set)(struct option *, unsigned);
};
// Specific font accessors (OT_FONT == type).
struct option_font_vtable {
  QFont (*get)(const struct option *);
  QFont (*def)(const struct option *);
  void (*set_def)(const struct option *, const QFont &font);
  QString (*target)(const struct option *);
  bool (*set)(struct option *, const QFont &);
};
// Specific color accessors (OT_COLOR == type).
struct option_color_vtable {
  struct ft_color (*get)(const struct option *);
  struct ft_color (*def)(const struct option *);
  bool (*set)(struct option *, struct ft_color);
};

/**
  The base class for options.
 */
struct option {
  // A link to the option set.
  const struct option_set *poptset;
  // Type of the option.
  enum option_type type;

  // Common accessors.
  // Common accessors.
  const struct option_common_vtable *common_vtable;
  // Specific typed accessors.
  union {
    // Specific boolean accessors (OT_BOOLEAN == type).
    const struct option_bool_vtable *bool_vtable;
    // Specific integer accessors (OT_INTEGER == type).
    const struct option_int_vtable *int_vtable;
    // Specific string accessors (OT_STRING == type).
    const struct option_str_vtable *str_vtable;
    // Specific enum accessors (OT_ENUM == type).
    const struct option_enum_vtable *enum_vtable;
    // Specific bitwise accessors (OT_BITWISE == type).
    const struct option_bitwise_vtable *bitwise_vtable;
    // Specific font accessors (OT_FONT == type).
    const struct option_font_vtable *font_vtable;
    // Specific color accessors (OT_COLOR == type).
    const struct option_color_vtable *color_vtable;
  };
  // Called after the value changed.
  void (*changed_callback)(struct option *option);

  int callback_data;

  // Volatile.
  void *gui_data;
};

#define OPTION(poption) ((struct option *) (poption))

#define OPTION_INIT(optset, spec_type, spec_table_var, common_table,        \
                    spec_table, changed_cb, cb_data)                        \
  {                                                                         \
    .poptset = optset, .type = spec_type, .common_vtable = &common_table,   \
    .spec_table_var = &spec_table, .changed_callback = changed_cb,          \
    .callback_data = cb_data, .gui_data = nullptr                           \
  }
#define OPTION_BOOL_INIT(optset, common_table, bool_table, changed_cb)      \
  OPTION_INIT(optset, OT_BOOLEAN, bool_vtable, common_table, bool_table,    \
              changed_cb, 0)
#define OPTION_INT_INIT(optset, common_table, int_table, changed_cb)        \
  OPTION_INIT(optset, OT_INTEGER, int_vtable, common_table, int_table,      \
              changed_cb, 0)
#define OPTION_STR_INIT(optset, common_table, str_table, changed_cb,        \
                        cb_data)                                            \
  OPTION_INIT(optset, OT_STRING, str_vtable, common_table, str_table,       \
              changed_cb, cb_data)
#define OPTION_ENUM_INIT(optset, common_table, enum_table, changed_cb)      \
  OPTION_INIT(optset, OT_ENUM, enum_vtable, common_table, enum_table,       \
              changed_cb, 0)
#define OPTION_BITWISE_INIT(optset, common_table, bitwise_table,            \
                            changed_cb)                                     \
  OPTION_INIT(optset, OT_BITWISE, bitwise_vtable, common_table,             \
              bitwise_table, changed_cb, 0)
#define OPTION_FONT_INIT(optset, common_table, font_table, changed_cb)      \
  OPTION_INIT(optset, OT_FONT, font_vtable, common_table, font_table,       \
              changed_cb, 0)
#define OPTION_COLOR_INIT(optset, common_table, color_table, changed_cb)    \
  OPTION_INIT(optset, OT_COLOR, color_vtable, common_table, color_table,    \
              changed_cb, 0)

/**
   Returns the option set owner of this option.
 */
const struct option_set *option_optset(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return poption->poptset;
}

/**
   Returns the number of the option.
 */
int option_number(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);

  return poption->common_vtable->number(poption);
}

/**
   Returns the name of the option.
 */
const char *option_name(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return poption->common_vtable->name(poption);
}

/**
   Returns the description (translated) of the option.
 */
const char *option_description(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return poption->common_vtable->description(poption);
}

/**
   Returns the help text (translated) of the option.
 */
QString option_help_text(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return poption->common_vtable->help_text(poption);
}

/**
   Returns the type of the option.
 */
enum option_type option_type(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, static_cast<enum option_type>(0));

  return poption->type;
}

/**
   Returns the name (translated) of the category of the option.
 */
QString option_category_name(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return optset_category_name(poption->poptset,
                              poption->common_vtable->category(poption));
}

/**
   Returns TRUE if this option can be modified.
 */
bool option_is_changeable(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, false);

  return poption->common_vtable->is_changeable(poption);
}

/**
   Returns the next option or nullptr if this is the last.
 */
struct option *option_next(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return poption->common_vtable->next(poption);
}

/**
   Set the option to its default value.  Returns TRUE if the option changed.
 */
bool option_reset(struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, false);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    return option_bool_set(poption, option_bool_def(poption));
  case OT_INTEGER:
    return option_int_set(poption, option_int_def(poption));
  case OT_STRING:
    return option_str_set(poption, option_str_def(poption));
  case OT_ENUM:
    return option_enum_set_int(poption, option_enum_def_int(poption));
  case OT_BITWISE:
    return option_bitwise_set(poption, option_bitwise_def(poption));
  case OT_FONT:
    return option_font_set(poption, option_font_def(poption));
  case OT_COLOR:
    return option_color_set(poption, option_color_def(poption));
  }
  return false;
}

/**
   Set the function to call every time this option changes.  Can be nullptr.
 */
void option_set_changed_callback(struct option *poption,
                                 void (*callback)(struct option *))
{
  fc_assert_ret(nullptr != poption);

  poption->changed_callback = callback;
}

/**
   Force to use the option changed callback.
 */
void option_changed(struct option *poption)
{
  fc_assert_ret(nullptr != poption);

  if (!options_fully_initialized) {
    // Prevent to use non-initialized datas.
    return;
  }

  if (poption->changed_callback) {
    poption->changed_callback(poption);
  }

  option_gui_update(poption);
}

/**
   Set the gui data for this option.
 */
void option_set_gui_data(struct option *poption, void *data)
{
  fc_assert_ret(nullptr != poption);

  poption->gui_data = data;
}

/**
   Returns the gui data of this option.
 */
void *option_get_gui_data(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);

  return poption->gui_data;
}

/**
   Returns the callback data of this option.
 */
int option_get_cb_data(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);

  return poption->callback_data;
}

/**
   Returns the current value of this boolean option.
 */
bool option_bool_get(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_BOOLEAN == poption->type, false);

  return poption->bool_vtable->get(poption);
}

/**
   Returns the default value of this boolean option.
 */
bool option_bool_def(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_BOOLEAN == poption->type, false);

  return poption->bool_vtable->def(poption);
}

/**
   Sets the value of this boolean option. Returns TRUE if the value changed.
 */
bool option_bool_set(struct option *poption, bool val)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_BOOLEAN == poption->type, false);

  if (poption->bool_vtable->set(poption, val)) {
    option_changed(poption);
    return true;
  }
  return false;
}

/**
   Returns the current value of this integer option.
 */
int option_int_get(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->get(poption);
}

/**
   Returns the default value of this integer option.
 */
int option_int_def(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->def(poption);
}

/**
   Returns the minimal value of this integer option.
 */
int option_int_min(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->minimum(poption);
}

/**
   Returns the maximal value of this integer option.
 */
int option_int_max(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_INTEGER == poption->type, 0);

  return poption->int_vtable->maximum(poption);
}

/**
   Sets the value of this integer option. Returns TRUE if the value changed.
 */
bool option_int_set(struct option *poption, int val)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_INTEGER == poption->type, false);

  if (poption->int_vtable->set(poption, val)) {
    option_changed(poption);
    return true;
  }
  return false;
}

/**
   Returns the current value of this string option.
 */
const char *option_str_get(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);
  fc_assert_ret_val(OT_STRING == poption->type, nullptr);

  return poption->str_vtable->get(poption);
}

/**
   Returns the default value of this string option.
 */
const char *option_str_def(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);
  fc_assert_ret_val(OT_STRING == poption->type, nullptr);

  return poption->str_vtable->def(poption);
}

/**
   Returns the possible string values of this string option.
 */
const QVector<QString> *option_str_values(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);
  fc_assert_ret_val(OT_STRING == poption->type, nullptr);

  return poption->str_vtable->values(poption);
}

/**
   Sets the value of this string option. Returns TRUE if the value changed.
 */
bool option_str_set(struct option *poption, const char *str)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_STRING == poption->type, false);
  fc_assert_ret_val(nullptr != str, false);

  if (poption->str_vtable->set(poption, str)) {
    option_changed(poption);
    return true;
  }
  return false;
}

/**
   Returns the value corresponding to the user-visible (translatable but not
   translated) string. Returns -1 if not matched.
 */
int option_enum_str_to_int(const struct option *poption, const char *str)
{
  const QVector<QString> *values;
  int val;

  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_ENUM == poption->type, 0);
  values = poption->enum_vtable->values(poption);
  fc_assert_ret_val(nullptr != values, 0);

  for (val = 0; val < values->count(); val++) {
    if (0
        == poption->enum_vtable->cmp(qUtf8Printable(values->at(val)), str)) {
      return val;
    }
  }
  return -1;
}

/**
   Returns the user-visible (translatable but not translated) string
   corresponding to the value. Returns nullptr on error.
 */
QString option_enum_int_to_str(const struct option *poption, int val)
{
  const QVector<QString> *values;

  fc_assert_ret_val(nullptr != poption, nullptr);
  fc_assert_ret_val(OT_ENUM == poption->type, nullptr);
  values = poption->enum_vtable->values(poption);
  fc_assert_ret_val(nullptr != values, nullptr);
  if (val < values->count()) {
    // TODO bug here - val is bigger than vector size
    return values->at(val);
  } else {
    return nullptr;
  }
}

/**
   Returns the current value of this enum option (as an integer).
 */
int option_enum_get_int(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, -1);
  fc_assert_ret_val(OT_ENUM == poption->type, -1);

  return poption->enum_vtable->get(poption);
}

/**
   Returns the default value of this enum option (as an integer).
 */
int option_enum_def_int(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, -1);
  fc_assert_ret_val(OT_ENUM == poption->type, -1);

  return poption->enum_vtable->def(poption);
}

/**
   Sets the value of this enum option. Returns TRUE if the value changed.
 */
bool option_enum_set_int(struct option *poption, int val)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_ENUM == poption->type, false);

  if (poption->enum_vtable->set(poption, val)) {
    option_changed(poption);
    return true;
  }
  return false;
}

/**
   Returns the current value of this bitwise option.
 */
unsigned option_bitwise_get(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_BITWISE == poption->type, 0);

  return poption->bitwise_vtable->get(poption);
}

/**
   Returns the default value of this bitwise option.
 */
unsigned option_bitwise_def(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_BITWISE == poption->type, 0);

  return poption->bitwise_vtable->def(poption);
}

/**
   Returns the mask of this bitwise option.
 */
unsigned option_bitwise_mask(const struct option *poption)
{
  const QVector<QString> *values;

  fc_assert_ret_val(nullptr != poption, 0);
  fc_assert_ret_val(OT_BITWISE == poption->type, 0);

  values = poption->bitwise_vtable->values(poption);
  fc_assert_ret_val(nullptr != values, 0);

  return (1 << values->count()) - 1;
}

/**
   Returns a vector of strings describing every bit of this option, as
   user-visible (translatable but not translated) strings.
 */
const QVector<QString> *option_bitwise_values(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, nullptr);
  fc_assert_ret_val(OT_BITWISE == poption->type, nullptr);

  return poption->bitwise_vtable->values(poption);
}

/**
   Sets the value of this bitwise option. Returns TRUE if the value changed.
 */
bool option_bitwise_set(struct option *poption, unsigned val)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_BITWISE == poption->type, false);

  if (0 != (val & ~option_bitwise_mask(poption))
      || !poption->bitwise_vtable->set(poption, val)) {
    return false;
  }

  option_changed(poption);
  return true;
}

/**
   Returns the current value of this font option.
 */
QFont option_font_get(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, QFont());
  fc_assert_ret_val(OT_FONT == poption->type, QFont());

  return poption->font_vtable->get(poption);
}

/**
   Returns the default value of this font option.
 */
QFont option_font_def(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, QFont());
  fc_assert_ret_val(OT_FONT == poption->type, QFont());

  return poption->font_vtable->def(poption);
}

/**
   Returns the default value of this font option.
 */
void option_font_set_default(const struct option *poption, const QFont &font)
{
  fc_assert_ret(nullptr != poption);
  fc_assert_ret(OT_FONT == poption->type);

  poption->font_vtable->set_def(poption, font);
}

/**
   Returns the target style name of this font option.
 */
QString option_font_target(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption, QString());
  fc_assert_ret_val(OT_FONT == poption->type, QString());

  return poption->font_vtable->target(poption);
}

/**
   Sets the value of this font option. Returns TRUE if the value changed.
 */
bool option_font_set(struct option *poption, const QFont &font)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_FONT == poption->type, false);

  if (poption->font_vtable->set(poption, font)) {
    option_changed(poption);
    return true;
  }
  return false;
}

/**
   Returns the current value of this color option.
 */
struct ft_color option_color_get(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption,
                    ft_color_construct(nullptr, nullptr));
  fc_assert_ret_val(OT_COLOR == poption->type,
                    ft_color_construct(nullptr, nullptr));

  return poption->color_vtable->get(poption);
}

/**
   Returns the default value of this color option.
 */
struct ft_color option_color_def(const struct option *poption)
{
  fc_assert_ret_val(nullptr != poption,
                    ft_color_construct(nullptr, nullptr));
  fc_assert_ret_val(OT_COLOR == poption->type,
                    ft_color_construct(nullptr, nullptr));

  return poption->color_vtable->def(poption);
}

/**
   Sets the value of this color option. Returns TRUE if the value
   changed.
 */
bool option_color_set(struct option *poption, struct ft_color color)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(OT_COLOR == poption->type, false);

  if (poption->color_vtable->set(poption, color)) {
    option_changed(poption);
    return true;
  }
  return false;
}

/**
  Client option set.
 */
static struct option *client_optset_option_by_number(int id);
static struct option *client_optset_option_first();
static int client_optset_category_number();
static const char *client_optset_category_name(int category);

static struct option_set client_optset_static = {
    .option_by_number = client_optset_option_by_number,
    .option_first = client_optset_option_first,
    .category_number = client_optset_category_number,
    .category_name = client_optset_category_name};
const struct option_set *client_optset = &client_optset_static;

struct copt_val_name {
  const char *support; /* Untranslated long support name, used
                        * for saving. */
  const char *pretty;  /* Translated, used to display to the
                        * users. */
};

/**
  Virtuals tables for the client options.
 */
static int client_option_number(const struct option *poption);
static const char *client_option_name(const struct option *poption);
static const char *client_option_description(const struct option *poption);
static const char *client_option_help_text(const struct option *poption);
static int client_option_category(const struct option *poption);
static bool client_option_is_changeable(const struct option *poption);
static struct option *client_option_next(const struct option *poption);

static const struct option_common_vtable client_option_common_vtable = {
    .number = client_option_number,
    .name = client_option_name,
    .description = client_option_description,
    .help_text = client_option_help_text,
    .category = client_option_category,
    .is_changeable = client_option_is_changeable,
    .next = client_option_next};

static bool client_option_bool_get(const struct option *poption);
static bool client_option_bool_def(const struct option *poption);
static bool client_option_bool_set(struct option *poption, bool val);

static const struct option_bool_vtable client_option_bool_vtable = {
    .get = client_option_bool_get,
    .def = client_option_bool_def,
    .set = client_option_bool_set};

static int client_option_int_get(const struct option *poption);
static int client_option_int_def(const struct option *poption);
static int client_option_int_min(const struct option *poption);
static int client_option_int_max(const struct option *poption);
static bool client_option_int_set(struct option *poption, int val);

static const struct option_int_vtable client_option_int_vtable = {
    .get = client_option_int_get,
    .def = client_option_int_def,
    .minimum = client_option_int_min,
    .maximum = client_option_int_max,
    .set = client_option_int_set};

static const char *client_option_str_get(const struct option *poption);
static const char *client_option_str_def(const struct option *poption);
static const QVector<QString> *
client_option_str_values(const struct option *poption);
static bool client_option_str_set(struct option *poption, const char *str);

static const struct option_str_vtable client_option_str_vtable = {
    .get = client_option_str_get,
    .def = client_option_str_def,
    .values = client_option_str_values,
    .set = client_option_str_set};

static QFont client_option_font_get(const struct option *poption);
static QFont client_option_font_def(const struct option *poption);
static void client_option_font_set_def(const struct option *poption,
                                       const QFont &font);
static QString client_option_font_target(const struct option *poption);
static bool client_option_font_set(struct option *poption,
                                   const QFont &font);

static const struct option_font_vtable client_option_font_vtable = {
    .get = client_option_font_get,
    .def = client_option_font_def,
    .set_def = client_option_font_set_def,
    .target = client_option_font_target,
    .set = client_option_font_set};

static struct ft_color client_option_color_get(const struct option *poption);
static struct ft_color client_option_color_def(const struct option *poption);
static bool client_option_color_set(struct option *poption,
                                    struct ft_color color);

static const struct option_color_vtable client_option_color_vtable = {
    .get = client_option_color_get,
    .def = client_option_color_def,
    .set = client_option_color_set};

enum client_option_category {
  COC_GRAPHICS,
  COC_OVERVIEW,
  COC_SOUND,
  COC_INTERFACE,
  COC_MAPIMG,
  COC_NETWORK,
  COC_FONT,
  COC_MAX
};

/**
  Derived class client option, inherinting of base class option.
 */
struct client_option {
  struct option base_option; // Base structure, must be the first!

  const char *name;        // Short name - used as an identifier
  const char *description; // One-line description
  const char *help_text;   // Paragraph-length help text
  enum client_option_category category;

  struct {
    // OT_BOOLEAN type option.
    struct {
      bool *pvalue;
      bool def;
    } boolean;
    // OT_INTEGER type option.
    struct {
      int *pvalue;
      int def, min, max;
    } integer;
    // OT_STRING type option.
    struct {
      char *pvalue;
      size_t size;
      const char *def;
      /*
       * A function to return a string vector of possible string values,
       * or nullptr for none.
       */
      const QVector<QString> *(*val_accessor)(const struct option *);
    } string;
    // OT_ENUM type option.
    struct {
      int *pvalue;
      int def;
      QVector<QString> *support_names, *pretty_names; // untranslated
      const struct copt_val_name *(*name_accessor)(int value);
    } enumerator;
    // OT_BITWISE type option.
    struct {
      unsigned *pvalue;
      unsigned def;
      QVector<QString> *support_names, *pretty_names; // untranslated
      const struct copt_val_name *(*name_accessor)(int value);
    } bitwise;
    // OT_FONT type option.
    struct {
      QFont *value;
      QFont def;
      QString target;
    } font;
    // OT_COLOR type option.
    struct {
      struct ft_color *pvalue;
      struct ft_color def;
    } color;
  } u;
};

#define CLIENT_OPTION(poption) ((struct client_option *) (poption))

/*
 * Generate a client option of type OT_BOOLEAN.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ospec: A gui_type enumerator which determin for what particular client
 *        gui this option is for. Sets to GUI_STUB for common options.
 * odef:  The default value of this client option (FALSE or TRUE).
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_BOOL_OPTION(oname, odesc, ohelp, ocat, odef, ocb)               \
  client_option                                                             \
  {                                                                         \
    .base_option = OPTION_BOOL_INIT(&client_optset_static,                  \
                                    client_option_common_vtable,            \
                                    client_option_bool_vtable, ocb),        \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat,                                                       \
    .u =                                                                    \
    {.boolean = {                                                           \
         .pvalue = &gui_options->oname,                                     \
         .def = odef,                                                       \
     } }                                                                    \
  }

/*
 * Generate a client option of type OT_INTEGER.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * odef:  The default value of this client option.
 * omin:  The minimal value of this client option.
 * omax:  The maximal value of this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_INT_OPTION(oname, odesc, ohelp, ocat, odef, omin, omax, ocb)    \
  client_option                                                             \
  {                                                                         \
    .base_option =                                                          \
        OPTION_INT_INIT(&client_optset_static, client_option_common_vtable, \
                        client_option_int_vtable, ocb),                     \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat, .u = {                                                \
      .integer = {.pvalue = &gui_options->oname,                            \
                  .def = odef,                                              \
                  .min = omin,                                              \
                  .max = omax}                                              \
    }                                                                       \
  }

/*
 * Generate a client option of type OT_STRING.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 *        Be sure to pass the array variable and not a pointer to it because
 *        the size is calculated with sizeof().
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * odef:  The default string for this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_STR_OPTION(oname, odesc, ohelp, ocat, odef, ocb, cbd)           \
  client_option                                                             \
  {                                                                         \
    .base_option =                                                          \
        OPTION_STR_INIT(&client_optset_static, client_option_common_vtable, \
                        client_option_str_vtable, ocb, cbd),                \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat, .u = {                                                \
      .string = {.pvalue = gui_options->oname,                              \
                 .size = sizeof(gui_options->oname),                        \
                 .def = odef,                                               \
                 .val_accessor = nullptr}                                   \
    }                                                                       \
  }

/*
 * Generate a client option of type OT_STRING with a string accessor
 * function.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 *        Be sure to pass the array variable and not a pointer to it because
 *        the size is calculated with sizeof().
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * odef:  The default string for this client option.
 * oacc:  The string accessor where to find the allowed values of type
 *        'const struct strvec * (*) (void)'.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_STR_LIST_OPTION(oname, odesc, ohelp, ocat, odef, oacc, ocb,     \
                            cbd)                                            \
  client_option                                                             \
  {                                                                         \
    .base_option =                                                          \
        OPTION_STR_INIT(&client_optset_static, client_option_common_vtable, \
                        client_option_str_vtable, ocb, cbd),                \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat, .u = {                                                \
      .string = {.pvalue = gui_options->oname,                              \
                 .size = sizeof(gui_options->oname),                        \
                 .def = odef,                                               \
                 .val_accessor = oacc}                                      \
    }                                                                       \
  }

/*
 * Generate a client option of type OT_ENUM.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * odef:  The default value for this client option.
 * oacc:  The name accessor of type 'const struct copt_val_name * (*) (int)'.
 * ocb:   A callback function of type void (*) (struct option *) called when
 *        the option changed.
 */
#define GEN_ENUM_OPTION(oname, odesc, ohelp, ocat, odef, oacc, ocb)         \
  client_option                                                             \
  {                                                                         \
    .base_option = OPTION_ENUM_INIT(&client_optset_static,                  \
                                    client_option_common_vtable,            \
                                    client_option_enum_vtable, ocb),        \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat, .u = {                                                \
      .enumerator = {.pvalue = (int *) &gui_options->oname,                 \
                     .def = odef,                                           \
                     .support_names = nullptr, /* Set in options_init(). */ \
                     .pretty_names = nullptr,                               \
                     .name_accessor = oacc}                                 \
    }                                                                       \
  }

/*
 * Generate a client option of type OT_BITWISE.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * odef:  The default value for this client option.
 * oacc:  The name accessor of type 'const struct copt_val_name * (*) (int)'.
 * ocb:   A callback function of type void (*) (struct option *) called when
 *        the option changed.
 */
#define GEN_BITWISE_OPTION(oname, odesc, ohelp, ocat, odef, oacc, ocb)      \
  client_option                                                             \
  {                                                                         \
    .base_option = OPTION_BITWISE_INIT(&client_optset_static,               \
                                       client_option_common_vtable,         \
                                       client_option_bitwise_vtable, ocb),  \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat, .u = {                                                \
      .bitwise = {.pvalue = &gui_options->oname,                            \
                  .def = odef,                                              \
                  .support_names = nullptr, /* Set in options_init(). */    \
                  .pretty_names = nullptr,                                  \
                  .name_accessor = oacc}                                    \
    }                                                                       \
  }

/*
 * Generate a client option of type OT_FONT.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 *        Be sure to pass the array variable and not a pointer to it because
 *        the size is calculated with sizeof().
 * otgt:  The target widget style.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_FONT_OPTION(oname, otgt, odesc, ohelp, ocat, ocb)               \
  client_option                                                             \
  {                                                                         \
    .base_option = OPTION_FONT_INIT(&client_optset_static,                  \
                                    client_option_common_vtable,            \
                                    client_option_font_vtable, ocb),        \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat,                                                       \
    .u =                                                                    \
    {.font = {                                                              \
         .value = &gui_options->oname,                                      \
         .def = QFont(),                                                    \
         .target = otgt,                                                    \
     } }                                                                    \
  }

/*
 * Generate a client option of type OT_COLOR.
 *
 * oname: The option data.  Note it is used as name to be loaded or saved.
 *        So, you shouldn't change the name of this variable in any case.
 * odesc: A short description of the client option.  Should be used with the
 *        N_() macro.
 * ohelp: The help text for the client option.  Should be used with the N_()
 *        macro.
 * ocat:  The client_option_class of this client option.
 * odef_fg, odef_bg:  The default values for this client option.
 * ocb:   A callback function of type void (*)(struct option *) called when
 *        the option changed.
 */
#define GEN_COLOR_OPTION(oname, odesc, ohelp, ocat, odef_fg, odef_bg, ocb)  \
  client_option                                                             \
  {                                                                         \
    .base_option = OPTION_COLOR_INIT(&client_optset_static,                 \
                                     client_option_common_vtable,           \
                                     client_option_color_vtable, ocb),      \
    .name = #oname, .description = odesc, .help_text = ohelp,               \
    .category = ocat, .u = {                                                \
      .color = {.pvalue = &gui_options->oname,                              \
                .def = FT_COLOR(odef_fg, odef_bg)}                          \
    }                                                                       \
  }

/**
  Enumerator name accessors.
 */

// Some changed callbacks.
static void reqtree_show_icons_callback(struct option *poption);
static void view_option_changed_callback(struct option *poption);
static void manual_turn_done_callback(struct option *poption);
static void voteinfo_bar_callback(struct option *poption);
static void font_changed_callback(struct option *poption);
static void allfont_changed_callback(struct option *poption);
/* See #2237
static void mapimg_changed_callback(struct option *poption);
*/
static void game_music_enable_callback(struct option *poption);
static void menu_music_enable_callback(struct option *poption);
static void sound_volume_callback(struct option *poption);

static std::vector<client_option> client_options;
static void init_client_options()
{
  client_options = {
      GEN_STR_OPTION(
          default_user_name, N_("Login name"),
          N_("This is the default login username that will be used "
             "in the connection dialogs or with the -a command-line "
             "parameter."),
          COC_NETWORK, nullptr, nullptr, 0),
      GEN_BOOL_OPTION(
          use_prev_server, N_("Default to previously used server"),
          N_("Automatically update \"Server\" and \"Server port\" "
             "options to match your latest connection, so by "
             "default you connect to the same server you used "
             "on the previous run. You should enable "
             "saving options on exit too, so that the automatic "
             "updates to the options get saved too."),
          COC_NETWORK, false, nullptr),
      GEN_STR_OPTION(
          default_server_host, N_("Server"),
          N_("This is the default server hostname that will be used "
             "in the connection dialogs or with the -a command-line "
             "parameter."),
          COC_NETWORK, "localhost", nullptr, 0),
      GEN_INT_OPTION(
          default_server_port, N_("Server port"),
          N_("This is the default server port that will be used "
             "in the connection dialogs or with the -a command-line "
             "parameter."),
          COC_NETWORK, DEFAULT_SOCK_PORT, 0, 65535, nullptr),
      GEN_STR_OPTION(
          default_metaserver, N_("Metaserver"),
          N_("The metaserver is a host that the client contacts to "
             "find out about games on the internet.  Don't change "
             "this from its default value unless you know what "
             "you're doing."),
          COC_NETWORK, DEFAULT_METASERVER_OPTION, nullptr, 0),
      GEN_BOOL_OPTION(
          heartbeat_enabled, N_("Send heartbeat messages to server"),
          N_("Periodically send an empty heartbeat message to the "
             "server to probe whether the connection is still up. "
             "This can help to make it obvious when the server has "
             "cut the connection due to a connectivity outage, if "
             "the client would otherwise sit idle for a long period."),
          COC_NETWORK, true, nullptr),
      GEN_STR_LIST_OPTION(
          default_sound_set_name, N_("Soundset"),
          N_("This is the soundset that will be used.  Changing "
             "this is the same as using the -S command-line "
             "parameter."),
          COC_SOUND, "stdsounds", get_soundset_list, nullptr, 0),
      GEN_STR_LIST_OPTION(
          default_music_set_name, N_("Musicset"),
          N_("This is the musicset that will be used.  Changing "
             "this is the same as using the -m command-line "
             "parameter."),
          COC_SOUND, "stdmusic", get_musicset_list,
          musicspec_reread_callback, 0),
      GEN_STR_LIST_OPTION(
          default_sound_plugin_name, N_("Sound plugin"),
          N_("If you have a problem with sound, try changing "
             "the sound plugin.  The new plugin won't take "
             "effect until you restart Freeciv21.  Changing this "
             "is the same as using the -P command-line option."),
          COC_SOUND, "", get_soundplugin_list, nullptr, 0),
      GEN_STR_OPTION(default_chat_logfile, N_("The chat log file"),
                     N_("The name of the chat log file."), COC_INTERFACE,
                     "freeciv-chat.log", nullptr, 0),
      GEN_STR_LIST_OPTION(gui_qt_default_theme_name, N_("Theme"),
                          N_("By changing this option you change the "
                             "active theme."),
                          COC_GRAPHICS, FC_QT_DEFAULT_THEME_NAME,
                          get_themes_list, theme_reread_callback, 0),

      /* It's important to give empty string instead of nullptr as as default
       * value. For nullptr value it would default to assigning first value
       * from the tileset list returned by get_tileset_list() as default
       * tileset. We don't want default tileset assigned at all here, but
       * leave it to tilespec code that can handle tileset priority. */
      GEN_STR_LIST_OPTION(
          default_tileset_square_name, N_("Tileset (Square)"),
          N_("Select the tileset used with Square based maps. "
             "This may change the currently active tileset, if "
             "you are playing on such a map, in which "
             "case this is the same as using the -t "
             "command-line parameter."),
          COC_GRAPHICS, "", get_tileset_list, tilespec_reread_callback, 0),
      GEN_STR_LIST_OPTION(
          default_tileset_hex_name, N_("Tileset (Hex)"),
          N_("Select the tileset used with Hex maps. "
             "This may change the currently active tileset, if "
             "you are playing on such a map, in which "
             "case this is the same as using the -t "
             "command-line parameter."),
          COC_GRAPHICS, "", get_tileset_list, tilespec_reread_callback,
          TF_HEX),
      GEN_STR_LIST_OPTION(
          default_tileset_isohex_name, N_("Tileset (Iso-Hex)"),
          N_("Select the tileset used with Iso-Hex maps. "
             "This may change the currently active tileset, if "
             "you are playing on such a map, in which "
             "case this is the same as using the -t "
             "command-line parameter."),
          COC_GRAPHICS, "", get_tileset_list, tilespec_reread_callback,
          TF_ISO | TF_HEX),

      GEN_STR_LIST_OPTION(default_city_bar_style_name, N_("City bar style"),
                          N_("Selects the style of the city bar."),
                          COC_GRAPHICS, "Polished",
                          citybar_painter::available_vector,
                          citybar_painter::option_changed, 0),

      GEN_BOOL_OPTION(draw_city_outlines, N_("Draw city outlines"),
                      N_("Setting this option will draw a line at the city "
                         "workable limit."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_city_output, N_("Draw city output"),
          N_("Setting this option will draw city output for every "
             "citizen."),
          COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_map_grid, N_("Draw the map grid"),
          N_("Setting this option will draw a grid over the map."),
          COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_city_names, N_("Draw the city names"),
          N_("Setting this option will draw the names of the cities "
             "on the map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_city_growth, N_("Draw the city growth"),
          N_("Setting this option will draw in how many turns the "
             "cities will grow or shrink."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_city_productions, N_("Draw the city productions"),
                      N_("Setting this option will draw what the cities are "
                         "currently building on the map."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_city_buycost, N_("Draw the city buy costs"),
                      N_("Setting this option will draw how much gold is "
                         "needed to buy the production of the cities."),
                      COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_city_trade_routes,
                      N_("Draw the city trade routes"),
                      N_("Setting this option will draw trade route lines "
                         "between cities which have trade routes."),
                      COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_coastline, N_("Draw the coast line"),
          N_("Setting this option will draw a line to separate the "
             "land from the ocean."),
          COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_roads_rails,
                      N_("Draw the roads and the railroads"),
                      N_("Setting this option will draw the roads and the "
                         "railroads on the map."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_irrigation, N_("Draw the irrigation"),
          N_("Setting this option will draw the irrigation systems "
             "on the map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_mines, N_("Draw the mines"),
          N_("Setting this option will draw the mines on the map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_fortress_airbase, N_("Draw the bases"),
          N_("Setting this option will draw the bases on the map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_specials, N_("Draw the resources"),
          N_("Setting this option will draw the resources on the "
             "map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_huts, N_("Draw the huts"),
                      N_("Setting this option will draw the huts on the "
                         "map."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_pollution,
                      N_("Draw the pollution/nuclear fallout"),
                      N_("Setting this option will draw pollution and "
                         "nuclear fallout on the map."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_cities, N_("Draw the cities"),
          N_("Setting this option will draw the cities on the map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_units, N_("Draw the units"),
          N_("Setting this option will draw the units on the map."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(solid_color_behind_units,
                      N_("Solid unit background color"),
                      N_("Setting this option will cause units on the map "
                         "view to be drawn with a solid background color "
                         "instead of the flag backdrop."),
                      COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_unit_shields, N_("Draw shield graphics for units"),
          N_("Setting this option will draw a shield icon "
             "as the flags on units.  If unset, the full flag will "
             "be drawn."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_focus_unit, N_("Draw the units in focus"),
          N_("Setting this option will cause the currently focused "
             "unit(s) to always be drawn, even if units are not "
             "otherwise being drawn (for instance if 'Draw the units' "
             "is unset)."),
          COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(draw_fog_of_war, N_("Draw the fog of war"),
                      N_("Setting this option will draw the fog of war."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_borders, N_("Draw the borders"),
          N_("Setting this option will draw the national borders."),
          COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          draw_native,
          N_("Draw whether tiles are native to "
             "selected unit"),
          N_("Setting this option will highlight tiles that the "
             "currently selected unit cannot enter unaided due to "
             "non-native terrain. (If multiple units are selected, "
             "only tiles that all of them can enter are indicated.)"),
          COC_GRAPHICS, false, view_option_changed_callback),
      GEN_BOOL_OPTION(player_dlg_show_dead_players,
                      N_("Show dead players in Nations report"),
                      N_("This option controls whether defeated nations are "
                         "shown on the Nations report page."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(zoom_scale_fonts, N_("Scale fonts when zooming"),
                      N_("When this option is set, the fonts and city "
                         "descriptions will be "
                         "scaled when the map is zoomed."),
                      COC_GRAPHICS, true, view_option_changed_callback),
      GEN_BOOL_OPTION(
          sound_bell_at_new_turn, N_("Sound bell at new turn"),
          N_("Set this option to have a \"bell\" event be generated "
             "at the start of a new turn.  You can control the "
             "behavior of the \"bell\" event by editing the message "
             "options."),
          COC_SOUND, false, nullptr),
      GEN_INT_OPTION(
          smooth_move_unit_msec,
          N_("Unit movement animation time (milliseconds)"),
          N_("This option controls how long unit \"animation\" takes "
             "when a unit moves on the map view.  Set it to 0 to "
             "disable animation entirely."),
          COC_GRAPHICS, 30, 0, 2000, nullptr),
      GEN_INT_OPTION(
          smooth_center_slide_msec,
          N_("Mapview recentering time (milliseconds)"),
          N_("When the map view is recentered, it will slide "
             "smoothly over the map to its new position.  This "
             "option controls how long this slide lasts.  Set it to "
             "0 to disable mapview sliding entirely."),
          COC_GRAPHICS, 200, 0, 5000, nullptr),
      GEN_INT_OPTION(
          smooth_combat_step_msec,
          N_("Combat animation step time (milliseconds)"),
          N_("This option controls the speed of combat animation "
             "between units on the mapview.  Set it to 0 to disable "
             "animation entirely."),
          COC_GRAPHICS, 10, 0, 100, nullptr),
      GEN_BOOL_OPTION(reqtree_show_icons,
                      N_("Show icons in the technology tree"),
                      N_("Setting this option will display icons "
                         "on the technology tree diagram. Turning "
                         "this option off makes the technology tree "
                         "more compact."),
                      COC_GRAPHICS, true, reqtree_show_icons_callback),
      GEN_BOOL_OPTION(reqtree_curved_lines,
                      N_("Use curved lines in the technology tree"),
                      N_("Setting this option make the technology tree "
                         "diagram use curved lines to show technology "
                         "relations. Turning this option off causes "
                         "the lines to be drawn straight."),
                      COC_GRAPHICS, false, reqtree_show_icons_callback),
      GEN_COLOR_OPTION(
          highlight_our_names,
          N_("Color to highlight your player/user name"),
          N_("If set, your player and user name in the new chat "
             "messages will be highlighted using this color as "
             "background.  If not set, it will just not highlight "
             "anything."),
          COC_GRAPHICS, "#000000", "#FFFF00", nullptr),
      GEN_BOOL_OPTION(ai_manual_turn_done, N_("Manual Turn Done in AI mode"),
                      N_("Disable this option if you do not want to "
                         "press the Turn Done button manually when watching "
                         "an AI player."),
                      COC_INTERFACE, true, manual_turn_done_callback),
      GEN_BOOL_OPTION(auto_center_on_unit, N_("Auto center on units"),
                      N_("Set this option to have the active unit centered "
                         "automatically when the unit focus changes."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(auto_center_on_automated, N_("Show automated units"),
                      N_("Disable this option if you do not want to see "
                         "automated units autocentered and animated."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          auto_center_on_combat, N_("Auto center on combat"),
          N_("Set this option to have any combat be centered "
             "automatically.  Disabling this will speed up the time "
             "between turns but may cause you to miss combat "
             "entirely."),
          COC_INTERFACE, false, nullptr),
      GEN_BOOL_OPTION(auto_center_each_turn, N_("Auto center on new turn"),
                      N_("Set this option to have the client automatically "
                         "recenter the map on a suitable location at the "
                         "start of each turn."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(wakeup_focus, N_("Focus on awakened units"),
                      N_("Set this option to have newly awoken units be "
                         "focused automatically."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          keyboardless_goto, N_("Keyboardless goto"),
          N_("If this option is set then a goto may be initiated "
             "by left-clicking and then holding down the mouse "
             "button while dragging the mouse onto a different "
             "tile."),
          COC_INTERFACE, false, nullptr),
      GEN_BOOL_OPTION(
          goto_into_unknown, N_("Allow goto into the unknown"),
          N_("Setting this option will make the game consider "
             "moving into unknown tiles.  If not, then goto routes "
             "will detour around or be blocked by unknown tiles."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(center_when_popup_city,
                      N_("Center map when popup city"),
                      N_("Setting this option makes the mapview center on a "
                         "city when its city dialog is popped up."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          show_previous_turn_messages,
          N_("Show messages from previous turn"),
          N_("Message Window shows messages also from previous turn. "
             "This makes sure you don't miss messages received in the end "
             "of "
             "the turn, just before the window gets cleared."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          concise_city_production, N_("Concise city production"),
          N_("Set this option to make the city production (as shown "
             "in the city dialog) to be more compact."),
          COC_INTERFACE, false, nullptr),
      GEN_BOOL_OPTION(
          auto_turn_done, N_("End turn when done moving"),
          N_("Setting this option makes your turn end automatically "
             "when all your units are done moving."),
          COC_INTERFACE, false, nullptr),
      GEN_BOOL_OPTION(
          ask_city_name, N_("Prompt for city names"),
          N_("Disabling this option will make the names of newly "
             "founded cities be chosen automatically by the server."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(popup_new_cities,
                      N_("Pop up city dialog for new cities"),
                      N_("Setting this option will pop up a newly-founded "
                         "city's city dialog automatically."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          popup_actor_arrival, N_("Pop up caravan and spy actions"),
          N_("If this option is enabled, when a unit arrives at "
             "a city where it can perform an action like "
             "establishing a trade route, helping build a wonder, or "
             "establishing an embassy, a window will pop up asking "
             "which action should be performed. "
             "Disabling this option means you will have to do the "
             "action manually by pressing either 'r' (for a trade "
             "route), 'b' (for building a wonder) or 'd' (for a "
             "spy action) when the unit is in the city."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          popup_attack_actions, N_("Pop up attack questions"),
          N_("If this option is enabled, when a unit arrives at a "
             "target it can attack, a window will pop up asking "
             "which action should be performed even if an attack "
             "action is legal and no other interesting action are. "
             "This allows you to change your mind or to select an "
             "uninteresting action."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          popup_last_move_to_allied,
          N_("Pop up actions last move to allied"),
          N_("If this option is enabled the final move in a unit's"
             " orders to a tile with allied units or cities it can"
             " perform an action to is interpreted as an attempted"
             " action. This makes the action selection dialog pop up"
             " while the unit is at the adjacent tile."
             " This can, in cases where the action requires that"
             " the actor unit has moves left, save a turn."
             " The down side is that the unit remains adjacent to"
             " rather than inside the protection of an allied city"
             " or unit stack."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(enable_cursor_changes, N_("Enable cursor changing"),
                      N_("This option controls whether the client should "
                         "try to change the mouse cursor depending on what "
                         "is being pointed at, as well as to indicate "
                         "changes in the client or server state."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          separate_unit_selection, N_("Select cities before units"),
          N_("If this option is enabled, when both cities and "
             "units are present in the selection rectangle, only "
             "cities will be selected. See the help on Controls."),
          COC_INTERFACE, false, nullptr),
      GEN_BOOL_OPTION(
          unit_selection_clears_orders, N_("Clear unit orders on selection"),
          N_("Enabling this option will cause unit orders to be "
             "cleared as soon as one or more units are selected. If "
             "this option is disabled, busy units will not stop "
             "their current activity when selected. Giving them "
             "new orders will clear their current ones; pressing "
             "<space> once will clear their orders and leave them "
             "selected, and pressing <space> a second time will "
             "dismiss them."),
          COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(voteinfo_bar_use, N_("Enable vote bar"),
                      N_("If this option is turned on, the vote bar will be "
                         "displayed to show vote information."),
                      COC_GRAPHICS, true, voteinfo_bar_callback),
      GEN_BOOL_OPTION(
          voteinfo_bar_always_show, N_("Always display the vote bar"),
          N_("If this option is turned on, the vote bar will never "
             "be hidden, even if there is no running vote."),
          COC_GRAPHICS, false, voteinfo_bar_callback),
      GEN_BOOL_OPTION(
          voteinfo_bar_hide_when_not_player,
          N_("Do not show vote bar if not a player"),
          N_("If this option is enabled, the client won't show the "
             "vote bar if you are not a player."),
          COC_GRAPHICS, false, voteinfo_bar_callback),
      GEN_BOOL_OPTION(voteinfo_bar_new_at_front,
                      N_("Set new votes at front"),
                      N_("If this option is enabled, then new votes will go "
                         "to the front of the vote list."),
                      COC_GRAPHICS, false, voteinfo_bar_callback),
      GEN_BOOL_OPTION(
          autoaccept_tileset_suggestion,
          N_("Autoaccept tileset suggestions"),
          N_("If this option is enabled, any tileset suggested by "
             "the ruleset is automatically used; otherwise you "
             "are prompted to change tileset."),
          COC_GRAPHICS, false, nullptr),

      GEN_BOOL_OPTION(sound_enable_effects, N_("Enable sound effects"),
                      N_("Play sound effects, assuming there's suitable "
                         "sound plugin and soundset with the sounds."),
                      COC_SOUND, true, nullptr),
      GEN_BOOL_OPTION(
          sound_enable_game_music, N_("Enable in-game music"),
          N_("Play music during the game, assuming there's suitable "
             "sound plugin and musicset with in-game tracks."),
          COC_SOUND, true, game_music_enable_callback),
      GEN_BOOL_OPTION(
          sound_enable_menu_music, N_("Enable menu music"),
          N_("Play music while not in actual game, "
             "assuming there's suitable "
             "sound plugin and musicset with menu music tracks."),
          COC_SOUND, true, menu_music_enable_callback),
      GEN_INT_OPTION(sound_effects_volume, N_("Sound volume"),
                     N_("Volume scale from 0-100"), COC_SOUND, 100, 0, 100,
                     sound_volume_callback),

      GEN_BOOL_OPTION(
          autoaccept_soundset_suggestion,
          N_("Autoaccept soundset suggestions"),
          N_("If this option is enabled, any soundset suggested by "
             "the ruleset is automatically used."),
          COC_SOUND, false, nullptr),
      GEN_BOOL_OPTION(
          autoaccept_musicset_suggestion,
          N_("Autoaccept musicset suggestions"),
          N_("If this option is enabled, any musicset suggested by "
             "the ruleset is automatically used."),
          COC_SOUND, false, nullptr),

      GEN_BOOL_OPTION(overview.layers[OLAYER_BACKGROUND],
                      N_("Background layer"),
                      N_("The background layer of the overview shows just "
                         "ocean and land."),
                      COC_OVERVIEW, true, nullptr),
      GEN_BOOL_OPTION(overview.layers[OLAYER_RELIEF],
                      N_("Terrain relief map layer"),
                      N_("The relief layer shows all terrains on the map."),
                      COC_OVERVIEW, true, overview_redraw_callback),
      GEN_BOOL_OPTION(
          overview.layers[OLAYER_BORDERS], N_("Borders layer"),
          N_("The borders layer of the overview shows which tiles "
             "are owned by each player."),
          COC_OVERVIEW, false, overview_redraw_callback),
      GEN_BOOL_OPTION(overview.layers[OLAYER_BORDERS_ON_OCEAN],
                      N_("Borders layer on ocean tiles"),
                      N_("The borders layer of the overview are drawn on "
                         "ocean tiles as well (this may look ugly with many "
                         "islands). This option is only of interest if you "
                         "have set the option \"Borders layer\" already."),
                      COC_OVERVIEW, true, overview_redraw_callback),
      GEN_BOOL_OPTION(overview.layers[OLAYER_UNITS], N_("Units layer"),
                      N_("Enabling this will draw units on the overview."),
                      COC_OVERVIEW, true, overview_redraw_callback),
      GEN_BOOL_OPTION(overview.layers[OLAYER_CITIES], N_("Cities layer"),
                      N_("Enabling this will draw cities on the overview."),
                      COC_OVERVIEW, true, overview_redraw_callback),
      GEN_BOOL_OPTION(overview.fog, N_("Overview fog of war"),
                      N_("Enabling this will show fog of war on the "
                         "overview."),
                      COC_OVERVIEW, true, overview_redraw_callback),

      /* See #2237
      // options for map images
      GEN_STR_LIST_OPTION(mapimg_format, N_("Image format"),
                          N_("The image toolkit and file format used for "
                             "map images."),
                          COC_MAPIMG, nullptr, get_mapimg_format_list,
                          nullptr, 0),
      GEN_INT_OPTION(mapimg_zoom, N_("Zoom factor for map images"),
                     N_("The magnification used for map images."),
                     COC_MAPIMG, 2, 1, 5, mapimg_changed_callback),
      GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_AREA],
                      N_("Show area within borders"),
                      N_("If set, the territory of each nation is shown "
                         "on the saved image."),
                      COC_MAPIMG, false, mapimg_changed_callback),
      GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_BORDERS], N_("Show borders"),
                      N_("If set, the border of each nation is shown on the "
                         "saved image."),
                      COC_MAPIMG, true, mapimg_changed_callback),
      GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_CITIES], N_("Show cities"),
                      N_("If set, cities are shown on the saved image."),
                      COC_MAPIMG, true, mapimg_changed_callback),
      GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_FOGOFWAR],
                      N_("Show fog of war"),
                      N_("If set, the extent of fog of war is shown on the "
                         "saved image."),
                      COC_MAPIMG, true, mapimg_changed_callback),
      GEN_BOOL_OPTION(
          mapimg_layer[MAPIMG_LAYER_TERRAIN], N_("Show full terrain"),
          N_("If set, terrain relief is shown with different colors "
             "in the saved image; otherwise, only land and water are "
             "distinguished."),
          COC_MAPIMG, true, mapimg_changed_callback),
      GEN_BOOL_OPTION(mapimg_layer[MAPIMG_LAYER_UNITS], N_("Show units"),
                      N_("If set, units are shown in the saved image."),
                      COC_MAPIMG, true, mapimg_changed_callback),
      GEN_STR_OPTION(
          mapimg_filename, N_("Map image file name"),
          N_("The base part of the filename for saved map images. "
             "A string identifying the game turn and map options will "
             "be appended."),
          COC_MAPIMG, GUI_DEFAULT_MAPIMG_FILENAME, nullptr, 0),
      */

      GEN_BOOL_OPTION(gui_qt_fullscreen, N_("Fullscreen"),
                      N_("If this option is set the client will use the "
                         "whole screen area for drawing."),
                      COC_INTERFACE, true, nullptr),
      GEN_BOOL_OPTION(
          gui_qt_show_titlebar, N_("Show titlebar"),
          N_("If this option is set the client will show a titlebar. "
             "If disabled, then no titlebar will be shown, and "
             "minimize/maximize/etc buttons will be placed on the "
             "menu bar."),
          COC_INTERFACE, true, nullptr),
      GEN_INT_OPTION(gui_qt_increase_fonts, N_("Change all fonts size"),
                     N_("Change size of all fonts at once by given percent. "
                        "This option cannot be saved. Hit the Apply button "
                        "after changing this."),
                     COC_FONT, 0, -100, 100, allfont_changed_callback),
      GEN_FONT_OPTION(gui_qt_font_default, "default_font",
                      N_("Default font"), N_("This is default font"),
                      COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(
          gui_qt_font_notify_label, "notify_label", N_("Notify Label"),
          N_("This font is used to display server reports such "
             "as the demographic report or historian publications."),
          COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(
          gui_qt_font_help_label, "help_label", N_("Help Label"),
          N_("This font is used to display the help labels in the "
             "help window."),
          COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(
          gui_qt_font_help_text, "help_text", N_("Help Text"),
          N_("This font is used to display the help body text in "
             "the help window."),
          COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(gui_qt_font_chatline, "chatline", N_("Chatline Area"),
                      N_("This font is used to display the text in the "
                         "chatline area."),
                      COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(gui_qt_font_city_names, "city_names", N_("City Names"),
                      N_("This font is used to the display the city names "
                         "on the map."),
                      COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(gui_qt_font_city_productions, "city_productions",
                      N_("City Productions"),
                      N_("This font is used to display the city production "
                         "on the map."),
                      COC_FONT, font_changed_callback),
      GEN_FONT_OPTION(
          gui_qt_font_reqtree_text, "reqtree_text", N_("Requirement Tree"),
          N_("This font is used to the display the requirement tree "
             "in the Research report."),
          COC_FONT, font_changed_callback),
      GEN_BOOL_OPTION(gui_qt_show_preview, N_("Show savegame information"),
                      N_("If this option is set the client will show "
                         "information and map preview of current savegame."),
                      COC_GRAPHICS, true, nullptr),
  };
}

/**
   Returns the next valid option pointer for the current gui type.
 */
static struct client_option *
client_option_next_valid(struct client_option *poption)
{
  return (poption < &client_options.back() ? poption : nullptr);
}

/**
   Returns the option corresponding to this id.
 */
static struct option *client_optset_option_by_number(int id)
{
  if (id < 0 || id >= client_options.size()) {
    return nullptr;
  }
  return OPTION(&client_options[id]);
}

/**
   Returns the first valid option pointer for the current gui type.
 */
static struct option *client_optset_option_first()
{
  return OPTION(client_option_next_valid(&client_options.front()));
}

/**
   Returns the number of client option categories.
 */
static int client_optset_category_number() { return COC_MAX; }

/**
   Returns the name (translated) of the option class.
 */
static const char *client_optset_category_name(int category)
{
  switch (category) {
  case COC_GRAPHICS:
    return _("Graphics");
  case COC_OVERVIEW:
    // TRANS: Options section for the minimap
    return _("Minimap");
  case COC_SOUND:
    return _("Sound");
  case COC_INTERFACE:
    return _("Interface");
  case COC_MAPIMG:
    return _("Map Image");
  case COC_NETWORK:
    return _("Network");
  case COC_FONT:
    return _("Font");
  case COC_MAX:
    break;
  }

  qCritical("%s: invalid option category number %d.", __FUNCTION__,
            category);
  return nullptr;
}

/**
   Returns the number of this client option.
 */
static int client_option_number(const struct option *poption)
{
  return CLIENT_OPTION(poption) - &client_options.front();
}

/**
   Returns the name of this client option.
 */
static const char *client_option_name(const struct option *poption)
{
  return CLIENT_OPTION(poption)->name;
}

/**
   Returns the description of this client option.
 */
static const char *client_option_description(const struct option *poption)
{
  return _(CLIENT_OPTION(poption)->description);
}

/**
   Returns the help text for this client option.
 */
static const char *client_option_help_text(const struct option *poption)
{
  return _(CLIENT_OPTION(poption)->help_text);
}

/**
   Returns the category of this client option.
 */
static int client_option_category(const struct option *poption)
{
  return CLIENT_OPTION(poption)->category;
}

/**
   Returns TRUE if this client option can be modified.
 */
static bool client_option_is_changeable(const struct option *poption)
{
  Q_UNUSED(poption)
  return true;
}

/**
   Returns the next valid option pointer for the current gui type.
 */
static struct option *client_option_next(const struct option *poption)
{
  return OPTION(client_option_next_valid(CLIENT_OPTION(poption) + 1));
}

/**
   Returns the value of this client option of type OT_BOOLEAN.
 */
static bool client_option_bool_get(const struct option *poption)
{
  return *(CLIENT_OPTION(poption)->u.boolean.pvalue);
}

/**
   Returns the default value of this client option of type OT_BOOLEAN.
 */
static bool client_option_bool_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.boolean.def;
}

/**
   Set the value of this client option of type OT_BOOLEAN.  Returns TRUE if
   the value changed.
 */
static bool client_option_bool_set(struct option *poption, bool val)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (*pcoption->u.boolean.pvalue == val) {
    return false;
  }

  *pcoption->u.boolean.pvalue = val;
  return true;
}

/**
   Returns the value of this client option of type OT_INTEGER.
 */
static int client_option_int_get(const struct option *poption)
{
  return *(CLIENT_OPTION(poption)->u.integer.pvalue);
}

/**
   Returns the default value of this client option of type OT_INTEGER.
 */
static int client_option_int_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.integer.def;
}

/**
   Returns the minimal value for this client option of type OT_INTEGER.
 */
static int client_option_int_min(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.integer.min;
}

/**
   Returns the maximal value for this client option of type OT_INTEGER.
 */
static int client_option_int_max(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.integer.max;
}

/**
   Set the value of this client option of type OT_INTEGER.  Returns TRUE if
   the value changed.
 */
static bool client_option_int_set(struct option *poption, int val)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (val < pcoption->u.integer.min || val > pcoption->u.integer.max
      || *pcoption->u.integer.pvalue == val) {
    return false;
  }

  *pcoption->u.integer.pvalue = val;
  return true;
}

/**
   Returns the value of this client option of type OT_STRING.
 */
static const char *client_option_str_get(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.string.pvalue;
}

/**
   Returns the default value of this client option of type OT_STRING.
 */
static const char *client_option_str_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.string.def;
}

/**
   Returns the possible string values of this client option of type
   OT_STRING.
 */
static const QVector<QString> *
client_option_str_values(const struct option *poption)
{
  return (CLIENT_OPTION(poption)->u.string.val_accessor
              ? CLIENT_OPTION(poption)->u.string.val_accessor(poption)
              : nullptr);
}

/**
   Set the value of this client option of type OT_STRING.  Returns TRUE if
   the value changed.
 */
static bool client_option_str_set(struct option *poption, const char *str)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (strlen(str) >= pcoption->u.string.size
      || 0 == strcmp(pcoption->u.string.pvalue, str)) {
    return false;
  }

  const auto allowed_values = client_option_str_values(poption);
  if (allowed_values && !allowed_values->contains(str)) {
    qWarning() << "Unrecognized value for option" << option_name(poption)
               << ":" << QString(str);
    return false;
  }

  fc_strlcpy(pcoption->u.string.pvalue, str, pcoption->u.string.size);
  return true;
}

/**
   Returns the "support" name of the value for this client option of type
   OT_ENUM (a string suitable for saving in a file).
   The prototype must match the 'secfile_enum_name_data_fn_t' type.
 */
static const char *client_option_enum_secfile_str(secfile_data_t data,
                                                  int val)
{
  const QVector<QString> *names =
      CLIENT_OPTION(data)->u.enumerator.support_names;

  return (0 <= val && val < names->count() ? qUtf8Printable(names->at(val))
                                           : nullptr);
}

/**
   Returns the "support" name of a single value for this client option of
 type OT_BITWISE (a string suitable for saving in a file). The prototype must
 match the 'secfile_enum_name_data_fn_t' type.
 */
static const char *client_option_bitwise_secfile_str(secfile_data_t data,
                                                     int val)
{
  const QVector<QString> *names =
      CLIENT_OPTION(data)->u.bitwise.support_names;

  return (0 <= val && val < names->count() ? qUtf8Printable(names->at(val))
                                           : nullptr);
}

/**
   Returns the value of this client option of type OT_FONT.
 */
static QFont client_option_font_get(const struct option *poption)
{
  return *CLIENT_OPTION(poption)->u.font.value;
}

/**
   Returns the default value of this client option of type OT_FONT.
 */
static QFont client_option_font_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.font.def;
}

/**
   Returns the default value of this client option of type OT_FONT.
 */
static void client_option_font_set_def(const struct option *poption,
                                       const QFont &font)
{
  CLIENT_OPTION(poption)->u.font.def = font;
}

/**
   Returns the default value of this client option of type OT_FONT.
 */
static QString client_option_font_target(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.font.target;
}

/**
   Set the value of this client option of type OT_FONT.  Returns TRUE if
   the value changed.
 */
static bool client_option_font_set(struct option *poption, const QFont &font)
{
  struct client_option *pcoption = CLIENT_OPTION(poption);

  if (*pcoption->u.font.value == font) {
    return false;
  }

  *pcoption->u.font.value = font;
  return true;
}

/**
   Returns the value of this client option of type OT_COLOR.
 */
static struct ft_color client_option_color_get(const struct option *poption)
{
  return *CLIENT_OPTION(poption)->u.color.pvalue;
}

/**
   Returns the default value of this client option of type OT_COLOR.
 */
static struct ft_color client_option_color_def(const struct option *poption)
{
  return CLIENT_OPTION(poption)->u.color.def;
}

/**
   Set the value of this client option of type OT_COLOR.  Returns TRUE if
   the value changed.
 */
static bool client_option_color_set(struct option *poption,
                                    struct ft_color color)
{
  struct ft_color *pcolor = CLIENT_OPTION(poption)->u.color.pvalue;
  bool changed = false;

#define color_set(color_tgt, color)                                         \
  if (nullptr == color_tgt) {                                               \
    if (nullptr != color) {                                                 \
      color_tgt = fc_strdup(color);                                         \
      changed = true;                                                       \
    }                                                                       \
  } else {                                                                  \
    if (nullptr == color) {                                                 \
      delete[] color_tgt;                                                   \
      color_tgt = nullptr;                                                  \
      changed = true;                                                       \
    } else if (0 != strcmp(color_tgt, color)) {                             \
      delete[] color_tgt;                                                   \
      color_tgt = fc_strdup(color);                                         \
      changed = true;                                                       \
    }                                                                       \
  }

  color_set(pcolor->foreground, color.foreground);
  color_set(pcolor->background, color.background);

#undef color_set

  return changed;
}

/**
   Load the option from a file.  Returns TRUE if the option changed.
 */
static bool client_option_load(struct option *poption,
                               struct section_file *sf)
{
  fc_assert_ret_val(nullptr != poption, false);
  fc_assert_ret_val(nullptr != sf, false);

  switch (option_type(poption)) {
  case OT_BOOLEAN: {
    bool value;

    return (
        secfile_lookup_bool(sf, &value, "client.%s", option_name(poption))
        && option_bool_set(poption, value));
  }
  case OT_INTEGER: {
    int value;

    return (secfile_lookup_int(sf, &value, "client.%s", option_name(poption))
            && option_int_set(poption, value));
  }
  case OT_STRING: {
    const char *string;

    return (
        (string = secfile_lookup_str(sf, "client.%s", option_name(poption)))
        && option_str_set(poption, string));
  }
  case OT_ENUM: {
    int value;

    return (secfile_lookup_enum_data(sf, &value, false,
                                     client_option_enum_secfile_str, poption,
                                     "client.%s", option_name(poption))
            && option_enum_set_int(poption, value));
  }
  case OT_BITWISE: {
    int value;

    return (secfile_lookup_enum_data(
                sf, &value, true, client_option_bitwise_secfile_str, poption,
                "client.%s", option_name(poption))
            && option_bitwise_set(poption, value));
  }
  case OT_FONT: {
    const char *string =
        secfile_lookup_str(sf, "client.%s", option_name(poption));
    if (!string) {
      return false;
    }

    QFont font;
    return font.fromString(string) && option_font_set(poption, font);
  }
  case OT_COLOR: {
    struct ft_color color;

    return ((color.foreground = secfile_lookup_str(
                 sf, "client.%s.foreground", option_name(poption)))
            && (color.background = secfile_lookup_str(
                    sf, "client.%s.background", option_name(poption)))
            && option_color_set(poption, color));
  }
  }
  return false;
}

/**
   Save the option to a file.
 */
static void client_option_save(struct option *poption,
                               struct section_file *sf)
{
  fc_assert_ret(nullptr != poption);
  fc_assert_ret(nullptr != sf);

  switch (option_type(poption)) {
  case OT_BOOLEAN:
    secfile_insert_bool(sf, option_bool_get(poption), "client.%s",
                        option_name(poption));
    break;
  case OT_INTEGER:
    secfile_insert_int(sf, option_int_get(poption), "client.%s",
                       option_name(poption));
    break;
  case OT_STRING:
    secfile_insert_str(sf, option_str_get(poption), "client.%s",
                       option_name(poption));
    break;
  case OT_ENUM:
    secfile_insert_enum_data(sf, option_enum_get_int(poption), false,
                             client_option_enum_secfile_str, poption,
                             "client.%s", option_name(poption));
    break;
  case OT_BITWISE:
    secfile_insert_enum_data(sf, option_bitwise_get(poption), true,
                             client_option_bitwise_secfile_str, poption,
                             "client.%s", option_name(poption));
    break;
  case OT_FONT:
    secfile_insert_str(sf,
                       qUtf8Printable(option_font_get(poption).toString()),
                       "client.%s", option_name(poption));
    break;
  case OT_COLOR: {
    struct ft_color color = option_color_get(poption);

    secfile_insert_str(sf, color.foreground, "client.%s.foreground",
                       option_name(poption));
    secfile_insert_str(sf, color.background, "client.%s.background",
                       option_name(poption));
  } break;
  }
}

/**
  Server options variables.
 */
static char **server_options_categories = nullptr;
static struct server_option *server_options = nullptr;

static int server_options_categories_num = 0;
static int server_options_num = 0;

/**
  Server option set.
 */
static struct option *server_optset_option_by_number(int id);
static struct option *server_optset_option_first();
static int server_optset_category_number();
static const char *server_optset_category_name(int category);

static struct option_set server_optset_static = {
    .option_by_number = server_optset_option_by_number,
    .option_first = server_optset_option_first,
    .category_number = server_optset_category_number,
    .category_name = server_optset_category_name};
const struct option_set *server_optset = &server_optset_static;

/**
  Virtuals tables for the client options.
 */
static int server_option_number(const struct option *poption);
static const char *server_option_name(const struct option *poption);
static const char *server_option_description(const struct option *poption);
static const char *server_option_help_text(const struct option *poption);
static int server_option_category(const struct option *poption);
static bool server_option_is_changeable(const struct option *poption);
static struct option *server_option_next(const struct option *poption);

static const struct option_common_vtable server_option_common_vtable = {
    .number = server_option_number,
    .name = server_option_name,
    .description = server_option_description,
    .help_text = server_option_help_text,
    .category = server_option_category,
    .is_changeable = server_option_is_changeable,
    .next = server_option_next};

static bool server_option_bool_get(const struct option *poption);
static bool server_option_bool_def(const struct option *poption);
static bool server_option_bool_set(struct option *poption, bool val);

static const struct option_bool_vtable server_option_bool_vtable = {
    .get = server_option_bool_get,
    .def = server_option_bool_def,
    .set = server_option_bool_set};

static int server_option_int_get(const struct option *poption);
static int server_option_int_def(const struct option *poption);
static int server_option_int_min(const struct option *poption);
static int server_option_int_max(const struct option *poption);
static bool server_option_int_set(struct option *poption, int val);

static const struct option_int_vtable server_option_int_vtable = {
    .get = server_option_int_get,
    .def = server_option_int_def,
    .minimum = server_option_int_min,
    .maximum = server_option_int_max,
    .set = server_option_int_set};

static const char *server_option_str_get(const struct option *poption);
static const char *server_option_str_def(const struct option *poption);
static const QVector<QString> *
server_option_str_values(const struct option *poption);
static bool server_option_str_set(struct option *poption, const char *str);

static const struct option_str_vtable server_option_str_vtable = {
    .get = server_option_str_get,
    .def = server_option_str_def,
    .values = server_option_str_values,
    .set = server_option_str_set};

static int server_option_enum_get(const struct option *poption);
static int server_option_enum_def(const struct option *poption);
static const QVector<QString> *
server_option_enum_pretty(const struct option *poption);
static bool server_option_enum_set(struct option *poption, int val);

static const struct option_enum_vtable server_option_enum_vtable = {
    .get = server_option_enum_get,
    .def = server_option_enum_def,
    .values = server_option_enum_pretty,
    .set = server_option_enum_set,
    .cmp = strcmp};

static unsigned server_option_bitwise_get(const struct option *poption);
static unsigned server_option_bitwise_def(const struct option *poption);
static const QVector<QString> *
server_option_bitwise_pretty(const struct option *poption);
static bool server_option_bitwise_set(struct option *poption, unsigned val);

static const struct option_bitwise_vtable server_option_bitwise_vtable = {
    .get = server_option_bitwise_get,
    .def = server_option_bitwise_def,
    .values = server_option_bitwise_pretty,
    .set = server_option_bitwise_set};

/**
  Derived class server option, inheriting from base class option.
 */
struct server_option {
  struct option base_option; // Base structure, must be the first!

  char *name;        // Short name - used as an identifier
  char *description; // One-line description
  char *help_text;   // Paragraph-length help text
  unsigned char category;
  bool desired_sent;
  bool is_changeable;
  bool is_visible;
  enum setting_default_level setdef;

  union {
    // OT_BOOLEAN type option.
    struct {
      bool value;
      bool def;
    } boolean;
    // OT_INTEGER type option.
    struct {
      int value;
      int def, min, max;
    } integer;
    // OT_STRING type option.
    struct {
      char *value;
      char *def;
    } string;
    // OT_ENUM type option.
    struct {
      int value;
      int def;
      QVector<QString> *support_names;
      QVector<QString> *pretty_names; // untranslated
    } enumerator;
    // OT_BITWISE type option.
    struct {
      unsigned value;
      unsigned def;
      QVector<QString> *support_names;
      QVector<QString> *pretty_names; // untranslated
    } bitwise;
  };
};

#define SERVER_OPTION(poption) ((struct server_option *) (poption))

static void desired_settable_option_send(struct option *poption);

/**
   Initialize the server options (not received yet).
 */
void server_options_init()
{
  fc_assert(nullptr == server_options_categories);
  fc_assert(nullptr == server_options);
  fc_assert(0 == server_options_categories_num);
  fc_assert(0 == server_options_num);
}

/**
   Free one server option.
 */
static void server_option_free(struct server_option *poption)
{
  switch (poption->base_option.type) {
  case OT_STRING:
    delete[] poption->string.value;
    delete[] poption->string.def;
    poption->string.value = nullptr;
    poption->string.def = nullptr;
    break;

  case OT_ENUM:
    delete poption->enumerator.support_names;
    delete poption->enumerator.pretty_names;
    poption->enumerator.support_names = nullptr;
    poption->enumerator.pretty_names = nullptr;
    break;

  case OT_BITWISE:
    delete poption->bitwise.support_names;
    delete poption->bitwise.pretty_names;
    poption->bitwise.support_names = nullptr;
    poption->bitwise.pretty_names = nullptr;
    break;

  case OT_BOOLEAN:
  case OT_INTEGER:
  case OT_FONT:
  case OT_COLOR:
    break;
  }

  delete[] poption->name;
  delete[] poption->description;
  delete[] poption->help_text;
  poption->name = nullptr;
  poption->description = nullptr;
  poption->help_text = nullptr;
}

/**
   Free the server options, if already received.
 */
void server_options_free()
{
  int i;

  // Don't keep this dialog open.
  option_dialog_popdown(server_optset);

  // Free the options themselves.
  if (nullptr != server_options) {
    for (i = 0; i < server_options_num; i++) {
      server_option_free(server_options + i);
    }
    delete[] server_options;
    server_options = nullptr;
    server_options_num = 0;
  }

  // Free the categories.
  if (nullptr != server_options_categories) {
    for (i = 0; i < server_options_categories_num; i++) {
      if (nullptr != server_options_categories[i]) {
        delete[] server_options_categories[i];
        server_options_categories[i] = nullptr;
      }
    }
    delete[] server_options_categories;
    server_options_categories = nullptr;
    server_options_categories_num = 0;
  }
}

/**
   Allocate the server options and categories.
 */
void handle_server_setting_control(
    const struct packet_server_setting_control *packet)
{
  int i;

  // This packet should be received only once.
  fc_assert_ret(nullptr == server_options_categories);
  fc_assert_ret(nullptr == server_options);
  fc_assert_ret(0 == server_options_categories_num);
  fc_assert_ret(0 == server_options_num);

  // Allocate server option categories.
  if (0 < packet->categories_num) {
    server_options_categories_num = packet->categories_num;
    server_options_categories = new char *[server_options_categories_num]();

    for (i = 0; i < server_options_categories_num; i++) {
      // NB: Translate now.
      server_options_categories[i] = fc_strdup(_(packet->category_names[i]));
    }
  }

  // Allocate server options.
  if (0 < packet->settings_num) {
    server_options_num = packet->settings_num;
    server_options = new server_option[server_options_num]();
  }
}

/**
   Receive a server setting info packet.
 */
void handle_server_setting_const(
    const struct packet_server_setting_const *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(nullptr != poption);

  fc_assert(nullptr == psoption->name);
  psoption->name = fc_strdup(packet->name);
  fc_assert(nullptr == psoption->description);
  // NB: Translate now.
  psoption->description = fc_strdup(_(packet->short_help));
  fc_assert(nullptr == psoption->help_text);
  // NB: Translate now.
  psoption->help_text = fc_strdup(_(packet->extra_help));
  psoption->category = packet->category;
}

/**
  Common part of handle_server_setting_*() functions. See below.
 */
#define handle_server_setting_common(psoption, packet)                      \
  psoption->is_changeable = packet->is_changeable;                          \
  psoption->setdef = packet->setdef;                                        \
  psoption->is_visible = packet->is_visible;                                \
                                                                            \
  if (!psoption->desired_sent && psoption->is_visible                       \
      && psoption->is_changeable && is_server_running()                     \
      && packet->initial_setting) {                                         \
    /* Only send our private settings if we are running                     \
     * on a forked local server, i.e. started by the                        \
     * client with the "Start New Game" button.                             \
     * Do now override settings that are already saved to savegame          \
     * and now loaded. */                                                   \
    desired_settable_option_send(OPTION(poption));                          \
    psoption->desired_sent = true;                                          \
  }                                                                         \
                                                                            \
  /* Update the GUI. */                                                     \
  option_gui_update(poption);

/**
   Receive a boolean server setting info packet.
 */
void handle_server_setting_bool(
    const struct packet_server_setting_bool *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(nullptr != poption);

  if (nullptr == poption->common_vtable) {
    // Not initialized yet.
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_BOOLEAN;
    poption->bool_vtable = &server_option_bool_vtable;
  }
  fc_assert_ret_msg(OT_BOOLEAN == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_BOOLEAN), OT_BOOLEAN);

  if (packet->is_visible) {
    psoption->boolean.value = packet->val;
    psoption->boolean.def = packet->default_val;
  }

  handle_server_setting_common(psoption, packet);
}

/**
   Receive a integer server setting info packet.
 */
void handle_server_setting_int(
    const struct packet_server_setting_int *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(nullptr != poption);

  if (nullptr == poption->common_vtable) {
    // Not initialized yet.
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_INTEGER;
    poption->int_vtable = &server_option_int_vtable;
  }
  fc_assert_ret_msg(OT_INTEGER == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_INTEGER), OT_INTEGER);

  if (packet->is_visible) {
    psoption->integer.value = packet->val;
    psoption->integer.def = packet->default_val;
    psoption->integer.min = packet->min_val;
    psoption->integer.max = packet->max_val;
  }

  handle_server_setting_common(psoption, packet);
}

/**
   Receive a string server setting info packet.
 */
void handle_server_setting_str(
    const struct packet_server_setting_str *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(nullptr != poption);

  if (nullptr == poption->common_vtable) {
    // Not initialized yet.
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_STRING;
    poption->str_vtable = &server_option_str_vtable;
  }
  fc_assert_ret_msg(OT_STRING == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_STRING), OT_STRING);

  if (packet->is_visible) {
    if (nullptr == psoption->string.value) {
      psoption->string.value = fc_strdup(packet->val);
    } else if (0 != strcmp(packet->val, psoption->string.value)) {
      delete[] psoption->string.value;
      psoption->string.value = fc_strdup(packet->val);
    }
    if (nullptr == psoption->string.def) {
      psoption->string.def = fc_strdup(packet->default_val);
    } else if (0 != strcmp(packet->default_val, psoption->string.def)) {
      delete[] psoption->string.def;
      psoption->string.def = fc_strdup(packet->default_val);
    }
  }

  handle_server_setting_common(psoption, packet);
}

/**
   Receive an enumerator server setting info packet.
 */
void handle_server_setting_enum(
    const struct packet_server_setting_enum *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(nullptr != poption);

  if (nullptr == poption->common_vtable) {
    // Not initialized yet.
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_ENUM;
    poption->enum_vtable = &server_option_enum_vtable;
  }
  fc_assert_ret_msg(OT_ENUM == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_ENUM), OT_ENUM);

  if (packet->is_visible) {
    int i;

    psoption->enumerator.value = packet->val;
    psoption->enumerator.def = packet->default_val;

    if (nullptr == psoption->enumerator.support_names) {
      // First time we get this packet.
      fc_assert(nullptr == psoption->enumerator.pretty_names);
      psoption->enumerator.support_names = new QVector<QString>;
      psoption->enumerator.support_names->resize(packet->values_num);
      psoption->enumerator.pretty_names = new QVector<QString>;
      psoption->enumerator.pretty_names->resize(packet->values_num);
      for (i = 0; i < packet->values_num; i++) {
        psoption->enumerator.support_names->replace(
            i, packet->support_names[i]);
        // Store untranslated string from server.
        psoption->enumerator.pretty_names->replace(i,
                                                   packet->pretty_names[i]);
      }
    } else if (psoption->enumerator.support_names->count()
               != packet->values_num) {
      fc_assert(psoption->enumerator.support_names->count()
                == psoption->enumerator.pretty_names->count());
      /* The number of values have changed, we need to reset the list
       * of possible values. */
      psoption->enumerator.support_names->resize(packet->values_num);
      psoption->enumerator.pretty_names->resize(packet->values_num);
      for (i = 0; i < packet->values_num; i++) {
        psoption->enumerator.support_names->replace(
            i, packet->support_names[i]);
        // Store untranslated string from server.
        psoption->enumerator.pretty_names->replace(i,
                                                   packet->pretty_names[i]);
      }
    } else {
      /* Check if a value changed, then we need to reset the list
       * of possible values. */
      QString str;

      for (i = 0; i < packet->values_num; i++) {
        str = psoption->enumerator.pretty_names->at(i);
        if (str.isEmpty() || (str == packet->pretty_names[i])) {
          // Store untranslated string from server.
          psoption->enumerator.pretty_names->replace(
              i, packet->pretty_names[i]);
        }
        /* Support names are not visible, we don't need to check if it
         * has changed. */
        psoption->enumerator.support_names->replace(
            i, packet->support_names[i]);
      }
    }
  }

  handle_server_setting_common(psoption, packet);
}

/**
   Receive a bitwise server setting info packet.
 */
void handle_server_setting_bitwise(
    const struct packet_server_setting_bitwise *packet)
{
  struct option *poption = server_optset_option_by_number(packet->id);
  struct server_option *psoption = SERVER_OPTION(poption);

  fc_assert_ret(nullptr != poption);

  if (nullptr == poption->common_vtable) {
    // Not initialized yet.
    poption->poptset = server_optset;
    poption->common_vtable = &server_option_common_vtable;
    poption->type = OT_BITWISE;
    poption->bitwise_vtable = &server_option_bitwise_vtable;
  }
  fc_assert_ret_msg(OT_BITWISE == poption->type,
                    "Server setting \"%s\" (nb %d) has type %s (%d), "
                    "expected %s (%d)",
                    option_name(poption), option_number(poption),
                    option_type_name(poption->type), poption->type,
                    option_type_name(OT_BITWISE), OT_BITWISE);

  if (packet->is_visible) {
    int i;

    psoption->bitwise.value = packet->val;
    psoption->bitwise.def = packet->default_val;

    if (nullptr == psoption->bitwise.support_names) {
      // First time we get this packet.
      fc_assert(nullptr == psoption->bitwise.pretty_names);
      psoption->bitwise.support_names = new QVector<QString>;
      psoption->bitwise.support_names->resize(packet->bits_num);
      psoption->bitwise.pretty_names = new QVector<QString>;
      psoption->bitwise.pretty_names->resize(packet->bits_num);
      for (i = 0; i < packet->bits_num; i++) {
        psoption->bitwise.support_names->replace(i,
                                                 packet->support_names[i]);
        // Store untranslated string from server.
        psoption->bitwise.pretty_names->replace(i, packet->pretty_names[i]);
      }
    } else if (psoption->bitwise.support_names->count()
               != packet->bits_num) {
      fc_assert(psoption->bitwise.support_names->count()
                == psoption->bitwise.pretty_names->count());
      /* The number of values have changed, we need to reset the list
       * of possible values. */
      psoption->bitwise.support_names->resize(packet->bits_num);
      psoption->bitwise.pretty_names->resize(packet->bits_num);
      for (i = 0; i < packet->bits_num; i++) {
        psoption->bitwise.support_names->replace(i,
                                                 packet->support_names[i]);
        // Store untranslated string from server.
        psoption->bitwise.pretty_names->replace(i, packet->pretty_names[i]);
      }
    } else {
      /* Check if a value changed, then we need to reset the list
       * of possible values. */
      QString str;

      for (i = 0; i < packet->bits_num; i++) {
        str = psoption->bitwise.pretty_names->at(i);
        if (str.isEmpty() || (str != packet->pretty_names[i])) {
          // Store untranslated string from server.
          psoption->bitwise.pretty_names->replace(i,
                                                  packet->pretty_names[i]);
        }
        /* Support names are not visible, we don't need to check if it
         * has changed. */
        psoption->bitwise.support_names->replace(i,
                                                 packet->support_names[i]);
      }
    }
  }

  handle_server_setting_common(psoption, packet);
}

/**
   Returns the next valid option pointer for the current gui type.
 */
static struct server_option *
server_option_next_valid(struct server_option *poption)
{
  const struct server_option *const max =
      server_options + server_options_num;

  while (nullptr != poption && poption < max && !poption->is_visible) {
    poption++;
  }

  return (poption < max ? poption : nullptr);
}

/**
   Returns the server option associated to the number
 */
struct option *server_optset_option_by_number(int id)
{
  if (0 > id || id > server_options_num) {
    return nullptr;
  }
  return OPTION(server_options + id);
}

/**
   Returns the first valid (visible) option pointer.
 */
struct option *server_optset_option_first()
{
  return OPTION(server_option_next_valid(server_options));
}

/**
   Returns the number of server option categories.
 */
int server_optset_category_number() { return server_options_categories_num; }

/**
   Returns the name (translated) of the server option category.
 */
const char *server_optset_category_name(int category)
{
  if (0 > category || category >= server_options_categories_num) {
    return nullptr;
  }

  return server_options_categories[category];
}

/**
   Returns the number of this server option.
 */
static int server_option_number(const struct option *poption)
{
  return SERVER_OPTION(poption) - server_options;
}

/**
   Returns the name of this server option.
 */
static const char *server_option_name(const struct option *poption)
{
  return SERVER_OPTION(poption)->name;
}

/**
   Returns the (translated) description of this server option.
 */
static const char *server_option_description(const struct option *poption)
{
  return SERVER_OPTION(poption)->description;
}

/**
   Returns the (translated) help text for this server option.
 */
static const char *server_option_help_text(const struct option *poption)
{
  return SERVER_OPTION(poption)->help_text;
}

/**
   Returns the category of this server option.
 */
static int server_option_category(const struct option *poption)
{
  return SERVER_OPTION(poption)->category;
}

/**
   Returns TRUE if this client option can be modified.
 */
static bool server_option_is_changeable(const struct option *poption)
{
  return SERVER_OPTION(poption)->is_changeable;
}

/**
   Returns the next valid (visible) option pointer.
 */
static struct option *server_option_next(const struct option *poption)
{
  return OPTION(server_option_next_valid(SERVER_OPTION(poption) + 1));
}

/**
   Returns the value of this server option of type OT_BOOLEAN.
 */
static bool server_option_bool_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->boolean.value;
}

/**
   Returns the default value of this server option of type OT_BOOLEAN.
 */
static bool server_option_bool_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->boolean.def;
}

/**
   Set the value of this server option of type OT_BOOLEAN.  Returns TRUE if
   the value changed.
 */
static bool server_option_bool_set(struct option *poption, bool val)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (psoption->boolean.value == val) {
    return false;
  }

  send_chat_printf("/set %s %s", psoption->name,
                   val ? "enabled" : "disabled");
  return true;
}

/**
   Returns the value of this server option of type OT_INTEGER.
 */
static int server_option_int_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.value;
}

/**
   Returns the default value of this server option of type OT_INTEGER.
 */
static int server_option_int_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.def;
}

/**
   Returns the minimal value for this server option of type OT_INTEGER.
 */
static int server_option_int_min(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.min;
}

/**
   Returns the maximal value for this server option of type OT_INTEGER.
 */
static int server_option_int_max(const struct option *poption)
{
  return SERVER_OPTION(poption)->integer.max;
}

/**
   Set the value of this server option of type OT_INTEGER.  Returns TRUE if
   the value changed.
 */
static bool server_option_int_set(struct option *poption, int val)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (val < psoption->integer.min || val > psoption->integer.max
      || psoption->integer.value == val) {
    return false;
  }

  send_chat_printf("/set %s %d", psoption->name, val);
  return true;
}

/**
   Returns the value of this server option of type OT_STRING.
 */
static const char *server_option_str_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->string.value;
}

/**
   Returns the default value of this server option of type OT_STRING.
 */
static const char *server_option_str_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->string.def;
}

/**
   Returns the possible string values of this server option of type
   OT_STRING.
 */
static const QVector<QString> *
server_option_str_values(const struct option *poption)
{
  Q_UNUSED(poption)
  return nullptr;
}

/**
   Set the value of this server option of type OT_STRING.  Returns TRUE if
   the value changed.
 */
static bool server_option_str_set(struct option *poption, const char *str)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (0 == strcmp(psoption->string.value, str)) {
    return false;
  }

  send_chat_printf("/set %s \"%s\"", psoption->name, str);
  return true;
}

/**
   Returns the current value of this server option of type OT_ENUM.
 */
static int server_option_enum_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->enumerator.value;
}

/**
   Returns the default value of this server option of type OT_ENUM.
 */
static int server_option_enum_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->enumerator.def;
}

/**
   Returns the user-visible, translatable (but untranslated) "pretty" names
   of this server option of type OT_ENUM.
 */
static const QVector<QString> *
server_option_enum_pretty(const struct option *poption)
{
  return SERVER_OPTION(poption)->enumerator.pretty_names;
}

/**
   Set the value of this server option of type OT_ENUM.  Returns TRUE if
   the value changed.
 */
static bool server_option_enum_set(struct option *poption, int val)
{
  struct server_option *psoption = SERVER_OPTION(poption);

  if (val == psoption->enumerator.value
      || val >= psoption->enumerator.support_names->size()) {
    return false;
  }

  send_chat_printf(
      "/set %s \"%s\"", psoption->name,
      qUtf8Printable(psoption->enumerator.support_names->at(val)));
  return true;
}

/**
   Returns the long support names of the values of the server option of type
   OT_ENUM.
 */
static void server_option_enum_support_name(const struct option *poption,
                                            QString *pvalue,
                                            QString *pdefault)
{
  const struct server_option *psoption = SERVER_OPTION(poption);
  const QVector<QString> *values = psoption->enumerator.support_names;

  if (nullptr != pvalue) {
    *pvalue = values->at(psoption->enumerator.value);
  }
  if (nullptr != pdefault) {
    *pdefault = values->at(psoption->enumerator.def);
  }
}

/**
   Returns the current value of this server option of type OT_BITWISE.
 */
static unsigned server_option_bitwise_get(const struct option *poption)
{
  return SERVER_OPTION(poption)->bitwise.value;
}

/**
   Returns the default value of this server option of type OT_BITWISE.
 */
static unsigned server_option_bitwise_def(const struct option *poption)
{
  return SERVER_OPTION(poption)->bitwise.def;
}

/**
   Returns the user-visible, translatable (but untranslated) "pretty" names
   of this server option of type OT_BITWISE.
 */
static const QVector<QString> *
server_option_bitwise_pretty(const struct option *poption)
{
  return SERVER_OPTION(poption)->bitwise.pretty_names;
}

/**
   Compute the long support names of a value.
 */
static void
server_option_bitwise_support_base(const QVector<QString> *values,
                                   unsigned val, char *buf, size_t buf_len)
{
  int bit;

  buf[0] = '\0';
  for (bit = 0; bit < values->count(); bit++) {
    if ((1 << bit) & val) {
      if ('\0' != buf[0]) {
        fc_strlcat(buf, "|", buf_len);
      }
      fc_strlcat(buf, qUtf8Printable(values->at(bit)), buf_len);
    }
  }
}

/**
   Set the value of this server option of type OT_BITWISE.  Returns TRUE if
   the value changed.
 */
static bool server_option_bitwise_set(struct option *poption, unsigned val)
{
  struct server_option *psoption = SERVER_OPTION(poption);
  char name[MAX_LEN_MSG];

  if (val == psoption->bitwise.value) {
    return false;
  }

  server_option_bitwise_support_base(psoption->bitwise.support_names, val,
                                     name, sizeof(name));
  send_chat_printf("/set %s \"%s\"", psoption->name, name);
  return true;
}

/**
   Compute the long support names of the values of the server option of type
   OT_BITWISE.
 */
static void server_option_bitwise_support_name(const struct option *poption,
                                               char *val_buf, size_t val_len,
                                               char *def_buf, size_t def_len)
{
  const struct server_option *psoption = SERVER_OPTION(poption);
  const QVector<QString> *values = psoption->bitwise.support_names;

  if (nullptr != val_buf && 0 < val_len) {
    server_option_bitwise_support_base(values, psoption->bitwise.value,
                                       val_buf, val_len);
  }
  if (nullptr != def_buf && 0 < def_len) {
    server_option_bitwise_support_base(values, psoption->bitwise.def,
                                       def_buf, def_len);
  }
}

/** Message Options: **/

int messages_where[E_COUNT];

/**
   These could be a static table initialisation, except
   its easier to do it this way.
 */
static void message_options_init()
{
  int none[] = {E_IMP_BUY,
                E_IMP_SOLD,
                E_UNIT_BUY,
                E_GAME_START,
                E_CITY_BUILD,
                E_NEXT_YEAR,
                E_CITY_PRODUCTION_CHANGED,
                E_CITY_MAY_SOON_GROW,
                E_WORKLIST,
                E_AI_DEBUG};
  int out_only[] = {
      E_NATION_SELECTED, E_CHAT_MSG,      E_CHAT_ERROR,   E_CONNECTION,
      E_LOG_ERROR,       E_SETTING,       E_VOTE_NEW,     E_VOTE_RESOLVED,
      E_VOTE_ABORTED,    E_UNIT_LOST_ATT, E_UNIT_WIN_ATT,
  };
  int all[] = {E_LOG_FATAL, E_SCRIPT, E_DEPRECATION_WARNING, E_MESSAGE_WALL};
  int i;

  for (i = 0; i <= event_type_max(); i++) {
    // Include possible undefined values.
    messages_where[i] = MW_MESSAGES;
  }
  for (i = 0; i < ARRAY_SIZE(none); i++) {
    messages_where[none[i]] = 0;
  }
  for (i = 0; i < ARRAY_SIZE(out_only); i++) {
    messages_where[out_only[i]] = MW_OUTPUT;
  }
  for (i = 0; i < ARRAY_SIZE(all); i++) {
    messages_where[all[i]] = MW_MESSAGES | MW_POPUP;
  }

  events_init();
}

/**
   Free resources allocated for message options system
 */
static void message_options_free() { events_free(); }

/**
   Load the message options; use the function defined by
   specnum.h (see also events.h).
 */
static void message_options_load(struct section_file *file,
                                 const char *prefix)
{
  enum event_type event;
  int i, num_events;
  const char *p;

  if (!secfile_lookup_int(file, &num_events, "messages.count")) {
    // version < 2.2
    // Order of the events in 2.1.
    const enum event_type old_events[] = {E_CITY_CANTBUILD,
                                          E_CITY_LOST,
                                          E_CITY_LOVE,
                                          E_CITY_DISORDER,
                                          E_CITY_FAMINE,
                                          E_CITY_FAMINE_FEARED,
                                          E_CITY_GROWTH,
                                          E_CITY_MAY_SOON_GROW,
                                          E_CITY_IMPROVEMENT,
                                          E_CITY_IMPROVEMENT_BLDG,
                                          E_CITY_NORMAL,
                                          E_CITY_NUKED,
                                          E_CITY_CMA_RELEASE,
                                          E_CITY_GRAN_THROTTLE,
                                          E_CITY_TRANSFER,
                                          E_CITY_BUILD,
                                          E_CITY_PRODUCTION_CHANGED,
                                          E_WORKLIST,
                                          E_UPRISING,
                                          E_CIVIL_WAR,
                                          E_ANARCHY,
                                          E_FIRST_CONTACT,
                                          E_NEW_GOVERNMENT,
                                          E_LOW_ON_FUNDS,
                                          E_POLLUTION,
                                          E_REVOLT_DONE,
                                          E_REVOLT_START,
                                          E_SPACESHIP,
                                          E_MY_DIPLOMAT_BRIBE,
                                          E_DIPLOMATIC_INCIDENT,
                                          E_MY_DIPLOMAT_ESCAPE,
                                          E_MY_DIPLOMAT_EMBASSY,
                                          E_MY_DIPLOMAT_FAILED,
                                          E_MY_DIPLOMAT_INCITE,
                                          E_MY_DIPLOMAT_POISON,
                                          E_MY_DIPLOMAT_SABOTAGE,
                                          E_MY_DIPLOMAT_THEFT,
                                          E_ENEMY_DIPLOMAT_BRIBE,
                                          E_ENEMY_DIPLOMAT_EMBASSY,
                                          E_ENEMY_DIPLOMAT_FAILED,
                                          E_ENEMY_DIPLOMAT_INCITE,
                                          E_ENEMY_DIPLOMAT_POISON,
                                          E_ENEMY_DIPLOMAT_SABOTAGE,
                                          E_ENEMY_DIPLOMAT_THEFT,
                                          E_CARAVAN_ACTION,
                                          E_SCRIPT,
                                          E_BROADCAST_REPORT,
                                          E_GAME_END,
                                          E_GAME_START,
                                          E_NATION_SELECTED,
                                          E_DESTROYED,
                                          E_REPORT,
                                          E_TURN_BELL,
                                          E_NEXT_YEAR,
                                          E_GLOBAL_ECO,
                                          E_NUKE,
                                          E_HUT_BARB,
                                          E_HUT_CITY,
                                          E_HUT_GOLD,
                                          E_HUT_BARB_KILLED,
                                          E_HUT_MERC,
                                          E_HUT_SETTLER,
                                          E_HUT_TECH,
                                          E_HUT_BARB_CITY_NEAR,
                                          E_IMP_BUY,
                                          E_IMP_BUILD,
                                          E_IMP_AUCTIONED,
                                          E_IMP_AUTO,
                                          E_IMP_SOLD,
                                          E_TECH_GAIN,
                                          E_TECH_LEARNED,
                                          E_TREATY_ALLIANCE,
                                          E_TREATY_BROKEN,
                                          E_TREATY_CEASEFIRE,
                                          E_TREATY_PEACE,
                                          E_TREATY_SHARED_VISION,
                                          E_UNIT_LOST_ATT,
                                          E_UNIT_WIN_ATT,
                                          E_UNIT_BUY,
                                          E_UNIT_BUILT,
                                          E_UNIT_LOST_DEF,
                                          E_UNIT_WIN_DEF,
                                          E_UNIT_BECAME_VET,
                                          E_UNIT_UPGRADED,
                                          E_UNIT_RELOCATED,
                                          E_UNIT_ORDERS,
                                          E_UNIT_WAKE,
                                          E_WONDER_BUILD,
                                          E_WONDER_OBSOLETE,
                                          E_WONDER_STARTED,
                                          E_WONDER_STOPPED,
                                          E_WONDER_WILL_BE_BUILT,
                                          E_DIPLOMACY,
                                          E_TREATY_EMBASSY,
                                          E_BAD_COMMAND,
                                          E_SETTING,
                                          E_CHAT_MSG,
                                          E_MESSAGE_WALL,
                                          E_CHAT_ERROR,
                                          E_CONNECTION,
                                          E_AI_DEBUG};
    const size_t old_events_num = ARRAY_SIZE(old_events);

    for (i = 0; i < old_events_num; i++) {
      messages_where[old_events[i]] =
          secfile_lookup_int_default(file, messages_where[old_events[i]],
                                     "%s.message_where_%02d", prefix, i);
    }
    return;
  }

  for (i = 0; i < num_events; i++) {
    p = secfile_lookup_str(file, "messages.event%d.name", i);
    if (nullptr == p) {
      qCritical("Corruption in file %s: %s", secfile_name(file),
                secfile_error());
      continue;
    }
    // Compatibility: Before 3.0 E_UNIT_WIN_DEF was called E_UNIT_WIN.
    if (!fc_strcasecmp("E_UNIT_WIN", p)) {
      qCWarning(deprecations_category,
                _("Deprecated event type E_UNIT_WIN in client options."));
      p = "E_UNIT_WIN_DEF";
    }
    // Compatibility: Before 3.1 E_CITY_IMPROVEMENT was called
    // E_CITY_AQUEDUCT
    if (!fc_strcasecmp("E_CITY_AQUEDUCT", p)) {
      qCWarning(
          deprecations_category,
          _("Deprecated event type E_CITY_AQUEDUCT in client options."));
      p = "E_CITY_IMPROVEMENT";
    }
    // Compatibility: Before 3.1 E_CITY_IMPROVEMENT_BLDG was called
    // E_CITY_AQ_BUILDING
    if (!fc_strcasecmp("E_CITY_AQ_BUILDING", p)) {
      qCWarning(
          deprecations_category,
          _("Deprecated event type E_CITY_AQ_BUILDING in client options."));
      p = "E_CITY_IMPROVEMENT_BLDG";
    }
    event = event_type_by_name(p, strcmp);
    if (!event_type_is_valid(event)) {
      qCritical("Event not supported: %s", p);
      continue;
    }

    if (!secfile_lookup_int(file, &messages_where[event],
                            "messages.event%d.where", i)) {
      qCritical("Corruption in file %s: %s", secfile_name(file),
                secfile_error());
    }
  }
}

/**
   Save the message options; use the function defined by
   specnum.h (see also events.h).
 */
static void message_options_save(struct section_file *file,
                                 const char *prefix)
{
  Q_UNUSED(prefix)
  enum event_type event;
  int i = 0;

  for (event = event_type_begin(); event != event_type_end();
       event = event_type_next(event)) {
    secfile_insert_str(file, event_type_name(event), "messages.event%d.name",
                       i);
    secfile_insert_int(file, messages_where[i], "messages.event%d.where", i);
    i++;
  }

  secfile_insert_int(file, i, "messages.count");
}

/**
   Does heavy lifting for looking up a preset.
 */
static void load_cma_preset(struct section_file *file, int i)
{
  struct cm_parameter parameter;
  const char *name =
      secfile_lookup_str_default(file, "preset", "cma.preset%d.name", i);

  output_type_iterate(o)
  {
    parameter.minimal_surplus[o] =
        secfile_lookup_int_default(file, 0, "cma.preset%d.minsurp%d", i, o);
    parameter.factor[o] =
        secfile_lookup_int_default(file, 0, "cma.preset%d.factor%d", i, o);
  }
  output_type_iterate_end;
  parameter.max_growth =
      secfile_lookup_bool_default(file, false, "cma.preset%d.max_growth", i);
  parameter.require_happy =
      secfile_lookup_bool_default(file, false, "cma.preset%d.reqhappy", i);
  parameter.happy_factor =
      secfile_lookup_int_default(file, 0, "cma.preset%d.happyfactor", i);
  parameter.allow_disorder = secfile_lookup_bool_default(
      file, false, "cma.preset%d.allow_disorder", i);
  parameter.allow_specialists = secfile_lookup_bool_default(
      file, true, "cma.preset%d.allow_specialists", i);

  cmafec_preset_add(name, &parameter);
}

/**
   Does heavy lifting for inserting a preset.
 */
static void save_cma_preset(struct section_file *file, int i)
{
  const struct cm_parameter *const pparam = cmafec_preset_get_parameter(i);
  char *name = cmafec_preset_get_descr(i);

  secfile_insert_str(file, name, "cma.preset%d.name", i);

  output_type_iterate(o)
  {
    secfile_insert_int(file, pparam->minimal_surplus[o],
                       "cma.preset%d.minsurp%d", i, o);
    secfile_insert_int(file, pparam->factor[o], "cma.preset%d.factor%d", i,
                       o);
  }
  output_type_iterate_end;
  secfile_insert_bool(file, pparam->require_happy, "cma.preset%d.reqhappy",
                      i);
  secfile_insert_int(file, pparam->happy_factor, "cma.preset%d.happyfactor",
                     i);
  secfile_insert_bool(file, pparam->max_growth, "cma.preset%d.max_growth",
                      i);
  secfile_insert_bool(file, pparam->allow_disorder,
                      "cma.preset%d.allow_disorder", i);
  secfile_insert_bool(file, pparam->allow_specialists,
                      "cma.preset%d.allow_specialists", i);
}

/**
   Insert all cma presets.
 */
static void save_cma_presets(struct section_file *file)
{
  int i;

  secfile_insert_int_comment(file, cmafec_preset_num(),
                             _("If you add a preset by hand,"
                               " also update \"number_of_presets\""),
                             "cma.number_of_presets");
  for (i = 0; i < cmafec_preset_num(); i++) {
    save_cma_preset(file, i);
  }
}

// Old rc file name.
#define OLD_OPTION_FILE_NAME ".civclientrc"
// New rc file name.
#define MID_OPTION_FILE_NAME ".freeciv-client-rc-%d.%d"
#define NEW_OPTION_FILE_NAME "freeciv-client-rc-%d.%d"
#if MINOR_VERSION >= 90
#define MAJOR_NEW_OPTION_FILE_NAME (MAJOR_VERSION + 1)
#define MINOR_NEW_OPTION_FILE_NAME 0
#else // MINOR_VERSION < 90
#define MAJOR_NEW_OPTION_FILE_NAME MAJOR_VERSION
#if IS_DEVEL_VERSION
#define MINOR_NEW_OPTION_FILE_NAME (MINOR_VERSION + 1)
#else
#define MINOR_NEW_OPTION_FILE_NAME MINOR_VERSION
#endif // IS_DEVEL_VERSION
#endif // MINOR_VERSION >= 90
// The first version the new option name appeared (2.6).
#define FIRST_MAJOR_NEW_OPTION_FILE_NAME 2
#define FIRST_MINOR_NEW_OPTION_FILE_NAME 6
// The first version the mid option name appeared (2.2).
#define FIRST_MAJOR_MID_OPTION_FILE_NAME 2
#define FIRST_MINOR_MID_OPTION_FILE_NAME 2
// The first version the new boolean values appeared (2.3).
#define FIRST_MAJOR_NEW_BOOLEAN 2
#define FIRST_MINOR_NEW_BOOLEAN 3

/**
   Returns pointer to static memory containing name of the current
   option file.  Usually used for saving.
   Ie, based on FREECIV_OPT env var, and freeciv storage root dir.
   (or a OPTION_FILE_NAME define defined in fc_config.h)
   Or nullptr if problem.
 */
static const char *get_current_option_file_name()
{
  static char name_buffer[256];

  auto name = QString::fromLocal8Bit(qgetenv("FREECIV_OPT"));

  if (!name.isEmpty()) {
    sz_strlcpy(name_buffer, qUtf8Printable(name));
  } else {
#ifdef OPTION_FILE_NAME
    fc_strlcpy(name_buffer, OPTION_FILE_NAME, sizeof(name_buffer));
#else
    name = freeciv_storage_dir();
    if (name.isEmpty()) {
      qCritical(_("Cannot find Freeciv21 storage directory"));
      return nullptr;
    }
    fc_snprintf(name_buffer, sizeof(name_buffer),
                "%s/freeciv-client-rc-%d.%d", qUtf8Printable(name),
                MAJOR_NEW_OPTION_FILE_NAME, MINOR_NEW_OPTION_FILE_NAME);
#endif // OPTION_FILE_NAME
  }
  qDebug("settings file is %s", name_buffer);
  return name_buffer;
}

/**
   Check the last option file we saved. Usually used to load. Ie, based on
   FREECIV_OPT env var, and home dir. (or a OPTION_FILE_NAME define defined
   in fc_config.h), or nullptr if not found.

   Set in allow_digital_boolean if we should look for old boolean values
   (saved as 0 and 1), so if the rc file version is older than 2.3.0.
 */
static const char *get_last_option_file_name(bool *allow_digital_boolean)
{
  static char name_buffer[256];
  static int last_minors[] = {
      0,  // There was no 0.x releases
      14, // 1.14
      7   // 2.7
  };

#if MINOR_VERSION >= 90
  FC_STATIC_ASSERT(MAJOR_VERSION < sizeof(last_minors) / sizeof(int),
                   missing_last_minor);
#else
  FC_STATIC_ASSERT(MAJOR_VERSION <= sizeof(last_minors) / sizeof(int),
                   missing_last_minor);
#endif

  *allow_digital_boolean = false;

  auto name = QString::fromLocal8Bit(qgetenv("FREECIV_OPT"));

  if (!name.isEmpty()) {
    sz_strlcpy(name_buffer, qUtf8Printable(name));
  } else {
#ifdef OPTION_FILE_NAME
    fc_strlcpy(name_buffer, OPTION_FILE_NAME, sizeof(name_buffer));
#else
    int major, minor;
    struct stat buf;

    name = freeciv_storage_dir();
    if (name.isEmpty()) {
      qCritical(_("Cannot find Freeciv21 storage directory"));

      return nullptr;
    }

    for (major = MAJOR_NEW_OPTION_FILE_NAME,
        minor = MINOR_NEW_OPTION_FILE_NAME;
         major >= FIRST_MAJOR_NEW_OPTION_FILE_NAME; major--) {
      for (; (major == FIRST_MAJOR_NEW_OPTION_FILE_NAME
                  ? minor >= FIRST_MINOR_NEW_OPTION_FILE_NAME
                  : minor >= 0);
           minor--) {
        fc_snprintf(name_buffer, sizeof(name_buffer),
                    "%s/freeciv-client-rc-%d.%d", qUtf8Printable(name),
                    major, minor);
        if (0 == fc_stat(name_buffer, &buf)) {
          if (MAJOR_NEW_OPTION_FILE_NAME != major
              || MINOR_NEW_OPTION_FILE_NAME != minor) {
            qInfo(_("Didn't find '%s' option file, "
                    "loading from '%s' instead."),
                  get_current_option_file_name(), name_buffer);
          }

          return name_buffer;
        }
      }
      minor = last_minors[major - 1];
    }

    /* minor having max value of FIRST_MINOR_NEW_OPTION_FILE_NAME
     * works since MID versioning scheme was used within major version 2
     * only (2.2 - 2.6) so the last minor is bigger than any earlier minor.
     */
    for (major = FIRST_MAJOR_MID_OPTION_FILE_NAME,
        minor = FIRST_MINOR_NEW_OPTION_FILE_NAME;
         minor >= FIRST_MINOR_MID_OPTION_FILE_NAME; minor--) {
      fc_snprintf(name_buffer, sizeof(name_buffer),
                  "%s/.freeciv-client-rc-%d.%d",
                  qUtf8Printable(QDir::homePath()), major, minor);
      if (0 == fc_stat(name_buffer, &buf)) {
        qInfo(_("Didn't find '%s' option file, "
                "loading from '%s' instead."),
              get_current_option_file_name()
                  + qstrlen(qUtf8Printable(QDir::homePath())) + 1,
              name_buffer);

        if (FIRST_MINOR_NEW_BOOLEAN > minor) {
          *allow_digital_boolean = true;
        }
        return name_buffer;
      }
    }

    // Try with the old one.
    fc_snprintf(name_buffer, sizeof(name_buffer), "%s/%s",
                qUtf8Printable(name), OLD_OPTION_FILE_NAME);
    if (0 == fc_stat(name_buffer, &buf)) {
      qInfo(_("Didn't find '%s' option file, "
              "loading from '%s' instead."),
            get_current_option_file_name(), OLD_OPTION_FILE_NAME);
      *allow_digital_boolean = true;
      return name_buffer;
    } else {
      return nullptr;
    }
#endif // OPTION_FILE_NAME
  }
  qDebug("settings file is %s", name_buffer);
  return name_buffer;
}
#undef OLD_OPTION_FILE_NAME
#undef MID_OPTION_FILE_NAME
#undef NEW_OPTION_FILE_NAME
#undef FIRST_MAJOR_NEW_OPTION_FILE_NAME
#undef FIRST_MINOR_NEW_OPTION_FILE_NAME
#undef FIRST_MAJOR_MID_OPTION_FILE_NAME
#undef FIRST_MINOR_MID_OPTION_FILE_NAME
#undef FIRST_MINOR_NEW_BOOLEAN

Q_GLOBAL_STATIC(optionsHash, settable_options)

/**
 * Migrate players using cimpletoon/toonhex to amplio2/hexemplio with the
 * cimpletoon option enabled.
 *
 * \since 3.1
 */
static void
tileset_options_migrate_cimpletoon(struct client_options *options)
{
  for (auto &name : {options->default_tileset_iso_name,
                     options->default_tileset_isohex_name,
                     options->default_tileset_square_name}) {
    if (name == QStringLiteral("cimpletoon")) {
      fc_strlcpy(name, "amplio2", sizeof(name));
      options->tileset_options[QStringLiteral("amplio2")]
                              [QStringLiteral("cimpletoon")] = true;
    } else if (name == QStringLiteral("toonhex")) {
      fc_strlcpy(name, "hexemplio", sizeof(name));
      options->tileset_options[QStringLiteral("hexemplio")]
                              [QStringLiteral("cimpletoon")] = true;
    }
  }
}

/**
 * Load tileset options.
 *
 * Every tileset has its own section called tileset_xxx. The options are
 * saved as name=value pairs.
 * \see tileset_options_save
 */
static void tileset_options_load(struct section_file *sf,
                                 struct client_options *options)
{
  // Gather all tileset_xxx sections
  auto sections =
      secfile_sections_by_name_prefix(sf, TILESET_OPTIONS_PREFIX);
  if (!sections) {
    return;
  }

  section_list_iterate(sections, psection)
  {
    // Extract the tileset name from the name of the section.
    auto tileset_name =
        section_name(psection) + strlen(TILESET_OPTIONS_PREFIX);

    // Get all values from the section and fill a map with them.
    auto entries = section_entries(psection);
    auto settings = std::map<QString, bool>();
    entry_list_iterate(entries, pentry)
    {
      bool value = true;
      if (entry_bool_get(pentry, &value)) {
        settings[entry_name(pentry)] = value;
      } else {
        // Ignore options we can't convert to a bool, but warn the user.
        qWarning("Could not load option %s for tileset %s",
                 entry_name(pentry), tileset_name);
      }
    }
    entry_list_iterate_end;

    // Store the loaded options for later use.
    options->tileset_options[tileset_name] = settings;
  }
  section_list_iterate_end;
}

/**
 * Save tileset options.
 *
 * \see tileset_options_load
 */
static void tileset_options_save(struct section_file *sf,
                                 const struct client_options *options)
{
  for (const auto &[tileset, settings] : options->tileset_options) {
    for (const auto &[name, value] : settings) {
      secfile_insert_bool(sf, value, "%s%s.%s", TILESET_OPTIONS_PREFIX,
                          qUtf8Printable(tileset), qUtf8Printable(name));
    }
  }
}

/**
   Load the server options.
 */
static void settable_options_load(struct section_file *sf)
{
  char buf[64];
  const struct section *psection;
  const struct entry_list *entries;
  const char *string;
  bool bval;
  int ival;

  settable_options->clear();

  psection = secfile_section_by_name(sf, "server");
  if (nullptr == psection) {
    // Does not exist!
    return;
  }

  entries = section_entries(psection);
  entry_list_iterate(entries, pentry)
  {
    string = nullptr;
    switch (entry_type_get(pentry)) {
    case ENTRY_BOOL:
      if (entry_bool_get(pentry, &bval)) {
        fc_strlcpy(buf, bval ? "enabled" : "disabled", sizeof(buf));
        string = buf;
      }
      break;

    case ENTRY_INT:
      if (entry_int_get(pentry, &ival)) {
        fc_snprintf(buf, sizeof(buf), "%d", ival);
        string = buf;
      }
      break;

    case ENTRY_STR:
      (void) entry_str_get(pentry, &string);
      break;

    case ENTRY_FLOAT:
    case ENTRY_FILEREFERENCE:
      // Not supported yet
      break;
    case ENTRY_ILLEGAL:
      fc_assert(entry_type_get(pentry) != ENTRY_ILLEGAL);
      break;
    }

    if (nullptr == string) {
      qCritical("Entry type variant of \"%s.%s\" is not supported.",
                section_name(psection), entry_name(pentry));
      continue;
    }

    settable_options->insert(entry_name(pentry), string);
  }
  entry_list_iterate_end;
}

/**
   Save the desired server options.
 */
static void settable_options_save(struct section_file *sf)
{
  optionsHash::const_iterator it = settable_options->constBegin();
  while (it != settable_options->constEnd()) {
    if (!it.key().compare(QLatin1String("gameseed"))
        || !it.key().compare(QLatin1String("mapseed"))) {
      // Do not save mapseed or gameseed.
      it++;
      continue;
    }
    if (!it.key().compare(QLatin1String("topology"))) {
      /* client_start_server() sets topology based on tileset. Don't store
       * its choice. The tileset is already stored. Storing topology leads
       * to all sort of breakage:
       * - it breaks ruleset default topology.
       * - it interacts badly with tileset ruleset change, ruleset tileset
       *   change and topology tileset change.
       * - its value is probably based on what tileset was loaded when
       *   client_start_server() decided to set topology, not on player
       *   choice.
       */
      it++;
      continue;
    }
    QByteArray qkey = it.key().toLocal8Bit();
    QByteArray qval = it.value().toLocal8Bit();
    secfile_insert_str(sf, qval.data(), "server.%s", qkey.data());
    it++; // IT comes 4 U
  }
}

/**
   Update the desired settable options hash table from the current
   setting configuration.
 */
void desired_settable_options_update()
{
  char val_buf[1024], def_buf[1024];
  QString value, def_val;

  options_iterate(server_optset, poption)
  {
    switch (option_type(poption)) {
    case OT_BOOLEAN:
      fc_strlcpy(val_buf, option_bool_get(poption) ? "enabled" : "disabled",
                 sizeof(val_buf));
      value = val_buf;
      fc_strlcpy(def_buf, option_bool_def(poption) ? "enabled" : "disabled",
                 sizeof(def_buf));
      def_val = def_buf;
      break;
    case OT_INTEGER:
      fc_snprintf(val_buf, sizeof(val_buf), "%d", option_int_get(poption));
      value = val_buf;
      fc_snprintf(def_buf, sizeof(def_buf), "%d", option_int_def(poption));
      def_val = def_buf;
      break;
    case OT_STRING:
      value = option_str_get(poption);
      def_val = option_str_def(poption);
      break;
    case OT_ENUM:
      server_option_enum_support_name(poption, &value, &def_val);
      break;
    case OT_BITWISE:
      server_option_bitwise_support_name(poption, val_buf, sizeof(val_buf),
                                         def_buf, sizeof(def_buf));
      value = val_buf;
      def_val = def_buf;
      break;
    case OT_FONT:
    case OT_COLOR:
      break;
    }

    if (nullptr == value || nullptr == def_val) {
      qCritical("Option type %s (%d) not supported for '%s'.",
                option_type_name(option_type(poption)), option_type(poption),
                option_name(poption));
      continue;
    }

    if (value == def_val) {
      // Not set, using default...
      settable_options->remove(option_name(poption));
    } else {
      // Really desired.
      settable_options->insert(option_name(poption), value);
    }
  }
  options_iterate_end;
}

/**
   Update a desired settable option in the hash table from a value
   which can be different of the current configuration.
 */
void desired_settable_option_update(const char *op_name,
                                    const char *op_value, bool allow_replace)
{
  Q_UNUSED(allow_replace)
  settable_options->insert(op_name, op_value);
}

/**
   Convert old integer to new values (Freeciv 2.2.x to Freeciv 2.3.x).
   Very ugly hack. TODO: Remove this later.
 */
static bool settable_option_upgrade_value(const struct option *poption,
                                          int old_value, char *buf,
                                          size_t buf_len)
{
  const char *name = option_name(poption);

#define SETTING_CASE(ARG_name, ...)                                         \
  if (0 == strcmp(ARG_name, name)) {                                        \
    static const char *values[] = {__VA_ARGS__};                            \
    if (0 <= old_value && old_value < ARRAY_SIZE(values)                    \
        && nullptr != values[old_value]) {                                  \
      fc_strlcpy(buf, values[old_value], buf_len);                          \
      return true;                                                          \
    } else {                                                                \
      return false;                                                         \
    }                                                                       \
  }

  SETTING_CASE("topology", "", "WRAPX", "WRAPY", "WRAPX|WRAPY", "ISO",
               "WRAPX|ISO", "WRAPY|ISO", "WRAPX|WRAPY|ISO", "HEX",
               "WRAPX|HEX", "WRAPY|HEX", "WRAPX|WRAPY|HEX", "ISO|HEX",
               "WRAPX|ISO|HEX", "WRAPY|ISO|HEX", "WRAPX|WRAPY|ISO|HEX");
  SETTING_CASE("generator", nullptr, "RANDOM", "FRACTAL", "ISLAND");
  SETTING_CASE("startpos", "DEFAULT", "SINGLE", "2or3", "ALL", "VARIABLE");
  SETTING_CASE("borders", "DISABLED", "ENABLED", "SEE_INSIDE", "EXPAND");
  SETTING_CASE("diplomacy", "ALL", "HUMAN", "AI", "TEAM", "DISABLED");
  SETTING_CASE("citynames", "NO_RESTRICTIONS", "PLAYER_UNIQUE",
               "GLOBAL_UNIQUE", "NO_STEALING");
  SETTING_CASE("barbarians", "DISABLED", "HUTS_ONLY", "NORMAL", "FREQUENT",
               "HORDES");
  SETTING_CASE("phasemode", "ALL", "PLAYER", "TEAM");
  SETTING_CASE("compresstype", "PLAIN", "LIBZ", "BZIP2");

#undef SETTING_CASE
  return false;
}

/**
   Send the desired server options to the server.
 */
static void desired_settable_option_send(struct option *poption)
{
  const char *desired;
  int value;

  if (!settable_options->contains(option_name(poption))) {
    // No change explicitly  desired.
    return;
  }
  QByteArray qval =
      settable_options->value(option_name(poption)).toLocal8Bit();
  desired = qval.data();
  switch (option_type(poption)) {
  case OT_BOOLEAN:
    if ((0 == fc_strcasecmp("enabled", desired)
         || (str_to_int(desired, &value) && 1 == value))
        && !option_bool_get(poption)) {
      send_chat_printf("/set %s enabled", option_name(poption));
    } else if ((0 == fc_strcasecmp("disabled", desired)
                || (str_to_int(desired, &value) && 0 == value))
               && option_bool_get(poption)) {
      send_chat_printf("/set %s disabled", option_name(poption));
    }
    return;
  case OT_INTEGER:
    if (str_to_int(desired, &value) && value != option_int_get(poption)) {
      send_chat_printf("/set %s %d", option_name(poption), value);
    }
    return;
  case OT_STRING:
    if (0 != strcmp(desired, option_str_get(poption))) {
      send_chat_printf("/set %s \"%s\"", option_name(poption), desired);
    }
    return;
  case OT_ENUM: {
    char desired_buf[256];

    // Handle old values.
    if (str_to_int(desired, &value)
        && settable_option_upgrade_value(poption, value, desired_buf,
                                         sizeof(desired_buf))) {
      desired = desired_buf;
    }

    QString value_str;
    server_option_enum_support_name(poption, &value_str, nullptr);
    if (desired == value_str) {
      send_chat_printf("/set %s \"%s\"", option_name(poption), desired);
    }
  }
    return;
  case OT_BITWISE: {
    char desired_buf[256], value_buf[256];

    // Handle old values.
    if (str_to_int(desired, &value)
        && settable_option_upgrade_value(poption, value, desired_buf,
                                         sizeof(desired_buf))) {
      desired = desired_buf;
    }

    server_option_bitwise_support_name(poption, value_buf, sizeof(value_buf),
                                       nullptr, 0);
    if (0 != strcmp(desired, value_buf)) {
      send_chat_printf("/set %s \"%s\"", option_name(poption), desired);
    }
  }
    return;
  case OT_FONT:
  case OT_COLOR:
    break;
  }

  qCritical("Option type %s (%d) not supported for '%s'.",
            option_type_name(option_type(poption)), option_type(poption),
            option_name(poption));
}

Q_GLOBAL_STATIC(dialOptionsHash, dialog_options)

/**
   Load the city and player report dialog options.
 */
static void options_dialogs_load(struct section_file *sf)
{
  const struct entry_list *entries;
  const char *prefixes[] = {"player_dlg_", "city_report_", nullptr};
  const char **prefix;
  bool visible;

  entries = section_entries(secfile_section_by_name(sf, "client"));

  if (nullptr != entries) {
    entry_list_iterate(entries, pentry)
    {
      for (prefix = prefixes; nullptr != *prefix; prefix++) {
        if (0 == strncmp(*prefix, entry_name(pentry), qstrlen(*prefix))
            && secfile_lookup_bool(sf, &visible, "client.%s",
                                   entry_name(pentry))) {
          dialog_options->insert(entry_name(pentry), visible);
          break;
        }
      }
    }
    entry_list_iterate_end;
  }
}

/**
   Save the city and player report dialog options.
 */
static void options_dialogs_save(struct section_file *sf)
{
  fc_assert_ret(nullptr != dialog_options);

  options_dialogs_update();
  dialOptionsHash::const_iterator it = dialog_options->constBegin();
  while (it != dialog_options->constEnd()) {
    QByteArray qba = it.key().toLocal8Bit();
    secfile_insert_bool(sf, (bool) it.value(), "client.%s", qba.data());
    it++;
  }
}

/**
   This set the city and player report dialog options to the
   current ones.  It's called when the client goes to
   C_S_DISCONNECTED state.
 */
void options_dialogs_update()
{
  char buf[64];
  int i;

  fc_assert_ret(nullptr != dialog_options);

  // Player report dialog options.
  for (i = 1; i < num_player_dlg_columns; i++) {
    fc_snprintf(buf, sizeof(buf), "player_dlg_%s",
                player_dlg_columns[i].tagname);
    dialog_options->insert(buf, player_dlg_columns[i].show);
  }

  // City report dialog options.
  for (i = 0; i < num_city_report_spec(); i++) {
    fc_snprintf(buf, sizeof(buf), "city_report_%s",
                city_report_spec_tagname(i));
    dialog_options->insert(buf, *city_report_spec_show_ptr(i));
  }
}

/**
   This set the city and player report dialog options.  It's called
   when the client goes to C_S_RUNNING state.
 */
void options_dialogs_set()
{
  char buf[64];
  int i;

  // Player report dialog options.
  for (i = 1; i < num_player_dlg_columns; i++) {
    fc_snprintf(buf, sizeof(buf), "player_dlg_%s",
                player_dlg_columns[i].tagname);
    if (dialog_options->contains(buf)) {
      player_dlg_columns[i].show = dialog_options->value(buf);
    }
  }

  // City report dialog options.
  for (i = 0; i < num_city_report_spec(); i++) {
    fc_snprintf(buf, sizeof(buf), "city_report_%s",
                city_report_spec_tagname(i));
    if (dialog_options->contains(buf)) {
      *city_report_spec_show_ptr(i) = dialog_options->value(buf);
    }
  }
}

/**
   Load from the rc file any options that are not ruleset specific.
   It is called after ui_init(), yet before ui_main().
   Unfortunately, this means that some clients cannot display.
   Instead, use log_*().
 */
void options_load()
{
  struct section_file *sf;
  bool allow_digital_boolean;
  int i, num;
  const char *name;
  const char *const prefix = "client";
  const char *str;

  // Ensure all options start with their default value
  for (auto &option : client_options) {
    option_reset(OPTION(&option));
  }

  name = get_last_option_file_name(&allow_digital_boolean);
  if (!name) {
    qInfo(_("Didn't find the option file. Creating a new one."));
    options_fully_initialized = true;
    create_default_cma_presets();
    gui_options->first_boot = true;
    return;
  }
  if (!(sf = secfile_load(name, true))) {
    log_debug("Error loading option file '%s':\n%s", name, secfile_error());
    // try to create the rc file
    sf = secfile_new(true);
    secfile_insert_str(sf, freeciv21_version(), "client.version");

    create_default_cma_presets();
    gui_options->first_boot = true;
    save_cma_presets(sf);

    // FIXME: need better messages
    if (!secfile_save(sf, name)) {
      qCritical(_("Save failed, cannot write to file %s"), name);
    } else {
      qInfo(_("Saved settings to file %s"), name);
    }
    secfile_destroy(sf);
    options_fully_initialized = true;
    return;
  }
  secfile_allow_digital_boolean(sf, allow_digital_boolean);

  // a "secret" option for the lazy. TODO: make this saveable
  if (client_url().password().isEmpty()) {
    client_url().setPassword(
        secfile_lookup_str_default(sf, "", "%s.password", prefix));
  }

  gui_options->save_options_on_exit =
      secfile_lookup_bool_default(sf, gui_options->save_options_on_exit,
                                  "%s.save_options_on_exit", prefix);
  gui_options->migrate_fullscreen = secfile_lookup_bool_default(
      sf, gui_options->migrate_fullscreen, "%s.fullscreen_mode", prefix);

  str = secfile_lookup_str_default(sf, nullptr,
                                   "client.default_tileset_overhead_name");
  if (str != nullptr) {
    sz_strlcpy(gui_options->default_tileset_overhead_name, str);
  }
  str = secfile_lookup_str_default(sf, nullptr,
                                   "client.default_tileset_iso_name");
  if (str != nullptr) {
    sz_strlcpy(gui_options->default_tileset_iso_name, str);
  }

  bool draw_full_citybar =
      secfile_lookup_bool_default(sf, true, "client.draw_full_citybar");
  if (draw_full_citybar) {
    fc_strlcpy(gui_options->default_city_bar_style_name, "Polished",
               sizeof(gui_options->default_city_bar_style_name));
  } else {
    fc_strlcpy(gui_options->default_city_bar_style_name, "Simple",
               sizeof(gui_options->default_city_bar_style_name));
  }

  /* Backwards compatibility for removed options replaced by entirely "new"
   * options. The equivalent "new" option will override these, if set. */

  // Renamed in 2.6
  gui_options->popup_actor_arrival = secfile_lookup_bool_default(
      sf, true, "%s.popup_caravan_arrival", prefix);

  // Load all the regular options
  for (auto &option : client_options) {
    client_option_load(OPTION(&option), sf);
  }

  /* More backwards compatibility, for removed options that had been
   * folded into then-existing options. Here, the backwards-compatibility
   * behaviour overrides the "destination" option. */

  // Removed in 2.4
  if (!secfile_lookup_bool_default(sf, true, "%s.do_combat_animation",
                                   prefix)) {
    gui_options->smooth_combat_step_msec = 0;
  }

  message_options_load(sf, prefix);
  options_dialogs_load(sf);

  /* Load cma presets. If cma.number_of_presets doesn't exist, don't load
   * any, the order here should be reversed to keep the order the same */
  if (secfile_lookup_int(sf, &num, "cma.number_of_presets")) {
    for (i = num - 1; i >= 0; i--) {
      load_cma_preset(sf, i);
    }
  } else {
    create_default_cma_presets();
  }

  tileset_options_load(sf, gui_options);
  tileset_options_migrate_cimpletoon(gui_options);
  settable_options_load(sf);
  global_worklists_load(sf);

  secfile_destroy(sf);
  options_fully_initialized = true;
}

/**
   Write messages from option saving to the output window.
 */
static void option_save_output_window_callback(QtMsgType lvl,
                                               const QString &msg)
{
  Q_UNUSED(lvl)
  output_window_append(ftc_client, qUtf8Printable(msg));
}

/**
   Save all options.
 */
void options_save(option_save_log_callback log_cb)
{
  struct section_file *sf;
  const char *name = get_current_option_file_name();
  char dir_name[2048];
  int i;

  if (log_cb == nullptr) {
    // Default callback
    log_cb = option_save_output_window_callback;
  }

  if (!name) {
    log_cb(LOG_ERROR, _("Save failed, cannot find a filename."));
    return;
  }

  sf = secfile_new(true);
  secfile_insert_str(sf, freeciv21_version(), "client.version");

  secfile_insert_bool(sf, gui_options->save_options_on_exit,
                      "client.save_options_on_exit");
  secfile_insert_bool_comment(sf, gui_options->migrate_fullscreen,
                              "deprecated", "client.fullscreen_mode");

  // prevent saving that option
  gui_options->gui_qt_increase_fonts = 0; // gui-enabled options
  for (const auto &option : client_options) {
    client_option_save(OPTION(&option), sf);
  }

  message_options_save(sf, "client");
  options_dialogs_save(sf);

  // Tileset options
  tileset_options_save(sf, gui_options);

  // server settings
  save_cma_presets(sf);
  settable_options_save(sf);

  // insert global worklists
  global_worklists_save(sf);

  // Directory name
  sz_strlcpy(dir_name, name);
  for (i = qstrlen(dir_name) - 1; i >= 0 && dir_name[i] != '/'; i--) {
    // Nothing
  }
  if (i > 0) {
    dir_name[i] = '\0';
    make_dir(dir_name);
  }

  // save to disk
  if (!secfile_save(sf, name)) {
    log_cb(LOG_ERROR, QString::asprintf(
                          _("Save failed, cannot write to file %s"), name));
  } else {
    log_cb(LOG_VERBOSE,
           QString::asprintf(_("Saved settings to file %s"), name));
  }
  secfile_destroy(sf);
}

/**
   Initialize lists of names for a client option.
 */
static void options_init_names(const struct copt_val_name *(*acc)(int),
                               QVector<QString> **support,
                               QVector<QString> **pretty)
{
  int val;
  const struct copt_val_name *name;
  fc_assert_ret(nullptr != acc);
  *support = new QVector<QString>;
  *pretty = new QVector<QString>;
  for (val = 0; (name = acc(val)); val++) {
    (*support)->append(name->support);
    (*pretty)->append(name->pretty);
  }
}

/**
   Initialize the option module.
 */
void options_init()
{
  gui_options = new struct client_options;
  init_client_options();

  message_options_init();
  options_extra_init();
  global_worklists_init();

  for (auto &option : client_options) {
    auto *poption = OPTION(&option);
    auto *pcoption = CLIENT_OPTION(&option);

    switch (option_type(poption)) {
    case OT_INTEGER:
      if (option_int_def(poption) < option_int_min(poption)
          || option_int_def(poption) > option_int_max(poption)) {
        int new_default =
            MAX(MIN(option_int_def(poption), option_int_max(poption)),
                option_int_min(poption));

        qCritical("option %s has default value of %d, which is "
                  "out of its range [%d; %d], changing to %d.",
                  option_name(poption), option_int_def(poption),
                  option_int_min(poption), option_int_max(poption),
                  new_default);
        *(const_cast<int *>(&(pcoption->u.integer.def))) = new_default;
      }
      break;

    case OT_STRING:
      if (gui_options->default_user_name == option_str_get(poption)) {
        // Hack to get a default value.
        *(const_cast<const char **>(&(pcoption->u.string.def))) =
            fc_strdup(gui_options->default_user_name);
      }

      if (nullptr == option_str_def(poption)) {
        const QVector<QString> *values = option_str_values(poption);

        if (nullptr == values || values->count() == 0) {
          qCritical("Invalid nullptr default string for option %s.",
                    option_name(poption));
        } else {
          *(const_cast<const char **>(&(pcoption->u.string.def))) =
              qstrdup(qUtf8Printable(values->at(0)));
        }
      }
      break;

    case OT_ENUM:
      fc_assert(nullptr == pcoption->u.enumerator.support_names);
      fc_assert(nullptr == pcoption->u.enumerator.pretty_names);
      options_init_names(pcoption->u.enumerator.name_accessor,
                         &pcoption->u.enumerator.support_names,
                         &pcoption->u.enumerator.pretty_names);
      fc_assert(nullptr != pcoption->u.enumerator.support_names);
      fc_assert(nullptr != pcoption->u.enumerator.pretty_names);
      break;

    case OT_BITWISE:
      fc_assert(nullptr == pcoption->u.bitwise.support_names);
      fc_assert(nullptr == pcoption->u.bitwise.pretty_names);
      options_init_names(pcoption->u.bitwise.name_accessor,
                         &pcoption->u.bitwise.support_names,
                         &pcoption->u.bitwise.pretty_names);
      fc_assert(nullptr != pcoption->u.bitwise.support_names);
      fc_assert(nullptr != pcoption->u.bitwise.pretty_names);
      break;

    case OT_COLOR: {
      // Duplicate the string pointers.
      struct ft_color *pcolor = pcoption->u.color.pvalue;

      if (nullptr != pcolor->foreground) {
        pcolor->foreground = fc_strdup(pcolor->foreground);
      }
      if (nullptr != pcolor->background) {
        pcolor->background = fc_strdup(pcolor->background);
      }
    }

    case OT_BOOLEAN:
    case OT_FONT:
      break;
    }

    // Set to default.
    option_reset(poption);
  }
}

/**
   Free the option module.
 */
void options_free()
{
  for (auto option : client_options) {
    auto *poption = OPTION(&option);
    auto *pcoption = CLIENT_OPTION(&option);

    switch (option_type(poption)) {
    case OT_ENUM:
      fc_assert_action(nullptr != pcoption->u.enumerator.support_names,
                       break);
      delete pcoption->u.enumerator.support_names;
      pcoption->u.enumerator.support_names = nullptr;
      fc_assert_action(nullptr != pcoption->u.enumerator.pretty_names,
                       break);
      delete pcoption->u.enumerator.pretty_names;
      pcoption->u.enumerator.pretty_names = nullptr;
      break;

    case OT_BITWISE:
      fc_assert_action(nullptr != pcoption->u.bitwise.support_names, break);
      delete pcoption->u.bitwise.support_names;
      pcoption->u.bitwise.support_names = nullptr;
      fc_assert_action(nullptr != pcoption->u.bitwise.pretty_names, break);
      delete pcoption->u.bitwise.pretty_names;
      pcoption->u.bitwise.pretty_names = nullptr;
      break;

    case OT_BOOLEAN:
    case OT_INTEGER:
    case OT_STRING:
    case OT_FONT:
    case OT_COLOR:
      break;
    }
  }

  settable_options->clear();
  dialog_options->clear();

  message_options_free();
  global_worklists_free();

  delete gui_options;
}

/**
   Callback when the reqtree show icons option is changed. The tree is
   recalculated.
 */
static void reqtree_show_icons_callback(struct option *poption)
{
  Q_UNUSED(poption)
  science_report_dialog_redraw();
}

/**
   Callback for when any view option is changed.
 */
static void view_option_changed_callback(struct option *poption)
{
  Q_UNUSED(poption)
  menus_init();
  update_map_canvas_visible();
}

/**
   Callback for when ai_manual_turn_done is changed.
 */
static void manual_turn_done_callback(struct option *poption)
{
  Q_UNUSED(poption)
  update_turn_done_button_state();

  if (!gui_options->ai_manual_turn_done) {
    const player *pplayer = client_player();
    if (pplayer != nullptr && is_ai(pplayer) && can_end_turn()) {
      user_ended_turn();
    }
  }
}

/**
  Callback for changing music volume
 */
static void sound_volume_callback(struct option *poption)
{
  Q_UNUSED(poption)
  audio_set_volume(gui_options->sound_effects_volume / 100.0);
}

/**
   Callback for when any voteinfo bar option is changed.
 */
static void voteinfo_bar_callback(struct option *poption)
{
  Q_UNUSED(poption)
  voteinfo_gui_update();
}

/**
   Callback for font options.
 */
static void allfont_changed_callback(struct option *poption)
{
  Q_UNUSED(poption)
  gui_update_allfonts();
}

/**
   Callback for font options.
 */
static void font_changed_callback(struct option *poption)
{
  fc_assert_ret(OT_FONT == option_type(OPTION(poption)));
  gui_update_font(option_font_target(poption), option_font_get(poption));
}

/**
   Callback for mapimg options.
 */
/* See #2237
static void mapimg_changed_callback(struct option *poption)
{
  if (!mapimg_client_define()) {
    bool success;

    qInfo("Error setting the value for %s (%s). Restoring the default "
          "value.",
          option_name(poption), mapimg_error());

    // Reset the value to the default value.
    success = option_reset(poption);
    fc_assert_msg(success == true, "Failed to reset the option \"%s\".",
                  option_name(poption));
    success = mapimg_client_define();
    fc_assert_msg(success == true,
                  "Failed to restore mapimg definition for option \"%s\".",
                  option_name(poption));
  }
}
*/

/**
   Callback for music enabling option.
 */
static void game_music_enable_callback(struct option *poption)
{
  Q_UNUSED(poption)
  if (client_state() == C_S_RUNNING) {
    if (gui_options->sound_enable_game_music) {
      start_style_music();
    } else {
      stop_style_music();
    }
  }
}

/**
   Callback for music enabling option.
 */
static void menu_music_enable_callback(struct option *poption)
{
  Q_UNUSED(poption)
  if (client_state() != C_S_RUNNING) {
    if (gui_options->sound_enable_menu_music) {
      start_menu_music(QStringLiteral("music_menu"), nullptr);
    } else {
      stop_menu_music();
    }
  }
}

/**
   Option framework wrapper for mapimg_get_format_list()
 */
/* See #2237
static const QVector<QString> *
get_mapimg_format_list(const struct option *poption)
{
  Q_UNUSED(poption)
  return mapimg_get_format_list();
}
*/

/**
   What is the user defined tileset for the given topology
 */
const char *tileset_name_for_topology(int topology_id)
{
  const char *tsn = nullptr;

  switch (topology_id & (TF_ISO | TF_HEX)) {
  case 0:
  case TF_ISO:
    tsn = gui_options->default_tileset_square_name;
    break;
  case TF_HEX:
    tsn = gui_options->default_tileset_hex_name;
    break;
  case TF_ISO | TF_HEX:
    tsn = gui_options->default_tileset_isohex_name;
    break;
  }

  return tsn;
}

/**
   Does topology-specific tileset option lack value?
 */
static bool is_ts_option_unset(const char *optname)
{
  struct option *opt;
  const char *val;

  opt = optset_option_by_name(client_optset, optname);

  if (opt == nullptr) {
    return true;
  }

  val = opt->str_vtable->get(opt);

  return val == nullptr || val[0] == '\0';
}

/**
   Fill default tilesets for topology-specific settings.
 */
void fill_topo_ts_default()
{
  if (is_ts_option_unset("default_tileset_square_name")) {
    if (gui_options->default_tileset_iso_name[0] != '\0') {
      qstrncpy(gui_options->default_tileset_square_name,
               gui_options->default_tileset_iso_name,
               sizeof(gui_options->default_tileset_square_name));
    } else if (gui_options->default_tileset_overhead_name[0] != '\0') {
      qstrncpy(gui_options->default_tileset_square_name,
               gui_options->default_tileset_overhead_name,
               sizeof(gui_options->default_tileset_square_name));
    } else {
      log_debug("Setting tileset for square topologies.");
      tilespec_try_read(QString(), false, 0);
    }
  }
  if (is_ts_option_unset("default_tileset_hex_name")) {
    log_debug("Setting tileset for hex topology.");
    tilespec_try_read(QString(), false, TF_HEX);
  }
  if (is_ts_option_unset("default_tileset_isohex_name")) {
    log_debug("Setting tileset for isohex topology.");
    tilespec_try_read(QString(), false, TF_ISO | TF_HEX);
  }
}
