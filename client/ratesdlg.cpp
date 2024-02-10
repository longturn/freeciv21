/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "ratesdlg.h"
// Qt
#include <QApplication>
#include <QGroupBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>
// common
#include "effects.h"
#include "fc_types.h"
#include "government.h"
#include "multipliers.h"
#include "packets.h"
// client
#include "client_main.h"
#include "dialogs.h"
#include "fc_client.h"
#include "icons.h"
#include "tileset/tilespec.h"
#include "widgets/multi_slider.h"

static int scale_to_mult(const struct multiplier *pmul, int scale);
static int mult_to_scale(const struct multiplier *pmul, int val);

/**
   Dialog constructor for changing rates with sliders.
   Automatic destructor will clean qobjects, so there is no one
 */
national_budget_dialog::national_budget_dialog(QWidget *parent)
    : qfc_dialog(parent)
{
  QHBoxLayout *some_layout;
  QVBoxLayout *main_layout;
  QPushButton *cancel_button;
  QPushButton *ok_button;
  QPushButton *apply_button;
  QLabel *l2;
  QString str;

  setWindowTitle(_("National Budget"));
  main_layout = new QVBoxLayout;

  l2 = new QLabel(_("Select tax, luxury and science rates"));
  l2->setAlignment(Qt::AlignHCenter);
  m_info = new QLabel(str);
  m_info->setAlignment(Qt::AlignHCenter);
  main_layout->addWidget(l2);
  main_layout->addWidget(m_info);
  main_layout->addSpacing(20);

  cancel_button = new QPushButton(_("Cancel"));
  ok_button = new QPushButton(_("Ok"));
  apply_button = new QPushButton(_("Apply"));
  some_layout = new QHBoxLayout;
  connect(cancel_button, &QAbstractButton::pressed, this, &QWidget::close);
  connect(apply_button, &QAbstractButton::pressed, this,
          &national_budget_dialog::apply);
  connect(ok_button, &QAbstractButton::pressed, this,
          &national_budget_dialog::apply);
  connect(ok_button, &QAbstractButton::pressed, this, &QWidget::close);
  some_layout->addWidget(cancel_button);
  some_layout->addWidget(apply_button);
  some_layout->addWidget(ok_button);

  slider = new freeciv::multi_slider;
  main_layout->addWidget(slider);

  main_layout->addSpacing(20);
  main_layout->addLayout(some_layout);
  setLayout(main_layout);
}

/**
 * Refreshes tax rate data
 */
void national_budget_dialog::refresh()
{
  if (!client.conn.playing) {
    // The budget dialog doesn't make sense without a player
    return;
  }

  const int max = get_player_bonus(client.conn.playing, EFT_MAX_RATES);

  // Trans: Government - max rate (of taxes) x%
  m_info->setText(QString(_("%1 - max rate: %2%"))
                      .arg(government_name_for_player(client.conn.playing),
                           QString::number(max)));

  if (!slider_init) {
    for (auto tax : {O_GOLD, O_SCIENCE, O_LUXURY}) {
      auto sprite = get_tax_sprite(tileset, tax);
      slider->add_category(sprite->scaled(sprite->size() * 2));
    }
    slider_init = true;
  }
  slider->set_range(0, 0, max / 10);
  slider->set_range(1, 0, max / 10);
  slider->set_range(2, 0, max / 10);
  slider->set_values({
      client.conn.playing->economic.tax / 10,
      client.conn.playing->economic.science / 10,
      client.conn.playing->economic.luxury / 10,
  });
}

/**
 * Send info to the server.
 */
void national_budget_dialog::apply()
{
  auto rates = slider->values();
  dsend_packet_player_rates(&client.conn,
                            10 * rates[0],  // Tax
                            10 * rates[2],  // Lux
                            10 * rates[1]); // Sci
}

/**
   Multipler rates dialog constructor
   Inheriting from qfc_dialog will cause crash in Qt5.2
 */
multipler_rates_dialog::multipler_rates_dialog(QWidget *parent)
    : QDialog(parent)
{
  QGroupBox *group_box;
  QHBoxLayout *some_layout;
  QLabel *label;
  QSlider *slider;
  QVBoxLayout *main_layout;
  struct player *pplayer = client_player();

  cancel_button = new QPushButton;
  ok_button = new QPushButton;
  setWindowTitle(_("Change policies"));
  main_layout = new QVBoxLayout;

  multipliers_iterate(pmul)
  {
    QHBoxLayout *hb = new QHBoxLayout;
    int val = player_multiplier_target_value(pplayer, pmul);

    group_box = new QGroupBox(multiplier_name_translation(pmul));
    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(mult_to_scale(pmul, pmul->start));
    slider->setMaximum(mult_to_scale(pmul, pmul->stop));
    slider->setValue(mult_to_scale(pmul, val));
    connect(slider, &QAbstractSlider::valueChanged, this,
            &multipler_rates_dialog::slot_set_value);
    slider_list.append(slider);
    label = new QLabel(QString::number(mult_to_scale(pmul, val)));
    hb->addWidget(slider);
    slider->setEnabled(multiplier_can_be_changed(pmul, client_player()));
    hb->addWidget(label);
    group_box->setLayout(hb);
    slider->setProperty("lab", QVariant::fromValue((void *) label));
    main_layout->addWidget(group_box);
  }
  multipliers_iterate_end;
  some_layout = new QHBoxLayout;
  cancel_button->setText(_("Cancel"));
  ok_button->setText(_("Ok"));
  connect(cancel_button, &QAbstractButton::pressed, this,
          &multipler_rates_dialog::slot_cancel_button_pressed);
  connect(ok_button, &QAbstractButton::pressed, this,
          &multipler_rates_dialog::slot_ok_button_pressed);
  some_layout->addWidget(cancel_button);
  some_layout->addWidget(ok_button);
  main_layout->addSpacing(20);
  main_layout->addLayout(some_layout);
  setLayout(main_layout);
}

/**
   Slider value changed
 */
void multipler_rates_dialog::slot_set_value(int i)
{
  Q_UNUSED(i)
  QSlider *qo;
  qo = (QSlider *) QObject::sender();
  QVariant qvar;
  QLabel *lab;

  qvar = qo->property("lab");
  lab = reinterpret_cast<QLabel *>(qvar.value<void *>());
  lab->setText(QString::number(qo->value()));
}

/**
   Cancel pressed
 */
void multipler_rates_dialog::slot_cancel_button_pressed()
{
  close();
  deleteLater();
}

/**
   Ok pressed - send mulipliers value.
 */
void multipler_rates_dialog::slot_ok_button_pressed()
{
  int j = 0;
  int value;
  struct packet_player_multiplier mul;

  multipliers_iterate(pmul)
  {
    Multiplier_type_id i = multiplier_index(pmul);

    value = slider_list.at(j)->value();
    mul.multipliers[i] = scale_to_mult(pmul, value);
    j++;
  }
  multipliers_iterate_end;
  mul.count = multiplier_count();
  send_packet_player_multiplier(&client.conn, &mul);
  close();
  deleteLater();
}

/**
   Convert real multiplier display value to scale value
 */
int mult_to_scale(const struct multiplier *pmul, int val)
{
  return (val - pmul->start) / pmul->step;
}

/**
   Convert scale units to real multiplier display value
 */
int scale_to_mult(const struct multiplier *pmul, int scale)
{
  return scale * pmul->step + pmul->start;
}

/**
   Update multipliers (policies) dialog.
 */
void real_multipliers_dialog_update(void *unused)
{ // PORTME
  Q_UNUSED(unused)
}

/**
   Popups multiplier dialog
 */
void popup_multiplier_dialog()
{
  multipler_rates_dialog *mrd;

  if (!can_client_issue_orders()) {
    return;
  }
  mrd = new multipler_rates_dialog(king()->central_wdg);
  mrd->show();
}
