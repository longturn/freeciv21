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
#include "support.h" /* bool type */

#define MAX_AUDIO_NAME_LEN 20
#define MAX_AUDIO_DESCR_LEN 200

#define MAX_ALT_AUDIO_FILES 5

typedef void (*audio_finished_callback)(void);

class QString;
struct audio_plugin {
  char name[MAX_AUDIO_NAME_LEN];
  char descr[MAX_AUDIO_DESCR_LEN];
  bool (*init)(void);
  void (*shutdown)(void);
  void (*stop)(void);
  void (*wait)(void);
  double (*get_volume)(void);
  void (*set_volume)(double volume);
  bool (*play)(const char *const tag, const char *const path, bool repeat,
               audio_finished_callback cb);
};

enum music_usage { MU_SINGLE, MU_MENU, MU_INGAME };

struct strvec;
struct option;
const struct strvec *get_soundplugin_list(const struct option *poption);
const struct strvec *get_soundset_list(const struct option *poption);
const struct strvec *get_musicset_list(const struct option *poption);

void audio_init(void);
void audio_real_init(QString &soundspec_name, QString &musicset_name,
                     QString &preferred_plugin_name);
void audio_add_plugin(struct audio_plugin *p);
void audio_shutdown(void);
void audio_stop(void);
void audio_stop_usage(void);
void audio_restart(QString soundset_name, QString musicset_name);

void audio_play_sound(const char *const tag, const char *const alt_tag);
void audio_play_music(const char *const tag, char *const alt_tag,
                      enum music_usage usage);
void audio_play_track(const char *const tag, char *const alt_tag);

double audio_get_volume(void);
void audio_set_volume(double volume);

bool audio_select_plugin(QString &name);
const char *audio_get_all_plugin_names(void);

