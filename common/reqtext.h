// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv and Freeciv21 Contributors

#pragma once

// common
#include "fc_types.h"

// std
#include <cstddef> // size_t

enum rt_verbosity { VERB_DEFAULT, VERB_ACTUAL };

bool req_text_insert(char *buf, size_t bufsz, struct player *pplayer,
                     const struct requirement *preq, enum rt_verbosity verb,
                     const char *prefix);

bool req_text_insert_nl(char *buf, size_t bufsz, struct player *pplayer,
                        const struct requirement *preq,
                        enum rt_verbosity verb, const char *prefix);
