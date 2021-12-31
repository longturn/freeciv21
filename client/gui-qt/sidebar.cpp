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
#include "sidebar.h"
#include "sprite.h"

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
sidebarWidget::sidebarWidget(QPixmap *pix, const QString &label,
                             const QString &pg, pfcn_bool func,
                             standards type)
    : QWidget(), blink(false), keep_blinking(false), disabled(false),
      standard(type), page(pg), hover(false), right_click(nullptr),
      wheel_down(nullptr), wheel_up(nullptr), left_click(func),
      def_pixmap(pix), desc(label)
{
  if (def_pixmap == nullptr) {
    def_pixmap = new QPixmap(5, 5);
  }
  scaled_pixmap = new QPixmap;
  final_pixmap = new QPixmap;
  sfont = fcFont::instance()->getFont(fonts::notify_label);
  setContextMenuPolicy(Qt::CustomContextMenu);
  timer = new QTimer;
  timer->setSingleShot(false);
  timer->setInterval(700);
  sfont.setCapitalization(QFont::SmallCaps);
  sfont.setItalic(true);
  info_font = QFont(sfont);
  info_font.setBold(true);
  info_font.setItalic(false);
  connect(timer, &QTimer::timeout, this, &sidebarWidget::sblink);
}

/**
   Sidewidget destructor
 */
sidebarWidget::~sidebarWidget()
{
  NFCN_FREE(scaled_pixmap);
  NFCN_FREE(def_pixmap);
  NFCN_FREE(final_pixmap);

  delete timer;
}

/**
   Sets default pixmap for sidewidget
 */
void sidebarWidget::setPixmap(QPixmap *pm)
{
  NFCN_FREE(def_pixmap);
  def_pixmap = pm;
}

/**
   Sets custom text visible on top of sidewidget
 */
void sidebarWidget::setCustomLabels(const QString &l) { custom_label = l; }

/**
   Sets tooltip for sidewidget
 */
void sidebarWidget::setTooltip(const QString &tooltip)
{
  setToolTip(tooltip);
}

/**
   Returns scaled (not default) pixmap for sidewidget
 */
QPixmap *sidebarWidget::get_pixmap() { return scaled_pixmap; }

/**
 * Reimplemented virtual method.
 */
int sidebarWidget::heightForWidth(int width) const
{
  switch (standard) {
  case SW_TAX:
    return get_tax_sprite(tileset, O_LUXURY)->height() + 8;
  case SW_INDICATORS:
    return get_tax_sprite(tileset, O_LUXURY)->height() + 8;
  case SW_STD:
    return (width * def_pixmap->height()) / def_pixmap->width() + 8;
  }

  fc_assert_ret_val(false, 0);
}

/**
 * Reimplemented virtual method.
 */
bool sidebarWidget::hasHeightForWidth() const { return true; }

/**
   Sets default label on bottom of sidewidget
 */
void sidebarWidget::setLabel(const QString &str) { desc = str; }

/**
   Resizes default_pixmap to scaled_pixmap to fit current width,
   leaves default_pixmap unchanged
 */
void sidebarWidget::resizePixmap(int width, int height)
{
  if (def_pixmap && scaled_pixmap) {
    *scaled_pixmap =
        def_pixmap->scaledToWidth(width, Qt::SmoothTransformation);
  }
}

/**
   Paint event for sidewidget
 */
void sidebarWidget::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Paints final pixmap on screeen
 */
void sidebarWidget::paint(QPainter *painter, QPaintEvent *event)
{
  updateFinalPixmap();
  if (final_pixmap) {
    painter->drawPixmap(event->rect(), *final_pixmap, event->rect());
  }
}

/**
   Mouse entered on widget area
 */
void sidebarWidget::enterEvent(QEvent *event)
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
void sidebarWidget::leaveEvent(QEvent *event)
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
void sidebarWidget::contextMenuEvent(QContextMenuEvent *event)
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
void sidebarWidget::setLeftClick(pfcn_bool func) { left_click = func; }

/**
   Sets callback for mouse right click
 */
void sidebarWidget::setRightClick(pfcn func) { right_click = func; }

/**
   Sets callback for mouse wheel down
 */
void sidebarWidget::setWheelDown(pfcn func) { wheel_down = func; }

/**
   Sets callback for mouse wheel up
 */
void sidebarWidget::setWheelUp(pfcn func) { wheel_up = func; }

/**
   Mouse press event for sidewidget
 */
void sidebarWidget::mousePressEvent(QMouseEvent *event)
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
void sidebarWidget::wheelEvent(QWheelEvent *event)
{
  if (event->angleDelta().y() < 0 && wheel_down) {
    wheel_down();
  } else if (event->angleDelta().y() > 0 && wheel_up) {
    wheel_up();
  }

  event->accept();
}

/**
   Blinks current sidebar widget
 */
void sidebarWidget::sblink()
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
void sidebarWidget::someSlot()
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
   Updates final pixmap and draws it on screen
 */
void sidebarWidget::updateFinalPixmap()
{
  const QPixmap *sprite;
  int w, h, pos, i;
  QPainter p;
  QPen pen;
  bool current = false;

  if (scaled_pixmap && size() == scaled_pixmap->size()) {
    return;
  }

  resizePixmap(width(), height());

  NFCN_FREE(final_pixmap);

  i = queen()->gimmeIndexOf(page);
  if (i == queen()->game_tab_widget->currentIndex()) {
    current = true;
  }
  final_pixmap = new QPixmap(size());
  final_pixmap->fill(Qt::transparent);

  if (scaled_pixmap->width() == 0 || scaled_pixmap->height() == 0) {
    return;
  }

  p.begin(final_pixmap);
  p.setFont(sfont);
  pen.setColor(QColor(232, 255, 0));
  p.setPen(pen);

  if (standard == SW_TAX && !client_is_global_observer()) {
    pos = 0;
    int d, modulo;
    sprite = get_tax_sprite(tileset, O_GOLD);
    if (sprite == nullptr) {
      return;
    }
    w = width() / 10;
    modulo = width() % 10;
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
    sprite = client_research_sprite();
    w = sprite->width();
    pos = scaled_pixmap->width() / 2 - 2 * w;
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

  } else {
    p.drawPixmap(0, (height() - scaled_pixmap->height()) / 2,
                 *scaled_pixmap);
    p.drawText(0, height() - 6, desc);
  }

  p.setPen(palette().color(QPalette::Text));
  if (!custom_label.isEmpty()) {
    p.setFont(info_font);
    p.drawText(0, 0, width(), height(), Qt::AlignLeft | Qt::TextWordWrap,
               custom_label);
  }

  if (current) {
    p.setPen(palette().color(QPalette::Highlight));
    p.drawRect(0, 0, width() - 1, height() - 1);
  }

  if (hover && !disabled) {
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    p.setPen(palette().color(QPalette::Highlight));
    p.setBrush(palette().color(QPalette::AlternateBase));
    p.drawRect(0, 0, width() - 1, height() - 1);
  }

  if (disabled) {
    p.setCompositionMode(QPainter::CompositionMode_Darken);
    p.setPen(QColor(0, 0, 0));
    p.setBrush(QColor(0, 0, 50, 95));
    p.drawRect(0, 0, width(), height());
  }

  if (blink) {
    p.setCompositionMode(QPainter::CompositionMode_ColorDodge);
    p.setPen(QColor(0, 0, 0));
    p.setBrush(palette().color(QPalette::HighlightedText));
    p.drawRect(0, 0, width(), height());
  }

  p.end();
}

/**
   Sidebar constructor
 */
sidebar::sidebar()
{
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
  setProperty("sidebar", true);
}

/**
   Sidebar destructor
 */
sidebar::~sidebar() = default;

/**
   Adds new sidebar widget
 */
void sidebar::addWidget(sidebarWidget *fsw)
{
  objects.append(fsw);
  layout->addWidget(fsw);
}

/**
 * Adds new spacer
 */
void sidebar::addSpacer() { layout->addStretch(); }

/**
   Paint event for sidebar
 */
void sidebar::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Paints dark rectangle as background for sidebar
 */
void sidebar::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QBrush(QColor(14, 14, 14)));
  painter->drawRect(event->rect());
}

void sidebar::resizeEvent(QResizeEvent *event)
{
  if (C_S_RUNNING <= client_state()) {
    resizeMe();
  }
}

/**************************************************************************
  Resize sidebar to take at least 20 pixels width and 100 pixels for FullHD
  desktop and scaled accordingly for bigger resolutions eg 200 pixels for 4k
  desktop.
**************************************************************************/
void sidebar::resizeMe()
{
  auto temp = (QGuiApplication::screens());
  auto hres = temp[0]->availableGeometry().width();

  auto w = (20 * gui_options.gui_qt_sidebar_width * hres) / 1920;
  setFixedWidth(qMax(w, 20));
}

/**
   Callback to show map
 */
void sidebarShowMap(bool nothing)
{
  Q_UNUSED(nothing)
  popdown_all_city_dialogs();
  queen()->game_tab_widget->setCurrentIndex(0);
}

/**
   Callback for finishing turn
 */
void sidebarFinishTurn(bool nothing) { key_end_turn(); }

/**
   Callback to popup rates dialog
 */
void sidebarRatesWdg(bool nothing)
{
  Q_UNUSED(nothing)
  if (!client_is_observer()) {
    popup_rates_dialog();
  }
}

/**
   Callback to center on current unit
 */
void sidebarCenterUnit()
{
  queen()->game_tab_widget->setCurrentIndex(0);
  request_center_focus_unit();
}

/**
   Disables end turn button if asked
 */
void sidebarDisableEndturn(bool do_restore)
{
  if (king()->current_page() != PAGE_GAME) {
    return;
  }
  queen()->sw_endturn->disabled = !do_restore;
  queen()->sw_endturn->update();
}

/**
   Changes background of endturn widget if asked
 */
void sidebarBlinkEndturn(bool do_restore)
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
void sidebarIndicatorsMenu()
{
  gov_menu *menu = new gov_menu(queen()->sidebar_wdg);

  menu->create();
  menu->update();
  menu->popup(QCursor::pos());
}

/**
   Right click for diplomacy
   Opens diplomacy meeting for player
   For observer popups menu
 */
void sidebarRightClickDiplomacy()
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
                       &sidebarWidget::someSlot);
      menu->addAction(eiskalt);
    }
    players_iterate_end

        if (!client_is_global_observer())
    {
      eiskalt = new QAction(_("Observe globally"), queen()->mapview_wdg);
      eiskalt->setData(-1);
      menu->addAction(eiskalt);
      QObject::connect(eiskalt, &QAction::triggered, queen()->sw_diplo,
                       &sidebarWidget::someSlot);
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
void sidebarRightClickScience()
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
                       &sidebarWidget::someSlot);
    }
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(QCursor::pos());
  }
}

/**
   Left click for science, allowing to close/open
 */
void sidebarLeftClickScience(bool nothing)
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

// Reloads all icons and resize sidebar width to new value
void gui_update_sidebar() { queen()->reloadSidebarIcons(); }
