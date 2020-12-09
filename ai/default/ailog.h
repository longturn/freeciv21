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

/* utility */
#include "support.h"

// Qt
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ai_category)

struct player;

void dai_city_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct city *pcity);
void dai_unit_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct unit *punit);

QString tech_log_prefix(ai_type *ait, const player *pplayer,
                        advance *padvance);
#define TECH_LOG(ait, _, pplayer, padvance, msg, ...)                       \
  {                                                                         \
    bool notify = BV_ISSET(pplayer->server.debug, PLAYER_DEBUG_TECH);       \
    QString message = tech_log_prefix(ait, pplayer, padvance)               \
                      + QStringLiteral(" ")                                 \
                      + QString::asprintf(msg, ##__VA_ARGS__);              \
    if (notify) {                                                           \
      qCInfo(ai_category).noquote() << message;                             \
      notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s",                    \
                  qPrintable(message));                                     \
    } else {                                                                \
      qCDebug(ai_category).noquote() << message;                            \
    }                                                                       \
  }

QString diplo_log_prefix(ai_type *ait, const player *pplayer,
                         const player *aplayer);
#define DIPLO_LOG(ait, loglevel, pplayer, aplayer, msg, ...)                \
  {                                                                         \
    bool notify = BV_ISSET(pplayer->server.debug, PLAYER_DEBUG_DIPLOMACY);  \
    QString message = diplo_log_prefix(ait, pplayer, aplayer)               \
                      + QStringLiteral(" ")                                 \
                      + QString::asprintf(msg, ##__VA_ARGS__);              \
    if (notify) {                                                           \
      qCInfo(ai_category).noquote() << message;                             \
      notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s",                    \
                  qPrintable(message));                                     \
    } else {                                                                \
      qCDebug(ai_category).noquote() << message;                            \
    }                                                                       \
  }

QString bodyguard_log_prefix(ai_type *ait, const unit *punit);
#define BODYGUARD_LOG(ait, loglevel, punit, msg, ...)                       \
  {                                                                         \
    bool notify = punit->server.debug;                                      \
    QString message = bodyguard_log_prefix(ait, punit)                      \
                      + QStringLiteral(" ")                                 \
                      + QString::asprintf(msg, ##__VA_ARGS__);              \
    if (notify) {                                                           \
      qCInfo(ai_category).noquote() << message;                             \
      notify_conn(NULL, NULL, E_AI_DEBUG, ftc_log, "%s",                    \
                  qPrintable(message));                                     \
    } else {                                                                \
      qCDebug(ai_category).noquote() << message;                            \
    }                                                                       \
  }
