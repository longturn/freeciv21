/**********************************************************************
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

#ifndef FC__NETFILE_H
#define FC__NETFILE_H

#include <functional>

// Forward declarations
class QString;
class QUrl;

struct section_file;

using nf_errmsg = std::function<void(const QString &message)>;

section_file *netfile_get_section_file(const QUrl &url, const nf_errmsg &cb);

bool netfile_download_file(const QUrl &url, const char *filename,
                           const nf_errmsg &cb);

#endif /* FC__NETFILE_H */
