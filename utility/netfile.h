/**************************************************************************
 Copyright (c) 1996-2025 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

// Forward declarations
class QJsonDocument;
class QString;
class QUrl;

// std
#include <functional>

struct section_file;

using nf_errmsg = std::function<void(const QString &message)>;

QJsonDocument netfile_get_json_file(const QUrl &url, const nf_errmsg &cb);

section_file *netfile_get_section_file(const QUrl &url, const nf_errmsg &cb);

bool netfile_download_file(const QUrl &url, const char *filename,
                           const nf_errmsg &cb);
