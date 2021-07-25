/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "hudwidget.h"
// Qt
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QRadioButton>
#include <QVBoxLayout>
// common
#include "movement.h"
#include "nation.h"
#include "research.h"
#include "tile.h"
#include "unit.h"
#include "unitlist.h"
// client
#include "calendar.h"
#include "client_main.h"
#include "goto.h"
#include "mapview_common.h"
#include "text.h"
// gui-qt
#include "canvas.h"
#include "fc_client.h"
#include "fonts.h"
#include "icons.h"
#include "mapview.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "sprite.h"
#include "widgetdecorations.h"

static QString popup_terrain_info(struct tile *ptile);

/**
   Returns true if player has any unit of unit_type
 */
bool has_player_unit_type(Unit_type_id utype)
{
  unit_list_iterate(client.conn.playing->units, punit)
  {
    if (utype_number(punit->utype) == utype) {
      return true;
    }
  }
  unit_list_iterate_end;

  return false;
}

/**
   Custom message box constructor
 */
hud_message_box::hud_message_box(QWidget *parent) : QMessageBox(parent)
{
  int size;
  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog
                 | Qt::FramelessWindowHint);
  f_text = *fcFont::instance()->getFont(fonts::default_font);
  f_title = *fcFont::instance()->getFont(fonts::default_font);

  size = f_text.pointSize();
  if (size > 0) {
    f_text.setPointSize(size * 4 / 3);
    f_title.setPointSize(size * 3 / 2);
  } else {
    size = f_text.pixelSize();
    f_text.setPixelSize(size * 4 / 3);
    f_title.setPointSize(size * 3 / 2);
  }
  f_title.setBold(true);
  f_title.setCapitalization(QFont::SmallCaps);
  fm_text = new QFontMetrics(f_text);
  fm_title = new QFontMetrics(f_title);
  top = 0;
  m_animate_step = 0;
  hide();
  mult = 1;
}

/**
   Custom message box destructor
 */
hud_message_box::~hud_message_box()
{
  delete fm_text;
  delete fm_title;
}

/**
   Key press event for hud message box
 */
void hud_message_box::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape) {
    close();
    destroy();
    event->accept();
  }
  QWidget::keyPressEvent(event);
}

/**
   Sets text and title and shows message box
 */
void hud_message_box::set_text_title(const QString &s1, const QString &s2)
{
  QSpacerItem *spacer;
  QGridLayout *layout;
  int w, w2, h;
  QPoint p;

  if (s1.contains('\n')) {
    int i;
    i = s1.indexOf('\n');
    cs1 = s1.left(i);
    cs2 = s1.right(s1.count() - i);
    mult = 2;
    w2 = qMax(fm_text->horizontalAdvance(cs1),
              fm_text->horizontalAdvance(cs2));
    w = qMax(w2, fm_title->horizontalAdvance(s2));
  } else {
    w = qMax(fm_text->horizontalAdvance(s1),
             fm_title->horizontalAdvance(s2));
  }
  w = w + 20;
  h = mult * (fm_text->height() * 3 / 2) + 2 * fm_title->height();
  top = 2 * fm_title->height();
  spacer =
      new QSpacerItem(w, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  layout = (QGridLayout *) this->layout();
  layout->addItem(spacer, layout->rowCount(), 0, 1, layout->columnCount());
  spacer =
      new QSpacerItem(0, h, QSizePolicy::Expanding, QSizePolicy::Minimum);
  layout->addItem(spacer, 0, 0, 1, layout->columnCount());

  text = s1;
  title = s2;

  p = QPoint((parentWidget()->width() - w) / 2,
             (parentWidget()->height() - h) / 2);
  p = parentWidget()->mapToGlobal(p);
  move(p);
  show();
  m_timer.start();
  startTimer(45);
}

/**
   Timer event used to animate message box
 */
void hud_message_box::timerEvent(QTimerEvent *event)
{
  m_animate_step = m_timer.elapsed() / 40;
  update();
}

/**
   Paint event for custom message box
 */
void hud_message_box::paintEvent(QPaintEvent *event)
{
  QPainter p;
  QRect rx, ry, rfull;
  QLinearGradient g;
  QColor c1;
  QColor c2;
  int step;

  step = m_animate_step % 300;
  if (step > 150) {
    step = step - 150;
    step = 150 - step;
  }
  step = step + 30;

  rfull = QRect(2, 2, width() - 4, height() - 4);
  rx = QRect(2, 2, width() - 4, top);
  ry = QRect(2, top, width() - 4, height() - top - 4);

  c1 = QColor(palette().color(QPalette::Highlight));
  c2 = QColor(palette().color(QPalette::AlternateBase));
  step = qMax(0, step);
  step = qMin(255, step);
  c1.setAlpha(step);
  c2.setAlpha(step);

  g = QLinearGradient(0, 0, width(), height());
  g.setColorAt(0, c1);
  g.setColorAt(1, c2);

  p.begin(this);
  p.fillRect(rx, QColor(palette().color(QPalette::Highlight)));
  p.fillRect(ry, QColor(palette().color(QPalette::AlternateBase)));
  p.fillRect(rfull, g);
  p.setFont(f_title);
  p.drawText((width() - fm_title->horizontalAdvance(title)) / 2,
             fm_title->height() * 4 / 3, title);
  p.setFont(f_text);
  if (mult == 1) {
    p.drawText((width() - fm_text->horizontalAdvance(text)) / 2,
               2 * fm_title->height() + fm_text->height() * 4 / 3, text);
  } else {
    p.drawText((width() - fm_text->horizontalAdvance(cs1)) / 2,
               2 * fm_title->height() + fm_text->height() * 4 / 3, cs1);
    p.drawText((width() - fm_text->horizontalAdvance(cs2)) / 2,
               2 * fm_title->height() + fm_text->height() * 8 / 3, cs2);
  }
  p.end();
  event->accept();
}

/**
   Hud text constructor takes text to display and time
 */
hud_text::hud_text(const QString &s, int time_secs, QWidget *parent)
    : QWidget(parent), text(s)
{
  int size;

  timeout = time_secs;

  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
  f_text = *fcFont::instance()->getFont(fonts::default_font);
  f_text.setBold(true);
  f_text.setCapitalization(QFont::SmallCaps);
  size = f_text.pointSize();
  if (size > 0) {
    f_text.setPointSize(size * 2);
  } else {
    size = f_text.pixelSize();
    f_text.setPixelSize(size * 2);
  }
  fm_text = new QFontMetrics(f_text);
  m_animate_step = 0;
  m_timer.start();
  startTimer(46);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setFocusPolicy(Qt::NoFocus);
}

/**
   Shows hud text
 */
void hud_text::show_me()
{
  show();
  center_me();
}

/**
   Moves to top center parent widget and sets size new size
 */
void hud_text::center_me()
{
  int w;
  QPoint p;

  w = width();
  if (!bound_rect.isEmpty()) {
    setFixedSize(bound_rect.width(), bound_rect.height());
  }
  p = QPoint((parentWidget()->width() - w) / 2,
             parentWidget()->height() / 20);
  move(p);
}

/**
   Destructor for hud text
 */
hud_text::~hud_text() { delete fm_text; }

/**
   Timer event, closes widget after timeout
 */
void hud_text::timerEvent(QTimerEvent *event)
{
  m_animate_step = m_timer.elapsed() / 40;
  if (m_timer.elapsed() > timeout * 1000) {
    close();
    deleteLater();
  }
  update();
}

/**
   Paint event for custom hud_text
 */
void hud_text::paintEvent(QPaintEvent *event)
{
  QPainter p;
  QRect rfull;
  QColor c1;
  QColor c2;
  float opacity;

  center_me();
  if (m_timer.elapsed() < timeout * 500) {
    opacity = static_cast<float>(m_timer.elapsed()) / (timeout * 300);
  } else {
    opacity = static_cast<float>(5000 - m_timer.elapsed()) / (timeout * 200);
  }
  opacity = qMin(1.0f, opacity);
  opacity = qMax(0.0f, opacity);
  rfull = QRect(0, 0, width(), height());
  c1 = QColor(Qt::white);
  c2 = QColor(35, 35, 35, 175);
  c1.setAlphaF(c1.alphaF() * opacity);
  c2.setAlphaF(c2.alphaF() * opacity);
  p.begin(this);
  p.setBrush(c2);
  p.setPen(QColor(0, 0, 0, 0));
  p.drawRoundedRect(rfull, height() / 6, height() / 6);
  p.setFont(f_text);
  p.setPen(c1);
  p.drawText(rfull, Qt::AlignCenter, text, &bound_rect);

  p.end();
}

/**
   Custom input box constructor
 */
hud_input_box::hud_input_box(QWidget *parent) : QDialog(parent)
{
  int size;

  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog
                 | Qt::FramelessWindowHint);

  f_text = *fcFont::instance()->getFont(fonts::default_font);
  f_title = *fcFont::instance()->getFont(fonts::default_font);

  size = f_text.pointSize();
  if (size > 0) {
    f_text.setPointSize(size * 4 / 3);
    f_title.setPointSize(size * 3 / 2);
  } else {
    size = f_text.pixelSize();
    f_text.setPixelSize(size * 4 / 3);
    f_title.setPointSize(size * 3 / 2);
  }
  f_title.setBold(true);
  f_title.setCapitalization(QFont::SmallCaps);
  fm_text = new QFontMetrics(f_text);
  fm_title = new QFontMetrics(f_title);
  top = 0;
  m_animate_step = 0;
  hide();
  mult = 1;
}

/**
   Custom input box destructor
 */
hud_input_box::~hud_input_box()
{
  delete fm_text;
  delete fm_title;
}

/**
   Sets text, title and default text and shows input box
 */
void hud_input_box::set_text_title_definput(const QString &s1,
                                            const QString &s2,
                                            const QString &def_input)
{
  QSpacerItem *spacer;
  QVBoxLayout *layout;
  int w, w2, h;
  QDialogButtonBox *button_box;
  QPoint p;

  button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  layout = new QVBoxLayout;
  if (s1.contains('\n')) {
    int i;
    i = s1.indexOf('\n');
    cs1 = s1.left(i);
    cs2 = s1.right(s1.count() - i);
    mult = 2;
    w2 = qMax(fm_text->horizontalAdvance(cs1),
              fm_text->horizontalAdvance(cs2));
    w = qMax(w2, fm_title->horizontalAdvance(s2));
  } else {
    w = qMax(fm_text->horizontalAdvance(s1),
             fm_title->horizontalAdvance(s2));
  }
  w = w + 20;
  h = mult * (fm_text->height() * 3 / 2) + 2 * fm_title->height();
  top = 2 * fm_title->height();

  spacer =
      new QSpacerItem(w, h, QSizePolicy::Expanding, QSizePolicy::Minimum);
  layout->addItem(spacer);
  layout->addWidget(&input_edit);
  layout->addWidget(button_box);
  input_edit.setFont(f_text);
  input_edit.setText(def_input);
  setLayout(layout);
  QObject::connect(button_box, &QDialogButtonBox::accepted, this,
                   &QDialog::accept);
  QObject::connect(button_box, &QDialogButtonBox::rejected, this,
                   &QDialog::reject);

  text = s1;
  title = s2;
  p = QPoint((parentWidget()->width() - w) / 2,
             (parentWidget()->height() - h) / 2);
  p = parentWidget()->mapToGlobal(p);
  move(p);
  input_edit.activateWindow();
  input_edit.setFocus();
  m_timer.start();
  startTimer(41);
  show();
  update();
}

/**
   Timer event used to animate input box
 */
void hud_input_box::timerEvent(QTimerEvent *event)
{
  m_animate_step = m_timer.elapsed() / 40;
  update();
}

/**
   Paint event for custom input box
 */
void hud_input_box::paintEvent(QPaintEvent *event)
{
  QPainter p;
  QRect rx, ry;
  QLinearGradient g;
  QColor c1;
  QColor c2;
  QColor c3;
  int step;
  float fstep;

  step = m_animate_step % 300;
  if (step > 150) {
    step = step - 150;
    step = 150 - step;
  }
  step = step + 10;
  rx = QRect(2, 2, width() - 4, top);
  ry = QRect(2, top, width() - 4, height() - top - 4);

  c1 = QColor(palette().color(QPalette::Highlight));
  c2 = QColor(Qt::transparent);
  c3 = QColor(palette().color(QPalette::Highlight)).lighter(145);
  step = qMax(0, step);
  step = qMin(255, step);
  c1.setAlpha(step);
  c2.setAlpha(step);
  c3.setAlpha(step);

  fstep = static_cast<float>(step) / 400;
  g = QLinearGradient(0, 0, width(), height());
  g.setColorAt(0, c2);
  g.setColorAt(fstep, c3);
  g.setColorAt(1, c2);

  p.begin(this);
  p.fillRect(rx, QColor(palette().color(QPalette::Highlight)));
  p.fillRect(ry, QColor(palette().color(QPalette::AlternateBase)));
  p.fillRect(rx, g);
  p.setFont(f_title);
  p.drawText((width() - fm_title->horizontalAdvance(title)) / 2,
             fm_title->height() * 4 / 3, title);
  p.setFont(f_text);
  if (mult == 1) {
    p.drawText((width() - fm_text->horizontalAdvance(text)) / 2,
               2 * fm_title->height() + fm_text->height() * 4 / 3, text);
  } else {
    p.drawText((width() - fm_text->horizontalAdvance(cs1)) / 2,
               2 * fm_title->height() + fm_text->height() * 4 / 3, cs1);
    p.drawText((width() - fm_text->horizontalAdvance(cs2)) / 2,
               2 * fm_title->height() + fm_text->height() * 8 / 3, cs2);
  }
  p.end();
  event->accept();
}

/**
   Constructor for hud_units (holds layout for whole uunits info)
 */
hud_units::hud_units(QWidget *parent)
    : QFrame(parent), ufont(nullptr), ul_units(nullptr),
      current_tile(nullptr)
{
  QVBoxLayout *vbox;
  QVBoxLayout *unit_lab;
  QSpacerItem *sp;
  setParent(parent);

  main_layout = new QHBoxLayout;
  sp = new QSpacerItem(50, 2);
  vbox = new QVBoxLayout;
  unit_lab = new QVBoxLayout;
  unit_lab->setContentsMargins(6, 9, 0, 3);
  vbox->setSpacing(0);
  unit_lab->addWidget(&unit_label);
  main_layout->addLayout(unit_lab);
  main_layout->addWidget(&tile_label);
  unit_icons = new unit_actions(this, nullptr);
  vbox->addSpacerItem(sp);
  vbox->addWidget(&text_label);
  vbox->addWidget(unit_icons);
  main_layout->addLayout(vbox);
  main_layout->setSpacing(0);
  main_layout->setSpacing(3);
  main_layout->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(3);
  vbox->setContentsMargins(0, 0, 0, 0);
  setLayout(main_layout);
  mw = new move_widget(this);
  setFocusPolicy(Qt::ClickFocus);
}

/**
   Hud_units destructor
 */
hud_units::~hud_units() = default;

/**
   Move Event for hud_units, used to save position
 */
void hud_units::moveEvent(QMoveEvent *event)
{
  king()->qt_settings.unit_info_pos_fx =
      static_cast<float>(event->pos().x()) / queen()->mapview_wdg->width();
  king()->qt_settings.unit_info_pos_fy =
      static_cast<float>(event->pos().y()) / queen()->mapview_wdg->height();
}

/**
   Update possible action for given units
 */
void hud_units::update_actions(unit_list *punits)
{
  int num;
  int wwidth;
  int font_width;
  int expanded_unit_width;
  QFont font = *fcFont::instance()->getFont(fonts::notify_label);
  QFontMetrics *fm;
  QImage cropped_img;
  QImage img;
  QPainter p;
  QPixmap pix, pix2;
  QRect crop, bounding_rect;
  QString mp;
  QString snum;
  QString fraction1, fraction2;
  QString text_str, move_pt_text;
  QPixmap *tile_pixmap;
  QPixmap *unit_pixmap;
  struct city *pcity;
  struct player *owner;
  struct tileset *tmp;
  struct unit *punit;

  punit = head_of_units_in_focus();
  if (punit == nullptr) {
    hide();
    return;
  }

  font.setCapitalization(QFont::AllUppercase);
  font.setBold(true);
  setFixedHeight(parentWidget()->height() / 12);
  text_label.setFixedHeight((height() * 2) / 10);

  move(qRound(queen()->mapview_wdg->width()
              * king()->qt_settings.unit_info_pos_fx),
       qRound((queen()->mapview_wdg->height()
               * king()->qt_settings.unit_info_pos_fy)));
  unit_icons->setFixedHeight((height() * 8) / 10);

  setUpdatesEnabled(false);

  tmp = nullptr;
  if (unscaled_tileset) {
    tmp = tileset;
    tileset = unscaled_tileset;
  }
  owner = punit->owner;
  pcity = player_city_by_number(owner, punit->homecity);
  if (punit->name.isEmpty()) {
    if (pcity == nullptr) {
      // TRANS: <unit> #<unit id>
      text_str = QString(_("%1 #%2"))
                     .arg(unit_name_translation(punit))
                     .arg(punit->id);
    } else {
      // TRANS: <unit> #<unit id> (<home city>)
      text_str = QString(_("%1 #%2 (%3)"))
                     .arg(unit_name_translation(punit))
                     .arg(punit->id)
                     .arg(city_name_get(pcity));
    }
  } else {
    if (pcity == nullptr) {
      // TRANS: <unit> #<unit id> "<unit name>"
      text_str = QString(_("%1 #%2 \"%3\""))
                     .arg(unit_name_translation(punit))
                     .arg(punit->id)
                     .arg(punit->name);
    } else {
      // TRANS: <unit> #<unit id> "<unit name>" (<home city>)
      text_str = QString(_("%1 #%2 \"%3\" (%4)"))
                     .arg(unit_name_translation(punit))
                     .arg(punit->id)
                     .arg(punit->name)
                     .arg(city_name_get(pcity));
    }
  }
  text_str = text_str + " ";
  mp = QString(move_points_text(punit->moves_left, false));
  if (utype_fuel(unit_type_get(punit))) {
    mp = mp + QStringLiteral("(")
         + QString(move_points_text(
             (unit_type_get(punit)->move_rate * ((punit->fuel) - 1)
              + punit->moves_left),
             false))
         + QStringLiteral(")");
  }
  // TRANS: MP = Movement points
  mp = QString(_("MP: ")) + mp;
  text_str = text_str + mp + " ";
  text_str += QString(_("HP:%1/%2"))
                  .arg(QString::number(punit->hp),
                       QString::number(unit_type_get(punit)->hp));
  num = unit_list_size(punit->tile->units);
  snum = QString::number(unit_list_size(punit->tile->units) - 1);
  if (unit_list_size(get_units_in_focus()) > 1) {
    int n = unit_list_size(get_units_in_focus());
    // TRANS: preserve leading space; always at least 2
    text_str =
        text_str
        + QString(PL_(" (Selected %1 unit)", " (Selected %1 units)", n))
              .arg(n);
  } else if (num > 1) {
    QByteArray ut_bytes;

    ut_bytes = snum.toLocal8Bit();
    // TRANS: preserve leading space
    text_str = text_str
               + QString(PL_(" +%1 unit", " +%1 units", num - 1))
                     .arg(ut_bytes.data());
  }
  text_label.setTextFormat(Qt::PlainText);
  text_label.setText(text_str);
  font.setPixelSize((text_label.height() * 9) / 10);
  text_label.setFont(font);
  fm = new QFontMetrics(font);
  text_label.setFixedWidth(fm->horizontalAdvance(text_str) + 20);
  delete fm;

  unit_pixmap = qtg_canvas_create(tileset_unit_width(tileset),
                                  tileset_unit_height(tileset));
  unit_pixmap->fill(Qt::transparent);
  put_unit(punit, unit_pixmap, 0, 0);
  img = unit_pixmap->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  img = cropped_img.scaledToHeight(height(), Qt::SmoothTransformation);
  expanded_unit_width = tileset_unit_width(tileset)
                        * ((height() + 0.0) / tileset_unit_height(tileset));
  pix = QPixmap::fromImage(img);
  /* add transparent borders if image is too slim, accounting for the
   * scaledToHeight() we've applied */
  if (pix.width() < expanded_unit_width) {
    pix2 = QPixmap(expanded_unit_width, pix.height());
    pix2.fill(Qt::transparent);
    p.begin(&pix2);
    p.drawPixmap(expanded_unit_width / 2 - pix.width() / 2, 0, pix);
    p.end();
    pix = pix2;
  }
  // Draw movement points
  move_pt_text = move_points_text(punit->moves_left, false);
  if (move_pt_text.contains('/')) {
    fraction2 = move_pt_text.right(1);
    move_pt_text.remove(move_pt_text.count() - 2, 2);
    fraction1 = move_pt_text.right(1);
    move_pt_text.remove(move_pt_text.count() - 1, 1);
  }
  crop = QRect(5, 5, pix.width() - 5, pix.height() - 5);
  font.setCapitalization(QFont::Capitalize);
  font.setPointSize((pix.height() * 2) / 5);
  p.begin(&pix);
  p.setFont(font);
  p.setPen(Qt::white);
  p.drawText(crop, Qt::AlignLeft | Qt::AlignBottom, move_pt_text,
             &bounding_rect);

  bounding_rect.adjust(bounding_rect.width(), 0, bounding_rect.width() * 2,
                       0);
  if (punit->fuel > 1) {
    int fuel;

    font.setPointSize(pix.height() / 4);
    p.setFont(font);
    fuel = punit->fuel - 1;
    fuel = fuel * punit->utype->move_rate / SINGLE_MOVE;
    p.drawText(bounding_rect, Qt::AlignCenter,
               QStringLiteral("+") + QString::number(fuel));
  }

  if (move_pt_text.isEmpty()) {
    move_pt_text = QStringLiteral(" ");
  }
  bounding_rect =
      p.boundingRect(crop, Qt::AlignLeft | Qt::AlignBottom, move_pt_text);
  font.setPointSize(pix.height() / 5);
  fm = new QFontMetrics(font);
  font_width = (fm->horizontalAdvance(move_pt_text) * 3) / 5;
  delete fm;
  p.setFont(font);
  if (!fraction1.isNull()) {
    int t = 2 * font.pointSize();

    crop = QRect(bounding_rect.right() - font_width, bounding_rect.top(), t,
                 (t / 5) * 4);
    p.drawText(crop, Qt::AlignLeft | Qt::AlignBottom, fraction1);
    crop = QRect(bounding_rect.right() - font_width,
                 (bounding_rect.bottom() + bounding_rect.top()) / 2, t,
                 (t / 5) * 4);
    p.drawText(crop, Qt::AlignLeft | Qt::AlignTop, fraction2);
    crop = QRect(bounding_rect.right() - font_width,
                 (bounding_rect.bottom() + bounding_rect.top()) / 2 - t / 16,
                 (t * 2) / 5, t / 8);
    p.fillRect(crop, Qt::white);
  }
  p.end();
  wwidth = 2 * 3 + pix.width();
  unit_label.setPixmap(pix);
  if (tileset_is_isometric(tileset)) {
    tile_pixmap = qtg_canvas_create(tileset_full_tile_width(tileset),
                                    tileset_tile_height(tileset) * 2);
  } else {
    tile_pixmap = qtg_canvas_create(tileset_full_tile_width(tileset),
                                    tileset_tile_height(tileset));
  }
  tile_pixmap->fill(QColor(0, 0, 0, 0));
  put_terrain(punit->tile, tile_pixmap, 0, 0);
  img = tile_pixmap->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  img = cropped_img.scaledToHeight(height() - 5, Qt::SmoothTransformation);
  pix = QPixmap::fromImage(img);
  tile_label.setPixmap(pix);
  unit_label.setToolTip(popup_info_text(punit->tile));
  tile_label.setToolTip(popup_terrain_info(punit->tile));
  wwidth = wwidth + pix.width();
  qtg_canvas_free(tile_pixmap);
  qtg_canvas_free(unit_pixmap);

  setFixedWidth(wwidth
                + qMax(unit_icons->update_actions() * (height() * 8) / 10,
                       text_label.width()));
  mw->put_to_corner();
  if (tmp != nullptr) {
    tileset = tmp;
  }
  setUpdatesEnabled(true);
  updateGeometry();
  update();

  show();
}

/**
   Custom label with extra mouse events
 */
click_label::click_label() : QLabel()
{
  connect(this, &click_label::left_clicked, this,
          &click_label::mouse_clicked);
}

/**
   Mouse event for click_label
 */
void click_label::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton) {
    emit left_clicked();
  }
}

/**
   Centers on current unit
 */
void click_label::mouse_clicked()
{
  queen()->game_tab_widget->setCurrentIndex(0);
  request_center_focus_unit();
}

/**
   Hud action constructor, used to show one action
 */
hud_action::hud_action(QWidget *parent) : QWidget(parent)
{
  connect(this, &hud_action::left_clicked, this, &hud_action::mouse_clicked);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  focus = false;
  action_pixmap = nullptr;
  action_shortcut = SC_NONE;
}

/**
   Sets given pixmap for hud_action
 */
void hud_action::set_pixmap(QPixmap *p) { action_pixmap = p; }

/**
   Custom painting for hud_action
 */
void hud_action::paintEvent(QPaintEvent *event)
{
  QRect rx, ry, rz;
  QPainter p;

  rx = QRect(0, 0, width(), height());
  ry = QRect(0, 0, action_pixmap->width(), action_pixmap->height());
  rz = QRect(0, 2, width(), height() - 6);
  p.begin(this);
  p.setCompositionMode(QPainter::CompositionMode_Source);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  p.drawPixmap(rx, *action_pixmap, ry);
  p.setPen(QColor(palette().color(QPalette::Text)));
  p.drawRect(rz);
  if (focus) {
    p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    p.fillRect(rx, QColor(palette().color(QPalette::Highlight)));
  }
  p.end();
}

/**
   Hud action destructor
 */
hud_action::~hud_action() { NFC_FREE(action_pixmap); }

/**
   Mouse press event for hud_action
 */
void hud_action::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::RightButton) {
    emit right_clicked();
  } else if (e->button() == Qt::LeftButton) {
    emit left_clicked();
  }
}

/**
   Mouse move event for hud_action, draw focus
 */
void hud_action::mouseMoveEvent(QMouseEvent *event)
{
  focus = true;
  update();
}

/**
   Leave event for hud_action, used to get status of pixmap higlight
 */
void hud_action::leaveEvent(QEvent *event)
{
  focus = false;
  update();
  QWidget::leaveEvent(event);
}

/**
   Enter event for hud_action, used to get status of pixmap higlight
 */
void hud_action::enterEvent(QEvent *event)
{
  focus = true;
  update();
  QWidget::enterEvent(event);
}

/**
   Right click event for hud_action
 */
void hud_action::mouse_right_clicked() {}

/**
   Left click event for hud_action
 */
void hud_action::mouse_clicked()
{
  king()->menu_bar->execute_shortcut(action_shortcut);
}

/**
   Units action contructor, holds possible hud_actions
 */
unit_actions::unit_actions(QWidget *parent, unit *punit) : QWidget(parent)
{
  layout = new QHBoxLayout(this);
  layout->setSpacing(3);
  layout->setContentsMargins(0, 0, 0, 0);
  current_unit = punit;
  init_layout();
  setFocusPolicy(Qt::ClickFocus);
}

/**
   Destructor for unit_actions
 */
unit_actions::~unit_actions()
{
  qDeleteAll(actions);
  actions.clear();
}

/**
   Initiazlizes layout ( layout needs to be changed after adding units )
 */
void unit_actions::init_layout()
{
  QSizePolicy size_fixed_policy(QSizePolicy::MinimumExpanding,
                                QSizePolicy::Fixed, QSizePolicy::Frame);
  setSizePolicy(size_fixed_policy);
  layout->setSpacing(0);
  setLayout(layout);
}

/**
   Updates avaialable actions, returns actions count
 */
int unit_actions::update_actions()
{
  hud_action *a;

  current_unit = head_of_units_in_focus();

  if (current_unit == nullptr) {
    clear_layout();
    hide();
    return 0;
  }
  /* HACK prevent crash with active goto when leaving widget,
   * just skip update because with active goto actions shouldn't change */
  if (goto_is_active()) {
    return actions.count();
  }
  hide();
  clear_layout();
  setUpdatesEnabled(false);

  for (auto *a : qAsConst(actions)) {
    delete a;
  }
  qDeleteAll(actions);
  actions.clear();

  // Create possible actions

  if (unit_can_add_or_build_city(current_unit)) {
    a = new hud_action(this);
    a->action_shortcut = SC_BUILDCITY;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("home")));
    actions.append(a);
  }

  if (can_unit_do_activity(current_unit, ACTIVITY_MINE)) {
    a = new hud_action(this);
    a->action_shortcut = SC_BUILDMINE;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("mine")));
    actions.append(a);
  }

  if (can_unit_do_activity(current_unit, ACTIVITY_PLANT)) {
    a = new hud_action(this);
    a->action_shortcut = SC_PLANT;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("plantforest")));
    actions.append(a);
  }

  if (can_unit_do_activity(current_unit, ACTIVITY_IRRIGATE)) {
    a = new hud_action(this);
    a->action_shortcut = SC_BUILDIRRIGATION;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("irrigation")));
    actions.append(a);
  }

  if (can_unit_do_activity(current_unit, ACTIVITY_CULTIVATE)) {
    a = new hud_action(this);
    a->action_shortcut = SC_CULTIVATE;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("chopchop")));
    actions.append(a);
  }

  if (can_unit_do_activity(current_unit, ACTIVITY_TRANSFORM)) {
    a = new hud_action(this);
    a->action_shortcut = SC_TRANSFORM;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("transform")));
    actions.append(a);
  }

  // Road
  {
    bool ok = false;
    extra_type_by_cause_iterate(EC_ROAD, pextra)
    {
      struct road_type *proad = extra_road_get(pextra);
      if (can_build_road(proad, current_unit, unit_tile(current_unit))) {
        ok = true;
      }
    }
    extra_type_by_cause_iterate_end;
    if (ok) {
      a = new hud_action(this);
      a->action_shortcut = SC_BUILDROAD;
      a->set_pixmap(
          fcIcons::instance()->getPixmap(QStringLiteral("buildroad")));
      actions.append(a);
    }
  }
  // Goto
  a = new hud_action(this);
  a->action_shortcut = SC_GOTO;
  a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("goto")));
  actions.append(a);

  if (can_unit_do_activity(current_unit, ACTIVITY_FORTIFYING)) {
    a = new hud_action(this);
    a->action_shortcut = SC_FORTIFY;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("fortify")));
    actions.append(a);
  }

  if (can_unit_do_activity(current_unit, ACTIVITY_SENTRY)) {
    a = new hud_action(this);
    a->action_shortcut = SC_SENTRY;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("sentry")));
    actions.append(a);
  }

  // Load
  if (unit_can_load(current_unit)) {
    a = new hud_action(this);
    a->action_shortcut = SC_LOAD;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("load")));
    actions.append(a);
  }

  // Set homecity
  if (tile_city(unit_tile(current_unit))) {
    if (can_unit_change_homecity_to(current_unit,
                                    tile_city(unit_tile(current_unit)))) {
      a = new hud_action(this);
      a->action_shortcut = SC_SETHOME;
      a->set_pixmap(
          fcIcons::instance()->getPixmap(QStringLiteral("set_homecity")));
      actions.append(a);
    }
  }

  // Upgrade
  if (UU_OK == unit_upgrade_test(current_unit, false)) {
    a = new hud_action(this);
    a->action_shortcut = SC_UPGRADE_UNIT;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("upgrade")));
    actions.append(a);
  }

  // Automate
  if (can_unit_do_autosettlers(current_unit)) {
    a = new hud_action(this);
    a->action_shortcut = SC_AUTOMATE;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("automate")));
    actions.append(a);
  }

  // Paradrop
  if (can_unit_paradrop(current_unit)) {
    a = new hud_action(this);
    a->action_shortcut = SC_PARADROP;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("paradrop")));
    actions.append(a);
  }

  // Clean pollution
  if (can_unit_do_activity(current_unit, ACTIVITY_POLLUTION)) {
    a = new hud_action(this);
    a->action_shortcut = SC_PARADROP;
    a->set_pixmap(
        fcIcons::instance()->getPixmap(QStringLiteral("pollution")));
    actions.append(a);
  }

  // Unload
  if (unit_transported(current_unit)
      && can_unit_unload(current_unit, unit_transport_get(current_unit))
      && can_unit_exist_at_tile(&(wld.map), current_unit,
                                unit_tile(current_unit))) {
    a = new hud_action(this);
    a->action_shortcut = SC_UNLOAD;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("unload")));
    actions.append(a);
  }

  // Nuke
  if (unit_can_do_action(current_unit, ACTION_NUKE)) {
    a = new hud_action(this);
    a->action_shortcut = SC_NUKE;
    a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("nuke")));
    actions.append(a);
  }

  // Wait
  a = new hud_action(this);
  a->action_shortcut = SC_WAIT;
  a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("wait")));
  actions.append(a);

  // Done moving
  a = new hud_action(this);
  a->action_shortcut = SC_DONE_MOVING;
  a->set_pixmap(fcIcons::instance()->getPixmap(QStringLiteral("done")));
  actions.append(a);

  for (auto *a : qAsConst(actions)) {
    a->setToolTip(
        king()->menu_bar->shortcut_2_menustring(a->action_shortcut));
    a->setFixedHeight(height());
    a->setFixedWidth(height());
    layout->addWidget(a);
  }

  setFixedWidth(actions.count() * height());
  setUpdatesEnabled(true);
  show();
  layout->update();
  updateGeometry();
  return actions.count();
}

/**
   Cleans layout - run it before layout initialization
 */
void unit_actions::clear_layout()
{
  int i = actions.count();
  hud_action *ui;
  int j;

  setUpdatesEnabled(false);
  for (j = 0; j < i; j++) {
    ui = actions[j];
    layout->removeWidget(ui);
    delete ui;
  }
  while (!actions.empty()) {
    actions.removeFirst();
  }
  setUpdatesEnabled(true);
}

/**
   Constructor for widget allowing loading units on transports
 */
hud_unit_loader::hud_unit_loader(struct unit *pcargo, struct tile *ptile)
{
  setProperty("showGrid", "false");
  setProperty("selectionBehavior", "SelectRows");
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::SingleSelection);
  verticalHeader()->setVisible(false);
  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  horizontalHeader()->setVisible(false);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &hud_unit_loader::selection_changed);
  cargo = pcargo;
  qtile = ptile;
}

/**
   Destructor for units loader
 */
hud_unit_loader::~hud_unit_loader() = default;

/**
   Shows unit loader, adds possible tranportsand units to table
   Calculates table size
 */
void hud_unit_loader::show_me()
{
  QTableWidgetItem *new_item;
  int max_size = 0;
  int i, j;
  int w, h;
  QPixmap *spite;

  unit_list_iterate(qtile->units, ptransport)
  {
    if (can_unit_transport(ptransport, cargo)
        && get_transporter_occupancy(ptransport)
               < get_transporter_capacity(ptransport)) {
      transports.append(ptransport);
      max_size = qMax(max_size, get_transporter_occupancy(ptransport));
    }
  }
  unit_list_iterate_end;

  setRowCount(transports.count());
  setColumnCount(max_size + 1);
  for (i = 0; i < transports.count(); i++) {
    QString str;
    spite = get_unittype_sprite(tileset, transports.at(i)->utype,
                                direction8_invalid());
    str = utype_rule_name(transports.at(i)->utype);
    // TRANS: MP - just movement points
    str = str + " ("
          + QString(move_points_text(transports.at(i)->moves_left, false))
          + _("MP") + ")";
    new_item = new QTableWidgetItem(QIcon(*spite), str);
    setItem(i, 0, new_item);
    j = 1;
    unit_list_iterate(transports.at(i)->transporting, tunit)
    {
      spite =
          get_unittype_sprite(tileset, tunit->utype, direction8_invalid());
      new_item = new QTableWidgetItem(QIcon(*spite), QLatin1String(""));
      setItem(i, j, new_item);
      j++;
    }
    unit_list_iterate_end;
  }

  w = verticalHeader()->width() + 4;
  for (i = 0; i < columnCount(); i++) {
    w += columnWidth(i);
  }
  h = horizontalHeader()->height() + 4;
  for (i = 0; i < rowCount(); i++) {
    h += rowHeight(i);
  }

  resize(w, h);
  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog
                 | Qt::FramelessWindowHint);
  show();
}

/**
   Selects given tranport and closes widget
 */
void hud_unit_loader::selection_changed(const QItemSelection &s1,
                                        const QItemSelection &s2)
{
  int curr_row;

  curr_row = s1.indexes().at(0).row();
  request_unit_load(cargo, transports.at(curr_row), qtile);
  close();
}

/**
   Constructor for unit_hud_selector
 */
unit_hud_selector::unit_hud_selector(QWidget *parent) : QFrame(parent)
{
  QHBoxLayout *hbox, *hibox;
  Unit_type_id utype_id;
  QGroupBox *no_name;
  QVBoxLayout *groupbox_layout;

  hide();
  struct unit *punit = head_of_units_in_focus();

  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog
                 | Qt::FramelessWindowHint);
  main_layout = new QVBoxLayout(this);

  unit_sel_type = new QComboBox();

  unit_type_iterate(utype)
  {
    utype_id = utype_index(utype);
    if (has_player_unit_type(utype_id)) {
      unit_sel_type->addItem(utype_name_translation(utype), utype_id);
    }
  }
  unit_type_iterate_end;

  if (punit) {
    int i;
    i = unit_sel_type->findText(utype_name_translation(punit->utype));
    unit_sel_type->setCurrentIndex(i);
  }
  no_name = new QGroupBox();
  no_name->setTitle(_("Unit type"));
  this_type = new QRadioButton(_("Selected type"), no_name);
  this_type->setChecked(true);
  any_type = new QRadioButton(_("All types"), no_name);
  connect(unit_sel_type, qOverload<int>(&QComboBox::currentIndexChanged),
          this, qOverload<int>(&unit_hud_selector::select_units));
  connect(this_type, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(any_type, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  groupbox_layout = new QVBoxLayout;
  groupbox_layout->addWidget(unit_sel_type);
  groupbox_layout->addWidget(this_type);
  groupbox_layout->addWidget(any_type);
  no_name->setLayout(groupbox_layout);
  hibox = new QHBoxLayout;
  hibox->addWidget(no_name);

  no_name = new QGroupBox();
  no_name->setTitle(_("Unit activity"));
  any_activity = new QRadioButton(_("Any activity"), no_name);
  any_activity->setChecked(true);
  fortified = new QRadioButton(_("Fortified"), no_name);
  idle = new QRadioButton(_("Idle"), no_name);
  sentried = new QRadioButton(_("Sentried"), no_name);
  connect(any_activity, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(idle, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(fortified, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(sentried, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  groupbox_layout = new QVBoxLayout;
  groupbox_layout->addWidget(any_activity);
  groupbox_layout->addWidget(idle);
  groupbox_layout->addWidget(fortified);
  groupbox_layout->addWidget(sentried);
  no_name->setLayout(groupbox_layout);
  hibox->addWidget(no_name);
  main_layout->addLayout(hibox);

  no_name = new QGroupBox();
  no_name->setTitle(_("Unit HP and MP"));
  any = new QRadioButton(_("Any unit"), no_name);
  full_hp = new QRadioButton(_("Full HP"), no_name);
  full_mp = new QRadioButton(_("Full MP"), no_name);
  full_hp_mp = new QRadioButton(_("Full HP and MP"), no_name);
  full_hp_mp->setChecked(true);
  connect(any, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(full_hp, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(full_mp, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(full_hp_mp, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  groupbox_layout = new QVBoxLayout;
  groupbox_layout->addWidget(any);
  groupbox_layout->addWidget(full_hp);
  groupbox_layout->addWidget(full_mp);
  groupbox_layout->addWidget(full_hp_mp);
  no_name->setLayout(groupbox_layout);
  hibox = new QHBoxLayout;
  hibox->addWidget(no_name);

  no_name = new QGroupBox();
  no_name->setTitle(_("Location"));
  anywhere = new QRadioButton(_("Anywhere"), no_name);
  this_tile = new QRadioButton(_("Current tile"), no_name);
  this_continent = new QRadioButton(_("Current continent"), no_name);
  main_continent = new QRadioButton(_("Main continent"), no_name);
  groupbox_layout = new QVBoxLayout;

  if (punit) {
    this_tile->setChecked(true);
  } else {
    this_tile->setDisabled(true);
    this_continent->setDisabled(true);
    main_continent->setChecked(true);
  }

  groupbox_layout->addWidget(this_tile);
  groupbox_layout->addWidget(this_continent);
  groupbox_layout->addWidget(main_continent);
  groupbox_layout->addWidget(anywhere);

  no_name->setLayout(groupbox_layout);
  hibox->addWidget(no_name);
  main_layout->addLayout(hibox);

  select = new QPushButton(_("Select"));
  cancel = new QPushButton(_("Cancel"));
  connect(anywhere, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(this_tile, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(this_continent, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(main_continent, &QRadioButton::toggled, this,
          qOverload<bool>(&unit_hud_selector::select_units));
  connect(select, &QAbstractButton::clicked, this,
          &unit_hud_selector::uhs_select);
  connect(cancel, &QAbstractButton::clicked, this,
          &unit_hud_selector::uhs_cancel);
  hbox = new QHBoxLayout;
  hbox->addWidget(cancel);
  hbox->addWidget(select);

  result_label.setAlignment(Qt::AlignCenter);
  main_layout->addWidget(&result_label, Qt::AlignHCenter);
  main_layout->addLayout(hbox);
  setLayout(main_layout);
}

/**
   Shows and moves to center unit_hud_selector
 */
void unit_hud_selector::show_me()
{
  QPoint p;

  p = QPoint((parentWidget()->width() - sizeHint().width()) / 2,
             (parentWidget()->height() - sizeHint().height()) / 2);
  p = parentWidget()->mapToGlobal(p);
  move(p);
  setVisible(true);
  show();
  select_units();
}

/**
   Unit_hud_selector destructor
 */
unit_hud_selector::~unit_hud_selector() = default;

/**
   Selects and closes widget
 */
void unit_hud_selector::uhs_select()
{
  const struct player *pplayer;

  pplayer = client_player();

  unit_list_iterate(pplayer->units, punit)
  {
    if (activity_filter(punit) && hp_filter(punit) && island_filter(punit)
        && type_filter(punit)) {
      unit_focus_add(punit);
    }
  }
  unit_list_iterate_end;
  close();
}

/**
   Closes current widget
 */
void unit_hud_selector::uhs_cancel() { close(); }

/**
   Shows number of selected units on label
 */
void unit_hud_selector::select_units(int x)
{
  int num = 0;
  const struct player *pplayer;

  pplayer = client_player();

  unit_list_iterate(pplayer->units, punit)
  {
    if (activity_filter(punit) && hp_filter(punit) && island_filter(punit)
        && type_filter(punit)) {
      num++;
    }
  }
  unit_list_iterate_end;
  result_label.setText(QString(PL_("%1 unit", "%1 units", num)).arg(num));
}

/**
   Convinient slot for ez connect
 */
void unit_hud_selector::select_units(bool x) { select_units(0); }

/**
   Key press event for unit_hud_selector
 */
void unit_hud_selector::keyPressEvent(QKeyEvent *event)
{
  if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter)) {
    uhs_select();
  }
  if (event->key() == Qt::Key_Escape) {
    close();
    event->accept();
  }
  QWidget::keyPressEvent(event);
}

/**
   Filter by activity
 */
bool unit_hud_selector::activity_filter(struct unit *punit)
{
  return (punit->activity == ACTIVITY_FORTIFIED && fortified->isChecked())
         || (punit->activity == ACTIVITY_SENTRY && sentried->isChecked())
         || (punit->activity == ACTIVITY_IDLE && idle->isChecked())
         || any_activity->isChecked();
}

/**
   Filter by hp/mp
 */
bool unit_hud_selector::hp_filter(struct unit *punit)
{
  return any->isChecked()
         || (full_mp->isChecked()
             && punit->moves_left >= punit->utype->move_rate)
         || (full_hp->isChecked() && punit->hp >= punit->utype->hp)
         || (full_hp_mp->isChecked() && punit->hp >= punit->utype->hp
             && punit->moves_left >= punit->utype->move_rate);
}

/**
   Filter by location
 */
bool unit_hud_selector::island_filter(struct unit *punit)
{
  int island = -1;
  struct unit *cunit = head_of_units_in_focus();

  if (this_tile->isChecked() && cunit) {
    if (punit->tile == cunit->tile) {
      return true;
    }
  }

  if (main_continent->isChecked()
      && player_primary_capital(client_player())) {
    island = player_primary_capital(client_player())->tile->continent;
  } else if (this_continent->isChecked() && cunit) {
    island = cunit->tile->continent;
  }

  if (island > -1) {
    if (punit->tile->continent == island) {
      return true;
    }
  }

  return anywhere->isChecked();
}

/**
   Filter by type
 */
bool unit_hud_selector::type_filter(struct unit *punit)
{
  QVariant qvar;
  Unit_type_id utype_id;

  if (this_type->isChecked()) {
    qvar = unit_sel_type->currentData();
    utype_id = qvar.toInt();
    return utype_id == utype_index(punit->utype);
  }
  return any_type->isChecked();
}

/**
   Tooltip text for terrain information
 */
QString popup_terrain_info(struct tile *ptile)
{
  int movement_cost;
  struct terrain *terr;
  QString ret, t, move_text;
  bool has_road = false;

  terr = ptile->terrain;
  ret = QString(_("Terrain: %1\n")).arg(tile_get_info_text(ptile, true, 0));
  ret =
      ret
      + QString(_("Food/Prod/Trade: %1\n")).arg(get_tile_output_text(ptile));
  t = get_infrastructure_text(ptile->extras);
  if (t != QLatin1String("")) {
    ret = ret + QString(_("Infrastructure: %1\n")).arg(t);
  }
  ret = ret + QString(_("Defense bonus: %1%\n")).arg(terr->defense_bonus);
  movement_cost = terr->movement_cost;

  extra_type_by_cause_iterate(EC_ROAD, pextra)
  {
    struct road_type *proad = extra_road_get(pextra);

    if (tile_has_road(ptile, proad)) {
      if (proad->move_cost <= movement_cost) {
        has_road = true;
        move_text = move_points_text(proad->move_cost, true);
        movement_cost = proad->move_cost;
      }
    }
  }
  extra_type_by_cause_iterate_end;

  if (has_road) {
    ret = ret + QString(_("Movement cost: %1")).arg(move_text);
  } else {
    ret = ret + QString(_("Movement cost: %1")).arg(movement_cost);
  }

  return ret;
}

/**
   Shows new turn information with big font
 */
void show_new_turn_info()
{
  QString s;
  hud_text *ht;
  QList<hud_text *> close_list;
  struct research *research;
  int i;
  char buf[25];

  if (!client_has_player() || !king()->qt_settings.show_new_turn_text) {
    return;
  }
  close_list = queen()->mapview_wdg->findChildren<hud_text *>();
  for (i = 0; i < close_list.size(); ++i) {
    close_list.at(i)->close();
    close_list.at(i)->deleteLater();
  }
  research = research_get(client_player());
  s = QString(_("Year: %1 (Turn: %2)"))
          .arg(calendar_text())
          .arg(game.info.turn)
      + "\n";
  s = s + QString(nation_plural_for_player(client_player()));
  s = s + " - "
      + QString(_("Population: %1"))
            .arg(population_to_text(civ_population(client.conn.playing)));
  if (research->researching != A_UNKNOWN && research->researching != A_UNSET
      && research->researching != A_NONE) {
    s = s + "\n"
        + QString(research_advance_name_translation(research,
                                                    research->researching))
        + " (" + QString::number(research->bulbs_researched) + "/"
        + QString::number(research->client.researching_cost) + ")";
  }
  s = s + "\n" + science_dialog_text() + "\n";

  /* Can't use QString().sprintf() as msys libintl.h defines sprintf() as a
   * macro */
  fc_snprintf(buf, sizeof(buf), "%+d",
              player_get_expected_income(client.conn.playing));

  /* TRANS: current gold, then loss/gain per turn */
  s = s
      + QString(_("Gold: %1 (%2)"))
            .arg(client.conn.playing->economic.gold)
            .arg(buf);
  ht = new hud_text(s, 5, queen()->mapview_wdg);
  ht->show_me();
}

/**
   Hud unit combat contructor, prepares images to show as result
 */
hud_unit_combat::hud_unit_combat(int attacker_unit_id, int defender_unit_id,
                                 int attacker_hp, int defender_hp,
                                 bool make_att_veteran,
                                 bool make_def_veteran, float scale,
                                 QWidget *parent)
    : QWidget(parent)
{
  hud_scale = scale;
  att_hp = attacker_hp;
  def_hp = defender_hp;

  attacker = game_unit_by_number(attacker_unit_id);
  defender = game_unit_by_number(defender_unit_id);
  if (!attacker || !defender) {
    return;
  }
  type_attacker = attacker->utype;
  type_defender = defender->utype;
  att_veteran = make_att_veteran;
  def_veteran = make_def_veteran;
  att_hp_loss = attacker->hp - att_hp;
  def_hp_loss = defender->hp - def_hp;
  if (defender_hp <= 0) {
    center_tile = attacker->tile;
  } else {
    center_tile = defender->tile;
  }
  init_images();
}

/****************************************************************************
  Draws images of units to pixmaps for later use
****************************************************************************/
void hud_unit_combat::init_images(bool redraw)
{
  QImage crdimg, acrimg, at, dt;
  QRect dr, ar;
  QPainter p;
  QPixmap *defender_pixmap;
  QPixmap *attacker_pixmap;
  int w;

  focus = false;
  w = 3 * hud_scale * tileset_unit_height(tileset) / 2;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  setFixedSize(2 * w, w);
  defender_pixmap = qtg_canvas_create(tileset_unit_width(tileset),
                                      tileset_unit_height(tileset));
  defender_pixmap->fill(Qt::transparent);
  if (defender != nullptr) {
    if (!redraw) {
      put_unit(defender, defender_pixmap, 0, 0);
    } else {
      put_unittype(type_defender, defender_pixmap, 0, 0);
    }
    dimg = defender_pixmap->toImage();
    dr = zealous_crop_rect(dimg);
    crdimg = dimg.copy(dr);
    dimg = crdimg.scaledToHeight(w, Qt::SmoothTransformation);
  }
  if (dimg.width() < w) {
    dt = QImage(w, dimg.height(), QImage::Format_ARGB32_Premultiplied);
    dt.fill(Qt::transparent);
    p.begin(&dt);
    p.drawImage(w / 2 - dimg.width() / 2, 0, dimg);
    p.end();
    dimg = dt;
  }
  dimg = dimg.scaled(w, w, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  attacker_pixmap = qtg_canvas_create(tileset_unit_width(tileset),
                                      tileset_unit_height(tileset));
  attacker_pixmap->fill(Qt::transparent);
  if (attacker != nullptr) {
    if (!redraw) {
      put_unit(attacker, attacker_pixmap, 0, 0);
    } else {
      put_unittype(type_attacker, attacker_pixmap, 0, 0);
    }
    aimg = attacker_pixmap->toImage();
    ar = zealous_crop_rect(aimg);
    acrimg = aimg.copy(ar);
    aimg = acrimg.scaledToHeight(w, Qt::SmoothTransformation);
  }
  if (aimg.width() < w) {
    at = QImage(w, dimg.height(), QImage::Format_ARGB32_Premultiplied);
    at.fill(Qt::transparent);
    p.begin(&at);
    p.drawImage(w / 2 - aimg.width() / 2, 0, aimg);
    p.end();
    aimg = at;
  }
  aimg = aimg.scaled(w, w, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  delete defender_pixmap;
  delete attacker_pixmap;
}

/****************************************************************************
  Sets scale for images
****************************************************************************/
void hud_unit_combat::set_scale(float scale)
{
  hud_scale = scale;
  init_images(true);
}

/**
   Hud unit combat destructor
 */
hud_unit_combat::~hud_unit_combat() = default;

/**
   Sets widget fading
 */
void hud_unit_combat::set_fading(float fade)
{
  fading = fade;
  update();
}

/**
   Returns true if widget has focus (used to prevent hiding parent)
 */
bool hud_unit_combat::get_focus() { return focus; }

/**
   Paint event for hud_unit combat
 */
void hud_unit_combat::paintEvent(QPaintEvent *event)
{
  QPainter p;
  QRect left, right;
  QColor c1, c2;
  QPen pen;
  QFont f = *fcFont::instance()->getFont(fonts::default_font);
  QString ahploss, dhploss;

  if (att_hp_loss > 0) {
    ahploss = "-" + QString::number(att_hp_loss);
  } else {
    ahploss = QStringLiteral("0");
  }
  if (def_hp_loss > 0) {
    dhploss = "-" + QString::number(def_hp_loss);
  } else {
    dhploss = QStringLiteral("0");
  }
  f.setBold(true);

  if (def_hp == 0) {
    c1 = QColor(25, 125, 25, 175);
    c2 = QColor(125, 25, 25, 175);
  } else {
    c1 = QColor(125, 25, 25, 175);
    c2 = QColor(25, 125, 25, 175);
  }
  int w = 3 * tileset_unit_height(tileset) / 2 * hud_scale;

  left = QRect(0, 0, w, w);
  right = QRect(w, 0, w, w);
  pen = QPen(QColor(palette().color(QPalette::AlternateBase)), 2.0);
  p.begin(this);
  if (fading < 1.0) {
    p.setOpacity(fading);
  }
  if (focus) {
    p.fillRect(left, QColor(palette().color(QPalette::Highlight)));
    p.fillRect(right, QColor(palette().color(QPalette::Highlight)));
    c1.setAlpha(110);
    c2.setAlpha(110);
  }
  p.fillRect(left, c1);
  p.fillRect(right, c2);
  p.setPen(pen);
  p.drawRect(1, 1, width() - 2, height() - 2);
  p.drawImage(left, aimg);
  p.setFont(f);
  p.setPen(QColor(Qt::white));
  if (def_veteran) {
    p.drawText(right,
               Qt::AlignHCenter | Qt::AlignJustify | Qt::AlignAbsolute,
               QStringLiteral("*"));
  }
  if (att_veteran) {
    p.drawText(left, Qt::AlignHCenter | Qt::AlignJustify | Qt::AlignAbsolute,
               QStringLiteral("*"));
  }
  p.drawText(left, Qt::AlignHorizontal_Mask, ahploss);
  p.drawImage(right, dimg);
  p.drawText(right, Qt::AlignHorizontal_Mask, dhploss);
  p.end();
}

/**
   Mouse press event, centers on highlighted combat
 */
void hud_unit_combat::mousePressEvent(QMouseEvent *e)
{
  center_tile_mapcanvas(center_tile);
}

/**
   Leave event for hud unit combat. Stops showing highlight.
 */
void hud_unit_combat::leaveEvent(QEvent *event)
{
  focus = false;
  update();
}

/**
   Leave event for hud unit combat. Shows highlight.
 */
void hud_unit_combat::enterEvent(QEvent *event)
{
  focus = true;
  update();
}

/**
   Hud battle log contructor
 */
hud_battle_log::hud_battle_log(QWidget *parent) : QWidget(parent)
{
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  main_layout = new QVBoxLayout(this);
  mw = new move_widget(this);
  setContentsMargins(0, 0, 0, 0);
  main_layout->setContentsMargins(0, 0, 0, 0);
  sw = new scale_widget(QRubberBand::Rectangle, this);
  sw->show();
  scale = 1.0;
}

/**
   Hud battle log destructor
 */
hud_battle_log::~hud_battle_log()
{
  delete sw;
  delete mw;
}

/****************************************************************************
  Updates size when scale has changed
****************************************************************************/
void hud_battle_log::update_size()
{
  int w = 3 * tileset_unit_height(tileset) / 2 * scale;

  king()->qt_settings.battlelog_scale = scale;
  delete layout();
  main_layout = new QVBoxLayout;
  for (auto *hudc : qAsConst(lhuc)) {
    hudc->set_scale(scale);
    main_layout->addWidget(hudc);
    hudc->set_fading(1.0);
  }
  setFixedSize(2 * w + 10, lhuc.count() * w + 10);
  setLayout(main_layout);

  update();
  show();
  m_timer.restart();
  startTimer(50);
}

/****************************************************************************
  Set scale
****************************************************************************/
void hud_battle_log::set_scale(float s)
{
  scale = s;
  sw->scale = s;
}

/**
   Adds comabt information to battle log
 */
void hud_battle_log::add_combat_info(hud_unit_combat *huc)
{
  hud_unit_combat *hudc;
  int w = 3 * tileset_unit_height(tileset) / 2 * scale;

  delete layout();
  main_layout = new QVBoxLayout;
  lhuc.prepend(huc);
  while (lhuc.count() > 5) {
    hudc = lhuc.takeLast();
    delete hudc;
  }
  for (auto *hudc : qAsConst(lhuc)) {
    main_layout->addWidget(hudc);
    hudc->set_fading(1.0);
  }
  setFixedSize(2 * w + 10, lhuc.count() * w + 10);
  setLayout(main_layout);

  update();
  show();
  m_timer.restart();
  startTimer(50);
}

/**
   Paint event for hud battle log
 */
void hud_battle_log::paintEvent(QPaintEvent *event)
{
  if (scale != sw->scale) {
    scale = sw->scale;
    update_size();
  }
  mw->put_to_corner();
  sw->move(width() - sw->width(), 0);
}

/**
   Move event, saves current position
 */
void hud_battle_log::moveEvent(QMoveEvent *event)
{
  QPoint p;

  p = pos();
  king()->qt_settings.battlelog_x =
      static_cast<float>(p.x()) / mapview.width;
  king()->qt_settings.battlelog_y =
      static_cast<float>(p.y()) / mapview.height;
  m_timer.restart();
}

/**
   Timer event. Starts/stops fading
 */
void hud_battle_log::timerEvent(QTimerEvent *event)
{
  if (m_timer.elapsed() > 4000 && m_timer.elapsed() < 5000) {
    for (auto *hudc : qAsConst(lhuc)) {
      if (hudc->get_focus()) {
        m_timer.restart();
        for (auto *hupdate : qAsConst(lhuc)) {
          hupdate->set_fading(1.0);
        }
        return;
      }
      hudc->set_fading((5000.0 - m_timer.elapsed()) / 1000);
    }
  }
  if (m_timer.elapsed() >= 5000) {
    hide();
  }
}

/**
   Show event, restart fading timer
 */
void hud_battle_log::showEvent(QShowEvent *event)
{
  for (auto *hupdate : qAsConst(lhuc)) {
    hupdate->set_fading(1.0);
  }
  m_timer.restart();
  setVisible(true);
}
