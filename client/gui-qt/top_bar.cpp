/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QStyle>
#include <QStyleOptionToolButton>
#include <QTimer>

// common
#include "chatline_common.h"
#include "government.h"
#include "nation.h"
#include "research.h"
// client
#include "citydlg_g.h"
#include "client_main.h"
#include "climisc.h"
#include "ratesdlg_g.h"
// gui-qt
#include "fc_client.h"
#include "fonts.h"
#include "mapview.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "sciencedlg.h"
#include "sprite.h"
#include "top_bar.h"

#include <cmath>

/**
 * Constructor
 */
tax_rates_widget::tax_rates_widget()
{
  setToolButtonStyle(Qt::ToolButtonIconOnly);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

/**
 * Destructor
 */
tax_rates_widget::~tax_rates_widget() {}

/**
 * Size hint
 */
QSize tax_rates_widget::sizeHint() const
{
  if (client_is_global_observer()) {
    // Nothing to show
    return QSize();
  }

  // Assume that all icons have the same size
  auto content_size = get_tax_sprite(tileset, O_GOLD)->size();
  content_size.setWidth(10 * content_size.width());

  // See QToolButton::sizeHint
  ensurePolished();

  QStyleOptionToolButton opt;
  initStyleOption(&opt);

  return style()
      ->sizeFromContents(QStyle::CT_ToolButton, &opt, content_size, this)
      .expandedTo(QApplication::globalStrut());
}

/**
 * Renders the tax rates widget
 */
void tax_rates_widget::paintEvent(QPaintEvent *event)
{
  if (client_is_global_observer()) {
    // Nothing to show
    return;
  }

  // Draw a button without contents
  QToolButton::paintEvent(event);

  // Draw the tax icons on top (centered; the style might expect something
  // else but screw it)
  auto tax = get_tax_sprite(tileset, O_GOLD);
  auto sci = get_tax_sprite(tileset, O_SCIENCE);
  auto lux = get_tax_sprite(tileset, O_LUXURY);

  // Assume that they have the same size
  auto icon_size = tax->size();
  auto center = size() / 2;

  auto x = center.width() - 5 * icon_size.width();
  auto y = center.height() - icon_size.height() / 2;

  QPainter p(this);
  for (int i = 0; i < 10; ++i) {
    if (i < client.conn.playing->economic.tax / 10) {
      p.drawPixmap(QPointF(x, y), *tax);
    } else if (i < (client.conn.playing->economic.tax
                    + client.conn.playing->economic.science)
                       / 10) {
      p.drawPixmap(QPointF(x, y), *sci);
    } else {
      p.drawPixmap(QPointF(x, y), *lux);
    }

    x += icon_size.width();
  }
}

/**
 * Constructor
 */
indicators_widget::indicators_widget()
{
  setToolButtonStyle(Qt::ToolButtonIconOnly);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

/**
 * Destructor
 */
indicators_widget::~indicators_widget() {}

/**
 * Size hint
 */
QSize indicators_widget::sizeHint() const
{
  // Assume that all icons have the same size
  auto content_size = client_research_sprite()->size();
  if (client_is_global_observer()) {
    // Global observers can only see climate change
    content_size.setWidth(2 * content_size.width());
  } else {
    content_size.setWidth(4 * content_size.width());
  }

  // See QToolButton::sizeHint
  ensurePolished();

  QStyleOptionToolButton opt;
  initStyleOption(&opt);

  return style()
      ->sizeFromContents(QStyle::CT_ToolButton, &opt, content_size, this)
      .expandedTo(QApplication::globalStrut());
}

/**
 * Renders the indicators widget
 */
void indicators_widget::paintEvent(QPaintEvent *event)
{
  // Draw a button without contents
  QToolButton::paintEvent(event);

  // Draw the icons on top (centered; the style might expect something else
  // but screw it)
  // Assume that they have the same size
  auto icon_size = client_warming_sprite()->size();
  auto center = size() / 2;

  auto x = center.width()
           - (client_is_global_observer() ? 1 : 2) * icon_size.width();
  auto y = center.height() - icon_size.height() / 2;

  QPainter p(this);
  p.drawPixmap(QPointF(x, y), *client_warming_sprite());
  x += icon_size.width();
  p.drawPixmap(QPointF(x, y), *client_cooling_sprite());

  if (!client_is_global_observer()) {
    x += icon_size.width();
    p.drawPixmap(QPointF(x, y), *client_research_sprite());
    x += icon_size.width();
    p.drawPixmap(QPointF(x, y), *client_government_sprite());
  }
}

/**
   Sidewidget constructor
 */
top_bar_widget::top_bar_widget(const QString &label, const QString &pg,
                               pfcn func)
    : QToolButton(), blink(false), keep_blinking(false), page(pg),
      right_click(nullptr), wheel_down(nullptr), wheel_up(nullptr),
      left_click(func)
{
  setText(label);
  setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setContextMenuPolicy(Qt::CustomContextMenu);

  timer = new QTimer;
  timer->setSingleShot(false);
  timer->setInterval(700);
  connect(timer, &QTimer::timeout, this, &top_bar_widget::sblink);
}

/**
   Sidewidget destructor
 */
top_bar_widget::~top_bar_widget() { delete timer; }

/**
   Sets custom text visible on top of sidewidget
 */
void top_bar_widget::setCustomLabels(const QString &l) { setText(l); }

/**
   Sets tooltip for sidewidget
 */
void top_bar_widget::setTooltip(const QString &tooltip)
{
  setToolTip(tooltip);
}

/**
 * Paint event for top bar widget
 */
void top_bar_widget::paintEvent(QPaintEvent *event)
{
  // HACK Should improve this logic, paintEvent is NOT the right place.
  int i = queen()->gimmeIndexOf(page);
  setChecked(i == queen()->game_tab_widget->currentIndex());

  QToolButton::paintEvent(event);

  if (blink) {
    QPainter p;
    p.begin(this);
    p.setPen(Qt::NoPen);
    p.setCompositionMode(QPainter::CompositionMode_SoftLight);
    p.setBrush(palette().color(QPalette::HighlightedText));
    p.drawRect(0, 0, width(), height());
    p.end();
  }
}

/**
   Sets callback for mouse left click
 */
void top_bar_widget::setLeftClick(pfcn func) { left_click = func; }

/**
   Sets callback for mouse right click
 */
void top_bar_widget::setRightClick(pfcn func) { right_click = func; }

/**
   Sets callback for mouse wheel down
 */
void top_bar_widget::setWheelDown(pfcn func) { wheel_down = func; }

/**
   Sets callback for mouse wheel up
 */
void top_bar_widget::setWheelUp(pfcn func) { wheel_up = func; }

/**
   Mouse press event for sidewidget
 */
void top_bar_widget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && left_click != nullptr) {
    left_click();
  }
  if (event->button() == Qt::RightButton && right_click != nullptr) {
    right_click();
  }
  if (event->button() == Qt::RightButton && right_click == nullptr) {
    queen()->game_tab_widget->setCurrentIndex(0);
  }
}

/**
   Mouse wheel event
 */
void top_bar_widget::wheelEvent(QWheelEvent *event)
{
  if (event->angleDelta().y() < 0 && wheel_down) {
    wheel_down();
  } else if (event->angleDelta().y() > 0 && wheel_up) {
    wheel_up();
  }

  event->accept();
}

/**
   Blinks current top_bar widget
 */
void top_bar_widget::sblink()
{
  if (keep_blinking) {
    if (!timer->isActive()) {
      timer->start();
    }
    blink = !blink;
  } else {
    blink = false;
    if (timer->isActive()) {
      timer->stop();
    }
  }
  update();
}

/**
   Miscelanous slot, helping observe players currently, and changing science
   extra functionality might be added,
   eg by setting properties
 */
void top_bar_widget::someSlot()
{
  QVariant qvar;
  struct player *obs_player;
  QAction *act;

  act = qobject_cast<QAction *>(sender());
  qvar = act->data();

  if (!qvar.isValid()) {
    return;
  }

  if (act->property("scimenu").toBool()) {
    dsend_packet_player_research(&client.conn, qvar.toInt());
    return;
  }

  if (qvar.toInt() == -1) {
    send_chat("/observe");
    return;
  }

  obs_player = reinterpret_cast<struct player *>(qvar.value<void *>());
  if (obs_player != nullptr) {
    QString s;
    QByteArray cn_bytes;

    s = QStringLiteral("/observe \"%1\"").arg(obs_player->name);
    cn_bytes = s.toLocal8Bit();
    send_chat(cn_bytes.data());
  }
}

/**
   Sidebar constructor
 */
top_bar::top_bar()
{
  layout = new QHBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);
  setProperty("top_bar", true);
  setAutoFillBackground(true);
}

/**
   Sidebar destructor
 */
top_bar::~top_bar() = default;

/**
   Adds new top_bar widget
 */
void top_bar::addWidget(QWidget *fsw)
{
  objects.append(fsw);
  layout->addWidget(fsw);
}

/**
   Callback to show map
 */
void top_bar_show_map()
{
  qtg_popdown_all_city_dialogs();
  queen()->game_tab_widget->setCurrentIndex(0);
}

/**
   Callback for finishing turn
 */
void top_bar_finish_turn() { key_end_turn(); }

/**
   Callback to popup rates dialog
 */
void top_bar_rates_wdg()
{
  if (!client_is_observer()) {
    popup_rates_dialog();
  }
}

/**
   Callback to center on current unit
 */
void top_bar_center_unit()
{
  queen()->game_tab_widget->setCurrentIndex(0);
  request_center_focus_unit();
}

/**
   Disables end turn button if asked
 */
void top_bar_disable_end_turn(bool do_restore)
{
  if (king()->current_page() != PAGE_GAME) {
    return;
  }
  queen()->sw_endturn->setEnabled(do_restore);
}

/**
   Changes background of endturn widget if asked
 */
void top_bar_blink_end_turn(bool do_restore)
{
  if (king()->current_page() != PAGE_GAME) {
    return;
  }
  queen()->sw_endturn->blink = !do_restore;
  queen()->sw_endturn->update();
}

/**
   Popups menu on indicators widget
 */
void top_bar_indicators_menu()
{
  gov_menu *menu = new gov_menu(queen()->top_bar_wdg);

  menu->create();
  menu->update();
  menu->popup(QCursor::pos());
}

/**
   Right click for diplomacy
   Opens diplomacy meeting for player
   For observer popups menu
 */
void top_bar_right_click_diplomacy()
{
  if (client_is_observer()) {
    QMenu *menu = new QMenu(king()->central_wdg);
    QAction *eiskalt;
    QString erwischt;

    players_iterate(pplayer)
    {
      if (pplayer == client.conn.playing) {
        continue;
      }
      erwischt = QString(_("Observe %1")).arg(pplayer->name);
      erwischt =
          erwischt + " (" + nation_plural_translation(pplayer->nation) + ")";
      eiskalt = new QAction(erwischt, queen()->mapview_wdg);
      eiskalt->setData(QVariant::fromValue((void *) pplayer));
      QObject::connect(eiskalt, &QAction::triggered, queen()->sw_diplo,
                       &top_bar_widget::someSlot);
      menu->addAction(eiskalt);
    }
    players_iterate_end

        if (!client_is_global_observer())
    {
      eiskalt = new QAction(_("Observe globally"), queen()->mapview_wdg);
      eiskalt->setData(-1);
      menu->addAction(eiskalt);
      QObject::connect(eiskalt, &QAction::triggered, queen()->sw_diplo,
                       &top_bar_widget::someSlot);
    }

    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(QCursor::pos());
  } else {
    int i;
    i = queen()->gimmeIndexOf(QStringLiteral("DDI"));
    if (i < 0) {
      return;
    }
    queen()->game_tab_widget->setCurrentIndex(i);
  }
}

/**
   Right click for science, allowing to choose current tech
 */
void top_bar_right_click_science()
{
  QMenu *menu;
  QAction *act;
  QVariant qvar;
  QVector<qlist_item> curr_list;
  qlist_item item;

  if (!client_is_observer()) {
    struct research *research = research_get(client_player());

    advance_index_iterate(A_FIRST, i)
    {
      if (TECH_PREREQS_KNOWN == research->inventions[i].state
          && research->researching != i) {
        item.tech_str = QString::fromUtf8(
            advance_name_translation(advance_by_number(i)));
        item.id = i;
        curr_list.append(item);
      }
    }
    advance_index_iterate_end;
    if (curr_list.isEmpty()) {
      return;
    }
    std::sort(curr_list.begin(), curr_list.end(), comp_less_than);
    menu = new QMenu(king()->central_wdg);
    for (int i = 0; i < curr_list.count(); i++) {
      QIcon ic;

      qvar = curr_list.at(i).id;
      auto sp = get_tech_sprite(tileset, curr_list.at(i).id);
      if (sp) {
        ic = QIcon(*sp);
      }
      act = new QAction(ic, curr_list.at(i).tech_str, queen()->mapview_wdg);
      act->setData(qvar);
      act->setProperty("scimenu", true);
      menu->addAction(act);
      QObject::connect(act, &QAction::triggered, queen()->sw_science,
                       &top_bar_widget::someSlot);
    }
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(QCursor::pos());
  }
}

/**
   Left click for science, allowing to close/open
 */
void top_bar_left_click_science()
{
  science_report *sci_rep;
  int i;
  QWidget *w;

  if (client_is_global_observer()) {
    return;
  }
  if (!queen()->isRepoDlgOpen(QStringLiteral("SCI"))) {
    sci_rep = new science_report;
    sci_rep->init(true);
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("SCI"));
    w = queen()->game_tab_widget->widget(i);
    if (w->isVisible()) {
      queen()->game_tab_widget->setCurrentIndex(0);
      return;
    }
    sci_rep = reinterpret_cast<science_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(sci_rep);
  }
}

// Reloads all icons and resize top_bar width to new value
void gui_update_top_bar() { queen()->reloadSidebarIcons(); }
