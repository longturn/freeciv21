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
#ifndef FC__MODPACK_DOWNLOAD_H
#define FC__MODPACK_DOWNLOAD_H

#include <functional>

// Qt
#include <QObject>

#include "netfile.h"

/* modinst */
#include "modinst.h"

#define MODPACKDL_SUFFIX ".mpdl"

#define MODPACK_CAPSTR "+Freeciv-mpdl-Devel-3.1-2019.Jul.15"
#define MODLIST_CAPSTR "+Freeciv-modlist-Devel-3.1-2019.Jul.15"

#define FCMP_CONTROLD ".control"

using dl_msg_callback = nf_errmsg;
using dl_pb_callback = std::function<void(int downloaded, int max)>;

const char *download_modpack(const char *URL, const struct fcmp_params *fcmp,
                             const dl_msg_callback &mcb,
                             const dl_pb_callback &pbcb);

using modpack_list_setup_cb = std::function<void(
    const char *name, const char *URL, const char *version,
    const char *license, enum modpack_type type, const char *subtype,
    const char *notes)>;

const char *download_modpack_list(const struct fcmp_params *fcmp,
                                  const modpack_list_setup_cb &cb,
                                  const dl_msg_callback &mcb);

#endif /* FC__MODPACK_DOWNLOAD_H */
