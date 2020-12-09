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

#pragma once
// Qt
#include <QObject>

class civtimer;
class QTcpServer;
class QTimer;

namespace freeciv {

/// @brief A Freeciv server.
class server : public QObject {
public:
  explicit server();
  virtual ~server();

  bool is_ready() const;

private slots:
  // Low-level stuff
  void error_on_socket();
  void input_on_socket();
  void input_on_stdin();
  void accept_connections();
  void send_pings();

  // Higher-level stuff
  void prepare_game();
  void begin_turn();
  void begin_phase();
  void end_phase();
  void end_turn();
  void update_game_state();
  bool shut_game_down();
  void quit_idle();
  void pulse();

private:
  void init_interactive();

  bool m_interactive = false;
  QObject *m_stdin_notifier = nullptr; // Actual type is OS-dependent

  QTcpServer *m_tcp_server = nullptr;

  civtimer *m_eot_timer = nullptr, *m_between_turns_timer = nullptr;

  bool m_is_new_turn, m_need_send_pending_events, m_skip_mapimg;
  int m_save_counter = 0;

  bool m_someone_ever_connected = false;
  QTimer *m_quitidle_timer = nullptr;

  QTimer *m_pulse_timer = nullptr;
};

} // namespace freeciv

