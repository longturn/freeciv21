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

// utility
#include "support.h" // bool type and fc__attribute

#include <QString>
#include <QVector>

#include <stddef.h>

class QIODevice;

// Opaque types.
struct section_file;
struct section;
struct entry;

// Typedefs.
typedef const void *secfile_data_t;

typedef bool (*secfile_enum_is_valid_fn_t)(int enumerator);
typedef const char *(*secfile_enum_name_fn_t)(int enumerator);
typedef int (*secfile_enum_by_name_fn_t)(const char *enum_name,
                                         int (*strcmp_fn)(const char *,
                                                          const char *));
typedef int (*secfile_enum_iter_fn_t)();
typedef int (*secfile_enum_next_fn_t)(int enumerator);
typedef const char *(*secfile_enum_name_data_fn_t)(secfile_data_t data,
                                                   int enumerator);

// Create a 'struct section_list' and related functions:
#define SPECLIST_TAG section
#include "speclist.h"
#define section_list_iterate(seclist, psection)                             \
  TYPED_LIST_ITERATE(struct section, seclist, psection)
#define section_list_iterate_end LIST_ITERATE_END
#define section_list_iterate_rev(seclist, psection)                         \
  TYPED_LIST_ITERATE_REV(struct section, seclist, psection)
#define section_list_iterate_rev_end LIST_ITERATE_REV_END

// Create a 'struct entry_list' and related functions:
#define SPECLIST_TAG entry
#include "speclist.h"
#define entry_list_iterate(entlist, pentry)                                 \
  TYPED_LIST_ITERATE(struct entry, entlist, pentry)
#define entry_list_iterate_end LIST_ITERATE_END

// Main functions.
struct section_file *secfile_load_section(const QString &filename,
                                          const QString &section,
                                          bool allow_duplicates);
struct section_file *secfile_from_stream(QIODevice *stream,
                                         bool allow_duplicates);

bool secfile_save(const struct section_file *secfile, QString filename);
void secfile_check_unused(const struct section_file *secfile);
const char *secfile_name(const struct section_file *secfile);

enum entry_special_type { EST_NORMAL, EST_INCLUDE, EST_COMMENT };

// Insertion functions.
struct entry *secfile_insert_bool_full(struct section_file *secfile,
                                       bool value, const char *comment,
                                       bool allow_replace, const char *path,
                                       ...)
    fc__attribute((__format__(__printf__, 5, 6)));
#define secfile_insert_bool(secfile, value, path, ...)                      \
  secfile_insert_bool_full(secfile, value, nullptr, false, path,            \
                           ##__VA_ARGS__)
#define secfile_insert_bool_comment(secfile, value, comment, path, ...)     \
  secfile_insert_bool_full(secfile, value, comment, false, path,            \
                           ##__VA_ARGS__)
#define secfile_replace_bool(secfile, value, path, ...)                     \
  secfile_insert_bool_full(secfile, value, nullptr, true, path,             \
                           ##__VA_ARGS__)
#define secfile_replace_bool_comment(secfile, value, comment, path, ...)    \
  secfile_insert_bool_full(secfile, value, comment, true, path,             \
                           ##__VA_ARGS__)
size_t secfile_insert_bool_vec_full(struct section_file *secfile,
                                    const bool *values, size_t dim,
                                    const char *comment, bool allow_replace,
                                    const char *path, ...)
    fc__attribute((__format__(__printf__, 6, 7)));
#define secfile_insert_bool_vec(secfile, values, dim, path, ...)            \
  secfile_insert_bool_vec_full(secfile, values, dim, nullptr, false, path,  \
                               ##__VA_ARGS__)
#define secfile_insert_bool_vec_comment(secfile, values, dim, comment,      \
                                        path, ...)                          \
  secfile_insert_bool_vec_full(secfile, values, dim, comment, false, path,  \
                               ##__VA_ARGS__)
#define secfile_replace_bool_vec(secfile, values, dim, path, ...)           \
  secfile_insert_bool_vec_full(secfile, values, dim, nullptr, true, path,   \
                               ##__VA_ARGS__)
#define secfile_replace_bool_vec_comment(secfile, values, dim, comment,     \
                                         path, ...)                         \
  secfile_insert_bool_vec_full(secfile, values, comment, true, path,        \
                               ##__VA_ARGS__)

struct entry *secfile_insert_int_full(struct section_file *secfile,
                                      int value, const char *comment,
                                      bool allow_replace, const char *path,
                                      ...)
    fc__attribute((__format__(__printf__, 5, 6)));
#define secfile_insert_int(secfile, value, path, ...)                       \
  secfile_insert_int_full(secfile, value, nullptr, false, path,             \
                          ##__VA_ARGS__)
#define secfile_insert_int_comment(secfile, value, comment, path, ...)      \
  secfile_insert_int_full(secfile, value, comment, false, path,             \
                          ##__VA_ARGS__)
#define secfile_replace_int(secfile, value, path, ...)                      \
  secfile_insert_int_full(secfile, value, nullptr, true, path, ##__VA_ARGS__)
#define secfile_replace_int_comment(secfile, value, comment, path, ...)     \
  secfile_insert_int_full(secfile, value, comment, true, path, ##__VA_ARGS__)
size_t secfile_insert_int_vec_full(struct section_file *secfile,
                                   const int *values, size_t dim,
                                   const char *comment, bool allow_replace,
                                   const char *path, ...)
    fc__attribute((__format__(__printf__, 6, 7)));
#define secfile_insert_int_vec(secfile, values, dim, path, ...)             \
  secfile_insert_int_vec_full(secfile, values, dim, nullptr, false, path,   \
                              ##__VA_ARGS__)
#define secfile_insert_int_vec_comment(secfile, values, dim, comment, path, \
                                       ...)                                 \
  secfile_insert_int_vec_full(secfile, values, dim, comment, false, path,   \
                              ##__VA_ARGS__)
#define secfile_replace_int_vec(secfile, values, dim, path, ...)            \
  secfile_insert_int_vec_full(secfile, values, dim, nullptr, true, path,    \
                              ##__VA_ARGS__)
#define secfile_replace_int_vec_comment(secfile, values, dim, comment,      \
                                        path, ...)                          \
  secfile_insert_int_vec_full(secfile, values, dim, comment, true, path,    \
                              ##__VA_ARGS__)

struct entry *secfile_insert_float_full(struct section_file *secfile,
                                        float value, const char *comment,
                                        bool allow_replace, const char *path,
                                        ...)
    fc__attribute((__format__(__printf__, 5, 6)));
#define secfile_insert_float(secfile, value, path, ...)                     \
  secfile_insert_float_full(secfile, value, nullptr, false, path,           \
                            ##__VA_ARGS__)

struct section *secfile_insert_include(struct section_file *secfile,
                                       const char *filename);

struct section *secfile_insert_long_comment(struct section_file *secfile,
                                            const char *comment);

struct entry *secfile_insert_str_full(struct section_file *secfile,
                                      const char *str, const char *comment,
                                      bool allow_replace, bool no_escape,
                                      enum entry_special_type stype,
                                      const char *path, ...)
    fc__attribute((__format__(__printf__, 7, 8)));
#define secfile_insert_str(secfile, string, path, ...)                      \
  secfile_insert_str_full(secfile, string, nullptr, false, false,           \
                          EST_NORMAL, path, ##__VA_ARGS__)
#define secfile_insert_str_noescape(secfile, string, path, ...)             \
  secfile_insert_str_full(secfile, string, nullptr, false, true,            \
                          EST_NORMAL, path, ##__VA_ARGS__)
#define secfile_insert_str_comment(secfile, string, comment, path, ...)     \
  secfile_insert_str_full(secfile, string, comment, false, true,            \
                          EST_NORMAL, path, ##__VA_ARGS__)
#define secfile_insert_str_noescape_comment(secfile, string, comment, path, \
                                            ...)                            \
  secfile_insert_str_full(secfile, string, comment, false, true,            \
                          EST_NORMAL, path, ##__VA_ARGS__)
#define secfile_replace_str(secfile, string, path, ...)                     \
  secfile_insert_str_full(secfile, string, nullptr, true, false,            \
                          EST_NORMAL, path, ##__VA_ARGS__)
#define secfile_replace_str_noescape(secfile, string, path, ...)            \
  secfile_insert_str_full(secfile, string, nullptr, true, true, EST_NORMAL, \
                          path, ##__VA_ARGS__)
#define secfile_replace_str_comment(secfile, string, comment, path, ...)    \
  secfile_insert_str_full(secfile, string, comment, true, true, EST_NORMAL, \
                          path, ##__VA_ARGS__)
#define secfile_replace_str_noescape_comment(secfile, string, comment,      \
                                             path, ...)                     \
  secfile_insert_str_full(secfile, string, comment, true, true, EST_NORMAL, \
                          path, ##__VA_ARGS__)
size_t secfile_insert_str_vec_full(struct section_file *secfile,
                                   const char *const *strings, size_t dim,
                                   const char *comment, bool allow_replace,
                                   bool no_escape, const char *path, ...)
    fc__attribute((__format__(__printf__, 7, 8)));
size_t secfile_insert_str_vec_full(struct section_file *secfile,
                                   const QVector<QString> &strings,
                                   size_t dim, const char *comment,
                                   bool allow_replace, bool no_escape,
                                   const char *path, ...)
    fc__attribute((__format__(__printf__, 7, 8)));
#define secfile_insert_str_vec(secfile, strings, dim, path, ...)            \
  secfile_insert_str_vec_full(secfile, strings, dim, nullptr, false, false, \
                              path, ##__VA_ARGS__)
#define secfile_insert_str_vec_noescape(secfile, strings, dim, path, ...)   \
  secfile_insert_str_vec_full(secfile, strings, dim, nullptr, false, true,  \
                              path, ##__VA_ARGS__)
#define secfile_insert_str_vec_comment(secfile, strings, dim, comment,      \
                                       path, ...)                           \
  secfile_insert_str_vec_full(secfile, strings, dim, comment, false, true,  \
                              path, ##__VA_ARGS__)
#define secfile_insert_str_vec_noescape_comment(secfile, strings, dim,      \
                                                comment, path, ...)         \
  secfile_insert_str_vec_full(secfile, strings, dim, comment, false, true,  \
                              path, ##__VA_ARGS__)
#define secfile_replace_str_vec(secfile, strings, dim, path, ...)           \
  secfile_insert_str_vec_full(secfile, strings, dim, nullptr, true, false,  \
                              path, ##__VA_ARGS__)
#define secfile_replace_str_vec_noescape(secfile, strings, dim, path, ...)  \
  secfile_insert_str_vec_full(secfile, strings, dim, nullptr, true, true,   \
                              path, ##__VA_ARGS__)
#define secfile_replace_str_vec_comment(secfile, strings, dim, comment,     \
                                        path, ...)                          \
  secfile_insert_str_vec_full(secfile, strings, dim, comment, true, true,   \
                              path, ##__VA_ARGS__)
#define secfile_replace_str_vec_noescape_comment(secfile, strings, dim,     \
                                                 comment, path, ...)        \
  secfile_insert_str_vec_full(secfile, strings, dim, comment, true, true,   \
                              path, ##__VA_ARGS__)

struct entry *secfile_insert_plain_enum_full(
    struct section_file *secfile, int enumerator,
    secfile_enum_name_fn_t name_fn, const char *comment, bool allow_replace,
    const char *path, ...) fc__attribute((__format__(__printf__, 6, 7)));
struct entry *secfile_insert_bitwise_enum_full(
    struct section_file *secfile, int bitwise_val,
    secfile_enum_name_fn_t name_fn, secfile_enum_iter_fn_t begin_fn,
    secfile_enum_iter_fn_t end_fn, secfile_enum_next_fn_t next_fn,
    const char *comment, bool allow_replace, const char *path, ...)
    fc__attribute((__format__(__printf__, 9, 10)));
#define secfile_insert_enum_full(secfile, enumerator, specenum_type,        \
                                 comment, allow_replace, path, ...)         \
  (specenum_type##_is_bitwise()                                             \
       ? secfile_insert_bitwise_enum_full(                                  \
           secfile, enumerator,                                             \
           (secfile_enum_name_fn_t) specenum_type##_name,                   \
           (secfile_enum_iter_fn_t) specenum_type##_begin,                  \
           (secfile_enum_iter_fn_t) specenum_type##_end,                    \
           (secfile_enum_next_fn_t) specenum_type##_next, comment,          \
           allow_replace, path, ##__VA_ARGS__)                              \
       : secfile_insert_plain_enum_full(                                    \
           secfile, enumerator,                                             \
           (secfile_enum_name_fn_t) specenum_type##_name, comment,          \
           allow_replace, path, ##__VA_ARGS__))
#define secfile_insert_enum(secfile, enumerator, specenum_type, path, ...)  \
  secfile_insert_enum_full(secfile, enumerator, specenum_type, nullptr,     \
                           false, path, ##__VA_ARGS__)
#define secfile_insert_enum_comment(secfile, enumerator, specenum_type,     \
                                    comment, path, ...)                     \
  secfile_insert_enum_full(secfile, enumerator, specenum_type, comment,     \
                           false, path, ##__VA_ARGS__)
#define secfile_replace_enum(secfile, enumerator, specenum_type, path, ...) \
  secfile_insert_enum_full(secfile, enumerator, specenum_type, nullptr,     \
                           true, path, ##__VA_ARGS__)
#define secfile_replace_enum_comment(secfile, enumerator, specenum_type,    \
                                     comment, path, ...)                    \
  secfile_insert_enum_full(secfile, enumerator, specenum_type, comment,     \
                           true, path, ##__VA_ARGS__)
size_t secfile_insert_plain_enum_vec_full(
    struct section_file *secfile, const int *enumurators, size_t dim,
    secfile_enum_name_fn_t name_fn, const char *comment, bool allow_replace,
    const char *path, ...) fc__attribute((__format__(__printf__, 7, 8)));
size_t secfile_insert_bitwise_enum_vec_full(
    struct section_file *secfile, const int *bitwise_vals, size_t dim,
    secfile_enum_name_fn_t name_fn, secfile_enum_iter_fn_t begin_fn,
    secfile_enum_iter_fn_t end_fn, secfile_enum_next_fn_t next_fn,
    const char *comment, bool allow_replace, const char *path, ...)
    fc__attribute((__format__(__printf__, 10, 11)));
#define secfile_insert_enum_vec_full(secfile, enumerators, dim,             \
                                     specenum_type, comment, allow_replace, \
                                     path, ...)                             \
  (specenum_type##_is_bitwise()                                             \
       ? secfile_insert_bitwise_enum_vec_full(                              \
           secfile, (const int *) enumerators, dim,                         \
           (secfile_enum_name_fn_t) specenum_type##_name,                   \
           (secfile_enum_iter_fn_t) specenum_type##_begin,                  \
           (secfile_enum_iter_fn_t) specenum_type##_end,                    \
           (secfile_enum_next_fn_t) specenum_type##_next, comment,          \
           allow_replace, path, ##__VA_ARGS__)                              \
       : secfile_insert_plain_enum_vec_full(                                \
           secfile, (const int *) enumerators, dim,                         \
           (secfile_enum_name_fn_t) specenum_type##_name, comment,          \
           allow_replace, path, ##__VA_ARGS__))
#define secfile_insert_enum_vec(secfile, enumerators, dim, specenum_type,   \
                                path, ...)                                  \
  secfile_insert_enum_vec_full(secfile, enumerators, dim, specenum_type,    \
                               nullptr, false, path, ##__VA_ARGS__)
#define secfile_insert_enum_vec_comment(secfile, enumerators, dim,          \
                                        specenum_type, comment, path, ...)  \
  secfile_insert_enum_vec_full(secfile, enumerators, dim, specenum_type,    \
                               comment, false, path, ##__VA_ARGS__)
#define secfile_replace_enum_vec(secfile, enumerators, dim, specenum_type,  \
                                 path, ...)                                 \
  secfile_insert_enum_vec_full(secfile, enumerators, dim, specenum_type,    \
                               nullptr, true, path, ##__VA_ARGS__)
#define secfile_replace_enum_vec_comment(secfile, enumerators, dim,         \
                                         specenum_type, comment, path, ...) \
  secfile_insert_enum_vec_full(secfile, enumerators, dim, specenum_type,    \
                               comment, true, path, ##__VA_ARGS__)

struct entry *secfile_insert_enum_data_full(
    struct section_file *secfile, int value, bool bitwise,
    secfile_enum_name_data_fn_t name_fn, secfile_data_t data,
    const char *comment, bool allow_replace, const char *path, ...)
    fc__attribute((__format__(__printf__, 8, 9)));
#define secfile_insert_enum_data(secfile, value, bitwise, name_fn, data,    \
                                 path, ...)                                 \
  secfile_insert_enum_data_full(secfile, value, bitwise, name_fn, data,     \
                                nullptr, false, path, ##__VA_ARGS__)
#define secfile_insert_enum_data_comment(secfile, value, bitwise, name_fn,  \
                                         data, path, ...)                   \
  secfile_insert_enum_data_full(secfile, value, bitwise, name_fn, data,     \
                                comment, false, path, ##__VA_ARGS__)
#define secfile_replace_enum_data(secfile, value, bitwise, name_fn, data,   \
                                  path, ...)                                \
  secfile_insert_enum_data_full(secfile, value, bitwise, name_fn, data,     \
                                nullptr, true, path, ##__VA_ARGS__)
#define secfile_replace_enum_data_comment(secfile, value, bitwise, name_fn, \
                                          data, path, ...)                  \
  secfile_insert_enum_data_full(secfile, value, bitwise, name_fn, data,     \
                                comment, true, path, ##__VA_ARGS__)
size_t secfile_insert_enum_vec_data_full(
    struct section_file *secfile, const int *values, size_t dim,
    bool bitwise, secfile_enum_name_data_fn_t name_fn, secfile_data_t data,
    const char *comment, bool allow_replace, const char *path, ...)
    fc__attribute((__format__(__printf__, 9, 10)));
#define secfile_insert_enum_vec_data(secfile, values, dim, bitwise,         \
                                     name_fn, data, path, ...)              \
  secfile_insert_enum_vec_data_full(secfile, values, dim, bitwise, name_fn, \
                                    data, nullptr, false, path,             \
                                    ##__VA_ARGS__)
#define secfile_insert_enum_vec_data_comment(secfile, values, dim, bitwise, \
                                             name_fn, data, path, ...)      \
  secfile_insert_enum_vec_data_full(secfile, values, dim, bitwise, name_fn, \
                                    data, comment, false, path,             \
                                    ##__VA_ARGS__)
#define secfile_replace_enum_vec_data(secfile, values, dim, bitwise,        \
                                      name_fn, data, path, ...)             \
  secfile_insert_enum_vec_data_full(secfile, values, dim, bitwise, name_fn, \
                                    data, nullptr, true, path,              \
                                    ##__VA_ARGS__)
#define secfile_replace_enum_vec_data_comment(                              \
    secfile, values, dim, bitwise, name_fn, data, path, ...)                \
  secfile_insert_enum_vec_data_full(secfile, values, dim, bitwise, name_fn, \
                                    data, comment, true, path,              \
                                    ##__VA_ARGS__)

struct entry *secfile_insert_filereference(struct section_file *secfile,
                                           const char *filename,
                                           const char *path, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

// Deletion function.
bool secfile_entry_delete(struct section_file *secfile, const char *path,
                          ...) fc__attribute((__format__(__printf__, 2, 3)));

// Lookup functions.
struct entry *secfile_entry_by_path(const struct section_file *secfile,
                                    const char *path);
struct entry *secfile_entry_lookup(const struct section_file *secfile,
                                   const char *path, ...)
    fc__attribute((__format__(__printf__, 2, 3)));

bool secfile_lookup_bool(const struct section_file *secfile, bool *bval,
                         const char *path, ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 3, 4)));
bool secfile_lookup_bool_default(const struct section_file *secfile,
                                 bool def, const char *path,
                                 ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 3, 4)));

bool secfile_lookup_int(const struct section_file *secfile, int *ival,
                        const char *path, ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 3, 4)));
int secfile_lookup_int_default(const struct section_file *secfile, int def,
                               const char *path, ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 3, 4)));
int secfile_lookup_int_def_min_max(const struct section_file *secfile,
                                   int defval, int minval, int maxval,
                                   const char *path,
                                   ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
int *secfile_lookup_int_vec(const struct section_file *secfile, size_t *dim,
                            const char *path, ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 3, 4)));

const char *secfile_lookup_str(const struct section_file *secfile,
                               const char *path, ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 2, 3)));
const char *secfile_lookup_str_default(const struct section_file *secfile,
                                       const char *def, const char *path,
                                       ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 3, 4)));
const char **secfile_lookup_str_vec(const struct section_file *secfile,
                                    size_t *dim, const char *path, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

bool secfile_lookup_plain_enum_full(const struct section_file *secfile,
                                    int *penumerator,
                                    secfile_enum_is_valid_fn_t is_valid_fn,
                                    secfile_enum_by_name_fn_t by_name_fn,
                                    const char *path,
                                    ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
bool secfile_lookup_bitwise_enum_full(const struct section_file *secfile,
                                      int *penumerator,
                                      secfile_enum_is_valid_fn_t is_valid_fn,
                                      secfile_enum_by_name_fn_t by_name_fn,
                                      const char *path,
                                      ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
#define secfile_lookup_enum(secfile, enumerator, specenum_type, path, ...)  \
  (specenum_type##_is_bitwise()                                             \
       ? secfile_lookup_bitwise_enum_full(                                  \
           secfile, FC_ENUM_PTR(enumerator),                                \
           (secfile_enum_is_valid_fn_t) specenum_type##_is_valid,           \
           (secfile_enum_by_name_fn_t) specenum_type##_by_name, path,       \
           ##__VA_ARGS__)                                                   \
       : secfile_lookup_plain_enum_full(                                    \
           secfile, FC_ENUM_PTR(enumerator),                                \
           (secfile_enum_is_valid_fn_t) specenum_type##_is_valid,           \
           (secfile_enum_by_name_fn_t) specenum_type##_by_name, path,       \
           ##__VA_ARGS__))
int secfile_lookup_plain_enum_default_full(
    const struct section_file *secfile, int defval,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path,
    ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
int secfile_lookup_bitwise_enum_default_full(
    const struct section_file *secfile, int defval,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path,
    ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
#define secfile_lookup_enum_default(secfile, defval, specenum_type, path,   \
                                    ...)                                    \
  (specenum_type##_is_bitwise()                                             \
       ? secfile_lookup_bitwise_enum_default_full(                          \
           secfile, defval,                                                 \
           (secfile_enum_is_valid_fn_t) specenum_type##_is_valid,           \
           (secfile_enum_by_name_fn_t) specenum_type##_by_name, path,       \
           ##__VA_ARGS__)                                                   \
       : secfile_lookup_plain_enum_default_full(                            \
           secfile, defval,                                                 \
           (secfile_enum_is_valid_fn_t) specenum_type##_is_valid,           \
           (secfile_enum_by_name_fn_t) specenum_type##_by_name, path,       \
           ##__VA_ARGS__))
int *secfile_lookup_plain_enum_vec_full(
    const struct section_file *secfile, size_t *dim,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path,
    ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
int *secfile_lookup_bitwise_enum_vec_full(
    const struct section_file *secfile, size_t *dim,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path,
    ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 5, 6)));
#define secfile_lookup_enum_vec(secfile, dim, specenum_type, path, ...)     \
  (specenum_type##_is_bitwise()                                             \
       ? (enum specenum_type *) secfile_lookup_bitwise_enum_vec_full(       \
           secfile, dim,                                                    \
           (secfile_enum_is_valid_fn_t) specenum_type##_is_valid,           \
           (secfile_enum_by_name_fn_t) specenum_type##_by_name, path,       \
           ##__VA_ARGS__)                                                   \
       : (enum specenum_type *) secfile_lookup_plain_enum_vec_full(         \
           secfile, dim,                                                    \
           (secfile_enum_is_valid_fn_t) specenum_type##_is_valid,           \
           (secfile_enum_by_name_fn_t) specenum_type##_by_name, path,       \
           ##__VA_ARGS__))

bool secfile_lookup_enum_data(const struct section_file *secfile,
                              int *pvalue, bool bitwise,
                              secfile_enum_name_data_fn_t name_fn,
                              secfile_data_t data, const char *path,
                              ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 6, 7)));
int secfile_lookup_enum_default_data(const struct section_file *secfile,
                                     int defval, bool bitwise,
                                     secfile_enum_name_data_fn_t name_fn,
                                     secfile_data_t data, const char *path,
                                     ...) fc__warn_unused_result
    fc__attribute((__format__(__printf__, 6, 7)));

// Sections functions.
struct section *secfile_section_by_name(const struct section_file *secfile,
                                        const QString &section_name);
struct section *secfile_section_lookup(const struct section_file *secfile,
                                       const char *path, ...)
    fc__attribute((__format__(__printf__, 2, 3)));
const struct section_list *
secfile_sections(const struct section_file *secfile);
struct section_list *
secfile_sections_by_name_prefix(const struct section_file *secfile,
                                const char *prefix);
struct section *secfile_section_new(struct section_file *secfile,
                                    const QString &section_name);

// Independant section functions.
void section_destroy(struct section *psection);
void section_clear_all(struct section *psection);

// Entry functions.
const struct entry_list *section_entries(const struct section *psection);
struct entry *section_entry_by_name(const struct section *psection,
                                    const QString &entry_name);
struct entry *section_entry_int_new(struct section *psection,
                                    const QString &entry_name, int value);
struct entry *section_entry_bool_new(struct section *psection,
                                     const QString &entry_name, bool value);
struct entry *section_entry_float_new(struct section *psection,
                                      const QString &entry_name,
                                      float value);
struct entry *section_entry_str_new(struct section *psection,
                                    const QString &entry_name,
                                    const QString &value, bool escaped);

// Independant entry functions.
enum entry_type {
  ENTRY_BOOL,
  ENTRY_INT,
  ENTRY_FLOAT,
  ENTRY_STR,
  ENTRY_FILEREFERENCE,
  ENTRY_ILLEGAL
};

void entry_destroy(struct entry *pentry);

struct section *entry_section(const struct entry *pentry);
enum entry_type entry_type_get(const struct entry *pentry);
int entry_path(const struct entry *pentry, char *buf, size_t buf_len);

const char *entry_name(const struct entry *pentry);
bool entry_set_name(struct entry *pentry, const char *entry_name);

const char *entry_comment(const struct entry *pentry);
void entry_set_comment(struct entry *pentry, const QString &comment);

bool entry_int_get(const struct entry *pentry, int *value);
bool entry_int_set(struct entry *pentry, int value);

bool entry_bool_get(const struct entry *pentry, bool *value);
bool entry_bool_set(struct entry *pentry, bool value);

bool entry_float_get(const struct entry *pentry, float *value);
bool entry_float_set(struct entry *pentry, float value);

bool entry_str_get(const struct entry *pentry, const char **value);
bool entry_str_set(struct entry *pentry, const char *value);
bool entry_str_set_gt_marking(struct entry *pentry, bool gt_marking);
