/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2022 Freeciv21 and Freeciv
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

#include <fc_config.h>

// Qt
#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>

// utility
#include "fcintl.h"
// common
#include "version.h"

// tools
#include "download.h"
#include "modinst.h"
#include "mpcmdline.h"
#include "mpdb.h"
#include "mpgui_qt_worker.h"

#include "mpgui_qt.h"

struct fcmp_params fcmp = {
    QUrl::fromUserInput(QStringLiteral(MODPACK_LIST_URL)), QLatin1String(),
    QLatin1String()};

static mpgui *gui;

static mpqt_worker *worker = nullptr;

static int mpcount = 0;

#define ML_COL_NAME 0
#define ML_COL_VER 1
#define ML_COL_INST 2
#define ML_COL_TYPE 3
#define ML_COL_SUBTYPE 4
#define ML_COL_LIC 5
#define ML_COL_URL 6

#define ML_TYPE 7

#define ML_COL_COUNT 8

static void setup_modpack_list(const QString &name, const QUrl &url,
                               const QString &version,
                               const QString &license,
                               enum modpack_type type,
                               const QString &subtype, const QString &notes);
static void msg_callback(const QString &msg);
static void msg_callback_thr(const QString &msg);
static void progress_callback_thr(int downloaded, int max);

static void gui_download_modpack(const QString &url);

/**
   Entry point for whole freeciv-mp-qt program.
 */
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(freeciv21_version());
  app.setDesktopFileName(QStringLiteral("net.longturn.freeciv21.modpack"));

  // Load window icons
  QIcon::setThemeSearchPaths(get_data_dirs() + QIcon::themeSearchPaths());
  QIcon::setFallbackThemeName(QIcon::themeName());
  QIcon::setThemeName(QStringLiteral("icons"));

  qApp->setWindowIcon(QIcon::fromTheme(QStringLiteral("freeciv21-modpack")));

  // Delegate option parsing to the common function.
  fcmp_parse_cmdline(app);

  fcmp_init();

  // Start
  mpgui_main *main_window;
  QWidget *central;
  const char *errmsg;

  load_install_info_lists(&fcmp);

  central = new QWidget;
  main_window = new mpgui_main(&app, central);

  main_window->resize(820, 140);
  main_window->setWindowTitle(
      QString::fromUtf8(_("Freeciv21 modpack installer (Qt)")));

  gui = new mpgui;

  gui->setup(central, &fcmp);

  main_window->setCentralWidget(central);
  main_window->setVisible(true);

  errmsg = download_modpack_list(&fcmp, setup_modpack_list, msg_callback);
  if (errmsg != nullptr) {
    gui->display_msg(errmsg);
  }

  app.exec();

  if (worker != nullptr) {
    if (worker->isRunning()) {
      worker->wait();
    }
    delete worker;
  }

  delete gui;

  close_mpdbs();

  fcmp_deinit();

  return EXIT_SUCCESS;
}

/**
   Progress indications from downloader
 */
static void msg_callback(const QString &msg) { gui->display_msg(msg); }

/**
   Progress indications from downloader thread
 */
static void msg_callback_thr(const QString &msg)
{
  gui->display_msg_thr(msg);
}

/**
   Progress indications from downloader
 */
static void progress_callback_thr(int downloaded, int max)
{
  gui->progress_thr(downloaded, max);
}

/**
   Setup GUI object
 */
void mpgui::setup(QWidget *central, struct fcmp_params *params)
{
#define URL_LABEL_TEXT N_("Modpack URL")
  QVBoxLayout *main_layout = new QVBoxLayout();
  QHBoxLayout *hl = new QHBoxLayout();
  QPushButton *install_button =
      new QPushButton(QString::fromUtf8(_("Install modpack")));
  QStringList headers;
  QLabel *URL_label;

  auto version_label =
      new QLabel(QString(_("Version %1")).arg(freeciv21_version()));
  version_label->setAlignment(Qt::AlignHCenter);
  version_label->setParent(central);
  version_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  main_layout->addWidget(version_label);

  mplist_table = new QTableWidget();
  mplist_table->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);
  mplist_table->setColumnCount(ML_COL_COUNT);
  headers << QString::fromUtf8(_("Name")) << QString::fromUtf8(_("Version"));
  headers << QString::fromUtf8(_("Installed"))
          << QString::fromUtf8(Q_("?modpack:Type"));
  headers << QString::fromUtf8(_("Subtype"))
          << QString::fromUtf8(_("License"));
  headers << QString::fromUtf8(_("URL")) << QStringLiteral("typeint");
  mplist_table->setHorizontalHeaderLabels(headers);
  mplist_table->verticalHeader()->setVisible(false);
  mplist_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mplist_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  mplist_table->setSelectionMode(QAbstractItemView::SingleSelection);
  mplist_table->hideColumn(ML_TYPE);

  connect(mplist_table, &QTableWidget::cellClicked, this,
          &mpgui::row_selected);
  connect(mplist_table, &QAbstractItemView::doubleClicked, this,
          &mpgui::row_download);
  connect(this, &mpgui::display_msg_thr_signal, this, &mpgui::display_msg);
  connect(this, &mpgui::progress_thr_signal, this, &mpgui::progress);
  connect(this, &mpgui::refresh_list_versions_thr_signal, this,
          &mpgui::refresh_list_versions);

  main_layout->addWidget(mplist_table);

  URL_label = new QLabel(QString::fromUtf8(_(URL_LABEL_TEXT)));
  URL_label->setParent(central);
  hl->addWidget(URL_label);

  URLedit = new QLineEdit(central);
  if (params->autoinstall == nullptr) {
    URLedit->setText(DEFAULT_URL_START);
  } else {
    URLedit->setText(params->autoinstall);
  }
  URLedit->setFocus();

  connect(URLedit, &QLineEdit::returnPressed, this, &mpgui::URL_given);

  hl->addWidget(URLedit);
  main_layout->addLayout(hl);

  connect(install_button, &QAbstractButton::pressed, this,
          &mpgui::URL_given);
  main_layout->addWidget(install_button);

  bar = new QProgressBar(central);
  main_layout->addWidget(bar);

  msg_dspl = new QLabel(QString::fromUtf8(_("Select modpack to install")));
  msg_dspl->setParent(central);
  main_layout->addWidget(msg_dspl);

  msg_dspl->setAlignment(Qt::AlignHCenter);

  central->setLayout(main_layout);
}

/**
   Display status message
 */
void mpgui::display_msg(const QString &msg)
{
  QByteArray msg_bytes = msg.toLocal8Bit();

  qDebug("%s", msg_bytes.data());
  msg_dspl->setText(msg);
}

/**
   Display status message from another thread
 */
void mpgui::display_msg_thr(const QString &msg)
{
  emit display_msg_thr_signal(msg);
}

/**
   Update progress bar
 */
void mpgui::progress(int downloaded, int max)
{
  bar->setMaximum(max);
  bar->setValue(downloaded);
}

/**
   Update progress bar from another thread
 */
void mpgui::progress_thr(int downloaded, int max)
{
  emit progress_thr_signal(downloaded, max);
}

/**
   Download modpack from given URL
 */
static void gui_download_modpack(const QString &URL)
{
  if (worker != nullptr) {
    if (worker->isRunning()) {
      gui->display_msg(_("Another download already active"));
      return;
    }
  } else {
    worker = new mpqt_worker;
  }

  worker->download(QUrl::fromUserInput(URL), gui, &fcmp, msg_callback_thr,
                   progress_callback_thr);
}

/**
   User entered URL
 */
void mpgui::URL_given() { gui_download_modpack(URLedit->text()); }

/**
   Refresh display of modpack list modpack versions
 */
void mpgui::refresh_list_versions()
{
  for (int i = 0; i < mpcount; i++) {
    QString name_str;
    int type_int;
    enum modpack_type type;
    QByteArray name_bytes;

    name_str = mplist_table->item(i, ML_COL_NAME)->text();
    type_int = mplist_table->item(i, ML_TYPE)->text().toInt();
    type = (enum modpack_type) type_int;
    name_bytes = name_str.toUtf8();

    auto tmp = mpdb_installed_version(qUtf8Printable(name_bytes), type);
    QString new_inst = tmp ? tmp : _("Not installed");
    delete[] tmp;

    mplist_table->item(i, ML_COL_INST)->setText(new_inst);
  }

  mplist_table->resizeColumnsToContents();
}

/**
   Refresh display of modpack list modpack versions from another thread
 */
void mpgui::refresh_list_versions_thr()
{
  emit refresh_list_versions_thr_signal();
}

/**
   Build main modpack list view
 */
void mpgui::setup_list(const QString &name, const QUrl &url,
                       const QString &version, const QString &license,
                       enum modpack_type type, const QString &subtype,
                       const QString &notes)
{
  // TRANS: Unknown modpack type
  QString type_str =
      modpack_type_is_valid(type) ? (modpack_type_name(type)) : _("?");

  // TRANS: License of modpack is not known
  QString lic_str = license.isEmpty() ? Q_("?license:Unknown") : license;

  const char *tmp = mpdb_installed_version(qUtf8Printable(name), type);
  QString inst_str = tmp ? tmp : _("Not installed");
  delete[] tmp;

  QString type_nbr;
  QTableWidgetItem *item;

  mplist_table->setRowCount(mpcount + 1);

  item = new QTableWidgetItem(name);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_NAME, item);
  item = new QTableWidgetItem(version);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_VER, item);
  item = new QTableWidgetItem(inst_str);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_INST, item);
  item = new QTableWidgetItem(type_str);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_TYPE, item);
  item = new QTableWidgetItem(subtype);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_SUBTYPE, item);
  item = new QTableWidgetItem(lic_str);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_LIC, item);
  item = new QTableWidgetItem(url.toDisplayString());
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_COL_URL, item);

  type_nbr.setNum(type);
  item = new QTableWidgetItem(type_nbr);
  item->setToolTip(notes);
  mplist_table->setItem(mpcount, ML_TYPE, item);

  mplist_table->resizeColumnsToContents();
  mpcount++;
}

/**
   Build main modpack list view
 */
static void setup_modpack_list(const QString &name, const QUrl &url,
                               const QString &version,
                               const QString &license,
                               enum modpack_type type,
                               const QString &subtype, const QString &notes)
{
  // Just call setup_list for gui singleton
  gui->setup_list(name, url, version, license, type, subtype, notes);
}

/**
   User activated another table row
 */
void mpgui::row_selected(int row, int column)
{
  QString URL = mplist_table->item(row, ML_COL_URL)->text();

  URLedit->setText(URL);
}

/**
   User activated another table row
 */
void mpgui::row_download(const QModelIndex &index)
{
  QString URL = mplist_table->item(index.row(), ML_COL_URL)->text();

  URLedit->setText(URL);

  URL_given();
}

/**
   Main window constructor
 */
mpgui_main::mpgui_main(QApplication *qapp_in, QWidget *central_in)
    : QMainWindow()
{
  qapp = qapp_in;
  central = central_in;
}

/**
   Open dialog to confirm that user wants to quit modpack installer.
 */
void mpgui_main::popup_quit_dialog()
{
  QMessageBox ask(central);
  int ret;

  ask.setText(_(
      "Modpack installation in progress.\nAre you sure you want to quit?"));
  ask.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  ask.setDefaultButton(QMessageBox::No);
  ask.setIcon(QMessageBox::Warning);
  ask.setWindowTitle(_("Quit?"));
  ret = ask.exec();

  switch (ret) {
  case QMessageBox::Cancel:
    return;
    break;
  case QMessageBox::Ok:
    qapp->quit();
    break;
  }
}

/**
   User clicked windows close button.
 */
void mpgui_main::closeEvent(QCloseEvent *event)
{
  if (worker != nullptr && worker->isRunning()) {
    // Download in progress. Confirm quit from user.
    popup_quit_dialog();
    event->ignore();
  }
}
