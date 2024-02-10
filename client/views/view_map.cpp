/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

/*
 * This file contains functions to generate the GUI for the
 * the main map view.
 */

#include <memory>

// Qt
#include <QCommandLinkButton>
#include <QMouseEvent>
#include <QPainter>

// utility
#include "log.h"
// client
#include "citybar.h"
#include "citydlg_g.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors_common.h"
#include "fc_client.h"
#include "fonts.h"
#include "hudwidget.h"
#include "mapctrl_common.h"
#include "mapview_g.h"
#include "messagewin.h"
#include "minimap_panel.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "renderer.h"
#include "text.h"
#include "tileset/sprite.h"
#include "tileset/tilespec.h"
#include "top_bar.h"
#include "views/view_map.h"
#include "views/view_map_common.h"
#include "views/view_research.h"
#include "widgets/decorations.h"

static int mapview_frozen_level = 0;
extern void destroy_city_dialog();
extern QPixmap *canvas;
static QRegion dirty;

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
  QPen pen;

  if (!can_client_control() || king()->trade_gen.cities.empty()) {
    return;
  }
  auto color = get_color(tileset, COLOR_MAPVIEW_TRADE_ROUTES_NO_BUILT);
  // Draw calculated trade routes
  if (gui_options->draw_city_trade_routes) {
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
        color = get_color(tileset, COLOR_MAPVIEW_TRADE_ROUTES_SOME_BUILT);
      }

      pen.setColor(color);
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
map_view::map_view()
    : QWidget(), m_renderer(new freeciv::renderer(this)),
      m_origin_animation(
          std::make_unique<QPropertyAnimation>(m_renderer, "origin")),
      m_scale_animation(std::make_unique<QPropertyAnimation>(this, "scale"))
{
  connect(m_renderer, &freeciv::renderer::repaint_needed, this,
          qOverload<const QRegion &>(&map_view::update));

  cursor = -1;
  QTimer *timer = new QTimer(this);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  connect(timer, &QTimer::timeout, this, &map_view::timer_event);
  timer->start(200);
  resize(0, 0);
  setMouseTracking(true);
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
  for (auto &widget : m_hidden_fcwidgets) {
    if (widget) {
      widget->show();
    }
  }
  m_hidden_fcwidgets.clear();
}

/**
 * Centers the view on a tile.
 */
void map_view::center_on_tile(tile *tile, bool animate)
{
  int tile_x, tile_y;
  index_to_map_pos(&tile_x, &tile_y, tile_index(tile));

  float gui_x, gui_y;
  map_to_gui_pos(tileset, &gui_x, &gui_y, tile_x, tile_y);

  // Put the center pixel of the tile at the exact center of the mapview.
  gui_x -= (mapview.width - tileset_tile_width(tileset)) / 2;
  gui_y -= (mapview.height - tileset_tile_height(tileset)) / 2;

  m_origin_animation->stop();
  if (animate) {
    m_origin_animation->setDuration(gui_options->smooth_center_slide_msec);
    m_origin_animation->setCurrentTime(0);

    const auto start = QPointF(mapview.gui_x0, mapview.gui_y0);
    m_origin_animation->setStartValue(start);

    // To wrap correctly, we first find the direction in which the animation
    // should go and then choose the end point for Qt's linear interpolation.
    float diff_x, diff_y;
    gui_distance_vector(tileset, &diff_x, &diff_y, mapview.gui_x0,
                        mapview.gui_y0, gui_x, gui_y);
    m_origin_animation->setEndValue(start + QPointF(diff_x, diff_y));

    m_origin_animation->start();
  } else {
    m_renderer->set_origin(QPointF(gui_x, gui_y));
  }
}

/**
 * Zooms in by 20%.
 */
void map_view::zoom_in() { set_scale(1.2 * scale()); }

/**
 * Resets the zoom level.
 */
void map_view::zoom_reset() { set_scale(1); }

/**
 * Zooms out by 20%.
 */
void map_view::zoom_out() { set_scale(scale() / 1.2); }

/**
 * Retrieves the current scale (zoom level) of the map.
 */
double map_view::scale() const { return m_renderer->scale(); }

/**
 * Sets the map scale.
 */
void map_view::set_scale(double scale)
{
  m_scale_animation->stop();
  m_scale_animation->setDuration(gui_options->smooth_center_slide_msec);
  m_scale_animation->setEndValue(scale);
  m_scale_animation->setCurrentTime(0);
  m_scale_animation->start();
}

/**
 * Sets the map scale immediately without doing any animation.
 */
void map_view::set_scale_now(double scale)
{
  m_renderer->set_scale(scale);
  emit scale_changed(scale);
}

/**
 * Opens the tileset debugger.
 */
void map_view::show_debugger()
{
  if (!m_debugger) {
    // We never destroy it once it's created.
    m_debugger = new freeciv::tileset_debugger(this);
    connect(m_debugger, &freeciv::tileset_debugger::tile_picking_requested,
            [](bool active) {
              if (active) {
                set_hover_state({}, HOVER_DEBUG_TILE, ACTIVITY_LAST, nullptr,
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
  if (queen()->minimap_panel->underMouse()
      || queen()->top_bar_wdg->underMouse()) {
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
  m_renderer->render(painter, event->region());
  painter.scale(1 / scale(), 1 / scale());
  draw_calculated_trade_routes(&painter);
  painter.end();
}

/**
 * The widget has been resized.
 */
void map_view::resizeEvent(QResizeEvent *event)
{
  m_renderer->set_viewport_size(event->size());
  event->accept();
}

/**
 * The user wants to scroll.
 */
void map_view::wheelEvent(QWheelEvent *event)
{
  auto delta = event->pixelDelta();
  if (delta.isNull()) {
    delta = event->angleDelta();
  }

  if (event->modifiers() == Qt::NoModifier) {
    // Scrolling
    m_renderer->set_origin(m_renderer->origin() - delta);
  } else if (event->modifiers() == Qt::ShiftModifier) {
    // Horizontal scrolling
    std::swap(delta.rx(), delta.ry()); // FIXME Qt 6 QPoint::transposed()
    m_renderer->set_origin(m_renderer->origin() - delta);
  } else if (event->modifiers() == Qt::ControlModifier) {
    // Zooming
    if (delta.y() > 0) {
      zoom_in();
    } else if (delta.y() < 0) {
      zoom_out();
    }
  }
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
void update_unit_info_label(const std::vector<unit *> &unit_list)
{
  if (queen()->unitinfo_wdg->isVisible()) {
    queen()->unitinfo_wdg->update_actions();
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
  // queen()->minimap_panel->turn_done()->blink();
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
  auto scale = queen()->mapview_wdg->scale();
  dirty |= QRect(canvas_x * scale, canvas_y * scale, pixel_width * scale,
                 pixel_height * scale);
}

/**
   Mark the entire screen area as "dirty" so that we can flush it later.
 */
void dirty_all(void)
{
  if (mapview_is_frozen()) {
    return;
  }
  dirty |= QRect(QPoint(), queen()->mapview_wdg->size());
}

/**
   Flush all regions that have been previously marked as dirty.  See
   dirty_rect and dirty_all.  This function is generally called after we've
   processed a batch of drawing operations.
 */
void flush_dirty()
{
  if (mapview_is_frozen()) {
    return;
  }

  queen()->mapview_wdg->repaint(dirty);
  dirty = QRegion();
}

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
    debugger->refresh(tileset);
  }

  update_unit_info_label(get_units_in_focus());
  popdown_city_dialog();
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
   Constructor for info_tile
 */
info_tile::info_tile(struct tile *ptile, QWidget *parent)
    : QLabel(parent), itile(ptile)
{
  setFont(fcFont::instance()->getFont(fonts::notify_label));
  setText(popup_info_text(itile).trimmed());
  setWordWrap(true);

  calc_size();
}

/**
   Calculates size of info_tile and moves it to be fully visible
 */
void info_tile::calc_size()
{
  auto size = sizeHint();

  // Prevent it from getting too wide
  QFontMetrics fm = fontMetrics();
  const auto max_width = fm.averageCharWidth() * 70; // About 700px
  if (size.width() > max_width) {
    size.setWidth(max_width + 10); // Safety margin
    size.setHeight(heightForWidth(size.width()) + 10);
  }

  setFixedSize(size);

  float x, y;
  if (tile_to_canvas_pos(&x, &y, itile)) {
    x *= queen()->mapview_wdg->scale();
    y *= queen()->mapview_wdg->scale();
    if (y - height() > 0) {
      y -= height();
    } else {
      y += tileset_tile_height(tileset) * queen()->mapview_wdg->scale();
    }
    // Make sure it's visible
    x = std::clamp(int(x), 0, parentWidget()->width() - width());
    move(x, y);
  }
}

/**
   Deletes current instance
 */
void info_tile::drop()
{
  delete m_instance;
  m_instance = nullptr;
}

/**
 * Check if the info tile is currently shown.
 */
bool info_tile::shown() { return m_instance && m_instance->isVisible(); }

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
void start_turn()
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
