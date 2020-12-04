/**************************************************************************
             ____             Copyright (c) 1996-2020 Freeciv21 and Freeciv
            /    \__          contributors. This file is part of Freeciv21.
|\         /    @   \   Freeciv21 is free software: you can redistribute it
\ \_______|    \  .:|>         and/or modify it under the terms of the GNU
 \      ##|    | \__/     General Public License  as published by the Free
  |    ####\__/   \   Software Foundation, either version 3 of the License,
  /  /  ##       \|                  or (at your option) any later version.
 /  /__________\  \                 You should have received a copy of the
 L_JJ           \__JJ      GNU General Public License along with Freeciv21.
                                 If not, see https://www.gnu.org/licenses/.
**************************************************************************/

#include "pregameoptions.h"
// Qt
#include <QAction>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QPainter>
#include <QSpinBox>
#include <QSplitter>
#include <QTreeWidget>
// utility
#include "fcintl.h"
// common
#include "chatline_common.h"
#include "colors_common.h"
#include "connectdlg_common.h"
#include "game.h"
// client
#include "client_main.h"
#include "climisc.h"
// gui-qt
#include "canvas.h"
#include "chatline.h"
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"
#include "icons.h"
#include "page_pregame.h"
#include "sprite.h"
#include "voteinfo_bar.h"

extern "C" void option_dialog_popup(const char *name,
                                    const struct option_set *poptset);
/************************************************************************/ /**
   Pregame options contructor
 ****************************************************************************/
pregame_options::pregame_options(QWidget *parent) : QWidget(parent)
{
  int level;
  ui.setupUi(this);

  ui.max_players->setRange(1, MAX_NUM_PLAYERS);
  connect(ui.nation, &QPushButton::clicked, this,
          &pregame_options::pick_nation);
  ui.qclientoptions->setText(_("Client Options"));
  connect(ui.qclientoptions, &QAbstractButton::clicked,
          [=]() { popup_client_options(); });
  for (level = 0; level < AI_LEVEL_COUNT; level++) {
    if (is_settable_ai_level(static_cast<ai_level>(level))) {
      const char *level_name =
          ai_level_translated_name(static_cast<ai_level>(level));
      ui.ailevel->addItem(level_name, level);
    }
  }
  ui.ailevel->setCurrentIndex(-1);
  connect(ui.max_players, SIGNAL(valueChanged(int)),
          SLOT(max_players_change(int)));
  connect(ui.ailevel, SIGNAL(currentIndexChanged(int)),
          SLOT(ailevel_change(int)));
  connect(ui.cruleset, SIGNAL(currentIndexChanged(int)),
          SLOT(ruleset_change(int)));
  ui.qserveroptions->setText(_("More Game Options"));
  ui.qserveroptions->setIcon(
      fc_icons::instance()->get_icon(QStringLiteral("preferences-other")));
  connect(ui.qserveroptions, &QPushButton::clicked, [=]() {
    option_dialog_popup(_("Set server options"), server_optset);
  });
  ui.lnations->setText(_("Nation:"));
  ui.lrules->setText(_("Rules:"));
  ui.lplayers->setText(_("Players:"));
  setLayout(ui.gridLayout);
  update_buttons();
}

/************************************************************************/ /**
   Update the ruleset list
 ****************************************************************************/
void pregame_options::set_rulesets(int num_rulesets, char **rulesets)
{
  int i;
  int def_idx = -1;

  ui.cruleset->clear();
  ui.cruleset->blockSignals(true);
  for (i = 0; i < num_rulesets; i++) {
    ui.cruleset->addItem(rulesets[i], i);
    if (!strcmp("default", rulesets[i])) {
      def_idx = i;
    }
  }

  /* HACK: HAXXOR WAS HERE : server should tell us the current ruleset. */
  ui.cruleset->setCurrentIndex(def_idx);
  ui.cruleset->blockSignals(false);
}

/************************************************************************/ /**
   Sets the value of the "aifill" option. Doesn't send the new value to the
   server
 ****************************************************************************/
void pregame_options::set_aifill(int aifill)
{
  ui.max_players->blockSignals(true);
  ui.max_players->setValue(aifill);
  ui.max_players->blockSignals(false);
}

/************************************************************************/ /**
   Updates the buttons whenever the game state has changed
 ****************************************************************************/
void pregame_options::update_buttons()
{
  struct sprite *psprite = nullptr;
  QPixmap *pixmap = nullptr;
  const struct player *pplayer = client_player();

  // Update the "Select Nation" button
  if (pplayer != nullptr) {
    if (pplayer->nation != nullptr) {
      // Defeat keyboard shortcut mnemonics
      ui.nation->setText(
          QString(nation_adjective_for_player(pplayer))
              .replace(QLatin1String("&"), QLatin1String("&&")));
      psprite = get_nation_shield_sprite(tileset, pplayer->nation);
      pixmap = psprite->pm;
      ui.nation->setIconSize(pixmap->size());
      ui.nation->setIcon(QIcon(*pixmap));
    } else {
      ui.nation->setText(_("Random"));
      ui.nation->setIcon(
          fc_icons::instance()->get_icon(QStringLiteral("flush-random")));
    }
  }
}

/************************************************************************/ /**
   Updates the AI skill level control
 ****************************************************************************/
void pregame_options::update_ai_level()
{
  enum ai_level level = server_ai_level();

  if (ai_level_is_valid(level)) {
    int i = ui.ailevel->findData(level);

    ui.ailevel->setCurrentIndex(i);
  } else {
    ui.ailevel->setCurrentIndex(-1);
  }
}

/************************************************************************/ /**
   Slot for changing aifill value
 ****************************************************************************/
void pregame_options::max_players_change(int i)
{
  option_int_set(optset_option_by_name(server_optset, "aifill"), i);
}

/************************************************************************/ /**
   Slot for changing level of AI
 ****************************************************************************/
void pregame_options::ailevel_change(int i)
{
  QVariant v = ui.ailevel->currentData();

  if (v.isValid()) {
    enum ai_level k = static_cast<ai_level>(v.toInt());

    /* Suppress changes provoked by server rather than local user */
    if (server_ai_level() != k) {
      const char *name = ai_level_cmd(k);

      send_chat_printf("/%s", name);
    }
  }
}

/************************************************************************/ /**
   Slot for changing ruleset
 ****************************************************************************/
void pregame_options::ruleset_change(int i)
{
  if (!ui.cruleset->currentText().isEmpty()) {
    QByteArray rn_bytes;

    rn_bytes = ui.cruleset->currentText().toLocal8Bit();
    set_ruleset(rn_bytes.data());
  }
}

/************************************************************************/ /**
   Slot for picking a nation
 ****************************************************************************/
void pregame_options::pick_nation() { popup_races_dialog(client_player()); }
