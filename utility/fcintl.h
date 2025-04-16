// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// generated
#include <fc_config.h> // FREECIV_ENABLE_NLS

// utiltiy
#include "support.h" // fc__attribute

// dependency
#ifdef FREECIV_ENABLE_NLS

/**
 * Include libintl.h only if nls enabled.
 * It defines some wrapper macros that
 * we don't want defined when nls is disabled.
 */
#include <libintl.h> // IWYU pragma: keep

// MSYS libintl redefines asprintf/vasprintf as macros, and this clashes with
// QString::asprintf and QString::vasprintf.
#ifdef asprintf
#undef asprintf
#endif

#ifdef vasprintf
#undef vasprintf
#endif

// Core freeciv
#define _(String) gettext(String)
#define DG_(domain, String) dgettext(domain, String)
#define N_(String) String
#define Q_(String) skip_intl_qualifier_prefix(gettext(String))
#define PL_(String1, String2, n) ngettext((String1), (String2), (n))

// Ruledit
#define R__(String) dgettext("freeciv-ruledit", String)
#define RQ_(String)                                                         \
  skip_intl_qualifier_prefix(dgettext("freeciv-ruledit", String))

#else // FREECIV_ENABLE_NLS

// Core freeciv
#define _(String) (String)
#define DG_(domain, String) (String)
#define N_(String) String
#define Q_(String) skip_intl_qualifier_prefix(String)
#define PL_(String1, String2, n) ((n) == 1 ? (String1) : (String2))
#define C_(String) capitalized_string(String)

// Ruledit
#define R__(String) (String)
#define RQ_(String) skip_intl_qualifier_prefix(String)

#endif // FREECIV_ENABLE_NLS

/* This provides an untranslated version of Q_ that allows the caller to
 * get access to the original string.  This may be needed for comparisons,
 * for instance. */
#define Qn_(String) skip_intl_qualifier_prefix(String)

const char *skip_intl_qualifier_prefix(const char *str)
    fc__attribute((__format_arg__(1)));

char *capitalized_string(const char *str);
void free_capitalized(char *str);
void capitalization_opt_in(bool opt_in);
bool is_capitalization_enabled();

const char *get_locale_dir();
