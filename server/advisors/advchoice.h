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

/* Uncomment to have choice information tracked */
/* #define ADV_CHOICE_TRACK */

#ifdef ADV_CHOICE_TRACK
#define ADV_CHOICE_QtMsgType LOG_NORMAL
#endif

enum choice_type {
  CT_NONE = 0,
  CT_BUILDING = 1,
  CT_CIVILIAN,
  CT_ATTACKER,
  CT_DEFENDER,
  CT_LAST
};

struct adv_choice {
  enum choice_type type;
  universals_u value; /* what the advisor wants */
  adv_want want;      /* how much it wants it */
  bool need_boat;     /* unit being built wants a boat */
#ifdef ADV_CHOICE_TRACK
  char *use;
  bool log_if_chosen;
#endif /* ADV_CHOICE_TRACK */
};

void adv_init_choice(struct adv_choice *choice);
void adv_deinit_choice(struct adv_choice *choice);

struct adv_choice *adv_new_choice(void);
void adv_free_choice(struct adv_choice *choice);

struct adv_choice *adv_better_choice(struct adv_choice *first,
                                     struct adv_choice *second);
struct adv_choice *adv_better_choice_free(struct adv_choice *first,
                                          struct adv_choice *second);

bool is_unit_choice_type(enum choice_type type);

#ifdef ADV_CHOICE_TRACK
void adv_choice_copy(struct adv_choice *dest, struct adv_choice *src);
void adv_choice_set_use(struct adv_choice *choice, const char *use);
void adv_choice_log_info(struct adv_choice *choice, const char *loc1,
                         const char *loc2);
const char *adv_choice_get_use(const struct adv_choice *choice);
#else /* ADV_CHOICE_TRACK */
static inline void adv_choice_copy(struct adv_choice *dest,
                                   struct adv_choice *src)
{
  if (dest != src) {
    *dest = *src;
  }
}
#define adv_choice_set_use(_choice, _use)
#define adv_choice_log_info(_choice, _loc1, _loc2)
static inline const char *adv_choice_get_use(const struct adv_choice *choice)
{
  return "(unknown)";
}
#endif /* ADV_CHOICE_TRACK */

#define ADV_CHOICE_ASSERT(c)                                                \
  do {                                                                      \
    if ((c).want > 0) {                                                     \
      fc_assert((c).type > CT_NONE && (c).type < CT_LAST);                  \
      if (!is_unit_choice_type((c).type)) {                                 \
        int _iindex = improvement_index((c).value.building);                \
        fc_assert(_iindex >= 0 && _iindex < improvement_count());           \
      } else {                                                              \
        int _uindex = utype_index((c).value.utype);                         \
        fc_assert(_uindex >= 0 && _uindex < utype_count());                 \
      }                                                                     \
    }                                                                       \
  } while (FALSE);


