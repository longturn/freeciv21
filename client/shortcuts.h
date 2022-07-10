/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <map>

#include <QDialog>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>

class QDialogButtonBox;
class QLineEdit;
class QVBoxLayout;
struct fc_shortcut;
class map_view;

void popup_shortcuts_dialog();

// Assing numbers for casting
enum shortcut_id {
  SC_NONE = 0,
  SC_SCROLL_MAP = 1,
  SC_CENTER_VIEW = 2,
  SC_FULLSCREEN = 3,
  SC_MINIMAP = 4,
  SC_CITY_OUTPUT = 5,
  SC_MAP_GRID = 6,
  SC_NAT_BORDERS = 7,
  SC_QUICK_BUY = 8,
  SC_QUICK_SELECT = 9,
  SC_SELECT_BUTTON = 10,
  SC_ADJUST_WORKERS = 11,
  SC_APPEND_FOCUS = 12,
  SC_POPUP_INFO = 13,
  SC_WAKEUP_SENTRIES = 14,
  SC_MAKE_LINK = 15,
  SC_PASTE_PROD = 16,
  SC_COPY_PROD = 17,
  SC_HIDE_WORKERS = 18,
  SC_SHOW_UNITS = 19,
  SC_TRADE_ROUTES = 20,
  SC_CITY_PROD = 21,
  SC_CITY_NAMES = 22,
  SC_DONE_MOVING = 23,
  SC_GOTOAIRLIFT = 24,
  SC_AUTOEXPLORE = 25,
  SC_PATROL = 26,
  SC_UNSENTRY_TILE = 27,
  SC_DO = 28,
  SC_UPGRADE_UNIT = 29,
  SC_SETHOME = 30,
  SC_BUILDMINE = 31,
  SC_PLANT = 32,
  SC_BUILDIRRIGATION = 33,
  SC_CULTIVATE = 34,
  SC_BUILDROAD = 35,
  SC_BUILDCITY = 36,
  SC_SENTRY = 37,
  SC_FORTIFY = 38,
  SC_GOTO = 39,
  SC_WAIT = 40,
  SC_TRANSFORM = 41,
  SC_NUKE = 42,
  SC_LOAD = 43,
  SC_UNLOAD = 44,
  SC_BUY_MAP = 45,
  SC_IFACE_LOCK = 46,
  SC_AUTOMATE = 47,
  SC_PARADROP = 48,
  SC_POPUP_COMB_INF = 49,
  SC_RELOAD_THEME = 50,
  SC_RELOAD_TILESET = 51,
  SC_SHOW_FULLBAR = 52,
  SC_ZOOM_IN = 53,
  SC_ZOOM_OUT = 54,
  SC_LOAD_LUA = 55,
  SC_RELOAD_LUA = 56,
  SC_ZOOM_RESET = 57,
  SC_GOBUILDCITY = 58,
  SC_GOJOINCITY = 59,
  SC_PILLAGE = 60,
  SC_LAST_SC = 61 // It must be last, add before it
};

/**************************************************************************
  Base shortcut struct
**************************************************************************/
struct fc_shortcut {
  enum type_id { keyboard, mouse };

  shortcut_id id;

  type_id type;
  QKeySequence keys;
  Qt::MouseButton buttons;
  Qt::KeyboardModifiers modifiers;

  QString str;

  QString to_string() const;

  bool conflicts(const fc_shortcut &other) const
  {
    return type == other.type && keys == other.keys
           && buttons == other.buttons && modifiers == other.modifiers;
  }
};

/**************************************************************************
  Class with static members holding all shortcuts
**************************************************************************/
class fc_shortcuts : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(fc_shortcuts);

  fc_shortcuts();

public:
  virtual ~fc_shortcuts();

  void init_default(bool read);

  /// Returns all existing shortcuts
  auto shortcuts() const { return m_shortcuts_by_id; }

  fc_shortcut get_shortcut(shortcut_id id) const;
  void set_shortcut(const fc_shortcut &sc);

  QString get_desc(shortcut_id id) const;

  void link_action(shortcut_id id, QAction *action);
  void create_no_action_shortcuts(map_view *parent);

  static fc_shortcuts *sc();
  static void drop();

  bool read();
  void write() const;

private:
  void setup_action(const fc_shortcut &sc, QAction *action);

  static fc_shortcuts *m_instance;

  std::map<shortcut_id, QPointer<QAction>> m_actions;
  std::map<shortcut_id, QPointer<QShortcut>> m_shortcuts;
  std::map<shortcut_id, fc_shortcut> m_shortcuts_by_id;
};

/**************************************************************************
  Widget for picking shortcuts
**************************************************************************/
class shortcut_edit : public QKeySequenceEdit {
  Q_OBJECT
public:
  shortcut_edit(const fc_shortcut &sc);

  fc_shortcut shortcut() const;
  void set_shortcut(const fc_shortcut &shortcut);

protected:
  bool event(QEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QLineEdit *m_line;
  fc_shortcut m_shortcut;
  bool m_ignore_next_mouse_event = false;
};

/**************************************************************************
  Shortcut dialog
**************************************************************************/
class fc_shortcuts_dialog : public QDialog {
  Q_OBJECT
  QVBoxLayout *main_layout;
  QVBoxLayout *scroll_layout;
  QDialogButtonBox *button_box;
  void add_option(const fc_shortcut &sc);
  void init();
  void refresh();

public:
  fc_shortcuts_dialog(QWidget *parent = 0);
  ~fc_shortcuts_dialog() override;

  bool shortcut_exists(const fc_shortcut &shortcut, QString &where) const;

private slots:
  void apply_option(int response);
  void edit_shortcut();
};
