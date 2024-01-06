/*
  /\ ___ /\                 Copyright (c) 1996-2020 Freeciv21 and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                  If not, see https://www.gnu.org/licenses/.
 */

#include "page_load.h"

// Qt
#include <QDateTime>
#include <QFileDialog>
#include <QPushButton>

// utility
#include "fcintl.h"
#include "section_file.h"

// common
#include "rgbcolor.h"

// client
#include "fc_client.h"
#include "options.h"

static struct terrain *char2terrain(char ch);

/**
   Helper function for drawing map of savegames. Converts stored map char in
   savefile to proper terrain.
 */
static struct terrain *char2terrain(char ch)
{
  if (ch == TERRAIN_UNKNOWN_IDENTIFIER) {
    return T_UNKNOWN;
  }
  terrain_type_iterate(pterrain)
  {
    if (pterrain->identifier_load == ch) {
      return pterrain;
    }
  }
  terrain_type_iterate_end;
  return nullptr;
}

page_load::page_load(QWidget *parent, fc_client *c) : QWidget(parent)
{
  gui = c;
  ui.setupUi(this);
  ui.show_preview->setText(_("Show preview"));
  ui.load_pix->setProperty("themed_border", true);
  ui.load_pix->setFixedSize(0, 0);
  ui.load_save_text->setText(QLatin1String(""));
  ui.load_save_text->setTextFormat(Qt::RichText);
  ui.load_save_text->setWordWrap(true);
  ui.show_preview->setChecked(gui_options->gui_qt_show_preview);
  ui.saves_load->setRowCount(0);
  QStringList sav;
  sav << _("Choose Saved Game to Load") << _("Date");
  ui.saves_load->setColumnCount(sav.count());
  ui.saves_load->setHorizontalHeaderLabels(sav);
  ui.saves_load->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);

  connect(ui.saves_load->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &page_load::slot_selection_changed);
  connect(ui.show_preview, &QCheckBox::stateChanged, this,
          &page_load::state_preview);

  auto browse =
      ui.buttons->addButton(_("Browse..."), QDialogButtonBox::ActionRole);
  browse->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
  connect(browse, &QAbstractButton::clicked, this, &page_load::browse_saves);

  connect(ui.buttons->button(QDialogButtonBox::Cancel),
          &QAbstractButton::clicked, gui, &fc_client::slot_disconnect);

  auto load = ui.buttons->button(QDialogButtonBox::Ok);
  load->setText(_("Load"));
  connect(load, &QAbstractButton::clicked, this,
          &page_load::start_from_save);
}

page_load::~page_load() = default;

/**
   Updates saves to load and updates in tableview = saves_load
 */
void page_load::update_load_page()
{
  int row;

  row = 0;
  ui.saves_load->clearContents();
  ui.saves_load->setRowCount(0);
  ui.show_preview->setChecked(gui_options->gui_qt_show_preview);

  const auto files =
      find_files_in_path(get_save_dirs(), QStringLiteral("*.sav*"), false);
  for (const auto &info : files) {
    auto item = new QTableWidgetItem();
    item->setData(Qt::UserRole, info.absoluteFilePath());
    ui.saves_load->insertRow(row);
    item->setText(info.fileName());
    ui.saves_load->setItem(row, 0, item);
    item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, QDateTime(info.lastModified()));
    ui.saves_load->setItem(row, 1, item);
    row++;
  }

  ui.saves_load->sortByColumn(1, Qt::DescendingOrder);

  if (!files.isEmpty()) {
    ui.saves_load->setCurrentCell(0, 0); // Select the latest save
  }
}

/**
   Starts game from chosen save - chosen_file (save or scenario)
 */
void page_load::start_from_save() { king()->start_from_file(current_file); }

/**
   Browse saves directory
 */
void page_load::browse_saves()
{
  QString str;
  str = QString(_("Save Files"))
        + QStringLiteral(" (*.sav *.sav.bz2 *.sav.gz *.sav.xz *.sav.zst)");
  current_file = QFileDialog::getOpenFileName(this, _("Open Save File"),
                                              QDir::homePath(), str);
  if (!current_file.isEmpty()) {
    start_from_save();
  }
}

/**
   State of preview has been changed
 */
void page_load::state_preview()
{
  gui_options->gui_qt_show_preview =
      ui.show_preview->checkState() != Qt::Unchecked;
  auto selection = ui.saves_load->selectionModel()->selection();
  ui.saves_load->selectionModel()->clearSelection();
  ui.saves_load->selectionModel()->select(
      selection,
      QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
}

void page_load::slot_selection_changed(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{
  Q_UNUSED(deselected)
  QModelIndexList indexes = selected.indexes();
  QStringList sl;
  QModelIndex index;
  QVariant qvar;
  QString str_pixmap;

  const char *terr_name;
  int ii = 0;
  int nat_y, nat_x;
  QByteArray fn_bytes;

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  current_file = qvar.toString();
  if (ui.show_preview->checkState() == Qt::Unchecked) {
    ui.load_pix->setPixmap(*(new QPixmap));
    ui.load_save_text->setText(QLatin1String(""));
    return;
  }
  fn_bytes = current_file.toLocal8Bit();

  auto sf = std::unique_ptr<section_file, decltype(&secfile_destroy)>(
      nullptr, &secfile_destroy);
  sf.reset(
      secfile_load_section(fn_bytes.data(), QStringLiteral("game"), true));
  if (sf) {
    const char *sname;
    bool sbool;
    int integer;
    QString final_str;
    QString pl_str = nullptr;
    int num_players = 0;
    int curr_player = 0;
    QByteArray pl_bytes;

    integer = secfile_lookup_int_default(sf.get(), -1, "game.turn");
    if (integer >= 0) {
      final_str = QStringLiteral("<b>") + _("Turn") + ":</b> "
                  + QString::number(integer).toHtmlEscaped() + "<br>";
    }
    sf.reset(secfile_load_section(fn_bytes.data(), QStringLiteral("players"),
                                  true));
    if (sf) {
      integer = secfile_lookup_int_default(sf.get(), -1, "players.nplayers");
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Players") + ":</b>" + " "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      num_players = integer;
    }
    for (int i = 0; i < num_players; i++) {
      pl_str = QStringLiteral("player") + QString::number(i);
      pl_bytes = pl_str.toLocal8Bit();
      sf.reset(secfile_load_section(fn_bytes.data(), pl_bytes.data(), true));
      if (sf) {
        if (!(sbool = secfile_lookup_bool_default(
                  sf.get(), true, "player%d.unassigned_user", i))) {
          curr_player = i;
          break;
        }
      }
    }
    // Break case (and return) if no human player found
    if (pl_str == nullptr) {
      ui.load_save_text->setText(final_str);
      return;
    }

    // Information about human player
    pl_bytes = pl_str.toLocal8Bit();
    sf.reset(secfile_load_section(fn_bytes.data(), pl_bytes.data(), true));
    if (sf) {
      sname = secfile_lookup_str_default(sf.get(), nullptr,
                                         "player%d.nation", curr_player);
      if (sname) {
        final_str = final_str + "<b>" + _("Nation") + ":</b> "
                    + QString(sname).toHtmlEscaped() + "<br>";
      }
      integer = secfile_lookup_int_default(sf.get(), -1, "player%d.ncities",
                                           curr_player);
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Cities") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      integer = secfile_lookup_int_default(sf.get(), -1, "player%d.nunits",
                                           curr_player);
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Units") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      integer = secfile_lookup_int_default(sf.get(), -1, "player%d.gold",
                                           curr_player);
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Gold") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      nat_x = 0;
      for (nat_y = 0; nat_y > -1; nat_y++) {
        const char *line = secfile_lookup_str_default(
            sf.get(), nullptr, "player%d.map_t%04d", curr_player, nat_y);
        if (line == nullptr) {
          break;
        }
        nat_x = qstrlen(line);
        str_pixmap = str_pixmap + line;
      }

      // Reset terrain information
      terrain_type_iterate(pterr) { pterr->identifier_load = '\0'; }
      terrain_type_iterate_end;

      // Load possible terrains and their identifiers (chars)
      sf.reset(secfile_load_section(fn_bytes.data(),
                                    QStringLiteral("savefile"), true));
      if (sf) {
        while ((terr_name = secfile_lookup_str_default(
                    sf.get(), nullptr, "savefile.terrident%d.name", ii))
               != nullptr) {
          struct terrain *pterr = terrain_by_rule_name(terr_name);
          if (pterr != nullptr) {
            const char *iptr = secfile_lookup_str_default(
                sf.get(), nullptr, "savefile.terrident%d.identifier", ii);
            fc_assert_ret(iptr != nullptr);
            pterr->identifier_load = *iptr;
          }
          ii++;
        }
      }
      // Create image
      QImage img(nat_x, nat_y, QImage::Format_ARGB32_Premultiplied);
      img.fill(Qt::black);
      for (int a = 0; a < nat_x; a++) {
        for (int b = 0; b < nat_y; b++) {
          struct terrain *tr;
          struct rgbcolor *rgb;
          tr = char2terrain(str_pixmap.at(b * nat_x + a).toLatin1());
          if (tr != nullptr) {
            rgb = tr->rgb;
            QColor col;
            col.setRgb(rgb->r, rgb->g, rgb->b);
            img.setPixel(a, b, col.rgb());
          }
        }
      }
      if (img.width() > 1) {
        ui.load_pix->setPixmap(QPixmap::fromImage(img).scaledToHeight(200));
      } else {
        ui.load_pix->setPixmap(*(new QPixmap));
      }
      ui.load_pix->setFixedSize(ui.load_pix->pixmap().width(),
                                ui.load_pix->pixmap().height());
      sf.reset(secfile_load_section(fn_bytes.data(),
                                    QStringLiteral("research"), true));
      if (sf) {
        sname = secfile_lookup_str_default(
            sf.get(), nullptr, "research.r%d.now_name", curr_player);
        if (sname) {
          final_str = final_str + "<b>" + _("Researching") + ":</b> "
                      + QString(sname).toHtmlEscaped();
        }
      }
    }
    ui.load_save_text->setText(final_str);

    ui.load_save_text->show();
    ui.load_pix->show();
    ui.buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
  } else {
    // Couldn't load the save. Clear the preview and prevent loading it.
    ui.load_save_text->hide();
    ui.load_pix->hide();
    ui.buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
  }
}
