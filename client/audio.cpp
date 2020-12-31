/***********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <QString>
#include <QVector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* client */
#include "audio_none.h"
#ifdef AUDIO_SDL
#include "audio_sdl.h"
#endif
#include "client_main.h"
#include "options.h"

#include "audio.h"

#define MAX_NUM_PLUGINS 2
#define SNDSPEC_SUFFIX ".soundspec"
#define MUSICSPEC_SUFFIX ".musicspec"

#define SOUNDSPEC_CAPSTR "+Freeciv-3.0-soundset"
#define MUSICSPEC_CAPSTR "+Freeciv-2.6-musicset"

/* keep it open throughout */
static struct section_file *ss_tagfile = NULL;
static struct section_file *ms_tagfile = NULL;

static struct audio_plugin plugins[MAX_NUM_PLUGINS];
static int num_plugins_used = 0;
static int selected_plugin = -1;
static int current_track = -1;
static enum music_usage current_usage;
static bool switching_usage = false;

static struct mfcb_data {
  struct section_file *sfile;
  QString tag;
} mfcb;

Q_GLOBAL_STATIC(QVector<QString>, plugin_list)

static int audio_play_tag(struct section_file *sfile, const QString &tag,
                          bool repeat, int exclude, bool keepstyle);

/**********************************************************************/ /**
   Returns a static string vector of all sound plugins
   available on the system.  This function is unfortunately similar to
   audio_get_all_plugin_names().
 **************************************************************************/
const QVector<QString> *get_soundplugin_list(const struct option *poption)
{
  if (plugin_list->isEmpty()) {
    for (int i = 0; i < num_plugins_used; i++) {
      plugin_list->append(plugins[i].name);
    }
  }

  return plugin_list;
}

/**********************************************************************/ /**
   Returns a static string vector of audiosets of given type available
   on the system by searching all data directories for files matching
   suffix.
   The list is NULL-terminated.
 **************************************************************************/
static const QVector<QString> *
get_audio_speclist(const char *suffix, QVector<QString> **audio_list)
{
  if (NULL == *audio_list) {
    *audio_list = fileinfolist(get_data_dirs(), suffix);
  }

  return *audio_list;
}

/**********************************************************************/ /**
   Returns a static string vector of soundsets available on the system.
 **************************************************************************/
const QVector<QString> *get_soundset_list(const struct option *poption)
{
  static QVector<QString> *sound_list = NULL;

  return get_audio_speclist(SNDSPEC_SUFFIX, &sound_list);
}

/**********************************************************************/ /**
   Returns a static string vector of musicsets available on the system.
 **************************************************************************/
const QVector<QString> *get_musicset_list(const struct option *poption)
{
  static QVector<QString> *music_list = NULL;

  return get_audio_speclist(MUSICSPEC_SUFFIX, &music_list);
}

/**********************************************************************/ /**
   Add a plugin.
 **************************************************************************/
void audio_add_plugin(struct audio_plugin *p)
{
  fc_assert_ret(num_plugins_used < MAX_NUM_PLUGINS);
  plugins[num_plugins_used].descr = p->descr;
  plugins[num_plugins_used].get_volume = p->get_volume;
  plugins[num_plugins_used].init = p->init;
  plugins[num_plugins_used].name = p->name;
  plugins[num_plugins_used].play = p->play;
  plugins[num_plugins_used].set_volume = p->set_volume;
  plugins[num_plugins_used].shutdown = p->shutdown;
  plugins[num_plugins_used].stop = p->stop;
  plugins[num_plugins_used].wait = p->wait;
  num_plugins_used++;
}

/**********************************************************************/ /**
   Choose plugin. Returns TRUE on success, FALSE if not
 **************************************************************************/
bool audio_select_plugin(const QString &name)
{
  int i;
  bool found = false;

  for (i = 0; i < num_plugins_used; i++) {
    if (QString(plugins[i].name) == name) {
      found = true;
      break;
    }
  }

  if (found && i != selected_plugin) {
    log_debug("Shutting down %s",
              qUtf8Printable(plugins[selected_plugin].name));
    plugins[selected_plugin].stop();
    plugins[selected_plugin].wait();
    plugins[selected_plugin].shutdown();
  }

  if (!found) {
    qFatal(_("Plugin '%s' isn't available. Available are %s"),
           qUtf8Printable(name),
           qUtf8Printable(audio_get_all_plugin_names()));
    exit(EXIT_FAILURE);
  }

  if (!plugins[i].init()) {
    qCritical("Plugin %s found, but can't be initialized.",
              qUtf8Printable(name));
    return false;
  }

  selected_plugin = i;
  qDebug("Plugin '%s' is now selected",
         qUtf8Printable(plugins[selected_plugin].name));

  plugins[selected_plugin].set_volume(gui_options.sound_effects_volume
                                      / 100.0);

  return true;
}

/**********************************************************************/ /**
   Initialize base audio system. Note that this function is called very
   early at the client startup. So for example logging isn't available.
 **************************************************************************/
void audio_init()
{
#ifdef AUDIO_SDL
  audio_sdl_init();
#endif

  /* Initialize dummy plugin last, as lowest priority plugin. This
   * affects which plugin gets selected as default in new installations. */
  audio_none_init();
  selected_plugin = 0;
}

/**********************************************************************/ /**
   Returns the filename for the given audio set. Returns NULL if
   set couldn't be found. Caller has to free the return value.
 **************************************************************************/
static const QString audiospec_fullname(const QString &audioset_name,
                                        bool music)
{
  const QString suffix = music ? MUSICSPEC_SUFFIX : SNDSPEC_SUFFIX;
  QString audioset_default =
      music ? QStringLiteral("stdmusic") : QStringLiteral("stdsounds");
  const char *dname;

  QString fname = QStringLiteral("%1%2").arg(audioset_name, suffix);
  dname = fileinfoname(get_data_dirs(), qUtf8Printable(fname));

  if (dname) {
    return QString(dname);
  }

  if (audioset_name == audioset_default) {
    /* avoid endless recursion */
    return NULL;
  }

  qCritical("Couldn't find audioset \"%s\", trying \"%s\".",
            qUtf8Printable(audioset_name), qUtf8Printable(audioset_default));

  return audiospec_fullname(audioset_default, music);
}

/**********************************************************************/ /**
   Check capabilities of the audio specfile
 **************************************************************************/
static bool check_audiofile_capstr(struct section_file *sfile,
                                   const QString *filename,
                                   const QString *our_cap,
                                   const QString *opt_path)
{
  const char *file_capstr;

  file_capstr = secfile_lookup_str(sfile, "%s", qUtf8Printable(*opt_path));
  if (NULL == file_capstr) {
    qFatal("Audio spec-file \"%s\" doesn't have capability string.",
           qUtf8Printable(*filename));
    exit(EXIT_FAILURE);
  }
  if (!has_capabilities(qUtf8Printable(*our_cap), file_capstr)) {
    qFatal("Audio spec-file appears incompatible:");
    qFatal("  file: \"%s\"", qUtf8Printable(*filename));
    qFatal("  file options: %s", file_capstr);
    qFatal("  supported options: %s", qUtf8Printable(*our_cap));
    exit(EXIT_FAILURE);
  }
  if (!has_capabilities(file_capstr, qUtf8Printable(*our_cap))) {
    qFatal("Audio spec-file claims required option(s) "
           "which we don't support:");
    qFatal("  file: \"%s\"", qUtf8Printable(*filename));
    qFatal("  file options: %s", file_capstr);
    qFatal("  supported options: %s", qUtf8Printable(*our_cap));
    exit(EXIT_FAILURE);
  }

  return true;
}

/**********************************************************************/ /**
   Initialize audio system and autoselect a plugin
 **************************************************************************/
void audio_real_init(const QString &soundset_name,
                     const QString &musicset_name,
                     const QString &preferred_plugin_name)
{
  QString ss_filename;
  QString ms_filename;
  const QString us_ss_capstr = SOUNDSPEC_CAPSTR;
  const QString us_ms_capstr = MUSICSPEC_CAPSTR;

  if (preferred_plugin_name == QLatin1String("none")) {
    /* We explicitly choose none plugin, silently skip the code below */
    qDebug("Proceeding with sound support disabled.");
    ss_tagfile = NULL;
    ms_tagfile = NULL;
    return;
  }
  if (num_plugins_used == 1) {
    /* We only have the dummy plugin, skip the code but issue an advertise */
    qInfo(_("No real audio plugin present."));
    qInfo(_("Proceeding with sound support disabled."));
    qInfo(_("For sound support, install SDL2_mixer"));
    qInfo("http://www.libsdl.org/projects/SDL_mixer/index.html");
    ss_tagfile = NULL;
    ms_tagfile = NULL;
    return;
  }
  if (soundset_name.isEmpty()) {
    qFatal("No sound spec-file given!");
    exit(EXIT_FAILURE);
  }
  if (musicset_name.isEmpty()) {
    qFatal("No music spec-file given!");
    exit(EXIT_FAILURE);
  }
  qDebug("Initializing sound using %s and %s...",
         qUtf8Printable(soundset_name), qUtf8Printable(musicset_name));
  ss_filename = audiospec_fullname(soundset_name, false);
  ms_filename = audiospec_fullname(musicset_name, true);
  if (ss_filename.isEmpty() || ms_filename.isEmpty()) {
    qCritical("Cannot find audio spec-file \"%s\" or \"%s\"",
              qUtf8Printable(soundset_name), qUtf8Printable(musicset_name));
    qInfo(_("To get sound you need to download a sound set!"));
    qInfo(_("Get sound sets from <%s>."),
          "http://www.freeciv.org/wiki/Sounds");
    qInfo(_("Proceeding with sound support disabled."));
    ss_tagfile = NULL;
    ms_tagfile = NULL;
    return;
  }
  ss_tagfile = secfile_load(qUtf8Printable(ss_filename), true);
  if (!ss_tagfile) {
    qFatal(_("Could not load sound spec-file '%s':\n%s"),
           qUtf8Printable(ss_filename), secfile_error());
    exit(EXIT_FAILURE);
  }
  ms_tagfile = secfile_load(qUtf8Printable(ms_filename), true);
  if (!ms_tagfile) {
    qFatal(_("Could not load music spec-file '%s':\n%s"),
           qUtf8Printable(ms_filename), secfile_error());
    exit(EXIT_FAILURE);
  }
  QString t0 = QStringLiteral("soundspec.options");
  check_audiofile_capstr(ss_tagfile, &ss_filename, &us_ss_capstr, &t0);

  QString t1 = QStringLiteral("musicspec.options");
  check_audiofile_capstr(ms_tagfile, &ms_filename, &us_ms_capstr, &t1);

  atexit(audio_shutdown);

  if (!preferred_plugin_name.isEmpty()) {
    if (!audio_select_plugin(preferred_plugin_name)) {
      qInfo(_("Proceeding with sound support disabled."));
}
    return;
  }

#ifdef AUDIO_SDL
  QString audio_str = QStringLiteral("sdl");
  if (audio_select_plugin(audio_str)) {
    return;
}
#endif
  qInfo(_("No real audio subsystem managed to initialize!"));
  qInfo(_("Perhaps there is some misconfiguration or bad permissions."));
  qInfo(_("Proceeding with sound support disabled."));
}

/**********************************************************************/ /**
   Switch soundset
 **************************************************************************/
void audio_restart(const QString &soundset_name,
                   const QString &musicset_name)
{
  audio_stop(); /* Fade down old one */

  sound_set_name = soundset_name;
  music_set_name = musicset_name;
  audio_real_init(sound_set_name, music_set_name, sound_plugin_name);
}

/**********************************************************************/ /**
   Callback to start new track
 **************************************************************************/
static void music_finished_callback()
{
  bool usage_enabled = true;

  if (switching_usage) {
    switching_usage = false;
    return;
  }

  switch (current_usage) {
  case MU_SINGLE:
    usage_enabled = false;
    break;
  case MU_MENU:
    usage_enabled = gui_options.sound_enable_menu_music;
    break;
  case MU_INGAME:
    usage_enabled = gui_options.sound_enable_game_music;
    break;
  }

  if (usage_enabled) {
    current_track =
        audio_play_tag(mfcb.sfile, mfcb.tag, true, current_track, false);
  }
}

/**********************************************************************/ /**
   INTERNAL. Returns id (>= 0) of the tag selected for playing, 0 when
   there's no alternative tags, or negative value in case of error.
 **************************************************************************/
static int audio_play_tag(struct section_file *sfile, const QString &tag,
                          bool repeat, int exclude, bool keepstyle)
{
  const char *soundfile;
  QString fullpath;
  audio_finished_callback cb = NULL;
  int ret = 0;

  if (tag.isEmpty() || (tag == QLatin1String("-"))) {
    return -1;
  }

  if (sfile) {
    soundfile = secfile_lookup_str(sfile, "files.%s", qUtf8Printable(tag));
    if (soundfile == NULL) {
      const char *files[MAX_ALT_AUDIO_FILES];
      int excluded = -1;
      int i;
      int j;

      j = 0;
      for (i = 0; i < MAX_ALT_AUDIO_FILES; i++) {
        const char *ftmp =
            secfile_lookup_str(sfile, "files.%s_%d", qUtf8Printable(tag), i);

        if (ftmp == NULL) {
          if (excluded != -1 && j == 0) {
            /* Cannot exclude the only track */
            excluded = -1;
            j++;
          }
          files[j] = NULL;
          break;
        }
        files[j] = ftmp;
        if (i != exclude) {
          j++;
        } else {
          excluded = j;
        }
      }

      if (j > 0) {
        ret = fc_rand(j);

        soundfile = files[ret];
        if (excluded != -1 && excluded < ret) {
          /* Exclude track was skipped earlier, include it to track number to
           * return */
          ret++;
        }
        if (repeat) {
          if (!keepstyle) {
            mfcb.sfile = sfile;
            mfcb.tag = tag;
          }
          cb = music_finished_callback;
        }
      }
    }
    if (NULL == soundfile) {
      qDebug("No sound file for tag %s", qUtf8Printable(tag));
    } else {
      fullpath = fileinfoname(get_data_dirs(), soundfile);
      if (fullpath.isEmpty()) {
        qCritical("Cannot find audio file %s for tag %s", soundfile,
                  qUtf8Printable(tag));
      }
    }
  }

  if (!plugins[selected_plugin].play(tag, fullpath, repeat, cb)) {
    return -1;
  }

  return ret;
}

/**********************************************************************/ /**
   Play tag from sound set
 **************************************************************************/
static bool audio_play_sound_tag(const QString &tag, bool repeat)
{
  return (audio_play_tag(ss_tagfile, tag, repeat, -1, false) >= 0);
}

/**********************************************************************/ /**
   Play tag from music set
 **************************************************************************/
static int audio_play_music_tag(const QString &tag, bool repeat,
                                bool keepstyle)
{
  return audio_play_tag(ms_tagfile, tag, repeat, -1, keepstyle);
}

/**********************************************************************/ /**
   Play an audio sample as suggested by sound tags
 **************************************************************************/
void audio_play_sound(const QString &tag, const QString &alt_tag)
{
  const QString pretty_alt_tag =
      alt_tag.isEmpty() ? QStringLiteral("(null)") : alt_tag;

  if (gui_options.sound_enable_effects) {
    fc_assert_ret(tag != NULL);

    log_debug("audio_play_sound('%s', '%s')", qUtf8Printable(tag),
              qUtf8Printable(pretty_alt_tag));

    /* try playing primary tag first, if not go to alternative tag */
    if (!audio_play_sound_tag(tag, false)
        && !audio_play_sound_tag(alt_tag, false)) {
      qDebug("Neither of tags %s or %s found", qUtf8Printable(tag),
             qUtf8Printable(pretty_alt_tag));
    }
  }
}

/**********************************************************************/ /**
   Play music, either in loop or just one track in the middle of the style
   music.
 **************************************************************************/
static void real_audio_play_music(const QString &tag, const QString &alt_tag,
                                  bool keepstyle)
{
  QString pretty_alt_tag = alt_tag.isEmpty() ? ("(null)") : alt_tag;

  fc_assert_ret(tag != NULL);

  log_debug("audio_play_music('%s', '%s')", qUtf8Printable(tag),
            qUtf8Printable(pretty_alt_tag));

  /* try playing primary tag first, if not go to alternative tag */
  current_track = audio_play_music_tag(tag, true, keepstyle);

  if (current_track < 0) {
    current_track = audio_play_music_tag(alt_tag, true, keepstyle);

    if (current_track < 0) {
      qDebug("Neither of tags %s or %s found", qUtf8Printable(tag),
             qUtf8Printable(pretty_alt_tag));
    }
  }
}

/**********************************************************************/ /**
   Loop music as suggested by sound tags
 **************************************************************************/
void audio_play_music(const QString &tag, const QString &alt_tag,
                      enum music_usage usage)
{
  current_usage = usage;

  real_audio_play_music(tag, alt_tag, false);
}

/**********************************************************************/ /**
   Play single track as suggested by sound tags
 **************************************************************************/
void audio_play_track(const QString &tag, const QString &alt_tag)
{
  current_usage = MU_SINGLE;

  real_audio_play_music(tag, alt_tag, true);
}

/**********************************************************************/ /**
   Stop sound. Music should die down in a few seconds.
 **************************************************************************/
void audio_stop() { plugins[selected_plugin].stop(); }

/**********************************************************************/ /**
   Stop looping sound. Music should die down in a few seconds.
 **************************************************************************/
void audio_stop_usage()
{
  switching_usage = true;
  plugins[selected_plugin].stop();
}

/**********************************************************************/ /**
   Stop looping sound. Music should die down in a few seconds.
 **************************************************************************/
double audio_get_volume()
{
  return plugins[selected_plugin].get_volume();
}

/**********************************************************************/ /**
   Stop looping sound. Music should die down in a few seconds.
 **************************************************************************/
void audio_set_volume(double volume)
{
  plugins[selected_plugin].set_volume(volume);
}

/**********************************************************************/ /**
   Call this at end of program only.
 **************************************************************************/
void audio_shutdown()
{
  /* avoid infinite loop at end of game */
  audio_stop();

  audio_play_sound(QStringLiteral("e_game_quit"), NULL);
  plugins[selected_plugin].wait();
  plugins[selected_plugin].shutdown();

  if (NULL != ss_tagfile) {
    secfile_destroy(ss_tagfile);
    ss_tagfile = NULL;
  }
  if (NULL != ms_tagfile) {
    secfile_destroy(ms_tagfile);
    ms_tagfile = NULL;
  }
}

/**********************************************************************/ /**
   Returns a string which list all available plugins. You don't have to
   free the string.
 **************************************************************************/
const QString audio_get_all_plugin_names()
{
  QString buffer;
  int i;

  buffer = QLatin1String("[");

  for (i = 0; i < num_plugins_used; i++) {
    buffer = buffer + plugins[i].name;
    if (i != num_plugins_used - 1) {
      buffer = buffer + ", ";
    }
  }
  buffer += QLatin1String("]");
  return buffer;
}
