/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


// Qt
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QToolTip>
#include <QWheelEvent>

// utility
#include "log.h"
#include "support.h"

// common
#include "calendar.h"
#include "game.h"
#include "map.h"
#include "research.h"

// client
#include "climisc.h"
#include "colors_common.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "menu.h" // gov_menu
#include "movement.h"
#include "overview_common.h"
#include "sprite.h"
#include "text.h"
#include "tilespec.h"

// gui-qt
#include "colors.h"
#include "fc_client.h"
#include "icons.h"
#include "mapview.h"
#include "minimap.h"
#include "sciencedlg.h"
#include "qtg_cxxside.h"
#include "sidebar.h"

const char *get_timeout_label_text();
static int mapview_frozen_level = 0;
extern void destroy_city_dialog();
extern struct canvas *canvas;
extern QApplication *qapp;

#define MAX_DIRTY_RECTS 20
static int num_dirty_rects = 0;
static QRect dirty_rects[MAX_DIRTY_RECTS];

extern int last_center_enemy;
extern int last_center_capital;
extern int last_center_player_city;
extern int last_center_enemy_city;

/**********************************************************************/ /**
   Check if point x, y is in area (px -> pxe, py - pye)
 **************************************************************************/
bool is_point_in_area(int x, int y, int px, int py, int pxe, int pye)
{
  if (x >= px && y >= py && x <= pxe && y <= pye) {
    return true;
  }
  return false;
}

/**********************************************************************/ /**
   Draws calculated trade routes
 **************************************************************************/
void draw_calculated_trade_routes(QPainter *painter)
{
  int dx, dy;
  float w, h;
  float x1, y1, x2, y2;
  qtiles qgilles;
  struct city *pcity;
  struct color *pcolor;
  QPen pen;

  if (!can_client_control() || gui()->trade_gen.cities.empty()) {
    return;
  }
  pcolor = get_color(tileset, COLOR_MAPVIEW_TRADE_ROUTES_NO_BUILT);
  /* Draw calculated trade routes */
  if (gui_options.draw_city_trade_routes) {

    for (auto qgilles : qAsConst(gui()->trade_gen.lines)) {
      base_map_distance_vector(&dx, &dy, TILE_XY(qgilles.t1),
                               TILE_XY(qgilles.t2));
      map_to_gui_vector(tileset, 1.0, &w, &h, dx, dy);

      tile_to_canvas_pos(&x1, &y1, qgilles.t1);
      tile_to_canvas_pos(&x2, &y2, qgilles.t2);

      /* Dont draw if route was already established */
      if (tile_city(qgilles.t1) && tile_city(qgilles.t2)
          && have_cities_trade_route(tile_city(qgilles.t1),
                                     tile_city(qgilles.t2))) {
        continue;
      }

      if (qgilles.autocaravan != nullptr) {
        pcolor = get_color(tileset, COLOR_MAPVIEW_TRADE_ROUTES_SOME_BUILT);
      }

      pen.setColor(pcolor->qcolor);
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
  /* Draw virtual cities */
  for (auto pcity : qAsConst(gui()->trade_gen.virtual_cities)) {
    float canvas_x, canvas_y;
    if (pcity->tile != nullptr
        && tile_to_canvas_pos(&canvas_x, &canvas_y, pcity->tile)) {
      painter->drawPixmap(static_cast<int>(canvas_x),
                          static_cast<int>(canvas_y),
                          *get_attention_crosshair_sprite(tileset)->pm);
    }
  }
}

/**********************************************************************/ /**
   Constructor for idle callbacks
 **************************************************************************/
mr_idle::mr_idle()
{
  connect(&timer, &QTimer::timeout, this, &mr_idle::idling);
  timer.start(5);
}

/**********************************************************************/ /**
   Destructor for idle callbacks
 **************************************************************************/
mr_idle::~mr_idle()
{
  call_me_back *cb;

  while (!callback_list.isEmpty()) {
    cb = callback_list.dequeue();
    delete cb;
  }
}

/**********************************************************************/ /**
   Slot used to execute 1 callback from callbacks stored in idle list
 **************************************************************************/
void mr_idle::idling()
{
  call_me_back *cb;

  while (!callback_list.isEmpty()) {
    cb = callback_list.dequeue();
    (cb->callback)(cb->data);
    delete cb;
  }
}

/**********************************************************************/ /**
   Adds one callback to execute later
 **************************************************************************/
void mr_idle::add_callback(call_me_back *cb) { callback_list.enqueue(cb); }

/**********************************************************************/ /**
   Constructor for map
 **************************************************************************/
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

/**********************************************************************/ /**
   Updates cursor
 **************************************************************************/
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
  setCursor(*(gui()->fc_cursors[i][0]));
}

/**********************************************************************/ /**
   Timer for cursor
 **************************************************************************/
void map_view::timer_event()
{
  if (gui()->infotab->underMouse() || gui()->minimapview_wdg->underMouse()
      || gui()->sidebar_wdg->underMouse()) {
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
  setCursor(*(gui()->fc_cursors[cursor][cursor_frame]));
}

/**********************************************************************/ /**
   Updates fonts
 **************************************************************************/
void map_view::update_font(const QString &name, const QFont &font)
{
  if (name == fonts::city_names || name == fonts::city_productions) {
    update_map_canvas_visible();
  }
}

/**********************************************************************/ /**
   Focus lost event
 **************************************************************************/
void map_view::focusOutEvent(QFocusEvent *event)
{
  update_cursor(CURSOR_DEFAULT);
}

/**********************************************************************/ /**
   Leave event
 **************************************************************************/
void map_view::leaveEvent(QEvent *event) { update_cursor(CURSOR_DEFAULT); }

/**********************************************************************/ /**
   Slot inherited from QPixamp
 **************************************************************************/
void map_view::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**********************************************************************/ /**
   Redraws given rectangle on map
 **************************************************************************/
void map_view::paint(QPainter *painter, QPaintEvent *event)
{
  painter->drawPixmap(event->rect(), mapview.store->map_pixmap,
                      event->rect());
  draw_calculated_trade_routes(painter);
}

/**********************************************************************/ /**
   Sets new point for new search
 **************************************************************************/
void map_view::resume_searching(int pos_x, int pos_y, int &w, int &h,
                                int wdth, int hght, int recursive_nr)
{
  int new_pos_x, new_pos_y;

  recursive_nr++;
  new_pos_x = pos_x;
  new_pos_y = pos_y;

  if (pos_y + hght + 4 < height() && pos_x > width() / 2) {
    new_pos_y = pos_y + 5;
  } else if (pos_x > 0 && pos_y > 10) {
    new_pos_x = pos_x - 5;
  } else if (pos_y > 0) {
    new_pos_y = pos_y - 5;
  } else if (pos_x + wdth + 4 < this->width()) {
    new_pos_x = pos_x + 5;
  }
  find_place(new_pos_x, new_pos_y, w, h, wdth, hght, recursive_nr);
}

/**********************************************************************/ /**
   Searches place for widget with size w and height h
   Starts looking from position pos_x, pos_y, going clockwork
   Returns position as (w,h)
   Along with resume_searching its recursive function.
 **************************************************************************/
void map_view::find_place(int pos_x, int pos_y, int &w, int &h, int wdth,
                          int hght, int recursive_nr)
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
    resume_searching(pos_x, pos_y, w, h, wdth, hght, recursive_nr);
  }
}

/**********************************************************************/ /**
   Constructor for move widget
 **************************************************************************/
move_widget::move_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::SizeAllCursor);
  pix = fc_icons::instance()->get_pixmap("move");
  setPixmap(*pix);
  delete pix;
  setFixedSize(16, 16);
}

/**********************************************************************/ /**
   Puts move widget to left top corner
 **************************************************************************/
void move_widget::put_to_corner() { move(0, 0); }

/**********************************************************************/ /**
   Mouse handler for move widget (moves parent widget)
 **************************************************************************/
void move_widget::mouseMoveEvent(QMouseEvent *event)
{
  if (!gui()->interface_locked) {
    parentWidget()->move(event->globalPos() - point);
  }
}

/**********************************************************************/ /**
   Sets moving point for move widget;
 **************************************************************************/
void move_widget::mousePressEvent(QMouseEvent *event)
{
  if (!gui()->interface_locked) {
    point = event->globalPos() - parentWidget()->geometry().topLeft();
  }
  update();
}

/**********************************************************************/ /**
   Constructor for resize widget
 **************************************************************************/
resize_widget::resize_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::SizeFDiagCursor);
  pix = fc_icons::instance()->get_pixmap("resize");
  setPixmap(*pix);
  delete pix;
}

/**********************************************************************/ /**
   Puts resize widget to left top corner
 **************************************************************************/
void resize_widget::put_to_corner()
{
  move(parentWidget()->width() - width(),
       parentWidget()->height() - height());
}

/**********************************************************************/ /**
   Mouse handler for resize widget (resizes parent widget)
 **************************************************************************/
void resize_widget::mouseMoveEvent(QMouseEvent *event)
{
  QPoint qp, np;

  if (gui()->interface_locked) {
    return;
  }
  qp = event->globalPos();
  np.setX(qp.x() - point.x());
  np.setY(qp.y() - point.y());
  np.setX(qMax(np.x(), 32));
  np.setY(qMax(np.y(), 32));
  parentWidget()->resize(np.x(), np.y());
}

/**********************************************************************/ /**
   Sets moving point for resize widget;
 **************************************************************************/
void resize_widget::mousePressEvent(QMouseEvent *event)
{
  QPoint qp;

  if (gui()->interface_locked) {
    return;
  }
  qp = event->globalPos();
  point.setX(qp.x() - parentWidget()->width());
  point.setY(qp.y() - parentWidget()->height());
  update();
}

/**********************************************************************/ /**
   Constructor for close widget
 **************************************************************************/
close_widget::close_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::ArrowCursor);
  pix = fc_icons::instance()->get_pixmap("close");
  setPixmap(*pix);
  delete pix;
}

/**********************************************************************/ /**
   Puts close widget to right top corner
 **************************************************************************/
void close_widget::put_to_corner()
{
  move(parentWidget()->width() - width(), 0);
}

/**********************************************************************/ /**
   Mouse handler for close widget, hides parent widget
 **************************************************************************/
void close_widget::mousePressEvent(QMouseEvent *event)
{
  if (gui()->interface_locked) {
    return;
  }
  if (event->button() == Qt::LeftButton) {
    parentWidget()->hide();
    notify_parent();
  }
}

/**********************************************************************/ /**
   Notifies parent to do custom action, parent is already hidden.
 **************************************************************************/
void close_widget::notify_parent()
{
  fcwidget *fcw;

  fcw = reinterpret_cast<fcwidget *>(parentWidget());
  fcw->update_menu();
}

/**********************************************************************/ /**
   Typically an info box is provided to tell the player about the state
   of their civilization.  This function is called when the label is
   changed.
 **************************************************************************/
void update_info_label(void) { gui()->update_info_label(); }

/**********************************************************************/ /**
   Real update, updates only once per 300 ms.
 **************************************************************************/
void fc_client::update_info_label(void)
{
  QString s, eco_info;

  if (current_page() != PAGE_GAME) {
    return;
  }
  if (update_info_timer == nullptr) {
    update_info_timer = new QTimer();
    update_info_timer->setSingleShot(true);
    connect(update_info_timer, &QTimer::timeout, this,
            &fc_client::update_info_label);
    update_info_timer->start(300);
    return;
  }

  if (update_info_timer->remainingTime() > 0) {
    return;
  }
  update_sidebar_tooltips();
  if (head_of_units_in_focus() != nullptr) {
    real_menus_update();
  }
  /* TRANS: T is shortcut from Turn */
  s = QString(_("%1 \nT:%2"))
          .arg(calendar_text(), QString::number(game.info.turn));

  sw_map->set_custom_labels(s);
  sw_map->update_final_pixmap();

  set_indicator_icons(client_research_sprite(), client_warming_sprite(),
                      client_cooling_sprite(), client_government_sprite());

  if (client.conn.playing != NULL) {
    if (player_get_expected_income(client.conn.playing) > 0) {
      eco_info =
          QString(_("%1 (+%2)"))
              .arg(QString::number(client.conn.playing->economic.gold),
                   QString::number(
                       player_get_expected_income(client.conn.playing)));
    } else {
      eco_info =
          QString(_("%1 (%2)"))
              .arg(QString::number(client.conn.playing->economic.gold),
                   QString::number(
                       player_get_expected_income(client.conn.playing)));
    }
    sw_economy->set_custom_labels(eco_info);
  } else {
    sw_economy->set_custom_labels("");
  }
  sw_tax->update_final_pixmap();
  sw_economy->update_final_pixmap();
  delete update_info_timer;
  update_info_timer = nullptr;
}

/**********************************************************************/ /**
   Update the information label which gives info on the current unit
   and the tile under the current unit, for specified unit.  Note that
   in practice punit is always the focus unit.

   Clears label if punit is NULL.

   Typically also updates the cursor for the map_canvas (this is
   related because the info label may includes "select destination"
   prompt etc).  And it may call update_unit_pix_label() to update the
   icons for units on this tile.
 **************************************************************************/
void update_unit_info_label(struct unit_list *punitlist)
{
  if (gui()->unitinfo_wdg->isVisible()) {
    gui()->unitinfo_wdg->update_actions(nullptr);
  }
}

/**********************************************************************/ /**
   Update the mouse cursor. Cursor type depends on what user is doing and
   pointing.
 **************************************************************************/
void update_mouse_cursor(enum cursor_type new_cursor_type)
{
  gui()->mapview_wdg->update_cursor(new_cursor_type);
}

/**********************************************************************/ /**
   Update the timeout display.  The timeout is the time until the turn
   ends, in seconds.
 **************************************************************************/
void qtg_update_timeout_label(void)
{
  gui()->sw_endturn->set_custom_labels(QString(get_timeout_label_text()));
  gui()->sw_endturn->update_final_pixmap();
}

/**********************************************************************/ /**
   If do_restore is false it should change the turn button style (to
   draw the user's attention to it).  If called regularly from a timer
   this will give a blinking turn done button.  If do_restore is true
   this should reset the turn done button to the default style.
 **************************************************************************/
void update_turn_done_button(bool do_restore)
{
  if (!get_turn_done_button_state()) {
    return;
  }
  side_blink_endturn(do_restore);
}

/**********************************************************************/ /**
   Set information for the indicator icons typically shown in the main
   client window.  The parameters tell which sprite to use for the
   indicator.
 **************************************************************************/
void set_indicator_icons(struct sprite *bulb, struct sprite *sol,
                         struct sprite *flake, struct sprite *gov)
{
  gui()->sw_indicators->update_final_pixmap();
}

/**********************************************************************/ /**
   Flush the given part of the canvas buffer (if there is one) to the
   screen.
 **************************************************************************/
static void flush_mapcanvas(int canvas_x, int canvas_y, int pixel_width,
                            int pixel_height)
{
  gui()->mapview_wdg->repaint(canvas_x, canvas_y, pixel_width, pixel_height);
}

/**********************************************************************/ /**
   Mark the rectangular region as "dirty" so that we know to flush it
   later.
 **************************************************************************/
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

/**********************************************************************/ /**
   Mark the entire screen area as "dirty" so that we can flush it later.
 **************************************************************************/
void dirty_all(void)
{
  if (mapview_is_frozen()) {
    return;
  }
  num_dirty_rects = MAX_DIRTY_RECTS;
}

/**********************************************************************/ /**
   Flush all regions that have been previously marked as dirty.  See
   dirty_rect and dirty_all.  This function is generally called after we've
   processed a batch of drawing operations.
 **************************************************************************/
void flush_dirty(void)
{
  if (mapview_is_frozen()) {
    return;
  }
  if (num_dirty_rects == MAX_DIRTY_RECTS) {
    flush_mapcanvas(0, 0, gui()->mapview_wdg->width(),
                    gui()->mapview_wdg->height());
  } else {
    int i;

    for (i = 0; i < num_dirty_rects; i++) {
      flush_mapcanvas(dirty_rects[i].x(), dirty_rects[i].y(),
                      dirty_rects[i].width(), dirty_rects[i].height());
    }
  }
  num_dirty_rects = 0;
}

/**********************************************************************/ /**
   Do any necessary synchronization to make sure the screen is up-to-date.
   The canvas should have already been flushed to screen via flush_dirty -
   all this function does is make sure the hardware has caught up.
 **************************************************************************/
void gui_flush(void) { gui()->mapview_wdg->update(); }

/**********************************************************************/ /**
   Update (refresh) the locations of the mapview scrollbars (if it uses
   them).
 **************************************************************************/
void update_map_canvas_scrollbars(void) { gui()->mapview_wdg->update(); }

/**********************************************************************/ /**
   Update the size of the sliders on the scrollbars.
 **************************************************************************/
void update_map_canvas_scrollbars_size(void)
{ /* PORTME */
}

/**********************************************************************/ /**
   Update (refresh) all city descriptions on the mapview.
 **************************************************************************/
void update_city_descriptions(void) { update_map_canvas_visible(); }

/**********************************************************************/ /**
   Put overlay tile to pixmap
 **************************************************************************/
void pixmap_put_overlay_tile(int canvas_x, int canvas_y,
                             struct sprite *ssprite)
{
  if (!ssprite) {
    return;
  }

  /* PORTME */
}

/**********************************************************************/ /**
   Draw a cross-hair overlay on a tile.
 **************************************************************************/
void put_cross_overlay_tile(struct tile *ptile)
{
  float canvas_x, canvas_y;

  if (tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
    pixmap_put_overlay_tile(canvas_x, canvas_y,
                            get_attention_crosshair_sprite(tileset));
  }
}

/**********************************************************************/ /**
   Area Selection
 **************************************************************************/
void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h)
{
  /* PORTME */
}

/**********************************************************************/ /**
   This function is called when the tileset is changed.
 **************************************************************************/
void tileset_changed(void)
{
  int i;
  science_report *sci_rep;
  QWidget *w;

  update_unit_info_label(get_units_in_focus());
  destroy_city_dialog();
  /* Update science report if open */
  if (gui()->is_repo_dlg_open("SCI")) {
    i = gui()->gimme_index_of("SCI");
    fc_assert(i != -1);
    w = gui()->game_tab_widget->widget(i);
    sci_rep = reinterpret_cast<science_report *>(w);
    sci_rep->reset_tree();
    sci_rep->update_report();
    sci_rep->repaint();
  }
}


/**********************************************************************/ /**
   Return whether the map should be drawn or not.
 **************************************************************************/
bool mapview_is_frozen(void) { return (0 < mapview_frozen_level); }

/**********************************************************************/ /**
   Freeze the drawing of the map.
 **************************************************************************/
void mapview_freeze(void) { mapview_frozen_level++; }

/**********************************************************************/ /**
   Thaw the drawing of the map.
 **************************************************************************/
void mapview_thaw(void)
{
  if (1 < mapview_frozen_level) {
    mapview_frozen_level--;
  } else {
    fc_assert(0 < mapview_frozen_level);
    mapview_frozen_level = 0;
    dirty_all();
  }
}

/**********************************************************************/ /**
   Constructor for info_tile
 **************************************************************************/
info_tile::info_tile(struct tile *ptile, QWidget *parent) : QLabel(parent)
{
  setParent(parent);
  info_font = *fc_font::instance()->get_font(fonts::notify_label);
  itile = ptile;
  calc_size();
}

/**********************************************************************/ /**
   Calculates size of info_tile and moves it to be fully visible
 **************************************************************************/
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
  str_list = str.split("\n");

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

/**********************************************************************/ /**
   Redirected paint event for info_tile
 **************************************************************************/
void info_tile::paint(QPainter *painter, QPaintEvent *event)
{
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

/**********************************************************************/ /**
   Paint event for info_tile
 **************************************************************************/
void info_tile::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**********************************************************************/ /**
   Updates fonts
 **************************************************************************/
void info_tile::update_font(const QString &name, const QFont &font)
{
  if (name == fonts::notify_label) {
    info_font = font;
    calc_size();
    update();
  }
}

/**********************************************************************/ /**
   New turn callback
 **************************************************************************/
void qtg_start_turn()
{
  show_new_turn_info();
  last_center_enemy = 0;
  last_center_capital = 0;
  last_center_player_city = 0;
  last_center_enemy_city = 0;
}
