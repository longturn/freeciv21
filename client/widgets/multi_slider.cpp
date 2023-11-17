// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "widgets/multi_slider.h"

#include "log.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace {
/// Colors
namespace colors {
/// Focus indicator (thin line under one of the categories)
const QColor focus_indicator = Qt::gray;
/// Background of the handles
const QColor handle_background = Qt::lightGray;
/// Small dot on the handles
const QColor handle_indicator = Qt::gray;
/// Small dot on the handles, when the handle is hovered
const QColor handle_hover = Qt::darkGray;
/// Small dot on the handles, when the handle is being dragged
const QColor handle_dragged = handle_hover;
} // namespace colors
/// Widget dimensions, in logical pixels
namespace metrics {
/// Gap between the icons and the focus indicator
const double focus_bar_gap = 3;
/// Height of the focus indicator
const double focus_bar_height = 1;
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

/**
 * \class multi_slider
 * \brief A widget that lets the user distribute a fixed number of items
 * across multiple categories.
 *
 * This widget provides a slider with multiple handles. The width of the
 * slider represents a number of items that the user can distribute across
 * multiple categories. For instance, the items could be citizens that would
 * be distributed to perform various tasks.
 *
 * The widget needs an icon for each category. The icon should represent a
 * single item in the category. When possible, the widget displays each item
 * using one complete icon. It is important that all icons be of the same
 * size.
 *
 * Categories are initially added with \ref add_category. A minimum and
 * maximum number of items in each category can optionally be set with \ref
 * set_range. The displayed values are set using \ref set_values and
 * recovered with \ref values. The signal \ref values_changed is emitted each
 * time the user redistributes items (which can be quite frequent).
 *
 * Users can interact with this widget using the keyboard or the mouse, with
 * interaction patterns optimized for each device. When using the keyboard,
 * the user can navigate between categories using the left and right arrow
 * keys, and add or remove items to the current category using the up and
 * down arrows. Of course, in doing so they also modify other categories. The
 * current category is indicated with a slight underline and is also
 * integrated in tab navigation.
 *
 * When using the mouse, the user can grab handles shown between categories
 * and drag them wherever they want to adjust the number of items. It is also
 * possible to double-click, which moves the closest handle to the location
 * pointed to by the mouse.
 *
 * It is a good idea to have a legend explaining what the icons mean next to
 * this widget, as it is not self-explanatory.
 *
 * \internal
 * Handles are represented in two ways in the implementation:
 * * Sometimes we use an explicit <tt>struct handle</tt>;
 * * Sometimes we use a simple integer (index).
 * The index refers to the two categories between which the handle sits. When
 * some categories are hidden because no items are assigned to them, the
 * handles overlap and some of them are hidden. The <tt>struct handle</tt> is
 * used only for handles displayed on the screen.
 */

/**
 * \brief Constructor.
 */
multi_slider::multi_slider(QWidget *parent) : QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed,
                            QSizePolicy::Slider));
}

/**
 * \brief Adds a category.
 * \param icon An icon representing a single item in the category. All icons
 *             must have the same size.
 * \returns The index of the new category.
 */
std::size_t multi_slider::add_category(const QPixmap &icon)
{
  m_categories.push_back({icon});
  m_values.push_back(0);

  if (icon.size() != m_categories.front().icon.size()) {
    qWarning() << "Inconsistent icon sizes:" << icon.size() << "and"
               << m_categories.front().icon.size();
  }

  return m_categories.size() - 1;
}

/**
 * \brief Sets the minimum and maximum number of items a category can have.
 *
 * By default the minimum is zero and the maximum is very large.
 *
 * \param category The index of the category to modify.
 * \param min The smallest allowed value, may not be smaller than zero.
 * \param max The largest allowed value.
 */
void multi_slider::set_range(std::size_t category, int min, int max)
{
  fc_assert_ret(category < m_categories.size());
  fc_assert_ret(min >= 0);
  fc_assert_ret(min <= max);

  m_categories[category].minimum = min;
  m_categories[category].maximum = max;

  update_cached_geometry();
  updateGeometry();
}

/**
 * \brief Sets the contents of all item categories.
 *
 * \note It is the user's responsibility to ensure that min/max constraints
 * are satisfied.
 */
void multi_slider::set_values(const std::vector<int> &values)
{
  fc_assert_ret(values.size() == m_categories.size());

  m_values = values;
  update_cached_geometry();
  updateGeometry();

  emit values_changed(values);
}

/**
 * \brief Returns the total number of items controlled by this widget.
 */
std::size_t multi_slider::total() const
{
  return std::accumulate(m_values.begin(), m_values.end(), 0);
}

/**
 * \brief Preferred size of the widget.
 *
 * The width is the icon width times the number of items plus extra space for
 * handles, the height is the icon height plus space for handles.
 */
QSize multi_slider::sizeHint() const
{
  if (m_categories.empty()) {
    return QSize();
  }

  auto icon_size = m_categories.front().icon.size();
  return QSize(total() * icon_size.width() + 2 * metrics::handle_radius,
               icon_size.height() + metrics::extra_height);
}

/**
 * \brief Minimum size of the widget.
 *
 * The width is 5 times the number of items plus extra space for handles, the
 * height is the icon height plus space for handles.
 */
QSize multi_slider::minimumSizeHint() const
{
  if (m_categories.empty()) {
    return QSize();
  }

  auto icon_size = m_categories.front().icon.size();
  return QSize(total() * 5 + 2 * metrics::handle_radius,
               icon_size.height() + metrics::extra_height);
}

/**
 * \brief Overrides tab handling to also cycle through visible categories.
 */
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

/**
 * \brief Focuses the first or last category when focus is gained with the
 *        keyboard.
 */
void multi_slider::focusInEvent(QFocusEvent *event)
{
  if (!m_categories.empty()) {
    if (event->reason() == Qt::BacktabFocusReason) {
      m_focused_category = m_categories.size() - 1;
    } else if (event->reason() == Qt::TabFocusReason) {
      m_focused_category = 0;
    } else {
      // Keep old category alive if still present, making sure it's >= 0
      m_focused_category = std::max(m_focused_category, 0);
    }
  }
  QWidget::focusInEvent(event);
}

/**
 * \brief Handles arrow keys: left/right to change the focused category,
 * up/down to add or remove items.
 */
void multi_slider::keyPressEvent(QKeyEvent *event)
{
  if (m_categories.empty()) {
    return;
  }

  if (event->modifiers() == Qt::NoModifier) {
    switch (event->key()) {
    case Qt::Key_Up:
      if (grab_item(m_focused_category, 1)) {
        emit values_changed(values());
        event->accept();
        return;
      }
      break;
    case Qt::Key_Down:
      if (grab_item(m_focused_category, -1)) {
        emit values_changed(values());
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

/**
 * \brief Sopts highlighting the closest handle.
 */
void multi_slider::leaveEvent(QEvent *event)
{
  m_closest_handle = -1;
  update();
  QWidget::leaveEvent(event);
}

/**
 * \brief Moves the closest handle when double-clicking.
 */
void multi_slider::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->buttons() == Qt::LeftButton
      && event->modifiers() == Qt::NoModifier) {
    // Double click
    move_handle(handle_near(event->pos()), event->pos());
  }
  QWidget::mouseMoveEvent(event);
}

/**
 * \brief Moves the current handle when dragging the mouse.
 */
void multi_slider::mouseMoveEvent(QMouseEvent *event)
{
  if (m_dragged_handle >= 0 && event->buttons() == Qt::LeftButton
      && event->modifiers() == Qt::NoModifier) {
    // Drag
    move_handle(m_dragged_handle, event->pos());
  }

  // Update the closest handle
  auto new_handle = handle_near(event->pos());
  if (new_handle != m_closest_handle) {
    m_closest_handle = new_handle;
    update();
  }

  QWidget::mouseMoveEvent(event);
}

/**
 * \brief Sets the current handle when pressing a mouse button.
 */
void multi_slider::mousePressEvent(QMouseEvent *event)
{
  if (event->buttons() == Qt::LeftButton
      && event->modifiers() == Qt::NoModifier) {
    m_dragged_handle = handle_near(event->pos());
    update();
  }
  QWidget::mousePressEvent(event);
}

/**
 * \brief Unsets the current handle when releasing a mouse button.
 */
void multi_slider::mouseReleaseEvent(QMouseEvent *event)
{
  m_dragged_handle = -1;
  update();
  QWidget::mouseReleaseEvent(event);
}

/**
 * \brief Draws the widget.
 */
void multi_slider::paintEvent(QPaintEvent *event)
{
  if (m_categories.empty() || total() <= 0) {
    return;
  }

  // Assume all icons have the same width
  const auto iheight = m_categories.front().icon.height();

  // Center everything
  QPainter p(this);
  p.translate(m_geom.left_margin, 0);

  // Draw icons
  p.setPen(Qt::NoPen);
  p.setRenderHint(QPainter::Antialiasing);
  double xmin = 0, xmax = 0;
  for (std::size_t i = 0; i < m_values.size(); ++i) {
    xmax += m_values[i] * m_geom.item_width;
    p.drawTiledPixmap(QRectF(xmin, 0, xmax - xmin, iheight),
                      m_categories[i].icon, QPointF(xmin, 0));

    // Focus indicator
    if (hasFocus() && i == m_focused_category) {
      p.setBrush(colors::focus_indicator);
      p.drawRect(QRectF(xmin, iheight + metrics::focus_bar_gap, xmax - xmin,
                        metrics::focus_bar_height));
    }
    xmin = xmax;
  }

  // Draw handles (skipping the dummy last one)
  auto handles = visible_handles();
  for (auto h : handles) {
    auto x = m_geom.item_width * h.location;

    // Background
    p.setBrush(colors::handle_background);
    p.drawRect(QRectF(x - metrics::handle_bar_width / 2,
                      metrics::handle_bar_gap, metrics::handle_bar_width,
                      iheight + metrics::handle_gap));
    p.drawEllipse(QPointF(x, iheight + metrics::handle_gap
                                 + metrics::handle_radius - 1),
                  metrics::handle_radius, metrics::handle_radius);

    // Active handle indicator
    bool is_closest = h.index == m_closest_handle;
    bool is_dragged = h.index == m_dragged_handle;
    double inner_radius = is_dragged
                              ? metrics::handle_active_indicator_radius
                              : metrics::handle_indicator_radius;
    p.setBrush(is_dragged   ? colors::handle_dragged
               : is_closest ? colors::handle_hover
                            : colors::handle_indicator);
    p.drawEllipse(QPointF(x, iheight + metrics::handle_gap
                                 + metrics::handle_radius - 1),
                  inner_radius, inner_radius);
  }
}

/**
 * \brief Updates cached geometry information.
 */
void multi_slider::resizeEvent(QResizeEvent *event)
{
  update_cached_geometry();
}

/**
 * \brief Exchange items between two categories.
 * \warning This is a low-level function that doesn't check anything.
 */
void multi_slider::exchange(std::size_t giver, std::size_t taker, int amount)
{
  m_values[giver] -= amount;
  m_values[taker] += amount;
  focus_some_category();
  update();
}

/**
 * \brief Grab an item from elsewhere and adds it to the @c taker category.
 * \param taker Index of the category to add an item to.
 * \param amount -1 to give an item away instead.
 * \param from_left Allows taking items from (or giving them to) categories
 * on the left of \c taker. \param from_right Allows taking items from (or
 * giving them to) categories on the right of \c taker. \return Whether an
 * item could be found.
 */
bool multi_slider::grab_item(std::size_t taker, int amount, bool from_left,
                             bool from_right)
{
  fc_assert_ret_val(taker < m_categories.size(), false);

  const auto &category = m_categories[taker];
  if (!category.allowed(m_values[taker] + amount)) {
    return false;
  }

  // Find category to exchange with. First look to the right...
  if (from_right) {
    for (int i = taker + 1; i < m_categories.size(); ++i) {
      if (m_categories[i].allowed(m_values[i] - amount)) {
        exchange(i, taker, amount);
        return true;
      }
    }
  }

  // No luck to the right. Try on the other side
  if (from_left) {
    for (int i = taker - 1; i >= 0; --i) {
      if (m_categories[i].allowed(m_values[i] - amount)) {
        exchange(i, taker, amount);
        return true;
      }
    }
  }

  // No luck
  return false;
}

/**
 * \brief Makes sure the focused category is a visible one.
 */
void multi_slider::focus_some_category()
{
  if (m_categories.empty() || m_values[m_focused_category] > 0) {
    // Already good
    return;
  }

  // One of them will always succeed
  if (!move_focus(true)) {
    move_focus(false);
  }
}

/**
 * \brief Moves focus to the next or previous visible category.
 * \param forward Whether to move focus to the right (\c true) or to the left
 *                (\c false).
 * \returns True if a valid category is now focused.
 */
bool multi_slider::move_focus(bool forward)
{
  int step = forward ? 1 : -1;
  // Check if focus can be moved to the next visible category
  for (int i = m_focused_category + step; i >= 0 && i < m_categories.size();
       i += step) {
    if (m_values[i] > 0) {
      m_focused_category = i;
      update();
      return true;
    }
  }
  return false;
}

/**
 * \brief Finds the index of the handle closest to the given position.
 */
int multi_slider::handle_near(const QPoint &where)
{
  const auto handles = visible_handles();
  const auto handle_x = [this](const handle &h) {
    return m_geom.left_margin + h.location * m_geom.item_width;
  };
  const auto best_handle =
      std::min_element(handles.begin(), handles.end(),
                       [where, handle_x](const handle &a, const handle &b) {
                         return std::abs(where.x() - handle_x(a))
                                < std::abs(where.x() - handle_x(b));
                       });
  return best_handle->index;
}

/**
 * \brief Tries to move a handle closer to a given position.
 * \param handle The index of the handle to move.
 * \param where The location where to move it.
 * \returns True one success.
 */
bool multi_slider::move_handle(int handle, const QPoint &where)
{
  // Target location of the handle
  int target =
      std::round((where.x() - m_geom.left_margin) / m_geom.item_width);

  // Current location of the handle
  int current =
      std::accumulate(m_values.begin(), m_values.begin() + handle + 1, 0);

  // Direction in which we move the handle
  bool moving_left = current > target;

  // Category gaining items
  int taker = moving_left ? handle + 1 : handle;

  // Try to transfer items to the taker
  for (int i = 0; i < std::abs(current - target); ++i) {
    // grab_item works in units of 1 item
    if (!grab_item(taker, 1, moving_left, !moving_left)) {
      // Nothing more we can do to move the handle in this direction
      return false;
    }
  }
  emit values_changed(values());
  return true;
}

/**
 * \brief Updates cached geometry information.
 */
void multi_slider::update_cached_geometry()
{
  const auto icon_size = m_categories.front().icon.size();
  const auto items = total();

  // Safety - we shouldn't be used this way...
  if (items <= 0) {
    m_geom = {1, 0, 1};
    return;
  }

  m_geom.icons_width = items * icon_size.width();
  int total_width = m_geom.icons_width + 2 * metrics::handle_radius;

  // Adjust if we don't have enough space
  if (total_width > width()) {
    int available_width = width() - 2 * metrics::handle_radius;
    int icon_count =
        available_width / icon_size.width(); // Note we round down
    m_geom.icons_width = icon_count * icon_size.width();
  }

  m_geom.left_margin = (width() - m_geom.icons_width) / 2;

  // If we have enough space, this is equal to the width of one icon
  m_geom.item_width = static_cast<double>(m_geom.icons_width) / items;
}

/**
 * \brief Returns the list of all visible handles.
 */
std::vector<multi_slider::handle> multi_slider::visible_handles() const
{
  std::vector<handle> handles;
  bool first = true;
  int location = 0;
  for (int i = 0; i < m_values.size() - 1; ++i) {
    location += m_values[i];
    if (first || m_values[i] > 0) {
      handles.push_back({i, location});
    }
    first = false;
  }
  return handles;
}

} // namespace freeciv
