/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "shortcuts.h"
// Qt
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSettings>
#include <QWidget>
#include <qkeysequenceedit.h>
#include <qlineedit.h>
#include <qnamespace.h>
#include <qshortcut.h>
#include <qvariant.h>
// client
#include "options.h"
// gui-qt
#include "fc_client.h"
#include "hudwidget.h"

extern void real_menus_init();

static QHash<int, const char *> key_map;
static QString button_name(Qt::MouseButton bt);
fc_shortcuts *fc_shortcuts::m_instance = nullptr;

enum {
  RESPONSE_CANCEL,
  RESPONSE_OK,
  RESPONSE_APPLY,
  RESPONSE_RESET,
  RESPONSE_SAVE
};

fc_shortcut default_shortcuts[] = {
    {SC_SCROLL_MAP, fc_shortcut::mouse, QKeySequence(), Qt::RightButton,
     Qt::NoModifier, "Scroll map"},
    {SC_CENTER_VIEW, fc_shortcut::keyboard, Qt::Key_C, Qt::AllButtons,
     Qt::NoModifier, _("Center View")},
    {SC_FULLSCREEN, fc_shortcut::keyboard, Qt::Key_Return | Qt::AltModifier,
     Qt::AllButtons, Qt::NoModifier, _("Fullscreen")},
    {SC_MINIMAP, fc_shortcut::keyboard, Qt::Key_M | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("Show minimap")},
    {SC_CITY_OUTPUT, fc_shortcut::keyboard, Qt::Key_W, Qt::AllButtons,
     Qt::ControlModifier, _("City Output")},
    {SC_MAP_GRID, fc_shortcut::keyboard, Qt::Key_G | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("Map Grid")},
    {SC_NAT_BORDERS, fc_shortcut::keyboard, Qt::Key_B | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("National Borders")},
    {SC_QUICK_BUY, fc_shortcut::mouse, QKeySequence(), Qt::LeftButton,
     Qt::ControlModifier | Qt::ShiftModifier, _("Quick buy from map")},
    {SC_QUICK_SELECT, fc_shortcut::mouse, QKeySequence(), Qt::LeftButton,
     Qt::ControlModifier, _("Quick production select from map")},
    {SC_SELECT_BUTTON, fc_shortcut::mouse, QKeySequence(), Qt::LeftButton,
     Qt::NoModifier, _("Select button")},
    {SC_ADJUST_WORKERS, fc_shortcut::mouse, QKeySequence(), Qt::LeftButton,
     Qt::MetaModifier | Qt::ControlModifier, _("Adjust workers")},
    {SC_APPEND_FOCUS, fc_shortcut::mouse, QKeySequence(), Qt::LeftButton,
     Qt::ShiftModifier, _("Append focus")},
    {SC_POPUP_INFO, fc_shortcut::mouse, QKeySequence(), Qt::MiddleButton,
     Qt::NoModifier, _("Popup tile info")},
    {SC_WAKEUP_SENTRIES, fc_shortcut::mouse, QKeySequence(),
     Qt::MiddleButton, Qt::ControlModifier, _("Wakeup sentries")},
    {SC_MAKE_LINK, fc_shortcut::mouse, QKeySequence(), Qt::RightButton,
     Qt::ControlModifier | Qt::AltModifier, _("Show link to tile")},
    {SC_PASTE_PROD, fc_shortcut::mouse, QKeySequence(), Qt::RightButton,
     Qt::ShiftModifier | Qt::ControlModifier, _("Paste production")},
    {SC_COPY_PROD, fc_shortcut::mouse, QKeySequence(), Qt::RightButton,
     Qt::ShiftModifier, _("Copy production")},
    {SC_HIDE_WORKERS, fc_shortcut::mouse, QKeySequence(), Qt::RightButton,
     Qt::ShiftModifier | Qt::AltModifier, _("Show/hide workers")},
    {SC_SHOW_UNITS, fc_shortcut::keyboard,
     Qt::Key_Space | Qt::ControlModifier, Qt::AllButtons, Qt::NoModifier,
     _("Units selection (for tile under mouse position)")},
    {SC_TRADE_ROUTES, fc_shortcut::keyboard, Qt::Key_D | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("City Traderoutes")},
    {SC_CITY_PROD, fc_shortcut::keyboard, Qt::Key_P | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("City Production Levels")},
    {SC_CITY_NAMES, fc_shortcut::keyboard, Qt::Key_N | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("City Names")},
    {SC_DONE_MOVING, fc_shortcut::keyboard, Qt::Key_Space, Qt::AllButtons,
     Qt::NoModifier, _("Done Moving")},
    {SC_GOTOAIRLIFT, fc_shortcut::keyboard, Qt::Key_T, Qt::AllButtons,
     Qt::NoModifier, _("Go to/Airlift to City...")},
    {SC_AUTOEXPLORE, fc_shortcut::keyboard, Qt::Key_X, Qt::AllButtons,
     Qt::NoModifier, _("Auto Explore")},
    {SC_PATROL, fc_shortcut::keyboard, Qt::Key_Q, Qt::AllButtons,
     Qt::NoModifier, _("Patrol")},
    {SC_UNSENTRY_TILE, fc_shortcut::keyboard,
     Qt::Key_D | Qt::ShiftModifier | Qt::ControlModifier, Qt::AllButtons,
     Qt::NoModifier, _("Unsentry All On Tile")},
    {SC_DO, fc_shortcut::keyboard, Qt::Key_D, Qt::AllButtons, Qt::NoModifier,
     _("Do...")},
    {SC_UPGRADE_UNIT, fc_shortcut::keyboard, Qt::Key_U | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("Upgrade")},
    {SC_SETHOME, fc_shortcut::keyboard, Qt::Key_H, Qt::AllButtons,
     Qt::NoModifier, _("Set Home City")},
    {SC_BUILDMINE, fc_shortcut::keyboard, Qt::Key_M, Qt::AllButtons,
     Qt::NoModifier, _("Build Mine")},
    {SC_PLANT, fc_shortcut::keyboard, Qt::Key_M | Qt::ShiftModifier,
     Qt::AllButtons, Qt::NoModifier, _("Plant")},
    {SC_BUILDIRRIGATION, fc_shortcut::keyboard, Qt::Key_I, Qt::AllButtons,
     Qt::NoModifier, _("Build Irrigation")},
    {SC_CULTIVATE, fc_shortcut::keyboard, Qt::Key_I | Qt::ShiftModifier,
     Qt::AllButtons, Qt::NoModifier, _("Cultivate")},
    {SC_BUILDROAD, fc_shortcut::keyboard, Qt::Key_R, Qt::AllButtons,
     Qt::NoModifier, _("Build Road")},
    {SC_BUILDCITY, fc_shortcut::keyboard, Qt::Key_B, Qt::AllButtons,
     Qt::NoModifier, _("Build City")},
    {SC_SENTRY, fc_shortcut::keyboard, Qt::Key_S, Qt::AllButtons,
     Qt::NoModifier, _("Sentry")},
    {SC_FORTIFY, fc_shortcut::keyboard, Qt::Key_F, Qt::AllButtons,
     Qt::NoModifier, _("Fortify")},
    {SC_GOTO, fc_shortcut::keyboard, Qt::Key_G, Qt::AllButtons,
     Qt::NoModifier, _("Go to Tile")},
    {SC_WAIT, fc_shortcut::keyboard, Qt::Key_W, Qt::AllButtons,
     Qt::NoModifier, _("Wait")},
    {SC_TRANSFORM, fc_shortcut::keyboard, Qt::Key_O, Qt::AllButtons,
     Qt::NoModifier, _("Transform")},
    {SC_NUKE, fc_shortcut::keyboard, Qt::Key_N | Qt::ShiftModifier,
     Qt::AllButtons, Qt::NoModifier, _("Explode Nuclear")},
    {SC_LOAD, fc_shortcut::keyboard, Qt::Key_L, Qt::AllButtons,
     Qt::NoModifier, _("Load")},
    {SC_UNLOAD, fc_shortcut::keyboard, Qt::Key_U, Qt::AllButtons,
     Qt::NoModifier, _("Unload")},
    {SC_BUY_MAP, fc_shortcut::mouse, QKeySequence(), Qt::BackButton,
     Qt::NoModifier, _("Quick buy current production from map")},
    {SC_IFACE_LOCK, fc_shortcut::keyboard,
     Qt::Key_L | Qt::ControlModifier | Qt::ShiftModifier, Qt::AllButtons,
     Qt::NoModifier, _("Lock/unlock interface")},
    {SC_AUTOMATE, fc_shortcut::keyboard, Qt::Key_A, Qt::AllButtons,
     Qt::NoModifier, _("Auto worker")},
    {SC_PARADROP, fc_shortcut::keyboard, Qt::Key_P, Qt::AllButtons,
     Qt::NoModifier, _("Paradrop/clean pollution")},
    {SC_POPUP_COMB_INF, fc_shortcut::keyboard,
     Qt::Key_F1 | Qt::ControlModifier, Qt::AllButtons, Qt::NoModifier,
     _("Popup combat information")},
    {SC_RELOAD_THEME, fc_shortcut::keyboard,
     Qt::Key_F5 | Qt::ControlModifier | Qt::ShiftModifier, Qt::AllButtons,
     Qt::NoModifier, _("Reload theme")},
    {SC_RELOAD_TILESET, fc_shortcut::keyboard,
     Qt::Key_F6 | Qt::ControlModifier | Qt::ShiftModifier, Qt::AllButtons,
     Qt::NoModifier, _("Reload tileset")},
    {SC_SHOW_FULLBAR, fc_shortcut::keyboard, Qt::Key_F | Qt::ControlModifier,
     Qt::AllButtons, Qt::NoModifier, _("Toggle city full bar visibility")},
    {SC_ZOOM_IN, fc_shortcut::keyboard, Qt::Key_Plus, Qt::AllButtons,
     Qt::NoModifier, _("Zoom in")},
    {SC_ZOOM_OUT, fc_shortcut::keyboard, Qt::Key_Minus, Qt::AllButtons,
     Qt::NoModifier, _("Zoom out")},
    {SC_LOAD_LUA, fc_shortcut::keyboard,
     Qt::Key_J | Qt::ControlModifier | Qt::ShiftModifier, Qt::AllButtons,
     Qt::NoModifier, _("Load Lua script")},
    {SC_RELOAD_LUA, fc_shortcut::keyboard,
     Qt::Key_K | Qt::ControlModifier | Qt::ShiftModifier, Qt::AllButtons,
     Qt::NoModifier, _("Load last loaded Lua script")},
    {SC_ZOOM_RESET, fc_shortcut::keyboard,
     Qt::Key_Backspace | Qt::ControlModifier, Qt::AllButtons, Qt::NoModifier,
     _("Reload tileset with default scale")},
    {SC_GOBUILDCITY, fc_shortcut::keyboard, Qt::Key_B | Qt::ShiftModifier,
     Qt::AllButtons, Qt::NoModifier, _("Go And Build City")},
    {SC_GOJOINCITY, fc_shortcut::keyboard, Qt::Key_J | Qt::ShiftModifier,
     Qt::AllButtons, Qt::NoModifier, _("Go And Join City")},
    {SC_PILLAGE, fc_shortcut::keyboard, Qt::Key_P | Qt::ShiftModifier,
     Qt::AllButtons, Qt::NoModifier, _("Pillage")}};

/**
   Returns shortcut as string (eg. for menu)
 */
QString fc_shortcut::to_string() const
{
  switch (type) {
  case fc_shortcut::keyboard:
    return keys.toString(QKeySequence::NativeText);
    break;
  case fc_shortcut::mouse: {
    return QKeySequence(modifiers).toString(QKeySequence::NativeText)
           + button_name(buttons);
  } break;
  }

  return QString();
}

/**
   fc_shortcuts contructor
 */
fc_shortcuts::fc_shortcuts() { init_default(true); }

/**
   fc_shortcuts destructor
 */
fc_shortcuts::~fc_shortcuts() { m_shortcuts_by_id.clear(); }

/**
   Returns description for given shortcut
 */
QString fc_shortcuts::get_desc(shortcut_id id) const
{
  return m_shortcuts_by_id.at(id).str;
}

/**
   Returns shortcut for given id
 */
fc_shortcut fc_shortcuts::get_shortcut(shortcut_id id) const
{
  return m_shortcuts_by_id.at(id);
}

/**
   Sets given shortcut
 */
void fc_shortcuts::set_shortcut(const fc_shortcut &s)
{
  m_shortcuts_by_id[s.id] = s;

  if (m_actions.count(s.id) > 0) {
    auto action = m_actions[s.id];
    if (action) { // Might have been deleted
      setup_action(s, action);
    } else {
      m_actions.erase(s.id);
    }
  }

  king()->menu_bar->show(); // Apparently needed for shortcuts to update
}

/**
 * Links an action to a shortcut. This will synchronize the action with the
 * shortcut, and it will be triggered whenever the shortcut is entered.
 */
void fc_shortcuts::link_action(shortcut_id id, QAction *action)
{
  m_actions[id] = action;
  setup_action(shortcuts()[id], action);
}

/**
   Deletes current instance
 */
void fc_shortcuts::drop()
{
  delete m_instance;
  m_instance = nullptr;
}

/**
   Returns given instance
 */
fc_shortcuts *fc_shortcuts::sc()
{
  if (!m_instance) {
    m_instance = new fc_shortcuts;
  }
  return m_instance;
}

/**
   Inits defaults shortcuts or reads from settings
 */
void fc_shortcuts::init_default(bool read)
{
  bool suc = false;
  m_shortcuts_by_id.clear();

  if (read) {
    suc = this->read();
  }
  if (!suc) {
    for (int i = 0; i < SC_LAST_SC - 1; i++) {
      fc_shortcut s;
      s.id = default_shortcuts[i].id;
      s.type = default_shortcuts[i].type;
      s.keys = default_shortcuts[i].keys;
      s.buttons = default_shortcuts[i].buttons;
      s.modifiers = default_shortcuts[i].modifiers;
      s.str = default_shortcuts[i].str;
      m_shortcuts_by_id[default_shortcuts[i].id] = s;
    }
  }
}

/**
 * Sets up key bindings for the action.
 */
void fc_shortcuts::setup_action(const fc_shortcut &sc, QAction *action)
{
  if (sc.type == fc_shortcut::keyboard) {
    action->setShortcut(sc.keys);
  } else {
    action->setShortcut(QKeySequence());
  }
}

/**
 * Constructs a shortcut edit widget
 */
shortcut_edit::shortcut_edit(const fc_shortcut &sc)
    : QKeySequenceEdit(sc.keys), m_shortcut(sc)
{
  // We're eavesdropping into the implementation of QKeySequenceEdit here...
  for (auto widget : children()) {
    m_line = qobject_cast<QLineEdit *>(widget);
    if (m_line) {
      break;
    }
  }
  fc_assert_ret(m_line != nullptr);
  m_line->setContextMenuPolicy(Qt::NoContextMenu);
  m_line->installEventFilter(this);
  m_line->setText(m_shortcut.to_string());
  installEventFilter(this);
}

/**
 * Retrieves the shortcut entered by the user
 */
fc_shortcut shortcut_edit::shortcut() const
{
  auto shortcut = m_shortcut;
  if (auto keys = keySequence(); !keys.isEmpty()) {
    shortcut.type = fc_shortcut::keyboard;
    shortcut.keys = keySequence();
  } else {
    shortcut.type = fc_shortcut::mouse;
  }
  return shortcut;
}

/**
 * Changes the shortcut displayed by the widget
 */
void shortcut_edit::set_shortcut(const fc_shortcut &shortcut)
{
  m_shortcut = shortcut;
  switch (shortcut.type) {
  case fc_shortcut::keyboard:
    setKeySequence(shortcut.keys);
    break;
  case fc_shortcut::mouse:
    m_line->setText(shortcut.to_string());
    break;
  }
}

/**
 * \reimp
 */
bool shortcut_edit::event(QEvent *event)
{
  if (event->type() == QEvent::FocusIn) {
    auto focus_event = static_cast<QFocusEvent *>(event);
    if (focus_event->reason() == Qt::MouseFocusReason) {
      m_ignore_next_mouse_event = true;
    }
  }

  return QKeySequenceEdit::event(event);
}

/**
 * Mouse press event for shortcut edit widget
 */
bool shortcut_edit::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == m_line && event->type() == QEvent::MouseButtonPress
      && m_line->hasFocus()) {
    if (m_ignore_next_mouse_event) {
      // Ignore the mouse event when it gives focus
      m_ignore_next_mouse_event = false;
    } else {
      auto mouse_event = static_cast<QMouseEvent *>(event);
      m_shortcut.type = fc_shortcut::mouse;
      m_shortcut.buttons = mouse_event->button();
      m_shortcut.modifiers = mouse_event->modifiers();
      m_shortcut.keys = QKeySequence();
      setKeySequence(m_shortcut.keys); // Invalid

      m_line->setText(m_shortcut.to_string());

      emit editingFinished();
      mouse_event->accept();
      return true;
    }
  }
  return QKeySequenceEdit::eventFilter(watched, event);
}

/**
   Returns mouse button name
 */
QString button_name(Qt::MouseButton bt)
{
  switch (bt) {
  case Qt::NoButton:
    return _("NoButton");
  case Qt::LeftButton:
    return _("LeftButton");
  case Qt::RightButton:
    return _("RightButton");
  case Qt::MiddleButton:
    return _("MiddleButton");
  case Qt::BackButton:
    return _("BackButton");
  case Qt::ForwardButton:
    return _("ForwardButton");
  case Qt::TaskButton:
    return _("TaskButton");
  case Qt::ExtraButton4:
    return _("ExtraButton4");
  case Qt::ExtraButton5:
    return _("ExtraButton5");
  case Qt::ExtraButton6:
    return _("ExtraButton6");
  case Qt::ExtraButton7:
    return _("ExtraButton7");
  case Qt::ExtraButton8:
    return _("ExtraButton8");
  case Qt::ExtraButton9:
    return _("ExtraButton9");
  case Qt::ExtraButton10:
    return _("ExtraButton10");
  case Qt::ExtraButton11:
    return _("ExtraButton11");
  case Qt::ExtraButton12:
    return _("ExtraButton12");
  case Qt::ExtraButton13:
    return _("ExtraButton13");
  case Qt::ExtraButton14:
    return _("ExtraButton14");
  case Qt::ExtraButton15:
    return _("ExtraButton15");
  case Qt::ExtraButton16:
    return _("ExtraButton16");
  case Qt::ExtraButton17:
    return _("ExtraButton17");
  case Qt::ExtraButton18:
    return _("ExtraButton18");
  case Qt::ExtraButton19:
    return _("ExtraButton19");
  case Qt::ExtraButton20:
    return _("ExtraButton20");
  case Qt::ExtraButton21:
    return _("ExtraButton21");
  case Qt::ExtraButton22:
    return _("ExtraButton22");
  case Qt::ExtraButton23:
    return _("ExtraButton23");
  case Qt::ExtraButton24:
    return _("ExtraButton24");
  default:
    return QLatin1String("");
  }
}

/**
   Contructor for shortcut dialog
 */
fc_shortcuts_dialog::fc_shortcuts_dialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(_("Shortcuts options"));
  init();

  auto size = sizeHint();
  size.setWidth(size.width() + 10
                + style()->pixelMetric(QStyle::PM_ScrollBarExtent));
  resize(size);
  setAttribute(Qt::WA_DeleteOnClose);
}

/**
   Destructor for shortcut dialog
 */
fc_shortcuts_dialog::~fc_shortcuts_dialog() = default;

/**
   Inits shortcut dialog layout
 */
void fc_shortcuts_dialog::init()
{
  QPushButton *but;
  QScrollArea *scroll;
  QString desc;
  QWidget *widget;

  widget = new QWidget(this);
  scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll_layout = new QVBoxLayout;
  main_layout = new QVBoxLayout;
  for (const auto &[_, sc] : fc_shortcuts::sc()->shortcuts()) {
    add_option(sc);
  }
  widget->setProperty("shortcuts", true);
  widget->setLayout(scroll_layout);
  scroll->setWidget(widget);
  main_layout->addWidget(scroll);

  button_box = new QDialogButtonBox();
  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogResetButton),
                        _("Reset"));
  button_box->addButton(but, QDialogButtonBox::ResetRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_RESET); });

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogSaveButton),
                        _("Save"));
  button_box->addButton(but, QDialogButtonBox::ActionRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_SAVE); });

  but = new QPushButton(style()->standardIcon(QStyle::SP_DialogCloseButton),
                        _("Close"));
  button_box->addButton(but, QDialogButtonBox::AcceptRole);
  QObject::connect(but, &QPushButton::clicked,
                   [this]() { apply_option(RESPONSE_OK); });

  main_layout->addWidget(button_box);
  setLayout(main_layout);
}

/**
   Adds shortcut option for dialog
 */
void fc_shortcuts_dialog::add_option(const fc_shortcut &sc)
{
  auto l = new QLabel(sc.str);
  auto hb = new QHBoxLayout();

  auto fb = new shortcut_edit(sc);
  connect(fb, &shortcut_edit::editingFinished, this,
          &fc_shortcuts_dialog::edit_shortcut);

  hb->addWidget(l, 1, Qt::AlignLeft);
  hb->addStretch();
  hb->addWidget(fb, 1, Qt::AlignRight);

  scroll_layout->addLayout(hb);
}

/**
   Slot for editing shortcut
 */
void fc_shortcuts_dialog::edit_shortcut()
{
  auto edit = qobject_cast<shortcut_edit *>(sender());
  auto shortcut = edit->shortcut();
  auto old = fc_shortcuts::sc()->get_shortcut(shortcut.id);

  QString where;
  if (shortcut.conflicts(old) || !shortcut_exists(shortcut, where)) {
    fc_shortcuts::sc()->set_shortcut(shortcut);
  } else {
    // Duplicate shortcut
    edit->set_shortcut(old);

    const auto title = QString(_("Already in use"));
    // TRANS: Given shortcut(%1) is already assigned to action %2
    const auto text = QString(_("\"%1\" is already assigned to \"%2\""))
                          .arg(shortcut.to_string())
                          .arg(where);

    auto scinfo = new hud_message_box(this);
    scinfo->setStandardButtons(QMessageBox::Close);
    scinfo->set_text_title(text, title);
    scinfo->setAttribute(Qt::WA_DeleteOnClose);
    scinfo->show();
  }
}

/**
 * Checks if a shortcut already exists.
 */
bool fc_shortcuts_dialog::shortcut_exists(const fc_shortcut &shortcut,
                                          QString &where) const
{
  for (const auto &[_, fsc] : fc_shortcuts::sc()->shortcuts()) {
    if (shortcut.conflicts(fsc)) {
      qWarning("Trying to set a shortcut already used elsewhere");
      where = fc_shortcuts::sc()->get_desc(fsc.id);
      return true;
    }
  }

  // Also check the menu bar for conflicts
  return king()->menu_bar->shortcut_exists(shortcut, where);
}

/**
   Reinitializes layout
 */
void fc_shortcuts_dialog::refresh()
{
  QLayout *layout;
  QLayout *sublayout;
  QLayoutItem *item;
  QWidget *widget;

  layout = main_layout;
  while ((item = layout->takeAt(0))) {
    if ((sublayout = item->layout()) != 0) {
    } else if ((widget = item->widget()) != 0) {
      widget->hide();
      delete widget;
    } else {
      delete item;
    }
  }
  delete main_layout;
  init();
}

/**
   Slot for buttons on bottom of shortcut dialog
 */
void fc_shortcuts_dialog::apply_option(int response)
{
  switch (response) {
  case RESPONSE_OK:
    real_menus_init();
    king()->menuBar()->setVisible(true);
    close();
    break;
  case RESPONSE_SAVE:
    fc_shortcuts::sc()->write();
    break;
  case RESPONSE_RESET:
    fc_shortcuts::sc()->init_default(false);
    refresh();
    break;
  }
}

/**
   Popups shortcut dialog
 */
void popup_shortcuts_dialog()
{
  fc_shortcuts_dialog *sh = new fc_shortcuts_dialog(king());
  sh->show();
}

/**
   Writes shortcuts to file
 */
void fc_shortcuts::write() const
{
  QSettings s(QSettings::IniFormat, QSettings::UserScope,
              QStringLiteral("freeciv21-client"));
  s.beginWriteArray(QStringLiteral("ShortcutsV2"));
  for (auto &[id, sc] : shortcuts()) {
    s.setArrayIndex(id);
    s.setValue(QStringLiteral("id"), sc.id);
    s.setValue(QStringLiteral("type"), int(sc.type));
    s.setValue(QStringLiteral("keys"), sc.keys);
    s.setValue(QStringLiteral("buttons"), sc.buttons);
    s.setValue(QStringLiteral("modifiers"), QVariant(sc.modifiers));
  }
  s.endArray();
}

/**
   Reads shortcuts from file. Returns false if failed.
 */
bool fc_shortcuts::read()
{
  int num, i;
  QSettings s(QSettings::IniFormat, QSettings::UserScope,
              QStringLiteral("freeciv21-client"));
  num = s.beginReadArray(QStringLiteral("ShortcutsV2"));
  if (num <= SC_LAST_SC - 1) {
    for (i = 0; i < num; ++i) {
      fc_shortcut sc;
      s.setArrayIndex(i);
      sc.id =
          static_cast<shortcut_id>(s.value(QStringLiteral("id")).toInt());
      sc.type = static_cast<fc_shortcut::type_id>(
          s.value(QStringLiteral("type")).toInt());
      sc.keys = qvariant_cast<QKeySequence>(s.value(QStringLiteral("keys")));
      sc.buttons = static_cast<Qt::MouseButton>(
          s.value(QStringLiteral("buttons")).toInt());
      sc.modifiers = static_cast<Qt::KeyboardModifiers>(
          s.value(QStringLiteral("modifiers")).toInt());
      sc.str = default_shortcuts[i].str;
      set_shortcut(sc);
    }
    while (i < SC_LAST_SC - 1) {
      // initialize missing shortcuts
      fc_shortcut sc;
      sc.id = default_shortcuts[i].id;
      sc.type = default_shortcuts[i].type;
      sc.keys = default_shortcuts[i].keys;
      sc.buttons = default_shortcuts[i].buttons;
      sc.modifiers = default_shortcuts[i].modifiers;
      sc.str = default_shortcuts[i].str;
      set_shortcut(sc);
      ++i;
    }
  } else {
    s.endArray();
    return false;
  }
  s.endArray();
  return true;
}
