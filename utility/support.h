// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// Qt
class QString;

// std
#include <cstdio>

/**
 * Want to use GCC's __attribute__ keyword to check variadic
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

// __attribute__((warn_unused_result)) requires at least gcc 3.4
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

#ifdef FREECIV_MSWINDOWS
typedef long int fc_errno;
#else
typedef int fc_errno;
#endif

#define fc_malloc(sz) malloc(sz)
#define fc_realloc(ptr, sz) realloc(ptr, sz)

#define NFCPP_FREE(ptr)                                                     \
  do {                                                                      \
    if (ptr) {                                                              \
      delete[](ptr);                                                        \
    }                                                                       \
  } while (false)

#define NFC_FREE(ptr)                                                       \
  do {                                                                      \
    if (ptr) {                                                              \
      delete (ptr);                                                         \
    }                                                                       \
  } while (false)

#define NFCN_FREE(ptr)                                                      \
  do {                                                                      \
    if (ptr) {                                                              \
      delete (ptr);                                                         \
      (ptr) = nullptr;                                                      \
    }                                                                       \
  } while (false)

#define VOIDNFCN_FREE(ptr)                                                  \
  do {                                                                      \
    if (ptr) {                                                              \
      ::operator delete(ptr);                                               \
      (ptr) = nullptr;                                                      \
    }                                                                       \
  } while (false)

#define NFCNPP_FREE(ptr)                                                    \
  do {                                                                      \
    if (ptr) {                                                              \
      delete[](ptr);                                                        \
      (ptr) = nullptr;                                                      \
    }                                                                       \
  } while (false)

#define FCPP_FREE(ptr)                                                      \
  do {                                                                      \
    delete[](ptr);                                                          \
    (ptr) = NULL;                                                           \
  } while (false)

#define FC_FREE(ptr)                                                        \
  do {                                                                      \
    delete (ptr);                                                           \
    (ptr) = NULL;                                                           \
  } while (false)

#define fc_strdup(str) real_fc_strdup((str), "strdup", __FC_LINE__, __FILE__)

char *real_fc_strdup(const char *str, const char *called_as, int line,
                     const char *file) fc__warn_unused_result;

int fc_strcasecmp(const char *str0, const char *str1);
int fc_strncasecmp(const char *str0, const char *str1, size_t n);
int fc_strncasequotecmp(const char *str0, const char *str1, size_t n);

size_t effectivestrlenquote(const char *str);

int fc_strcoll(const char *str0, const char *str1);
int fc_stricoll(const char *str0, const char *str1);

FILE *fc_fopen(const char *filename, const char *opentype);

fc_errno fc_get_errno();
const char *fc_strerror(fc_errno err);
void fc_usleep(unsigned long usec);

bool fc_strrep(char *str, size_t len, const char *search,
               const char *replace);

size_t fc_strlcpy(char *dest, const char *src, size_t n);
size_t fc_strlcat(char *dest, const char *src, size_t n);

// convenience macros for use when dest is a char ARRAY:
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

int fc_break_lines(char *str, size_t desired_len);

void make_escapes(const char *str, char *buf, size_t buf_len);
QString remove_escapes(const QString &str, bool full_escapes);

int fc_at_quick_exit(void (*func)());
