// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "widgets/multi_slider.h"

#include "log.h"

#include <QBrush>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPainter>

#include <algorithm>
#include <qabstractslider.h>

namespace {
  // Handle metrics
  const double handle_bar_gap = 2;
  const double handle_bar_width = 4;
  const double handle_gap = 1;
  const double handle_radius = 8;
  const double handle_indicator_radius = 4;
  const double handle_active_indicator_radius = handle_indicator_radius + 1;
  const double handle_extra_height = handle_gap + handle_radius * 2;
} // anonymous namespace

namespace freeciv {

multi_slider::multi_slider(QWidget *parent): QAbstractSlider(parent)
{
  setFocusPolicy(Qt::StrongFocus);
}

std::size_t multi_slider::add_category(const QString &name, const QPixmap &icon)
{
  m_categories.push_back({name, icon});
  m_values.push_back(0);
  return m_categories.size() - 1;
}

void multi_slider::set_range(std::size_t category, unsigned min, unsigned max)
{
  fc_assert_ret(category < m_categories.size());
  fc_assert_ret(min <= max);
  m_categories[category].minimum = min;
  m_categories[category].maximum = max;
  // TODO modify current values if needed? -- user's responsibility
}

void multi_slider::set_values(const std::vector<unsigned> &values)
{
  fc_assert_ret(values.size() == m_categories.size());
  m_values = values;
  m_total = std::accumulate(m_values.begin(), m_values.end(), 0u);
}

std::size_t multi_slider::total() const
{
  return m_total;
}

QSize multi_slider::sizeHint() const
{
  if (m_categories.empty()) {
    return QSize();
  }

  auto icon_size = m_categories.front().icon.size();
  return QSize(total() * icon_size.width() + 2 * handle_radius,
               icon_size.height() + handle_extra_height);
}

QSize multi_slider::minimumSizeHint() const
{
  if (m_categories.empty()) {
    return QSize();
  }

  auto icon_size = m_categories.front().icon.size();
  return QSize(total() * 5 + 2 * handle_radius,
               icon_size.height() + handle_extra_height);
}

bool multi_slider::event(QEvent *event)
{
  // Allow using Tab and Backtab to move between visible handles
  // We need to trap those early to override the default behaviour
  if (event->type() == QEvent::KeyPress) {
    auto kevt = dynamic_cast<QKeyEvent *>(event);
    if (kevt->key() == Qt::Key_Tab) {
      // Tab - check if focus should be moved to the next handle
      auto handles = visible_handles();
      if (m_active_handle + 1 < handles.size()) {
        m_active_handle++;
        event->accept();
        update();
        return true;
      }
    } else if (kevt->key() == Qt::Key_Backtab) {
      // Backtab - check if focus should be moved to the previous handle
      if (m_active_handle > 0) {
        m_active_handle--;
        event->accept();
        update();
        return true;
      }
    }
  }
  return QAbstractSlider::event(event);
}

void multi_slider::focusInEvent(QFocusEvent *event)
{
  if (m_values.size() > 2) {
    if (event->reason() == Qt::BacktabFocusReason) {
      auto handles = visible_handles();
      m_active_handle = handles.size() - 1;
    } else {
//    if (event->reason() == Qt::TabFocusReason) {
      // TODO mouse focus
      m_active_handle = 0;
    }
  }
  QAbstractSlider::focusInEvent(event);
}

void multi_slider::keyPressEvent(QKeyEvent *event)
{
  if (event->modifiers() == Qt::NoModifier) {
    if (event->key() == Qt::Key_Left && move_handle_left()) {
      event->accept();
      update();
      return;
    } else if (event->key() == Qt::Key_Right && move_handle_right()) {
      event->accept();
      update();
      return;
    }
  }
  QAbstractSlider::keyPressEvent(event);
}

void multi_slider::paintEvent(QPaintEvent *event)
{
  if (m_categories.empty()) {
    return;
  }

  // Assume all icons have the same width
  const auto icon_size = m_categories.front().icon.size();
  const double step_width = std::min<double>(
      icon_size.width(), static_cast<double>(width()) / total());

  // Draw icons
  QPainter p(this);
  double xmin = 0, xmax = 0;
  for (std::size_t i = 0; i < m_values.size(); ++i) {
    xmax += m_values[i] * step_width;
    p.drawTiledPixmap(QRectF(xmin, 0, xmax - xmin, icon_size.height()),
                      m_categories[i].icon,
                      QPointF(xmin, 0));
    xmin = xmax;
  }

  // Draw handles (skipping the dummy last one)
  p.save();
  p.setRenderHint(QPainter::Antialiasing);
  p.setBrush(Qt::lightGray);
  p.setPen(Qt::NoPen);
  auto handles = visible_handles();
  for (auto location: handles) {
    auto x = step_width * location;

    // Background
    p.drawRect(QRectF(x - handle_bar_width / 2, handle_bar_gap,
                      handle_bar_width, icon_size.height() + handle_gap));
    p.drawEllipse(QPointF(x, icon_size.height() + handle_gap + handle_radius - 1),
                  handle_radius, handle_radius);

    // Active handle indicator
    bool is_active = hasFocus() && location == handles[m_active_handle];
    double inner_radius = is_active ? handle_active_indicator_radius
                                    : handle_indicator_radius;
    p.setBrush(is_active ? Qt::red : Qt::darkGray);
    p.drawEllipse(QPointF(x, icon_size.height() + handle_gap + handle_radius - 1),
                  inner_radius, inner_radius);
    p.setBrush(Qt::lightGray);
  }
  p.restore();
}

std::vector<unsigned> multi_slider::visible_handles() const
{
  std::vector<unsigned> handles;
  bool first = true;
  unsigned location = 0;
  for (auto it = m_values.begin(); it != m_values.end() - 1; ++it) {
    location += *it;
    if (first || *it > 0) {
      handles.push_back(location);
    }
    first = false;
  }
  return handles;
}

bool multi_slider::move_handle_left()
{
  auto handles = visible_handles();
  auto handle_location = handles[m_active_handle];
  if (handle_location == 0) {
    return false;
  }

  // Find categories to modify (starting from the left)
  auto location = 0;
  for (auto it = m_values.begin(); it != m_values.end(); ++it) {
    location += *it;
    if (location == handle_location) {
      // Found
      if (*it == 1) {
        m_active_handle--;
      }
      (*it)--;
      (*next(it))++;
      return true;
    }
  }
  return false;
}

bool multi_slider::move_handle_right()
{
  auto handles = visible_handles();
  auto handle_location = handles[m_active_handle];
  if (handle_location == m_total) {
    return false;
  }

  // Find categories to modify (starting from the right)
  auto location = m_total;
  for (auto it = m_values.rbegin(); it != m_values.rend(); ++it) {
    location -= *it;
    if (location == handle_location) {
      // Found
      (*it)--;
      if (*next(it) == 0) {
        m_active_handle++;
      }
      (*next(it))++;
      return true;
    }
  }
  return false;
}

} // namespace freeciv
