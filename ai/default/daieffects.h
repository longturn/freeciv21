/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once
adv_want dai_effect_value(struct player *pplayer, struct government *gov,
                          const struct adv_data *adv,
                          const struct city *pcity, const bool capital,
                          int turns, const struct effect *peffect,
                          const int c, const int nplayers);

adv_want dai_content_effect_value(const struct player *pplayer,
                                  const struct city *pcity, int amount,
                                  int num_cities, int happiness_step);

bool dai_can_requirement_be_met_in_city(const struct requirement *preq,
                                        const struct player *pplayer,
                                        const struct city *pcity);

