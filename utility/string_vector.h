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
#pragma once

#include <QVector>
#include <QString>
/* utility */
#include "support.h" /* bool type. */

struct astring;

struct strvec; /* Opaque. */

struct strvec *strvec_new(void);
void strvec_destroy(struct strvec *psv);

void strvec_reserve(struct strvec *psv, size_t reserve);
void strvec_store(struct strvec *psv, const char *const *vec, size_t size);
void strvec_clear(struct strvec *psv);
void strvec_prepend(struct strvec *psv, const char *string);
void strvec_append(struct strvec *psv, const char *string);
void strvec_insert(struct strvec *psv, size_t svindex, const char *string);
bool strvec_set(struct strvec *psv, size_t svindex, const char *string);
size_t strvec_size(const struct strvec *psv);
const char *const *strvec_data(const struct strvec *psv);
bool strvec_index_valid(const struct strvec *psv, size_t svindex);
const char *strvec_get(const struct strvec *psv, size_t svindex);
const char *strvec_to_or_list(const struct strvec *psv,
                              struct astring *astr);
const char *strvec_to_and_list(const struct strvec *psv,
                               struct astring *astr);
void qstrvec_store(QVector<QString> *psv, const char *const *vec,
                   size_t size);
