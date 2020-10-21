/*
 * (c) Copyright 2020 The Freeciv21 contributors
 *
 * This file is part of Freeciv21.
 *
 * Freeciv21 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Freeciv21 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Freeciv21.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FC__SERVER_H
#define FC__SERVER_H

// Qt
#include <QObject>

namespace freeciv {

/// @brief A Freeciv server.
class server : public QObject {
public:
  explicit server();
  virtual ~server();

private slots:
  void input_on_stdin();

private:
  void init_interactive();

  bool m_interactive = false;
  QObject *m_stdin_notifier = nullptr; // Actual type is OS-dependent
};

} // namespace freeciv

#endif // FC__SERVER_H
