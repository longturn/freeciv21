/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef AUDIO_SDL
#include <SDL2/SDL.h>
#endif // AUDIO_SDL

#include "gui_main.h"
#include <cstdio>
// Qt
#include <QApplication>
// utility
#include "fciconv.h"
#include "log.h"
// client
#include "client_main.h"
#include "clinet.h"
#include "mapview_g.h"
#include "options.h"
#include "sprite.h"
#include "themes_common.h"
#include "tilespec.h"
// gui-qt
#include "fc_client.h"
#include "fonts.h"
#include "helpdlg.h"
#include "hudwidget.h"
#include "messagewin.h"
#include "page_game.h"
#include "page_pregame.h"
#include "qtg_cxxside.h"
#include "unitselect.h"

extern "C" void real_science_report_dialog_update(void *);

extern void restart_notify_reports();
extern void city_font_update();

const bool gui_use_transliteration = false;
const char *const gui_character_encoding = "UTF-8";
const char *client_string = "gui-qt";
static fc_client *freeciv_qt;

void reset_unit_table();
static void apply_help_font(struct option *poption);
static void apply_notify_font(struct option *poption);
static void apply_sidebar(struct option *poption);
static void apply_titlebar(struct option *poption);

/**
   Return fc_client instance
 */
class fc_client *king() { return freeciv_qt; }

/**
   Do any necessary pre-initialization of the UI, if necessary.
 */
void qtg_ui_init() {}

/**
   Entry point for whole freeciv client program.
 */
int main(int argc, char **argv)
{
  setup_gui_funcs();
  return client_main(argc, argv);
}

/**
   Migrate Qt client specific options from freeciv-2.5 options
 */
static void migrate_options_from_2_5()
{
  qInfo(_("Migrating Qt-client options from freeciv-2.5 options."));

  gui_options.gui_qt_fullscreen = gui_options.migrate_fullscreen;

  gui_options.gui_qt_migrated_from_2_5 = true;
}

/**
   The main loop for the UI.  This is called from main(), and when it
   exits the client will exit.
 */
void qtg_ui_main()
{
  if (true) {
    QPixmap *qpm;
    QIcon app_icon;

    tileset_init(tileset);
    tileset_load_tiles(tileset);
    qpm = get_icon_sprite(tileset, ICON_FREECIV);
    app_icon = ::QIcon(*qpm);
    qApp->setWindowIcon(app_icon);
    if (gui_options.first_boot) {
      /* We're using fresh defaults for this version of this client,
       * so prevent any future migrations from other versions */
      gui_options.gui_qt_migrated_from_2_5 = true;
      configure_fonts();
    } else if (!gui_options.gui_qt_migrated_from_2_5) {
      migrate_options_from_2_5();
    }
    if (!load_theme(gui_options.gui_qt_default_theme_name)) {
      qtg_gui_clear_theme();
    }
    freeciv_qt = new fc_client();
    freeciv_qt->fc_main(qApp);
  }
}

/**
   Return the running QApplication.
 */
QApplication *current_app() { return qApp; }

/**
   Extra initializers for client options.
 */
void qtg_options_extra_init()
{
  struct option *poption;

#define option_var_set_callback(var, callback)                              \
  if ((poption = optset_option_by_name(client_optset, #var))) {             \
    option_set_changed_callback(poption, callback);                         \
  } else {                                                                  \
    qCritical("Didn't find option %s!", #var);                              \
  }
  option_var_set_callback(gui_qt_font_city_names, gui_qt_apply_font);
  option_var_set_callback(gui_qt_font_city_productions, gui_qt_apply_font);
  option_var_set_callback(gui_qt_font_reqtree_text, gui_qt_apply_font);
  option_var_set_callback(gui_qt_font_default, gui_qt_apply_font);
  option_var_set_callback(gui_qt_font_help_text, apply_help_font);
  option_var_set_callback(gui_qt_font_chatline, gui_qt_apply_font);
  option_var_set_callback(gui_qt_font_notify_label, apply_notify_font);
  option_var_set_callback(gui_qt_show_titlebar, apply_titlebar);
  option_var_set_callback(gui_qt_sidebar_left, apply_sidebar);
#undef option_var_set_callback
}

/**
   Do any necessary UI-specific cleanup
 */
void qtg_ui_exit() { delete freeciv_qt; }

/**
   Update the connected users list at pregame state.
 */
void qtg_real_conn_list_dialog_update(void *unused)
{
  if (qtg_get_current_client_page() == PAGE_NETWORK) {
    qtg_real_set_client_page(PAGE_START);
  }
  qobject_cast<page_pregame *>(king()->pages[PAGE_START])
      ->update_start_page();
}

/**
   Make a bell noise (beep).  This provides low-level sound alerts even
   if there is no real sound support.
 */
void qtg_sound_bell()
{
  QApplication::beep();
  QApplication::alert(king()->central_wdg);
}

/**
   Wait for data on the given socket.  Call input_from_server() when data
   is ready to be read.

   This function is called after the client succesfully has connected
   to the server.
 */
void qtg_add_net_input(QTcpSocket *sock) { king()->add_server_source(sock); }

/**
   Stop waiting for any server network data.  See add_net_input().

   This function is called if the client disconnects from the server.
 */
void qtg_remove_net_input() {}

/**
   Set one of the unit icons (specified by idx) in the information area
   based on punit.

   punit is the unit the information should be taken from. Use NULL to
   clear the icon.

   idx specified which icon should be modified. Use idx == -1 to indicate
   the icon for the active unit. Or idx in [0..num_units_below-1] for
   secondary (inactive) units on the same tile.
 */
void qtg_set_unit_icon(int idx, struct unit *punit)
{ // PORTME
}

/**
   Most clients use an arrow (e.g., sprites.right_arrow) to indicate when
   the units_below will not fit. This function is called to activate or
   deactivate the arrow.

   Is disabled by default.
 */
void qtg_set_unit_icons_more_arrow(bool onoff)
{ // PORTME
}

/**
   Called when the set of units in focus (get_units_in_focus()) changes.
   Standard updates like update_unit_info_label() are handled in the
 platform- independent code, so some clients will not need to do anything
 here.
 */
void qtg_real_focus_units_changed()
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != nullptr && unit_sel->isVisible()) {
    unit_sel->update_units();
  }
}

/**
   Enqueue a callback to be called during an idle moment.  The 'callback'
   function should be called sometimes soon, and passed the 'data' pointer
   as its data.
 */
void qtg_add_idle_callback(void(callback)(void *), void *data)
{
  call_me_back *cb = new call_me_back; // removed in mr_idler:idling()

  cb->callback = callback;
  cb->data = data;
  mrIdle::idlecb()->addCallback(cb);
}

/**
   Shows/Hides titlebar
 */
void apply_titlebar(struct option *poption)
{
  bool val;
  QWidget *w;
  Qt::WindowFlags flags = Qt::Window;
  val = option_bool_get(poption);

  if (king()->current_page() < PAGE_GAME) {
    return;
  }

  if (val) {
    w = new QWidget();
    king()->setWindowFlags(flags);
    delete king()->corner_wid;
    king()->corner_wid = nullptr;
    king()->menu_bar->setCornerWidget(w);
  } else {
    flags |= Qt::CustomizeWindowHint;
    king()->setWindowFlags(flags);
    king()->corner_wid = new fc_corner(king());
    king()->menu_bar->setCornerWidget(king()->corner_wid);
  }
  king()->show();
}

/**
   Change sidebar position
 */
void apply_sidebar(struct option *poption)
{
  queen()->updateSidebarPosition();
}

/**
   Change the given font.
 */
void gui_qt_apply_font(struct option *poption)
{
  QFont *f;
  QFont *remove_old;
  QString s;

  if (king()) {
    f = new QFont;
    s = option_font_get(poption);
    f->fromString(s);
    s = option_name(poption);
    remove_old = fcFont::instance()->getFont(s);
    delete remove_old;
    fcFont::instance()->setFont(s, f);
    update_city_descriptions();
    queen()->infotab->chtwdg->update_font();
    QApplication::setFont(*fcFont::instance()->getFont(fonts::default_font));
    real_science_report_dialog_update(nullptr);
    fcFont::instance()->getMapfontSize();
  }
  apply_help_font(poption);
}

/**
   Applies help font
 */
static void apply_help_font(struct option *poption)
{
  QFont *f;
  QFont *remove_old;
  QString s;

  if (king()) {
    f = new QFont;
    s = option_font_get(poption);
    f->fromString(s);
    s = option_name(poption);
    remove_old = fcFont::instance()->getFont(s);
    delete remove_old;
    fcFont::instance()->setFont(s, f);
    update_help_fonts();
  }
}

/**
   Applies help font
 */
static void apply_notify_font(struct option *poption)
{
  if (king()) {
    qtg_gui_update_font(QStringLiteral("notify_label"),
                        option_font_get(poption));
    restart_notify_reports();
  }
  if (king() && qtg_get_current_client_page() == PAGE_GAME) {
    qtg_gui_update_font(QStringLiteral("city_label"),
                        option_font_get(poption));
    city_font_update();
  }
}

/**
   Stub for editor function
 */
void qtg_editgui_tileset_changed() {}

/**
   Stub for editor function
 */
void qtg_editgui_refresh() {}

/**
   Stub for editor function
 */
void qtg_editgui_popup_properties(const struct tile_list *tiles, int objtype)
{
}

/**
   Stub for editor function
 */
void qtg_editgui_popdown_all() {}

/**
   Stub for editor function
 */
void qtg_editgui_notify_object_changed(int objtype, int object_id,
                                       bool removal)
{
}

/**
   Stub for editor function
 */
void qtg_editgui_notify_object_created(int tag, int id) {}

/**
   Updates a gui font style.
 */
void qtg_gui_update_font(const QString &font_name, const QString &font_value)
{
  QFont *f;
  QFont *remove_old;
  QString fname;

  fname = "gui_qt_font_" + QString(font_name);
  f = new QFont;
  f->fromString(font_value);
  remove_old = fcFont::instance()->getFont(fname);
  delete remove_old;
  fcFont::instance()->setFont(fname, f);
  fcFont::instance()->getMapfontSize();
}

void gui_update_allfonts()
{
  fcFont::instance()->setSizeAll(gui_options.gui_qt_increase_fonts);
  gui_options.gui_qt_increase_fonts = 0;
}

/**
   Returns gui type of the client
 */
enum gui_type qtg_get_gui_type() { return GUI_QT; }

/**
   Called when the tileset is changed to reset the unit pixmap table.
 */
void reset_unit_table()
{ // FIXME
}

/**
   Open dialog to confirm that user wants to quit client.
 */
void popup_quit_dialog()
{
  hud_message_box *ask = new hud_message_box(king()->central_wdg);

  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->set_text_title(_("Are you sure you want to quit?"), _("Quit?"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(ask, &hud_message_box::accepted, [=]() {
    start_quitting();
    if (client.conn.used) {
      disconnect_from_server();
    }
    king()->write_settings();
    qApp->quit();
  });
  ask->show();
}

/**
   Insert build information to help
 */
void qtg_insert_client_build_info(char *outbuf, size_t outlen)
{
  // There's also an separate entry about Qt in help menu.

  cat_snprintf(outbuf, outlen, _("\nBuilt against Qt %s, using %s"),
               QT_VERSION_STR, qVersion());

#ifdef FC_QT6_MODE
  cat_snprintf(outbuf, outlen, _("\nBuilt in Qt5x mode."));
#else  // FC_QT6_MODE
  cat_snprintf(outbuf, outlen, _("\nBuilt in Qt5 mode."));
#endif // FC_QT6_MODE
}
