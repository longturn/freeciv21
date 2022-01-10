/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include <memory>

// Qt
#include <QMouseEvent>
#include <QPainter>
// utility
#include "log.h"
// common
#include "calendar.h"
#include "game.h"
#include "map.h"
// client
#include "citybar.h"
#include "citydlg_g.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors_common.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "mapview_g.h"
#include "overview_common.h"
#include "sprite.h"
#include "text.h"
#include "tilespec.h"
// gui-qt
#include "canvas.h"
#include "colors.h"
#include "fc_client.h"
#include "fonts.h"
#include "hudwidget.h"
#include "mapview.h"
#include "messagewin.h"
#include "minimap.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "sciencedlg.h"
#include "sidebar.h"
#include "widgetdecorations.h"

static int mapview_frozen_level = 0;
extern void destroy_city_dialog();
extern QPixmap *canvas;
extern QApplication *qapp;

#define MAX_DIRTY_RECTS 20
static int num_dirty_rects = 0;
static QRect dirty_rects[MAX_DIRTY_RECTS];
info_tile *info_tile::m_instance = nullptr;
extern int last_center_enemy;
extern int last_center_capital;
extern int last_center_player_city;
extern int last_center_enemy_city;

/**
   Check if point x, y is in area (px -> pxe, py - pye)
 */
bool is_point_in_area(int x, int y, int px, int py, int pxe, int pye)
{
  return x >= px && y >= py && x <= pxe && y <= pye;
}

/**
   Draws calculated trade routes
 */
void draw_calculated_trade_routes(QPainter *painter)
{
  int dx, dy;
  float w, h;
  float x1, y1, x2, y2;
  QColor *pcolor;
  QPen pen;

  if (!can_client_control() || king()->trade_gen.cities.empty()) {
    return;
  }
  pcolor = get_color(tileset, COLOR_MAPVIEW_TRADE_ROUTES_NO_BUILT);
  // Draw calculated trade routes
  if (gui_options.draw_city_trade_routes) {
    for (auto qgilles : qAsConst(king()->trade_gen.lines)) {
      base_map_distance_vector(&dx, &dy, TILE_XY(qgilles.t1),
                               TILE_XY(qgilles.t2));
      map_to_gui_vector(tileset, &w, &h, dx, dy);

      tile_to_canvas_pos(&x1, &y1, qgilles.t1);
      tile_to_canvas_pos(&x2, &y2, qgilles.t2);

      // Dont draw if route was already established
      if (tile_city(qgilles.t1) && tile_city(qgilles.t2)
          && have_cities_trade_route(tile_city(qgilles.t1),
                                     tile_city(qgilles.t2))) {
        continue;
      }

      if (qgilles.autocaravan != nullptr) {
        pcolor = get_color(tileset, COLOR_MAPVIEW_TRADE_ROUTES_SOME_BUILT);
      }

      pen.setColor(*pcolor);
      pen.setStyle(Qt::DashLine);
      pen.setDashOffset(4);
      pen.setWidth(1);
      painter->setPen(pen);
      if (x2 - x1 == w && y2 - y1 == h) {
        painter->drawLine(x1 + tileset_tile_width(tileset) / 2,
                          y1 + tileset_tile_height(tileset) / 2,
                          x1 + tileset_tile_width(tileset) / 2 + w,
                          y1 + tileset_tile_height(tileset) / 2 + h);
        continue;
      }
      painter->drawLine(x2 + tileset_tile_width(tileset) / 2,
                        y2 + tileset_tile_height(tileset) / 2,
                        x2 + tileset_tile_width(tileset) / 2 - w,
                        y2 + tileset_tile_height(tileset) / 2 - h);
    }
  }
  // Draw virtual cities
  for (auto *pcity : qAsConst(king()->trade_gen.virtual_cities)) {
    float canvas_x, canvas_y;
    if (pcity->tile != nullptr
        && tile_to_canvas_pos(&canvas_x, &canvas_y, pcity->tile)) {
      painter->drawPixmap(static_cast<int>(canvas_x),
                          static_cast<int>(canvas_y),
                          *get_attention_crosshair_sprite(tileset));
    }
  }
}

/**
   Constructor for map
 */
map_view::map_view() : QWidget()
{
  menu_click = false;
  cursor = -1;
  QTimer *timer = new QTimer(this);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  connect(timer, &QTimer::timeout, this, &map_view::timer_event);
  timer->start(200);
  resize(0, 0);
  setMouseTracking(true);
  stored_autocenter = gui_options.auto_center_on_unit;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

/**
   Updates cursor
 */
void map_view::update_cursor(enum cursor_type ct)
{
  int i;

  if (ct == CURSOR_DEFAULT) {
    setCursor(Qt::ArrowCursor);
    cursor = -1;
    return;
  }
  cursor_frame = 0;
  i = static_cast<int>(ct);
  cursor = i;
  setCursor(*(king()->fc_cursors[i][0]));
}

/**
   Hides all fcwidgets (reports etc). Used to make room for the city dialog.
 */
void map_view::hide_all_fcwidgets()
{
  QList<fcwidget *> fcl = this->findChildren<fcwidget *>();
  for (auto *widget : qAsConst(fcl)) {
    if (widget->isVisible()) {
      widget->hide();
      m_hidden_fcwidgets.push_back(widget);
    }
  }
}

/**
   Shows all fcwidgets (reports etc). Used when closing the city dialog.
 */
void map_view::show_all_fcwidgets()
{
  for (auto *widget : m_hidden_fcwidgets) {
    widget->show();
  }
  m_hidden_fcwidgets.clear();
}

/**
 * Ppens the tileset debugger.
 */
void map_view::show_debugger()
{
  if (!m_debugger) {
    // We never destroy it once it's created.
    m_debugger = new freeciv::tileset_debugger(this);
    connect(m_debugger, &freeciv::tileset_debugger::tile_picking_requested,
            [](bool active) {
              if (active) {
                set_hover_state(NULL, HOVER_DEBUG_TILE, ACTIVITY_LAST, NULL,
                                NO_TARGET, NO_TARGET, ACTION_NONE,
                                ORDER_LAST);
              } else if (!active && hover_state == HOVER_DEBUG_TILE) {
                clear_hover_state();
              }
            });
  }

  m_debugger->show();
}

/**
 * Closes the tileset debugger if it is open.
 */
void map_view::hide_debugger()
{
  if (m_debugger) {
    m_debugger->set_tile(nullptr);
    m_debugger->close();
  }
}

/**
   Timer for cursor
 */
void map_view::timer_event()
{
  if (queen()->infotab->underMouse()
      || queen()->minimapview_wdg->underMouse()
      || queen()->sidebar_wdg->underMouse()) {
    update_cursor(CURSOR_DEFAULT);
    return;
  }
  if (cursor == -1) {
    return;
  }
  cursor_frame++;
  if (cursor_frame == NUM_CURSOR_FRAMES) {
    cursor_frame = 0;
  }
  setCursor(*(king()->fc_cursors[cursor][cursor_frame]));
}

/**
   Updates fonts
 */
void map_view::update_font(const QString &name, const QFont &font)
{
  if (name == fonts::city_names || name == fonts::city_productions) {
    update_map_canvas_visible();
  }
}

/**
   Focus lost event
 */
void map_view::focusOutEvent(QFocusEvent *event)
{
  Q_UNUSED(event)
  update_cursor(CURSOR_DEFAULT);
}

/**
   Leave event
 */
void map_view::leaveEvent(QEvent *event)
{
  Q_UNUSED(event);
  update_cursor(CURSOR_DEFAULT);
}

/**
   Slot inherited from QPixamp
 */
void map_view::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Redraws given rectangle on map
 */
void map_view::paint(QPainter *painter, QPaintEvent *event)
{
  painter->drawPixmap(event->rect(), *mapview.store, event->rect());
  draw_calculated_trade_routes(painter);
}

/**
   Sets new point for new search
 */
void map_view::resume_searching(int pos_x, int pos_y, int &w, int &h,
                                int wdth, int hght, int recursive_nr,
                                bool direction)
{
  int new_pos_x, new_pos_y;

  recursive_nr++;
  new_pos_x = pos_x;
  new_pos_y = pos_y;

  if (direction) {
    if (pos_y + hght + 4 < height() && pos_x < width() / 2) {
      new_pos_y = pos_y + 5;
    } else if (pos_x > 0 && pos_y > 10) {
      new_pos_x = pos_x - 5;
    } else if (pos_y > 0) {
      new_pos_y = pos_y - 5;
    } else if (pos_x + wdth + 4 < this->width()) {
      new_pos_x = pos_x + 5;
    }
  } else {
    if (pos_y + hght + 4 < height() && pos_x > width() / 2) {
      new_pos_y = pos_y + 5;
    } else if (pos_x > 0 && pos_y > 10) {
      new_pos_x = pos_x - 5;
    } else if (pos_y > 0) {
      new_pos_y = pos_y - 5;
    } else if (pos_x + wdth + 4 < this->width()) {
      new_pos_x = pos_x + 5;
    }
  }

  find_place(new_pos_x, new_pos_y, w, h, wdth, hght, recursive_nr,
             direction);
}

/**
   Searches place for widget with size w and height h
   Starts looking from position pos_x, pos_y, going clockwork
   Returns position as (w,h)
   Along with resume_searching its recursive function.
   direction = false (default) = clockwise
 */
void map_view::find_place(int pos_x, int pos_y, int &w, int &h, int wdth,
                          int hght, int recursive_nr, bool direction)
{
  int i;
  int x, y, xe, ye;
  QList<fcwidget *> widgets = this->findChildren<fcwidget *>();
  bool cont_searching = false;

  if (recursive_nr >= 1000) {
    /**
     * give up searching position
     */
    return;
  }
  /**
   * try position pos_x, pos_y,
   * check middle and borders if aren't  above other widget
   */

  for (i = 0; i < widgets.count(); i++) {
    if (!widgets[i]->isVisible()) {
      continue;
    }
    x = widgets[i]->pos().x();
    y = widgets[i]->pos().y();

    if (x == 0 && y == 0) {
      continue;
    }
    xe = widgets[i]->pos().x() + widgets[i]->width();
    ye = widgets[i]->pos().y() + widgets[i]->height();

    if (is_point_in_area(pos_x, pos_y, x, y, xe, ye)) {
      cont_searching = true;
    }
    if (is_point_in_area(pos_x + wdth, pos_y, x, y, xe, ye)) {
      cont_searching = true;
    }
    if (is_point_in_area(pos_x + wdth, pos_y + hght, x, y, xe, ye)) {
      cont_searching = true;
    }
    if (is_point_in_area(pos_x, pos_y + hght, x, y, xe, ye)) {
      cont_searching = true;
    }
    if (is_point_in_area(pos_x + wdth / 2, pos_y + hght / 2, x, y, xe, ye)) {
      cont_searching = true;
    }
  }
  w = pos_x;
  h = pos_y;
  if (cont_searching) {
    resume_searching(pos_x, pos_y, w, h, wdth, hght, recursive_nr,
                     direction);
  }
}

/**
   Update the information label which gives info on the current unit
   and the tile under the current unit, for specified unit.  Note that
   in practice punit is always the focus unit.

   Clears label if punit is nullptr.

   Typically also updates the cursor for the map_canvas (this is
   related because the info label may includes "select destination"
   prompt etc).  And it may call update_unit_pix_label() to update the
   icons for units on this tile.
 */
void update_unit_info_label(struct unit_list *punitlist)
{
  if (queen()->unitinfo_wdg->isVisible()) {
    queen()->unitinfo_wdg->update_actions(nullptr);
  }
}

/**
   Update the mouse cursor. Cursor type depends on what user is doing and
   pointing.
 */
void update_mouse_cursor(enum cursor_type new_cursor_type)
{
  queen()->mapview_wdg->update_cursor(new_cursor_type);
}

/**
   Update the timeout display.  The timeout is the time until the turn
   ends, in seconds.
 */
void qtg_update_timeout_label()
{
  queen()->sw_endturn->setCustomLabels(QString(get_timeout_label_text()));
  queen()->sw_endturn->update();
}

/**
   If do_restore is false it should change the turn button style (to
   draw the user's attention to it).  If called regularly from a timer
   this will give a blinking turn done button.  If do_restore is true
   this should reset the turn done button to the default style.
 */
void update_turn_done_button(bool do_restore)
{
  if (!get_turn_done_button_state()) {
    return;
  }
  sidebarBlinkEndturn(do_restore);
}

/**
   Flush the given part of the canvas buffer (if there is one) to the
   screen.
 */
static void flush_mapcanvas(int canvas_x, int canvas_y, int pixel_width,
                            int pixel_height)
{
  queen()->mapview_wdg->repaint(canvas_x, canvas_y, pixel_width,
                                pixel_height);
}

/**
   Mark the rectangular region as "dirty" so that we know to flush it
   later.
 */
void dirty_rect(int canvas_x, int canvas_y, int pixel_width,
                int pixel_height)
{
  if (mapview_is_frozen()) {
    return;
  }
  if (num_dirty_rects < MAX_DIRTY_RECTS) {
    dirty_rects[num_dirty_rects].setX(canvas_x);
    dirty_rects[num_dirty_rects].setY(canvas_y);
    dirty_rects[num_dirty_rects].setWidth(pixel_width);
    dirty_rects[num_dirty_rects].setHeight(pixel_height);
    num_dirty_rects++;
  }
}

/**
   Mark the entire screen area as "dirty" so that we can flush it later.
 */
void dirty_all(void)
{
  if (mapview_is_frozen()) {
    return;
  }
  num_dirty_rects = MAX_DIRTY_RECTS;
}

/**
   Flush all regions that have been previously marked as dirty.  See
   dirty_rect and dirty_all.  This function is generally called after we've
   processed a batch of drawing operations.
 */
void flush_dirty(void)
{
  if (mapview_is_frozen()) {
    return;
  }
  if (num_dirty_rects == MAX_DIRTY_RECTS) {
    flush_mapcanvas(0, 0, queen()->mapview_wdg->width(),
                    queen()->mapview_wdg->height());
  } else {
    int i;

    for (i = 0; i < num_dirty_rects; i++) {
      flush_mapcanvas(dirty_rects[i].x(), dirty_rects[i].y(),
                      dirty_rects[i].width(), dirty_rects[i].height());
    }
  }
  num_dirty_rects = 0;
}

/**
   Do any necessary synchronization to make sure the screen is up-to-date.
   The canvas should have already been flushed to screen via flush_dirty -
   all this function does is make sure the hardware has caught up.
 */
void gui_flush(void) { queen()->mapview_wdg->update(); }

/**
   Update (refresh) the locations of the mapview scrollbars (if it uses
   them).
 */
void update_map_canvas_scrollbars(void) { queen()->mapview_wdg->update(); }

/**
   Update (refresh) all city descriptions on the mapview.
 */
void update_city_descriptions(void) { update_map_canvas_visible(); }

/**
   Put overlay tile to pixmap
 */
void pixmap_put_overlay_tile(int canvas_x, int canvas_y,
                             const QPixmap *ssprite)
{
  if (!ssprite) {
    return;
  }

  // PORTME
}

/**
   Draw a cross-hair overlay on a tile.
 */
void put_cross_overlay_tile(struct tile *ptile)
{
  float canvas_x, canvas_y;

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
    pixmap_put_overlay_tile(canvas_x, canvas_y,
                            get_attention_crosshair_sprite(tileset));
  }
}

/**
   Area Selection
 */
void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h)
{
  // DON'T PORTME
}

/**
   This function is called when the tileset is changed.
 */
void tileset_changed(void)
{
  int i;
  science_report *sci_rep;
  QWidget *w;

  // Refresh the tileset debugger if it exists
  if (auto debugger = queen()->mapview_wdg->debugger();
      debugger != nullptr) {
    // When not zoomed in, unscaled_tileset is null
    // When zoomed in, unscaled_tileset is not null and holds the log
    debugger->refresh(unscaled_tileset ? unscaled_tileset : tileset);
  }

  update_unit_info_label(get_units_in_focus());
  destroy_city_dialog();
  // Update science report if open
  if (queen()->isRepoDlgOpen(QStringLiteral("SCI"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("SCI"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    sci_rep = reinterpret_cast<science_report *>(w);
    sci_rep->reset_tree();
    sci_rep->update_report();
    sci_rep->repaint();
  }

  // When the tileset has an error, tell the user and give him a link to the
  // debugger where the messages can be found.
  if (tileset_has_error(tileset) && king() != nullptr) {
    QMessageBox *ask = new QMessageBox(king()->central_wdg);
    ask->setWindowTitle(_("Error loading tileset"));
    ask->setAttribute(Qt::WA_DeleteOnClose);
    ask->setText(
        // TRANS: %1 is the name of the tileset
        QString(_("There was an error loading tileset \"%1\". You can still "
                  "use it, but it might be incomplete."))
            .arg(tileset_name_get(tileset)));
    ask->setIcon(QMessageBox::Warning);
    ask->setStandardButtons(QMessageBox::Close);

    auto button =
        ask->addButton(_("Open tileset &debugger"), QMessageBox::AcceptRole);
    QObject::connect(button, &QPushButton::clicked, queen()->mapview_wdg,
                     &map_view::show_debugger);

    ask->show();
  }
}

/**
   Return whether the map should be drawn or not.
 */
bool mapview_is_frozen() { return (0 < mapview_frozen_level); }

/**
   Freeze the drawing of the map.
 */
void mapview_freeze() { mapview_frozen_level++; }

/**
   Thaw the drawing of the map.
 */
void mapview_thaw()
{
  if (1 < mapview_frozen_level) {
    mapview_frozen_level--;
  } else {
    fc_assert(0 < mapview_frozen_level);
    mapview_frozen_level = 0;
    dirty_all();
  }
}

/**
   Constructor for info_tile
 */
info_tile::info_tile(struct tile *ptile, QWidget *parent) : QLabel(parent)
{
  setParent(parent);
  info_font = fcFont::instance()->getFont(fonts::notify_label);
  itile = ptile;
  calc_size();
}

/**
   Calculates size of info_tile and moves it to be fully visible
 */
void info_tile::calc_size()
{
  QFontMetrics fm(info_font);
  QString str;
  int hh = tileset_tile_height(tileset);
  int fin_x;
  int fin_y;
  float x, y;
  int w = 0;

  str = popup_info_text(itile);
  str_list = str.split(QStringLiteral("\n"));

  for (auto const &str : qAsConst(str_list)) {
    w = qMax(w, fm.horizontalAdvance(str));
  }
  setFixedHeight(str_list.count() * (fm.height() + 5));
  setFixedWidth(w + 10);
  if (tile_to_canvas_pos(&x, &y, itile)) {
    fin_x = x;
    fin_y = y;
    if (y - height() > 0) {
      fin_y = y - height();
    } else {
      fin_y = y + hh;
    }
    if (x + width() > parentWidget()->width()) {
      fin_x = parentWidget()->width() - width();
    }
    move(fin_x, fin_y);
  }
}

/**
   Redirected paint event for info_tile
 */
void info_tile::paint(QPainter *painter, QPaintEvent *event)
{
  Q_UNUSED(event)
  QFontMetrics fm(info_font);
  int pos, h;

  h = fm.height();
  pos = h;
  painter->setFont(info_font);
  for (int i = 0; i < str_list.count(); i++) {
    painter->drawText(5, pos, str_list.at(i));
    pos = pos + 5 + h;
  }
}

/**
   Paint event for info_tile
 */
void info_tile::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Updates fonts
 */
void info_tile::update_font(const QString &name, const QFont &font)
{
  if (name == fonts::notify_label) {
    info_font = font;
    calc_size();
    update();
  }
}

/**
   Deletes current instance
 */
void info_tile::drop() { NFCN_FREE(m_instance); }

/**
   Returns given instance
 */
info_tile *info_tile::i(struct tile *p)
{
  if (!m_instance && p) {
    m_instance = new info_tile(p, queen()->mapview_wdg);
  }
  return m_instance;
}

/**
   Popups information label tile
 */
void popup_tile_info(struct tile *ptile)
{
  struct unit *punit = nullptr;

  if (TILE_UNKNOWN != client_tile_get_known(ptile)) {
    mapdeco_set_crosshair(ptile, true);
    punit = find_visible_unit(ptile);
    if (punit) {
      mapdeco_set_gotoroute(punit);
      if (punit->goto_tile && unit_has_orders(punit)) {
        mapdeco_set_crosshair(punit->goto_tile, true);
      }
    }
    info_tile::i(ptile)->show();
  }
}

/**
   Popdowns information label tile
 */
void popdown_tile_info()
{
  mapdeco_clear_crosshairs();
  mapdeco_clear_gotoroutes();
  info_tile::i()->drop();
}

/**
   New turn callback
 */
void qtg_start_turn()
{
  show_new_turn_info();
  last_center_enemy = 0;
  last_center_capital = 0;
  last_center_player_city = 0;
  last_center_enemy_city = 0;
}

/**
   Draw a description for the given city.  This description may include the
   name, turns-to-grow, production, and city turns-to-build (depending on
   client options).

   (canvas_x, canvas_y) gives the location on the given canvas at which to
   draw the description.  This is the location of the city itself so the
   text must be drawn underneath it.  pcity gives the city to be drawn,
   while (*width, *height) should be set by show_city_desc to contain the
   width and height of the text block (centered directly underneath the
   city's tile).
 */
void show_city_desc(QPixmap *pcanvas, int canvas_x, int canvas_y,
                    struct city *pcity, int *width, int *height)
{
  if (is_any_city_dialog_open()) {
    return;
  }

  QPainter p;
  p.begin(pcanvas);

  canvas_x += tileset_tile_width(tileset) / 2;
  canvas_y += tileset_citybar_offset_y(tileset);

  auto *painter = citybar_painter::current();
  auto rect = painter->paint(p, QPointF(canvas_x, canvas_y), pcity);
  *width = rect.width();
  *height = rect.height();

  p.end();
}

/**
 * Callback to set the tile being debugged.
 */
void debug_tile(tile *tile)
{
  fc_assert_ret(queen()->mapview_wdg->m_debugger);
  queen()->mapview_wdg->m_debugger->set_tile(tile);
}
