/*
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
 */

// Qt
#include "fcintl.h"
#include <QAction>
#include <QComboBox>
#include <QGridLayout>
#include <QSpinBox>
#include <QSplitter>
// common
#include "chatline_common.h"
#include "colors_common.h"
#include "connectdlg_common.h"
// client
#include "client_main.h"
#include "climisc.h"
#include "dialogs.h"
#include "fc_client.h"
#include "icons.h"

#include "pregameoptions.h"

void option_dialog_popup(const char *name, const struct option_set *poptset);
/**
   Pregame options contructor
 */
pregame_options::pregame_options(QWidget *parent) : QWidget(parent)
{
  int level;
  ui.setupUi(this);

  ui.max_players->setRange(1, MAX_NUM_PLAYERS);
  connect(ui.nation, &QPushButton::clicked, this,
          &pregame_options::pick_nation);
  ui.qclientoptions->setText(_("Interface Options"));
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
  connect(ui.max_players, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &pregame_options::max_players_change);
  connect(ui.ailevel, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &pregame_options::ailevel_change);
  connect(ui.cruleset, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &pregame_options::ruleset_change);
  ui.qserveroptions->setText(_("More Game Options"));
  ui.qserveroptions->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("preferences-other")));
  connect(ui.qserveroptions, &QPushButton::clicked,
          [=]() { option_dialog_popup(_("Game Options"), server_optset); });
  ui.lnations->setText(_("Nation:"));
  ui.lrules->setText(_("Rules:"));
  ui.lplayers->setText(_("Players:"));
  setLayout(ui.gridLayout);
  update_buttons();
}

/**
   Update the ruleset list
 */
void pregame_options::set_rulesets(int num_rulesets, QStringList rulesets)
{
  int i = 0;
  int def_idx = -1;

  ui.cruleset->clear();
  ui.cruleset->blockSignals(true);

  for (auto r : rulesets) {
    ui.cruleset->addItem(r, i);
    if (QString("default") == r) {
      def_idx = i;
    }
    i++;
  }

  // HACK: HAXXOR WAS HERE : server should tell us the current ruleset.
  ui.cruleset->setCurrentIndex(def_idx);
  ui.cruleset->blockSignals(false);
}

/**
   Sets the value of the "aifill" option. Doesn't send the new value to the
   server
 */
void pregame_options::set_aifill(int aifill)
{
  ui.max_players->blockSignals(true);
  ui.max_players->setValue(aifill);
  ui.max_players->blockSignals(false);
}

/**
   Updates the buttons whenever the game state has changed
 */
void pregame_options::update_buttons()
{
  const struct player *pplayer = client_player();

  // Update the "Select Nation" button
  if (pplayer != nullptr) {
    if (pplayer->nation != nullptr) {
      // Defeat keyboard shortcut mnemonics
      ui.nation->setText(
          QString(nation_adjective_for_player(pplayer))
              .replace(QLatin1String("&"), QLatin1String("&&")));
      auto pixmap = get_nation_shield_sprite(tileset, pplayer->nation);
      ui.nation->setIconSize(pixmap->size());
      ui.nation->setIcon(QIcon(*pixmap));
    } else {
      ui.nation->setText(_("Random"));
      ui.nation->setIcon(
          fcIcons::instance()->getIcon(QStringLiteral("flush-random")));
    }
  }
}

/**
   Updates the AI skill level control
 */
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

/**
   Slot for changing aifill value
 */
void pregame_options::max_players_change(int i)
{
  option_int_set(optset_option_by_name(server_optset, "aifill"), i);
}

/**
   Slot for changing level of AI
 */
void pregame_options::ailevel_change(int i)
{
  Q_UNUSED(i)
  QVariant v = ui.ailevel->currentData();

  if (v.isValid()) {
    enum ai_level k = static_cast<ai_level>(v.toInt());

    // Suppress changes provoked by server rather than local user
    if (server_ai_level() != k) {
      const char *name = ai_level_cmd(k);

      send_chat_printf("/%s", name);
    }
  }
}

/**
   Slot for changing ruleset
 */
void pregame_options::ruleset_change(int i)
{
  Q_UNUSED(i)
  if (!ui.cruleset->currentText().isEmpty()) {
    QByteArray rn_bytes;

    rn_bytes = ui.cruleset->currentText().toLocal8Bit();
    set_ruleset(rn_bytes.data());
  }
}

/**
   Slot for picking a nation
 */
void pregame_options::pick_nation() { popup_races_dialog(client_player()); }
