/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once
// Qt
#include <QString>
#include <QtGlobal>

/* utility */
#include "bitvector.h"
#include "support.h"

/* common */
#include "fc_types.h"

// server
#include "notify.h"

struct ai_data;

/*
 * Change these and remake to watch logs from a specific
 * part of the AI code.
 * FIXME Obsolete, remove them
 */
#define LOGLEVEL_BODYGUARD LOG_DEBUG
#define LOGLEVEL_UNIT LOG_DEBUG
#define LOGLEVEL_GOTO LOG_DEBUG
#define LOGLEVEL_CITY LOG_DEBUG
#define LOGLEVEL_BUILD LOG_DEBUG
#define LOGLEVEL_HUNT LOG_DEBUG
#define LOGLEVEL_PLAYER LOG_DEBUG

#define LOG_AI_TEST LOG_NORMAL

enum ai_timer {
  AIT_ALL,
  AIT_MOVEMAP,
  AIT_UNITS,
  AIT_SETTLERS,
  AIT_WORKERS,
  AIT_AIDATA,
  AIT_GOVERNMENT,
  AIT_TAXES,
  AIT_CITIES,
  AIT_CITIZEN_ARRANGE,
  AIT_BUILDINGS,
  AIT_DANGER,
  AIT_TECH,
  AIT_FSTK,
  AIT_DEFENDERS,
  AIT_CARAVAN,
  AIT_HUNTER,
  AIT_AIRLIFT,
  AIT_DIPLOMAT,
  AIT_AIRUNIT,
  AIT_EXPLORER,
  AIT_EMERGENCY,
  AIT_CITY_MILITARY,
  AIT_CITY_TERRAIN,
  AIT_CITY_SETTLERS,
  AIT_ATTACK,
  AIT_MILITARY,
  AIT_RECOVER,
  AIT_BODYGUARD,
  AIT_FERRY,
  AIT_RAMPAGE,
  AIT_LAST
};

enum ai_timer_activity { TIMER_START, TIMER_STOP };

QString city_log_prefix(const city *pcity);
#define CITY_LOG(_, pcity, msg, ...)                                        \
  {                                                                         \
    bool notify = pcity->server.debug;                                      \
    QString message = city_log_prefix(pcity) + QStringLiteral(" ")          \
                      + QString::asprintf(msg, ##__VA_ARGS__);              \
    if (notify) {                                                           \
      qInfo().noquote() << message;                                         \
      notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s",                    \
                  qPrintable(message));                                     \
    } else {                                                                \
      qDebug().noquote() << message;                                        \
    }                                                                       \
  }

QString unit_log_prefix(const unit *punit);
#define UNIT_LOG(_, punit, msg, ...)                                        \
  {                                                                         \
    bool notify = punit->server.debug                                       \
                  || (tile_city(unit_tile(punit))                           \
                      && tile_city(unit_tile(punit))->server.debug);        \
    QString message = unit_log_prefix(punit) + QStringLiteral(" ")          \
                      + QString::asprintf(msg, ##__VA_ARGS__);              \
    if (notify) {                                                           \
      qInfo().noquote() << message;                                         \
      notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s",                    \
                  qPrintable(message));                                     \
    } else {                                                                \
      qDebug().noquote() << message;                                        \
    }                                                                       \
  }

void timing_log_init(void);
void timing_log_free(void);

void timing_log_real(enum ai_timer timer, enum ai_timer_activity activity);
void timing_results_real(void);

#ifdef FREECIV_DEBUG
#define TIMING_LOG(timer, activity) timing_log_real(timer, activity)
#define TIMING_RESULTS() timing_results_real()
#else /* FREECIV_DEBUG */
#define TIMING_LOG(timer, activity)
#define TIMING_RESULTS()
#endif /* FREECIV_DEBUG */

