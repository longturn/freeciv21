/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// Keep this first. In particular before any Qt include.
#ifdef AUDIO_SDL
#include <SDL2/SDL.h>
#endif // AUDIO_SDL

// Qt
#include <QApplication>

// client
#include "client_main.h"
#include "clinet.h"
#include "fc_client.h"
#include "fonts.h"
#include "helpdlg.h"
#include "hudwidget.h"
#include "messagewin.h"
#include "options.h"
#include "page_game.h"
#include "page_pregame.h"
#include "qtg_cxxside.h"
#include "themes_common.h"
#include "tileset/tilespec.h"
#include "unitselect.h"
#include "views/view_map.h"
#include "views/view_map_common.h"
#include "widgets/report_widget.h"

#include "gui_main.h"

void real_science_report_dialog_update(void *);

extern void city_font_update();

const bool gui_use_transliteration = false;
const char *const gui_character_encoding = "UTF-8";
const char *client_string = "gui-qt";
static fc_client *freeciv_qt;

static void apply_help_font(struct option *poption);
static void apply_notify_font(struct option *poption);
static void apply_titlebar(struct option *poption);

/**
   Return fc_client instance
 */
class fc_client *king() { return freeciv_qt; }

/**
   Entry point for whole freeciv client program.
 */
int main(int argc, char **argv) { return client_main(argc, argv); }

/**
   The main loop for the UI.  This is called from main(), and when it
   exits the client will exit.
 */
void ui_main()
{
  // Load window icons
  QIcon::setThemeSearchPaths(get_data_dirs() + QIcon::themeSearchPaths());
  QIcon::setFallbackThemeName(QIcon::themeName());
  QIcon::setThemeName(QStringLiteral("icons"));

  qApp->setWindowIcon(QIcon::fromTheme(QStringLiteral("freeciv21-client")));

  if (true) {
    tileset_init(tileset);
    tileset_load_tiles(tileset);
    if (!load_theme(gui_options->gui_qt_default_theme_name)) {
      gui_clear_theme();
    }
    freeciv_qt = new fc_client();
    freeciv_qt->fc_main(qApp);
  }
}

/**
   Extra initializers for client options.
 */
void options_extra_init()
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
#undef option_var_set_callback
}

/**
   Do any necessary UI-specific cleanup
 */
void ui_exit() { delete freeciv_qt; }

/**
   Update the connected users list at pregame state.
 */
void real_conn_list_dialog_update(void *unused)
{
  if (get_current_client_page() == PAGE_NETWORK) {
    real_set_client_page(PAGE_START);
  }
  qobject_cast<page_pregame *>(king()->pages[PAGE_START])
      ->update_start_page();
}

/**
   Make a bell noise (beep).  This provides low-level sound alerts even
   if there is no real sound support.
 */
void sound_bell()
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
void add_net_input(QIODevice *sock) { king()->add_server_source(sock); }

/**
   Called when the set of units in focus (get_units_in_focus()) changes.
   Standard updates like update_unit_info_label() are handled in the
 platform- independent code, so some clients will not need to do anything
 here.
 */
void real_focus_units_changed()
{
  units_select *unit_sel = queen()->unit_selector;
  if (unit_sel != nullptr && unit_sel->isVisible()) {
    unit_sel->update_units();
  }
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
   Change the given font.
 */
void gui_qt_apply_font(struct option *poption)
{
  if (king()) {
    auto f = option_font_get(poption);
    auto s = option_name(poption);
    fcFont::instance()->setFont(s, f);
    update_map_canvas_visible();
    queen()->chat->update_font();
    QApplication::setFont(fcFont::instance()->getFont(fonts::default_font));
    real_science_report_dialog_update(nullptr);
  }
  apply_help_font(poption);
}

/**
   Applies help font
 */
static void apply_help_font(struct option *poption)
{
  if (king()) {
    fcFont::instance()->setFont(option_name(poption),
                                option_font_get(poption));
    update_help_fonts();
  }
}

/**
   Applies help font
 */
static void apply_notify_font(struct option *poption)
{
  if (auto page_game = queen(); page_game) {
    gui_update_font(QStringLiteral("notify_label"),
                    option_font_get(poption));

    auto list =
        page_game->mapview_wdg->findChildren<freeciv::report_widget *>();
    for (auto report : list) {
      QApplication::postEvent(report, new QEvent(QEvent::FontChange));
    }
  }
  if (king() && get_current_client_page() == PAGE_GAME) {
    gui_update_font(QStringLiteral("city_label"), option_font_get(poption));
    city_font_update();
  }
}

/**
   Stub for editor function
 */
void editgui_tileset_changed() {}

/**
   Stub for editor function
 */
void editgui_refresh() {}

/**
   Stub for editor function
 */
void editgui_popup_properties(const struct tile_list *tiles, int objtype) {}

/**
   Stub for editor function
 */
void editgui_popdown_all() {}

/**
   Stub for editor function
 */
void editgui_notify_object_changed(int objtype, int object_id, bool removal)
{
}

/**
   Stub for editor function
 */
void editgui_notify_object_created(int tag, int id) {}

/**
   Updates a gui font style.
 */
void gui_update_font(const QString &font_name, const QFont &font)
{
  auto fname = QStringLiteral("gui_qt_font_") + QString(font_name);
  auto remove_old = fcFont::instance()->getFont(fname);
  fcFont::instance()->setFont(fname, font);
}

void gui_update_allfonts()
{
  fcFont::instance()->setSizeAll(gui_options->gui_qt_increase_fonts);
  gui_options->gui_qt_increase_fonts = 0;
}

/**
   Open dialog to confirm that user wants to quit client.
 */
void popup_quit_dialog()
{
  hud_message_box *ask = new hud_message_box(king()->central_wdg);

  ask->set_text_title(_("Are you sure you want to quit?"), _("Quit?"));
  ask->setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::No);
  ask->button(QMessageBox::Yes)->setText(_("Yes Quit"));
  ask->button(QMessageBox::No)->setText(_("Keep Playing"));
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
