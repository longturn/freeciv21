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
/// Widget dimensions, in logical pixels
namespace metrics {
  /// Gap between the icons and the focus indicator
  const double focus_bar_gap = 1;
  /// Height of the focus indicator
  const double focus_bar_height = 2;
  /// Gap at the top of the handle bar
  const double handle_bar_gap = 2;
  /// Width of the handle bar
  const double handle_bar_width = 4;
  /// Gap between the icons and the handle (circle)
  const double handle_gap = 1;
  /// Radius of the handle
  const double handle_radius = 8;
  /// Radius of the small disk inside the handle
  const double handle_indicator_radius = 4;
  /// Radius of the small disk inside the handle, when the handle is active
  const double handle_active_indicator_radius = handle_indicator_radius + 1;
  /// Height added to the icon height by control elements (handle etc)
  const double extra_height = std::max(focus_bar_gap + focus_bar_height,
                                       handle_gap + handle_radius * 2);
} // namespace metrics
} // anonymous namespace

namespace freeciv {

multi_slider::multi_slider(QWidget *parent): QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed,
                            QSizePolicy::Slider));
  setMouseTracking(true);
}

std::size_t multi_slider::add_category(const QString &name, const QPixmap &icon)
{
  m_categories.push_back({name, icon});
  m_values.push_back(0);
  return m_categories.size() - 1;
}

void multi_slider::set_range(std::size_t category, int min, int max)
{
  fc_assert_ret(category < m_categories.size());
  fc_assert_ret(min <= max);
  m_categories[category].minimum = min;
  m_categories[category].maximum = max;
  // TODO modify current values if needed? -- user's responsibility
}

void multi_slider::set_values(const std::vector<int> &values)
{
  fc_assert_ret(values.size() == m_categories.size());
  m_values = values;
  m_total = std::accumulate(m_values.begin(), m_values.end(), 0);
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
  return QSize(total() * icon_size.width() + 2 * metrics::handle_radius,
               icon_size.height() + metrics::extra_height);
}

QSize multi_slider::minimumSizeHint() const
{
  if (m_categories.empty()) {
    return QSize();
  }

  auto icon_size = m_categories.front().icon.size();
  return QSize(total() * 5 + 2 * metrics::handle_radius,
               icon_size.height() + metrics::extra_height);
}

bool multi_slider::event(QEvent *event)
{
  // Allow using Tab and Backtab to move between visible categories
  // We need to trap those early to override the default behaviour
  if (event->type() == QEvent::KeyPress) {
    auto kevt = dynamic_cast<QKeyEvent *>(event);
    // Check if focus can be moved to the next visible category
    if (kevt->key() == Qt::Key_Tab && move_focus(true)) {
      event->accept();
      return true;
    } else if (kevt->key() == Qt::Key_Backtab && move_focus(false)) {
      event->accept();
      return true;
    }
  }
  return QWidget::event(event);
}

void multi_slider::focusInEvent(QFocusEvent *event)
{
  if (m_values.size() > 2) {
    if (event->reason() == Qt::BacktabFocusReason) {
      m_focused_category = m_categories.size() - 1;
    } else {
//    if (event->reason() == Qt::TabFocusReason) {
      // TODO mouse focus
      m_focused_category = 0;
    }
  }
  QWidget::focusInEvent(event);
}

void multi_slider::keyPressEvent(QKeyEvent *event)
{
  if (event->modifiers() == Qt::NoModifier) {
    switch (event->key()) {
    case Qt::Key_Up:
      if (exchange(m_focused_category, 1)) {
        event->accept();
        return;
      }
      break;
    case Qt::Key_Down:
      if (exchange(m_focused_category, -1)) {
        event->accept();
        return;
      }
      break;
    case Qt::Key_Left:
      if (move_focus(false)) {
        event->accept();
        return;
      }
      break;
    case Qt::Key_Right:
      if (move_focus(true)) {
        event->accept();
        return;
      }
      break;
    default:
      break;
    }
  }
  QWidget::keyPressEvent(event);
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
  p.setPen(Qt::NoPen);
  p.setRenderHint(QPainter::Antialiasing);

  double xmin = 0, xmax = 0;
  for (std::size_t i = 0; i < m_values.size(); ++i) {
    xmax += m_values[i] * step_width;
    p.drawTiledPixmap(QRectF(xmin, 0, xmax - xmin, icon_size.height()),
                      m_categories[i].icon,
                      QPointF(xmin, 0));

    // Focus indicator
    if (hasFocus() && i == m_focused_category) {
      p.setBrush(Qt::white);
      p.drawRect(QRectF(xmin, icon_size.height() + metrics::focus_bar_gap,
                        xmax - xmin, metrics::focus_bar_height));
    }
    xmin = xmax;
  }

  // Draw handles (skipping the dummy last one)
  p.setBrush(Qt::lightGray);
  auto handles = visible_handles();
  for (auto location: handles) {
    auto x = step_width * location;

    // Background
    p.drawRect(QRectF(x - metrics::handle_bar_width / 2, metrics::handle_bar_gap,
                      metrics::handle_bar_width, icon_size.height() + metrics::handle_gap));
    p.drawEllipse(QPointF(x, icon_size.height() + metrics::handle_gap + metrics::handle_radius - 1),
                  metrics::handle_radius, metrics::handle_radius);

    // Active handle indicator
    bool is_active = false; // FIXME mouse
    double inner_radius = is_active ? metrics::handle_active_indicator_radius
                                    : metrics::handle_indicator_radius;
    p.setBrush(is_active ? Qt::red : Qt::darkGray);
    p.drawEllipse(QPointF(x, icon_size.height() + metrics::handle_gap + metrics::handle_radius - 1),
                  inner_radius, inner_radius);
    p.setBrush(Qt::lightGray);
  }
}

void multi_slider::exchange(std::size_t giver, std::size_t taker, int amount)
{
  m_values[giver] -= amount;
  m_values[taker] += amount;
  focus_some_category();
  update();
}

bool multi_slider::exchange(std::size_t taker, int amount)
{
  const auto &category = m_categories[m_focused_category];
  if (!category.allowed(m_values[m_focused_category] + amount)) {
    return false;
  }

  // Find category to exchange with. First look to the right...
  for (int i = m_focused_category + 1; i < m_categories.size(); ++i) {
    if (m_categories[i].allowed(m_values[i] - amount)) {
      exchange(i, m_focused_category, amount);
      return true;
    }
  }

  // No luck to the right. Try on the other side
  for (int i = m_focused_category - 1; i >= 0; --i) {
    if (m_categories[i].allowed(m_values[i] - amount)) {
      exchange(i, m_focused_category, amount);
      return true;
    }
  }

  // No luck
  return false;
}

void multi_slider::focus_some_category()
{
  if (m_values[m_focused_category] > 0) {
    // Already good
    return;
  }

  // One of them will always succeed
  if (!move_focus(true)) {
    move_focus(false);
  }
}

bool multi_slider::move_focus(bool forward)
{
  int step = forward ? 1 : -1;
  // Check if focus can be moved to the next visible category
  for (int i = m_focused_category + step; i >= 0 && i < m_categories.size(); i += step) {
    if (m_values[i] > 0) {
      m_focused_category = i;
      update();
      return true;
    }
  }
  return false;
}

std::vector<int> multi_slider::visible_handles() const
{
  std::vector<int> handles;
  bool first = true;
  int location = 0;
  for (auto it = m_values.begin(); it != m_values.end() - 1; ++it) {
    location += *it;
    if (first || *it > 0) {
      handles.push_back(location);
    }
    first = false;
  }
  return handles;
}

} // namespace freeciv
