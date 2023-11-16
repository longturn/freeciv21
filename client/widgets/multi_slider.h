// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include <QWidget>

#include <limits>
#include <vector>

namespace freeciv {

/**
 * \brief A widget that lets the user distribute a fixed number of items across
 *        multiple categories.
 *
 * Assumptions:
 * - Categories are identified by their id
 * - Categories are not added or removed dynamically
 * - No category can have a negative number of items
 * - Category names are translated
 * - Icons for all categories are equally sized
 * - Maximum is exclusive
 *
 * If space allows, one icon is used to represent one item.
 * TODO tooltips
 */
class multi_slider: public QWidget
{
  Q_OBJECT

  struct category
  {
    QString name;
    QPixmap icon;
    int minimum = 0, maximum = std::numeric_limits<int>::max();

    bool allowed(int value) const { return value >= minimum && value < maximum; }
  };

public:
  explicit multi_slider(QWidget *parent = nullptr);
  virtual ~multi_slider() = default;

  std::size_t add_category(const QString &name, const QPixmap &icon);
  void set_range(std::size_t category, int min, int max);

  void set_values(const std::vector<int> &values);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  std::size_t total() const;

protected:
  bool event(QEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  void exchange(std::size_t giver, std::size_t taker, int amount);
  bool exchange(std::size_t taker, int amount);

  void focus_some_category();
  bool move_focus(bool forward);

  std::vector<int> visible_handles() const;

  // Invariant: m_categories.size() == m_handles.size()
  std::vector<category> m_categories;
  std::vector<int> m_values;
  int m_total; // Cached
  int m_focused_category = 0;
};

} // namespace freeciv
