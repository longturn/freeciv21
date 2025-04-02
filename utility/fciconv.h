// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "support.h"

// std
#include <cstddef>
#include <cstdio>

[[deprecated]] char *data_to_internal_string_malloc(const char *text);
[[deprecated]] char *internal_to_data_string_malloc(const char *text);
[[deprecated]] char *internal_to_local_string_malloc(const char *text);
[[deprecated]] char *local_to_internal_string_malloc(const char *text);

[[deprecated]] char *
local_to_internal_string_buffer(const char *text, char *buf, size_t bufsz);

#define fc_printf(...) fc_fprintf(stdout, __VA_ARGS__)
void fc_fprintf(FILE *stream, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)));

size_t get_internal_string_length(const char *text);
