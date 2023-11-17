// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include <QAbstractSlider>

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
 *
 * If space allows, one icon is used to represent one item.
 */
class multi_slider: public QAbstractSlider
{
  Q_OBJECT

  struct category
  {
    QString name;
    QPixmap icon;
    unsigned minimum = 0, maximum = -1;
  };

public:
  explicit multi_slider(QWidget *parent = nullptr);
  virtual ~multi_slider() = default;

  std::size_t add_category(const QString &name, const QPixmap &icon);
  void set_range(std::size_t category, unsigned min, unsigned max);

  void set_values(const std::vector<unsigned> &values);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  std::size_t total() const;

protected:
  bool event(QEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  std::vector<unsigned> visible_handles() const;
  bool move_handle_left();
  bool move_handle_right();

  // Invariant: m_categories.size() == m_handles.size()
  std::vector<category> m_categories;
  std::vector<unsigned> m_values;
  unsigned m_total; // Cached
  std::size_t m_active_handle = 0;
};

} // namespace freeciv
