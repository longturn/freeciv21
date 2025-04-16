// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// Forward declarations
class QJsonDocument;
class QString;
class QUrl;

// std
#include <functional> // nf_errmsg

struct section_file;

using nf_errmsg = std::function<void(const QString &message)>;

QJsonDocument netfile_get_json_file(const QUrl &url, const nf_errmsg &cb);

section_file *netfile_get_section_file(const QUrl &url, const nf_errmsg &cb);

bool netfile_download_file(const QUrl &url, const char *filename,
                           const nf_errmsg &cb);
