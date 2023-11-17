// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include <QWidget>

#include <limits>
#include <vector>

namespace freeciv {

class multi_slider : public QWidget {
  Q_OBJECT

  struct category {
    QPixmap icon;
    int minimum = 0, maximum = std::numeric_limits<int>::max();

    /// Checks if the category could take some value
    bool allowed(int value) const
    {
      return value >= minimum && value <= maximum;
    }
  };

  struct handle {
    int index;
    int location;
  };

public:
  explicit multi_slider(QWidget *parent = nullptr);
  virtual ~multi_slider() = default;

  std::size_t add_category(const QPixmap &icon);
  void set_range(std::size_t category, int min, int max);

  /// Retrieves the number of items in each category.
  std::vector<int> values() const { return m_values; }
  void set_values(const std::vector<int> &values);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  std::size_t total() const;

signals:
  void values_changed(const std::vector<int> &values) const;

protected:
  bool event(QEvent *event) override;

  void focusInEvent(QFocusEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void exchange(std::size_t giver, std::size_t taker, int amount);
  bool grab_item(std::size_t taker, int amount, bool from_left = true,
                 bool from_right = true);

  void focus_some_category();
  bool move_focus(bool forward);

  int handle_near(const QPoint &where);
  bool move_handle(int handle, const QPoint &where);

  void update_cached_geometry();
  std::vector<handle> visible_handles() const;

  /// Category data
  std::vector<category> m_categories;
  // Invariant: m_categories.size() == m_handles.size()

  /// Number of items in each category
  std::vector<int> m_values;

  /// Index of the category receiving keyboard input
  int m_focused_category = 0;

  /// Index of the handle being dragged with the mouse
  int m_closest_handle = -1;
  int m_dragged_handle = -1;

  /// Cached geometry information
  struct {
    int icons_width = 1;   ///< Width of the area covered with icons
    int left_margin = 0;   ///< Empty space left of the icons
    double item_width = 1; ///< The logical width of one item
  } m_geom;
};

} // namespace freeciv
