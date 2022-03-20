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
#include "sciencedlg.h"
#include "sprite.h"
#include "top_bar.h"

#include <cmath>

extern void pixmap_copy(QPixmap *dest, QPixmap *src, int src_x, int src_y,
                        int dest_x, int dest_y, int width, int height);
static void reduce_mod(int &val, int &mod);

/**
   Helper function to fit tax sprites, reduces modulo, increasing value
 */
void reduce_mod(int &mod, int &val)
{
  if (mod > 0) {
    val++;
    mod--;
  }
}

/**
   Sidewidget constructor
 */
top_bar_widget::top_bar_widget(const QString &label, const QString &pg,
                               pfcn_bool func, standards type)
    : QToolButton(), blink(false), keep_blinking(false), standard(type),
      page(pg), hover(false), right_click(nullptr), wheel_down(nullptr),
      wheel_up(nullptr), left_click(func)
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
top_bar_widget::~top_bar_widget()
{
  delete timer;
}

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
   Paint event for sidewidget
 */
void top_bar_widget::paintEvent(QPaintEvent *event)
{
  int w, h, pos, i;
  QPainter p;
  QPen pen;
  bool current = false;

  i = queen()->gimmeIndexOf(page);
  if (i == queen()->game_tab_widget->currentIndex()) {
    current = true;
  }

  p.begin(this);
  pen.setColor(QColor(232, 255, 0));
  p.setPen(pen);

  if (standard == SW_TAX && !client_is_global_observer()) {
    pos = 0;
    int d, modulo;
    auto sprite = get_tax_sprite(tileset, O_GOLD);
    if (sprite == nullptr) {
      return;
    }
    w = width() / 10.;
    modulo = std::fmod(qreal(width()), 10);
    h = sprite->height();
    reduce_mod(modulo, pos);
    if (client.conn.playing == nullptr) {
      return;
    }
    for (d = 0; d < client.conn.playing->economic.tax / 10; ++d) {
      p.drawPixmap(pos, 5, sprite->scaled(w, h), 0, 0, w, h);
      pos = pos + w;
      reduce_mod(modulo, pos);
    }

    sprite = get_tax_sprite(tileset, O_SCIENCE);

    for (; d < (client.conn.playing->economic.tax
                + client.conn.playing->economic.science)
                   / 10;
         ++d) {
      p.drawPixmap(pos, 5, sprite->scaled(w, h), 0, 0, w, h);
      pos = pos + w;
      reduce_mod(modulo, pos);
    }

    sprite = get_tax_sprite(tileset, O_LUXURY);

    for (; d < 10; ++d) {
      p.drawPixmap(pos, 5, sprite->scaled(w, h), 0, 0, w, h);
      pos = pos + w;
      reduce_mod(modulo, pos);
    }
  } else if (standard == SW_INDICATORS) {
    auto sprite = client_research_sprite();
    w = sprite->width() / sprite->devicePixelRatioF();
    pos = width() / 2 - 2 * w;
    p.drawPixmap(pos, 5, *sprite);
    pos = pos + w;
    sprite = client_warming_sprite();
    p.drawPixmap(pos, 5, *sprite);
    pos = pos + w;
    sprite = client_cooling_sprite();
    p.drawPixmap(pos, 5, *sprite);
    pos = pos + w;
    sprite = client_government_sprite();
    p.drawPixmap(pos, 5, *sprite);
  }

  // Remove 1px for the border on the right and at the bottom
  const auto highlight_rect =
      QRectF(0.5, 0, width() - 1. / devicePixelRatio(),
             height() - 1. / devicePixelRatio());

  if (current) {
    p.setPen(QPen(palette().color(QPalette::Highlight), 0));
    p.drawRect(highlight_rect);
  }

  if (blink) {
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    p.setPen(QPen(Qt::black, 0));
    p.setBrush(palette().color(QPalette::HighlightedText));
    p.drawRect(highlight_rect);
  }

  p.end();

  QToolButton::paintEvent(event);
}

/**
   Mouse entered on widget area
 */
void top_bar_widget::enterEvent(QEvent *event)
{
  if (!hover) {
    hover = true;
    QWidget::enterEvent(event);
    update();
  }
}

/**
   Mouse left widget area
 */
void top_bar_widget::leaveEvent(QEvent *event)
{
  if (hover) {
    hover = false;
    QWidget::leaveEvent(event);
    update();
  }
}

/**
   Context menu requested
 */
void top_bar_widget::contextMenuEvent(QContextMenuEvent *event)
{
  if (hover) {
    hover = false;
    QWidget::contextMenuEvent(event);
    update();
  }
}

/**
   Sets callback for mouse left click
 */
void top_bar_widget::setLeftClick(pfcn_bool func) { left_click = func; }

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
    left_click(true);
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
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  layout = new QHBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);
  setProperty("top_bar", true);
}

/**
   Sidebar destructor
 */
top_bar::~top_bar() = default;

/**
   Adds new top_bar widget
 */
void top_bar::addWidget(top_bar_widget *fsw)
{
  objects.append(fsw);
  layout->addWidget(fsw);
}

/**
   Paint event for top_bar
 */
void top_bar::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Paints dark rectangle as background for top_bar
 */
void top_bar::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QBrush(QColor(14, 14, 14)));
  painter->drawRect(event->rect());
}

/**
   Callback to show map
 */
void top_bar_show_map(bool nothing)
{
  Q_UNUSED(nothing)
  popdown_all_city_dialogs();
  queen()->game_tab_widget->setCurrentIndex(0);
}

/**
   Callback for finishing turn
 */
void top_bar_finish_turn(bool nothing) { key_end_turn(); }

/**
   Callback to popup rates dialog
 */
void top_bar_rates_wdg(bool nothing)
{
  Q_UNUSED(nothing)
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
void top_bar_left_click_science(bool nothing)
{
  Q_UNUSED(nothing)
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
