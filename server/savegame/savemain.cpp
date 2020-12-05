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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDebug>
#include <QDir>
#include <QString>

/* utility */
#include "log.h"
#include "registry.h"

/* common */
#include "ai.h"
#include "capability.h"
#include "game.h"

/* server */
#include "console.h"
#include "notify.h"

/* server/savegame */
#include "savegame2.h"
#include "savegame3.h"

#include "savemain.h"

Q_GLOBAL_STATIC(fcThread, save_thread);

/************************************************************************/ /**
   Main entry point for loading a game.
 ****************************************************************************/
void savegame_load(struct section_file *sfile)
{
  const char *savefile_options;

  fc_assert_ret(sfile != NULL);

  civtimer *loadtimer = timer_new(TIMER_CPU, TIMER_DEBUG);
  timer_start(loadtimer);

  savefile_options = secfile_lookup_str(sfile, "savefile.options");

  if (!savefile_options) {
    qCritical("Missing savefile options. Can not load the savegame.");
    return;
  }

  if (has_capabilities("+version3", savefile_options)) {
    /* load new format (freeciv 3.0.x and newer) */
    qDebug("loading savefile in 3.0+ format ...");
    savegame3_load(sfile);
  } else if (has_capabilities("+version2", savefile_options)) {
    /* load old format (freeciv 2.3 - 2.6) */
    qDebug("loading savefile in 2.3 - 2.6 format ...");
    savegame2_load(sfile);
  } else {
    qCritical("Too old savegame format not supported any more.");
    return;
  }

  players_iterate(pplayer)
  {
    unit_list_iterate(pplayer->units, punit)
    {
      CALL_FUNC_EACH_AI(unit_created, punit);
      CALL_PLR_AI_FUNC(unit_got, pplayer, punit);
    }
    unit_list_iterate_end;

    city_list_iterate(pplayer->cities, pcity)
    {
      CALL_FUNC_EACH_AI(city_created, pcity);
      CALL_PLR_AI_FUNC(city_got, pplayer, pplayer, pcity);
    }
    city_list_iterate_end;
  }
  players_iterate_end;

  timer_stop(loadtimer);
  qCDebug(timers_category, "Loading secfile in %.3f seconds.",
          timer_read_seconds(loadtimer));
  timer_destroy(loadtimer);
}

/************************************************************************/ /**
   Main entry point for saving a game.
 ****************************************************************************/
void savegame_save(struct section_file *sfile, const char *save_reason,
                   bool scenario)
{
  savegame3_save(sfile, save_reason, scenario);
}

struct save_thread_data {
  struct section_file *sfile;
  char filepath[600];
  int save_compress_level;
  enum fz_method save_compress_type;
};

/************************************************************************/ /**
   Run game saving thread.
 ****************************************************************************/
static void save_thread_run(void *arg)
{
  struct save_thread_data *stdata = (struct save_thread_data *) arg;

  if (!secfile_save(stdata->sfile, stdata->filepath,
                    stdata->save_compress_level,
                    stdata->save_compress_type)) {
    con_write(C_FAIL, _("Failed saving game as %s"), stdata->filepath);
    qCritical("Game saving failed: %s", secfile_error());
    notify_conn(NULL, NULL, E_LOG_ERROR, ftc_warning,
                _("Failed saving game."));
  } else {
    con_write(C_OK, _("Game saved as %s"), stdata->filepath);
  }

  secfile_destroy(stdata->sfile);
  delete stdata;
}

/************************************************************************/ /**
   Unconditionally save the game, with specified filename.
   Always prints a message: either save ok, or failed.
 ****************************************************************************/
void save_game(const char *orig_filename, const char *save_reason,
               bool scenario)
{
  char *dot, *filename;
  civtimer *timer_cpu;
  struct save_thread_data *stdata = new save_thread_data();

  stdata->save_compress_type = game.server.save_compress_type;
  stdata->save_compress_level = game.server.save_compress_level;

  if (!orig_filename) {
    stdata->filepath[0] = '\0';
    filename = stdata->filepath;
  } else {
    sz_strlcpy(stdata->filepath, orig_filename);
    if ((filename = strrchr(stdata->filepath, '/'))) {
      filename++;
    } else {
      filename = stdata->filepath;
    }

    /* Ignores the dot at the start of the filename. */
    for (dot = filename; '.' == *dot; dot++) {
      /* Nothing. */
    }
    if ('\0' == *dot) {
      /* Only dots in this file name, consider it as empty. */
      filename[0] = '\0';
    } else {
      char *end_dot;
      const char *strip_extensions[] = {".sav", ".gz", ".bz2", ".xz", NULL};
      bool stripped = TRUE;

      while ((end_dot = strrchr(dot, '.')) && stripped) {
        int i;

        stripped = FALSE;

        for (i = 0; strip_extensions[i] != NULL && !stripped; i++) {
          if (!strcmp(end_dot, strip_extensions[i])) {
            *end_dot = '\0';
            stripped = TRUE;
          }
        }
      }
    }
  }

  /* If orig_filename is NULL or empty, use a generated default name. */
  if (filename[0] == '\0') {
    /* manual save */
    generate_save_name(
        game.server.save_name, filename,
        sizeof(stdata->filepath) + stdata->filepath - filename, "manual");
  }

  timer_cpu = timer_new(TIMER_CPU, TIMER_ACTIVE);
  timer_start(timer_cpu);

  /* Allowing duplicates shouldn't be allowed. However, it takes very too
   * long time for huge game saving... */
  stdata->sfile = secfile_new(TRUE);
  savegame_save(stdata->sfile, save_reason, scenario);

  /* We have consistent game state in stdata->sfile now, so
   * we could pass it to the saving thread already. We want to
   * handle below notify_conn() and directory creation in
   * main thread, though. */

  /* Append ".sav" to filename. */
  sz_strlcat(stdata->filepath, ".sav");

  if (stdata->save_compress_level > 0) {
    switch (stdata->save_compress_type) {
    case FZ_ZLIB:
      /* Append ".gz" to filename. */
      sz_strlcat(stdata->filepath, ".gz");
      break;
#ifdef FREECIV_HAVE_LIBBZ2
    case FZ_BZIP2:
      /* Append ".bz2" to filename. */
      sz_strlcat(stdata->filepath, ".bz2");
      break;
#endif
#ifdef FREECIV_HAVE_LIBLZMA
    case FZ_XZ:
      /* Append ".xz" to filename. */
      sz_strlcat(stdata->filepath, ".xz");
      break;
#endif
    case FZ_PLAIN:
      break;
    default:
      qCritical(_("Unsupported compression type %d."),
                stdata->save_compress_type);
      notify_conn(NULL, NULL, E_SETTING, ftc_warning,
                  _("Unsupported compression type %d."),
                  stdata->save_compress_type);
      break;
    }
  }

  if (!path_is_absolute(stdata->filepath)) {
    QString tmpname;

    if (!scenario) {
      /* Ensure the saves directory exists. */
      if (srvarg.saves_pathname.isNull()) {
        QDir().mkpath(srvarg.saves_pathname);
      }

      tmpname = srvarg.saves_pathname;
    } else {
      /* Make sure scenario directory exist */
      if (srvarg.saves_pathname.isNull()) {
        QDir().mkpath(srvarg.scenarios_pathname);
      }

      tmpname = srvarg.scenarios_pathname;
    }

    if (!tmpname.isEmpty()) {
      tmpname += QLatin1String("/");
    }
    tmpname += QString::fromUtf8(stdata->filepath);
    sz_strlcpy(stdata->filepath, qUtf8Printable(tmpname));
  }

  // Don't start another thread if game is being saved now
  if (!save_thread->isRunning()) {
    save_thread->set_func(save_thread_run, stdata);
    save_thread->start(QThread::LowestPriority);
  }

  log_time(QStringLiteral("Save time: %1 seconds")
               .arg(timer_read_seconds(timer_cpu)));
  timer_destroy(timer_cpu);
}

/************************************************************************/ /**
   Close saving system.
 ****************************************************************************/
void save_system_close(void) { save_thread->wait(); }
