// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "version.h"

// generated
#include <fc_version.h>

// utility
#include "fcintl.h"
#include "support.h"

/**
 * Returns the raw version string
 */
const char *freeciv21_version() { return VERSION_STRING; }

/**
   Return string containing both name of Freeciv21 and version.
 */
const char *freeciv_name_version()
{
  static char msgbuf[256];

  fc_snprintf(msgbuf, sizeof(msgbuf), _("Freeciv21 version %s"),
              freeciv21_version());

  return msgbuf;
}
