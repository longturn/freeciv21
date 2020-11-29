/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__SUPPORT_H
#define FC__SUPPORT_H

/***********************************************************************
  Replacements for functions which are not available on all platforms.
  Where the functions are available natively, these are just wrappers.
  See also mem.h, netintf.h, rand.h, and see support.c for more comments.
***********************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* size_t */
#include <sys/stat.h>

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE true
#define FALSE false

#include <inttypes.h>

/* Want to use GCC's __attribute__ keyword to check variadic
 * parameters to printf-like functions, without upsetting other
 * compilers: put any required defines magic here.
 * If other compilers have something equivalent, could also
 * work that out here.   Should this use configure stuff somehow?
 * --dwp
 */
#if defined(__GNUC__)
#define fc__attribute(x) __attribute__(x)
#else
#define fc__attribute(x)
#endif

/* __attribute__((warn_unused_result)) requires at least gcc 3.4 */
#if defined(__GNUC__)
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define fc__warn_unused_result __attribute__((warn_unused_result))
#endif
#endif
#ifndef fc__warn_unused_result
#define fc__warn_unused_result
#endif

/* TODO: C++17 compilers (also other than g++) could use [[fallthrough]]
   for C++ code */
#if defined(__GNUC__) && __GNUC__ >= 7
#define fc__fallthrough __attribute__((fallthrough))
#else
#define fc__fallthrough
#endif

#include <zlib.h>

#ifdef FREECIV_MSWINDOWS
typedef long int fc_errno;
#else
typedef int fc_errno;
#endif

int fc_strcasecmp(const char *str0, const char *str1);
int fc_strncasecmp(const char *str0, const char *str1, size_t n);
int fc_strncasequotecmp(const char *str0, const char *str1, size_t n);

size_t effectivestrlenquote(const char *str);

char *fc_strcasestr(const char *haystack, const char *needle);

int fc_strcoll(const char *str0, const char *str1);
int fc_stricoll(const char *str0, const char *str1);

FILE *fc_fopen(const char *filename, const char *opentype);
gzFile fc_gzopen(const char *filename, const char *opentype);
int fc_remove(const char *filename);
int fc_stat(const char *filename, struct stat *buf);

fc_errno fc_get_errno(void);
const char *fc_strerror(fc_errno err);
void fc_usleep(unsigned long usec);

bool fc_strrep(char *str, size_t len, const char *search,
               const char *replace);
char *fc_strrep_resize(char *str, size_t *len, const char *search,
                       const char *replace) fc__warn_unused_result;

size_t fc_strlcpy(char *dest, const char *src, size_t n);
size_t fc_strlcat(char *dest, const char *src, size_t n);

/* convenience macros for use when dest is a char ARRAY: */
#define sz_strlcpy(dest, src)                                               \
  ((void) fc_strlcpy((dest), (src), sizeof(dest)))
#define sz_strlcat(dest, src)                                               \
  ((void) fc_strlcat((dest), (src), sizeof(dest)))

int fc_snprintf(char *str, size_t n, const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)))
        fc__attribute((nonnull(1, 3)));
int fc_vsnprintf(char *str, size_t n, const char *format, va_list ap)
    fc__attribute((nonnull(1, 3)));
int cat_snprintf(char *str, size_t n, const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)))
        fc__attribute((nonnull(1, 3)));

int fc_gethostname(char *buf, size_t len);

bool is_reg_file_for_access(const char *name, bool write_access);

int fc_break_lines(char *str, size_t desired_len);

bool fc_isalnum(char c);
bool fc_isalpha(char c);
bool fc_isdigit(char c);
bool fc_isprint(char c);
bool fc_isspace(char c);
bool fc_isupper(char c);
char fc_toupper(char c);
char fc_tolower(char c);

const char *fc_basename(const char *path);

void make_escapes(const char *str, char *buf, size_t buf_len);
void remove_escapes(const char *str, bool full_escapes, char *buf,
                    size_t buf_len);

int fc_at_quick_exit(void (*func)(void));

#endif /* FC__SUPPORT_H */
