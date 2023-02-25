// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "widgets/city/governor_widget.h"

// client
#include "fc_types.h"
#include "governor.h"

// common
#include "cm.h"

// Qt
#include <QSlider>

namespace freeciv {

/**
 * \class governor_widget
 * A widget that lets the user edit governor settings.
 */

/**
 * Constructor.
 */
governor_widget::governor_widget(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);

  const auto labels = {
      ui.food_surplus_label,         ui.food_priority_label,
      ui.production_surplus_label,   ui.production_priority_label,
      ui.trade_surplus_label,        ui.trade_priority_label,
      ui.gold_surplus_label,         ui.gold_priority_label,
      ui.luxury_goods_surplus_label, ui.luxury_goods_priority_label,
      ui.science_surplus_label,      ui.science_priority_label,
      ui.celebrate_priority_label,
  };
  for (auto label : labels) {
    label->setNum(0);
  }

  const auto sliders = {
      ui.food_surplus,          ui.food_priority,   ui.production_surplus,
      ui.production_priority,   ui.trade_surplus,   ui.trade_priority,
      ui.gold_surplus,          ui.gold_priority,   ui.luxury_goods_surplus,
      ui.luxury_goods_priority, ui.science_surplus, ui.science_priority,
      ui.celebrate_priority,
  };
  for (auto slider : sliders) {
    connect(slider, &QSlider::valueChanged, this,
            &governor_widget::emit_params_changed);
  }

  const auto checkboxes = {
      ui.celebrate_surplus,
      ui.optimize_growth,
      ui.allow_disorder,
      ui.allow_specialists,
  };
  for (auto box : checkboxes) {
    connect(box, &QCheckBox::toggled, this,
            &governor_widget::emit_params_changed);
  }
}

/**
 * Returns the parameters currently shown by the widget.
 */
cm_parameter governor_widget::parameters() const
{
  auto params = cm_parameter();

#define get_output(name, O_TYPE)                                            \
  params.minimal_surplus[O_TYPE] = ui.name##_surplus->value();              \
  params.factor[O_TYPE] = ui.name##_priority->value();
  get_output(food, O_FOOD);
  get_output(production, O_SHIELD);
  get_output(trade, O_TRADE);
  get_output(gold, O_GOLD);
  get_output(luxury_goods, O_LUXURY);
  get_output(science, O_SCIENCE);
#undef get_output

  params.require_happy = ui.celebrate_surplus->isChecked();
  params.happy_factor = ui.celebrate_priority->value();

  params.max_growth = ui.optimize_growth->isChecked();
  params.allow_disorder = ui.allow_disorder->isChecked();
  params.allow_specialists = ui.allow_specialists->isChecked();

  return params;
}

/**
 * Changes the parameters displayed by this widget.
 */
void governor_widget::set_parameters(const cm_parameter &params)
{
  // Labels are updated automatically through signal connections

#define sync_output(name, O_TYPE)                                           \
  ui.name##_surplus->setValue(params.minimal_surplus[O_TYPE]);              \
  ui.name##_priority->setValue(params.factor[O_TYPE]);
  sync_output(food, O_FOOD);
  sync_output(production, O_SHIELD);
  sync_output(trade, O_TRADE);
  sync_output(gold, O_GOLD);
  sync_output(luxury_goods, O_LUXURY);
  sync_output(science, O_SCIENCE);
#undef sync_output

  ui.celebrate_surplus->setChecked(params.require_happy);
  ui.celebrate_priority->setValue(params.happy_factor);

  ui.optimize_growth->setChecked(params.max_growth);
  ui.allow_disorder->setChecked(params.allow_disorder);
  ui.allow_specialists->setChecked(params.allow_specialists);
}

/**
 * \fn governor_widget::parameters_changed
 * Signal emitted when the governor settings are changed. Note that this may
 * be emitted very often.
 */

/**
 * Helper to fill the argument of \ref parameters_changed.
 */
void governor_widget::emit_params_changed()
{
  emit parameters_changed(parameters());
}

} // namespace freeciv
