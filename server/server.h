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

#include <atomic>

#ifdef Q_OS_WIN
#include <QRecursiveMutex>
#include <QThread>
#endif // Q_OS_WIN

class civtimer;
class QTcpServer;
class QTimer;

namespace freeciv {

#ifdef Q_OS_WIN
namespace detail {
/**
 * Worker thread from which blocking calls to stdin are made.
 */
class async_readline_wrapper final : public QThread {
  Q_OBJECT
public:
  async_readline_wrapper(bool interactive, QObject *parent = nullptr);

  void stop();

public slots:
  void wait_for_input();

signals:
  void line_available(const QString &line);

private:
  bool m_interactive; // Whether to use readline.
  std::atomic<bool> m_stop;
};
} // namespace detail
#endif

/// @brief A Freeciv21 server.
class server : public QObject {
  Q_OBJECT
public:
  explicit server();
  ~server() override;

  bool is_ready() const;

private slots:
  // Low-level stuff
  void error_on_socket();
  void input_on_socket();
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

#ifdef Q_OS_WIN
  // On Windows, we need a ton of additional functions to work around the
  // lack of (constistent) notification system for stdin.  Native Windows
  // mechanisms work for either the console, named pipes, or files, but not
  // all of them at once.  So we resort to spawning a thread and making
  // blocking calls, for which we need additional synchronization.
signals:
  void input_requested();
private slots:
  void input_on_stdin(const QString &line);

private:
  static QRecursiveMutex s_stdin_mutex;
  static char **synchronized_completion(const char *text, int start,
                                        int end);
#else  // !Q_OS_WIN
private slots:
  void input_on_stdin();
#endif // Q_OS_WIN

private:
  bool m_ready = false;

  bool m_interactive = false;
  QObject *m_stdin_notifier = nullptr; // Actual type is OS-dependent

  QTcpServer *m_tcp_server = nullptr;

  civtimer *m_eot_timer = nullptr, *m_between_turns_timer = nullptr;

  bool m_is_new_turn{false}, m_need_send_pending_events{false},
      m_skip_mapimg{false};
  int m_save_counter = 0;

  bool m_someone_ever_connected = false;
  QTimer *m_quitidle_timer = nullptr;

  QTimer *m_pulse_timer = nullptr;
};

} // namespace freeciv
