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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "fcintl.h"
#include "shared.h"
#include "support.h"

// common
#include "fc_types.h"

#include "version.h"

#ifdef GITREV
#include "fc_gitrev_gen.h"
#endif // GITREV

/**
   Return string containing both name of Freeciv21 and version.
 */
const char *freeciv_name_version()
{
  static char msgbuf[256];

#if IS_BETA_VERSION
  fc_snprintf(msgbuf, sizeof(msgbuf), _("Freeciv21 version %s %s"),
              VERSION_STRING, _("(beta version)"));
#elif defined(GITREV) && !defined(FC_GITREV_OFF)
  fc_snprintf(msgbuf, sizeof(msgbuf), _("Freeciv21 version %s (%s)"),
              VERSION_STRING, fc_git_revision());
#else
  fc_snprintf(msgbuf, sizeof(msgbuf), _("Freeciv21 version %s"),
              VERSION_STRING);
#endif

  return msgbuf;
}

/**
   Return string describing version type.
 */
const char *word_version()
{
#if IS_BETA_VERSION
  return _("betatest version ");
#else
  return _("version ");
#endif
}

/**
   Returns string with git revision information if it is possible to
   determine. Can return also some fallback string or even nullptr.
 */
const char *fc_git_revision()
{
#if defined(GITREV) && !defined(FC_GITREV_OFF)
  static char buf[100];
  bool translate = FC_GITREV1[0] != '\0';

  fc_snprintf(buf, sizeof(buf), "%s%s",
              translate ? _(FC_GITREV1) : FC_GITREV1, FC_GITREV2);

  return buf; // Either revision, or modified revision
#else         // FC_GITREV_OFF
  return nullptr;
#endif        // FC_GITREV_OFF
}

/**
   Returns version string that can be used to compare two freeciv builds.
   This does not handle git revisions, as there's no way to compare
   which of the two commits is "higher".
 */
const char *fc_comparable_version() { return VERSION_STRING; }

/**
   Return the BETA message.
   If returns nullptr, not a beta version.
 */
const char *beta_message()
{
#if IS_BETA_VERSION
  static char msgbuf[500];
  static const char *month[] = {
      nullptr,       N_("January"),   N_("February"), N_("March"),
      N_("April"),   N_("May"),       N_("June"),     N_("July"),
      N_("August"),  N_("September"), N_("October"),  N_("November"),
      N_("December")};

  if (FREECIV_RELEASE_MONTH > 0) {
    fc_snprintf(msgbuf, sizeof(msgbuf),
                // TRANS: No full stop after the URL, could cause confusion.
                _("THIS IS A BETA VERSION\n"
                  "Freeciv21 %s will be released in %s, at %s"),
                NEXT_STABLE_VERSION, _(NEXT_RELEASE_MONTH), WIKI_URL);
  } else {
    fc_snprintf(msgbuf, sizeof(msgbuf),
                _("THIS IS A BETA VERSION\n"
                  "Freeciv21 %s will be released at %s"),
                NEXT_STABLE_VERSION, WIKI_URL);
  }
  return msgbuf;
#else  // IS_BETA_VERSION
  return nullptr;
#endif // IS_BETA_VERSION
}

/**
   Return version string in a format suitable to be written to created
   datafiles as human readable information.
 */
const char *freeciv_datafile_version()
{
  static char buf[500] = {'\0'};

  if (buf[0] == '\0') {
    const char *ver_rev;

    ver_rev = fc_git_revision();
    if (ver_rev != nullptr) {
      fc_snprintf(buf, sizeof(buf), "%s (%s)", VERSION_STRING, ver_rev);
    } else {
      fc_snprintf(buf, sizeof(buf), "%s", VERSION_STRING);
    }
  }

  return buf;
}
