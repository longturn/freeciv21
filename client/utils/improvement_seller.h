/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

class QAction;
class QMenu;
class QWidget;

struct city;
struct building;

namespace freeciv {

class improvement_seller {
public:
  improvement_seller(QWidget *parent, int city_id, int improvement_id);

  void operator()();

  static QAction *add_to_menu(QWidget *parent, QMenu *menu, const city *city,
                              int improvement_id);

private:
  int m_city, m_improvement;
  QWidget *m_parent;
};

} // namespace freeciv
