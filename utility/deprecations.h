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

#ifndef FC__DEPRECATIONS_H
#define FC__DEPRECATIONS_H

// Qt
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(deprecations_category)

typedef void (*deprecation_warn_callback)(const char *msg);

void deprecation_warn_cb_set(deprecation_warn_callback new_cb);

#endif /* FC__DEPRECATIONS_H */
