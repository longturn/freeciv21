/**************************************************************************
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
**************************************************************************/

#include "page_load.h"
// Qt
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
// utility
#include "fcintl.h"
// common
#include "chatline_common.h"
#include "connectdlg_common.h"
// client
#include "options.h"
// gui-qt
#include "fc_client.h"

static struct terrain *char2terrain(char ch);

/**********************************************************************/ /**
   Helper function for drawing map of savegames. Converts stored map char in
   savefile to proper terrain.
 **************************************************************************/
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
  QHeaderView *header;

  QStringList sav;
  gui = c;
  ui.setupUi(this);
  ui.show_preview->setText(_("Show preview"));
  ui.load_pix->setProperty("themed_border", true);
  ui.load_pix->setFixedSize(0, 0);
  sav << _("Choose Saved Game to Load") << _("Date");
  ui.load_save_text->setText("");
  ui.load_save_text->setTextFormat(Qt::RichText);
  ui.load_save_text->setWordWrap(true);
  ui.show_preview->setChecked(gui_options.gui_qt_show_preview);
  ui.saves_load->setAlternatingRowColors(true);
  ui.saves_load->setRowCount(0);
  ui.saves_load->setColumnCount(sav.count());
  ui.saves_load->setHorizontalHeaderLabels(sav);
  ui.saves_load->setProperty("showGrid", "false");
  ui.saves_load->setProperty("selectionBehavior", "SelectRows");
  ui.saves_load->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.saves_load->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.saves_load->verticalHeader()->setVisible(false);
  header = ui.saves_load->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);

  connect(ui.saves_load->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &page_load::slot_selection_changed);
  connect(ui.show_preview, &QCheckBox::stateChanged, this,
          &page_load::state_preview);

  ui.bbrowse->setText(_("Browse..."));
  ui.bbrowse->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DirIcon));

  connect(ui.bbrowse, &QAbstractButton::clicked, this,
          &page_load::browse_saves);

  ui.bcancel->setText(_("Cancel"));
  ui.bcancel->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
  connect(ui.bcancel, &QAbstractButton::clicked, gui,
          &fc_client::slot_disconnect);

  ui.bload->setText(_("Load"));
  ui.bload->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
  connect(ui.bload, &QAbstractButton::clicked, this,
          &page_load::start_from_save);
}

page_load::~page_load() {}

/**********************************************************************/ /**
   Updates saves to load and updates in tableview = saves_load
 **************************************************************************/
void page_load::update_load_page(void)
{
  struct fileinfo_list *files;
  int row;

  row = 0;
  files = fileinfolist_infix(get_save_dirs(), ".sav", FALSE);
  ui.saves_load->clearContents();
  ui.saves_load->setRowCount(0);
  ui.show_preview->setChecked(gui_options.gui_qt_show_preview);
  fileinfo_list_iterate(files, pfile)
  {
    QTableWidgetItem *item;
    QDateTime dt;

    item = new QTableWidgetItem();
    item->setData(Qt::UserRole, pfile->fullname);
    ui.saves_load->insertRow(row);
    item->setText(pfile->name);
    ui.saves_load->setItem(row, 0, item);
    item = new QTableWidgetItem();
    dt = QDateTime::fromSecsSinceEpoch(pfile->mtime);
    item->setText(dt.toString(Qt::TextDate));
    ui.saves_load->setItem(row, 1, item);
    row++;
  }
  fileinfo_list_iterate_end;
  fileinfo_list_destroy(files);
}

/**********************************************************************/ /**
   Starts game from chosen save - chosen_file (save or scenario)
 **************************************************************************/
void page_load::start_from_save()
{
  if (!is_server_running()) {
    client_start_server();
    send_chat("/detach");
  }
  if (is_server_running() && !current_file.isEmpty()) {
    QByteArray c_bytes;

    c_bytes = current_file.toLocal8Bit();
    send_chat_printf("/load %s", c_bytes.data());
    gui->switch_page(PAGE_GAME + 1);
  }
}

/**********************************************************************/ /**
   Browse saves directory
 **************************************************************************/
void page_load::browse_saves(void)
{
  QString str;
  str = QString(_("Save Files"))
        + QString(" (*.sav *.sav.bz2 *.sav.gz *.sav.xz)");
  current_file = QFileDialog::getOpenFileName(this, _("Open Save File"),
                                              QDir::homePath(), str);
  if (!current_file.isEmpty()) {
    start_from_save();
  }
}

/**********************************************************************/ /**
   State of preview has been changed
 **************************************************************************/
void page_load::state_preview(int new_state)
{
  QItemSelection slctn;

  if (ui.show_preview->checkState() == Qt::Unchecked) {
    gui_options.gui_qt_show_preview = false;
  } else {
    gui_options.gui_qt_show_preview = true;
  }
  slctn = ui.saves_load->selectionModel()->selection();
  ui.saves_load->selectionModel()->clearSelection();
  ui.saves_load->selectionModel()->select(
      slctn, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
}

void page_load::slot_selection_changed(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{

  QModelIndexList indexes = selected.indexes();
  QStringList sl;
  QModelIndex index;
  QTableWidgetItem *item;
  QItemSelectionModel *tw;
  QVariant qvar;
  QString str_pixmap;

  const char *terr_name;
  const struct server *pserver = NULL;
  int ii = 0;
  int k, col, n, nat_y, nat_x;
  struct section_file *sf;
  struct srv_list *srvrs;
  QByteArray fn_bytes;

  if (indexes.isEmpty()) {
    return;
  }

  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  current_file = qvar.toString();
  if (ui.show_preview->checkState() == Qt::Unchecked) {
    ui.load_pix->setPixmap(*(new QPixmap));
    ui.load_save_text->setText("");
    return;
  }
  fn_bytes = current_file.toLocal8Bit();
  if ((sf = secfile_load_section(fn_bytes.data(), "game", TRUE))) {
    const char *sname;
    bool sbool;
    int integer;
    QString final_str;
    QString pl_str = nullptr;
    int num_players = 0;
    int curr_player = 0;
    QByteArray pl_bytes;

    integer = secfile_lookup_int_default(sf, -1, "game.turn");
    if (integer >= 0) {
      final_str = QString("<b>") + _("Turn") + ":</b> "
                  + QString::number(integer).toHtmlEscaped() + "<br>";
    }
    if ((sf = secfile_load_section(fn_bytes.data(), "players", TRUE))) {
      integer = secfile_lookup_int_default(sf, -1, "players.nplayers");
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Players") + ":</b>" + " "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      num_players = integer;
    }
    for (int i = 0; i < num_players; i++) {
      pl_str = QString("player") + QString::number(i);
      pl_bytes = pl_str.toLocal8Bit();
      if ((sf = secfile_load_section(fn_bytes.data(), pl_bytes.data(),
                                     true))) {
        if (!(sbool = secfile_lookup_bool_default(
                  sf, true, "player%d.unassigned_user", i))) {
          curr_player = i;
          break;
        }
      }
    }
    /* Break case (and return) if no human player found */
    if (pl_str == nullptr) {
      ui.load_save_text->setText(final_str);
      return;
    }

    /* Information about human player */
    pl_bytes = pl_str.toLocal8Bit();
    if ((sf =
             secfile_load_section(fn_bytes.data(), pl_bytes.data(), true))) {
      sname = secfile_lookup_str_default(sf, nullptr, "player%d.nation",
                                         curr_player);
      if (sname) {
        final_str = final_str + "<b>" + _("Nation") + ":</b> "
                    + QString(sname).toHtmlEscaped() + "<br>";
      }
      integer = secfile_lookup_int_default(sf, -1, "player%d.ncities",
                                           curr_player);
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Cities") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      integer =
          secfile_lookup_int_default(sf, -1, "player%d.nunits", curr_player);
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Units") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      integer =
          secfile_lookup_int_default(sf, -1, "player%d.gold", curr_player);
      if (integer >= 0) {
        final_str = final_str + "<b>" + _("Gold") + ":</b> "
                    + QString::number(integer).toHtmlEscaped() + "<br>";
      }
      nat_x = 0;
      for (nat_y = 0; nat_y > -1; nat_y++) {
        const char *line = secfile_lookup_str_default(
            sf, nullptr, "player%d.map_t%04d", curr_player, nat_y);
        if (line == nullptr) {
          break;
        }
        nat_x = strlen(line);
        str_pixmap = str_pixmap + line;
      }

      /* Reset terrain information */
      terrain_type_iterate(pterr) { pterr->identifier_load = '\0'; }
      terrain_type_iterate_end;

      /* Load possible terrains and their identifiers (chars) */
      if ((sf = secfile_load_section(fn_bytes.data(), "savefile", true)))
        while ((terr_name = secfile_lookup_str_default(
                    sf, NULL, "savefile.terrident%d.name", ii))
               != NULL) {
          struct terrain *pterr = terrain_by_rule_name(terr_name);
          if (pterr != NULL) {
            const char *iptr = secfile_lookup_str_default(
                sf, NULL, "savefile.terrident%d.identifier", ii);
            pterr->identifier_load = *iptr;
          }
          ii++;
        }

      /* Create image */
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
      ui.load_pix->setFixedSize(ui.load_pix->pixmap()->width(),
                             ui.load_pix->pixmap()->height());
      if ((sf = secfile_load_section(fn_bytes.data(), "research", TRUE))) {
        sname = secfile_lookup_str_default(
            sf, nullptr, "research.r%d.now_name", curr_player);
        if (sname) {
          final_str = final_str + "<b>" + _("Researching") + ":</b> "
                      + QString(sname).toHtmlEscaped();
        }
      }
    }
    ui.load_save_text->setText(final_str);
  }
}
