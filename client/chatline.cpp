/**
   //           Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors.
 _oo\ This file is  part of Freeciv21. Freeciv21 is free software: you can
(__/ \  _  _   redistribute it and/or modify it under the terms of the GNU
   \  \/ \/ \    General Public License  as published by the Free Software
   (         )\        Foundation, either version 3 of the License, or (at
    \_______/  \  your option) any later version. You should have received
     [[] [[] a copy of the GNU General Public License along with Freeciv21.
     [[] [[]                     If not, see https://www.gnu.org/licenses/.
 */

#include "chatline.h"
// Qt
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QGridLayout>
#include <QHash>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBlock>
#include <QToolButton>

// common
#include "chat.h"
#include "chatline_common.h"

// client
#include "audio/audio.h"
#include "client_main.h"
#include "climap.h"
#include "colors_common.h"
#include "connectdlg_common.h"
#include "control.h"
#include "dialogs.h"
#include "fc_client.h"
#include "featured_text.h"
#include "fonts.h"
#include "game.h"
#include "icons.h"
#include "messagewin.h"
#include "page_game.h"
#include "views/view_map.h"
#include "views/view_map_common.h"

static bool is_plain_public_message(const QString &s);

QStringList chat_listener::history = QStringList();

namespace {

QHash<QString, QString> color_mapping;

} // namespace

/**
 * Sets color substitution map.
 */
void set_chat_colors(const QHash<QString, QString> &colors)
{
  color_mapping = colors;
}

/**
 * Constructor.
 */
chat_listener::chat_listener() : position(HISTORY_END) {}

/**
 * Called whenever a message is received. Default implementation does
 * nothing.
 */
void chat_listener::chat_message_received(const QString &,
                                          const struct text_tag_list *)
{
}

/**
 * Sends commands to server, but first searches for custom keys, if it finds
 * then it makes custom action.
 *
 * The history position is reset to HISTORY_END.
 */
void chat_listener::send_chat_message(const QString &message)
{
  QString splayer, s;

  history << message;
  reset_history_position();

  /*
   * If client send commands to take ai, set /away to disable AI
   */
  if (message.startsWith(QLatin1String("/take "))) {
    s = s.remove(QStringLiteral("/take "));
    players_iterate(pplayer)
    {
      splayer = QString(pplayer->name);
      splayer = "\"" + splayer + "\"";

      if (!splayer.compare(s)) {
        if (is_ai(pplayer)) {
          send_chat(message.toLocal8Bit());
          send_chat("/away");
          return;
        }
      }
    }
    players_iterate_end;
  }

  /*
   * Option to send to allies by default
   */
  if (!message.isEmpty()) {
    if (client_state() >= C_S_RUNNING && gui_options->gui_qt_allied_chat_only
        && is_plain_public_message(message)) {
      send_chat((QString(CHAT_ALLIES_PREFIX) + " " + message).toLocal8Bit());
    } else {
      send_chat(message.toLocal8Bit());
    }
  }
  // Empty messages aren't sent
  // FIXME Inconsistent behavior: "." will send an empty message to allies
}

/**
 * Goes back one position in history, and returns the message at the new
 * position.
 */
QString chat_listener::back_in_history()
{
  if (!history.empty() && position == HISTORY_END) {
    position = history.size() - 1;
  } else if (position > 0) {
    position--;
  }
  return history.empty() ? QLatin1String("") : history.at(position);
}

/**
 * Goes forward one position in history, and returns the message at the new
 * position. An empty string is returned if the new position is HISTORY_END.
 */
QString chat_listener::forward_in_history()
{
  if (position == HISTORY_END) {
    return QLatin1String("");
  }
  position++;
  if (position >= history.size()) {
    position = HISTORY_END;
    return QLatin1String("");
  } else {
    return history.at(position);
  }
}

/**
 * Go to the end of the history.
 */
void chat_listener::reset_history_position() { position = HISTORY_END; }

/**
 * Constructor
 */
chat_input::chat_input(QWidget *parent) : QLineEdit(parent)
{
  connect(this, &QLineEdit::returnPressed, this, &chat_input::send);
  chat_listener::listen();
}

/**
 * Sends the content of the input box
 */
void chat_input::send()
{
  send_chat_message(text());
  clear();
}

/**
 * Called whenever the completion word list changes.
 */
void chat_input::update_completion()
{
  QStringList word_list;

  // sourced from server/commands.cpp
  word_list << QStringLiteral("/start");
  word_list << QStringLiteral("/help");
  word_list << QStringLiteral("/list colors");
  word_list << QStringLiteral("/list connections");
  word_list << QStringLiteral("/list delegations");
  word_list << QStringLiteral("/list ignored users");
  word_list << QStringLiteral("/list map image definitions");
  word_list << QStringLiteral("/list players");
  word_list << QStringLiteral("/list rulesets");
  word_list << QStringLiteral("/list scenarios");
  word_list << QStringLiteral("/list nationsets");
  word_list << QStringLiteral("/list teams");
  word_list << QStringLiteral("/list votes");
  word_list << QStringLiteral("/quit");
  word_list << _("/cut <connection-name>");
  word_list << _("/explain <option-name>");
  word_list << _("/show <option-name>");
  word_list << QStringLiteral("/show all");
  word_list << QStringLiteral("/show vital");
  word_list << QStringLiteral("/show situational");
  word_list << QStringLiteral("/show rare");
  word_list << QStringLiteral("/show changed");
  word_list << QStringLiteral("/show locked");
  word_list << QStringLiteral("/show rulesetdir");
  word_list << _("/wall <message>");
  word_list << _("/connectmsg <message>");
  word_list << _("/vote yes|no|abstain [vote number]");
  word_list << _("/debug diplomacy <player>");
  word_list << _("/debug ferries");
  word_list << _("/debug tech <player>");
  word_list << _("/debug city <x> <y>");
  word_list << _("/debug units <x> <y>");
  word_list << _("/debug unit <id>");
  word_list << QStringLiteral("/debug timing");
  word_list << QStringLiteral("/debug info");
  word_list << _("/set <option-name> <value>");
  word_list << _("/team <player> <team>");
  word_list << _("/rulesetdir <directory>");
  word_list << _("/metamessage <meta-line>");
  word_list << _("/metapatches <meta-line>");
  word_list << QStringLiteral("/metaconnection up|down|?");
  word_list << _("/metaserver <address>");
  word_list << _("/aitoggle <player-name>");
  word_list << _("/take <player-name>");
  word_list << _("/observe <player-name>");
  word_list << _("/detach <connection-name>");
  word_list << _("/create <player-name> [ai type]");
  word_list << QStringLiteral("/away");
  word_list << _("/handicapped <player-name>");
  word_list << _("/novice <player-name>");
  word_list << _("/easy <player-name>");
  word_list << _("/normal <player-name>");
  word_list << _("/hard <player-name>");
  word_list << _("/cheating <player-name>");
  word_list << _("/experimental <player-name>");
  word_list << QStringLiteral("/cmdlevel none|info|basic|ctrl|admin|hack");
  word_list << QStringLiteral("/first");
  word_list << QStringLiteral("/timeoutshow");
  word_list << _("/timeoutset <time>");
  word_list << _("/timeoutadd <time>");
  word_list << _("/timeoutincrease <turn> <turninc> <value> <valuemult>");
  word_list << _("/cancelvote <vote number>");
  word_list << _("/ignore [type=]<pattern>");
  word_list << _("/unignore <range>");
  word_list << _("/playercolor <player-name> <color>");
  word_list << _("/playernation <player-name> [nation] [is-male] [leader] "
                 "[style]");
  word_list << QStringLiteral("/endgame");
  word_list << QStringLiteral("/surrender");
  word_list << _("/remove <player-name>");
  word_list << _("/save <file-name>");
  word_list << _("/scensave <file-name>");
  word_list << _("/load <file-name>");
  word_list << _("/read <file-name>");
  word_list << _("/write <file-name>");
  word_list << QStringLiteral("/reset game|ruleset|script|default");
  word_list << _("/default <option name>");
  word_list << _("/lua cmd <script line>");
  word_list << _("/lua unsafe-cmd <script line>");
  word_list << _("/lua file <script file>");
  word_list << _("/lua unsafe-file <script file>");
  word_list << _("/kick <user>");
  word_list << _("/delegate to <username>");
  word_list << QStringLiteral("/delegate cancel");
  word_list << _("/delegate take <player-name>");
  word_list << QStringLiteral("/delegate restore");
  word_list << _("/delegate show <player-name>");
  word_list << _("/aicmd <player> <command>");
  word_list << _("/fcdb lua <script>");
  word_list << _("/mapimg define <mapdef>");
  word_list << _("/mapimg show <id>|all");
  word_list << _("/mapimg create <id>|all");
  word_list << _("/mapimg delete <id>|all");
  word_list << QStringLiteral("/mapimg colortest");
  word_list << QStringLiteral("/rfcstyle");
  word_list << QStringLiteral("/serverid");

  conn_list_iterate(game.est_connections, pconn)
  {
    if (pconn->playing) {
      word_list << pconn->playing->name;
      word_list << pconn->playing->username;
    } else {
      word_list << pconn->username;
    }
  }
  conn_list_iterate_end;

  players_iterate(pplayer) { word_list << pplayer->name; }
  players_iterate_end;

  delete completer();

  auto cmplt = new QCompleter(word_list, this);
  cmplt->setCaseSensitivity(Qt::CaseInsensitive);
  cmplt->setCompletionMode(QCompleter::InlineCompletion);
  setCompleter(cmplt);
}

/**
 * Event handler for chat_input, used for history
 */
bool chat_input::event(QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    switch (keyEvent->key()) {
    case Qt::Key_Up:
      setText(back_in_history());
      return true;
    case Qt::Key_Down:
      setText(forward_in_history());
      return true;
    }
  }
  return QLineEdit::event(event);
}

/**
 * Event handler for chat_input, used for history
 */
void chat_input::focusInEvent(QFocusEvent *event)
{
  update_completion();
  QLineEdit::focusInEvent(event);
}

/**
 * Constructor for chat_widget
 */
chat_widget::chat_widget(QWidget *parent)
{
  QGridLayout *gl;
  setParent(parent);
  setMinimumSize(200, 100);
  setResizable(Qt::LeftEdge | Qt::RightEdge | Qt::TopEdge | Qt::BottomEdge);

  gl = new QGridLayout;
  gl->setVerticalSpacing(0);
  gl->setContentsMargins(QMargins());
  setLayout(gl);

  cb = new QToolButton;
  cb->setFixedSize(QSize(25, 25));
  cb->setIconSize(QSize(24, 24));
  cb->setToolTip(_("Set who can see your messages by default."));

  const auto current_icon = [] {
    return fcIcons::instance()->getIcon(gui_options->gui_qt_allied_chat_only
                                            ? QStringLiteral("private")
                                            : QStringLiteral("public"));
  };
  cb->setIcon(current_icon());

  cb_menu = new QMenu;
  cb->setMenu(cb_menu);
  cb->setPopupMode(QToolButton::InstantPopup);

  // Populate the menu
  auto group = new QActionGroup(cb_menu);
  auto action = cb_menu->addAction(
      fcIcons::instance()->getIcon(QStringLiteral("private")),
      _("Allies Only"));
  action->setCheckable(true);
  action->setChecked(gui_options->gui_qt_allied_chat_only);
  connect(action, &QAction::triggered, cb, [=] {
    gui_options->gui_qt_allied_chat_only = true;
    cb->setIcon(current_icon());
  });
  group->addAction(action);

  action = cb_menu->addAction(
      fcIcons::instance()->getIcon(QStringLiteral("public")), _("Everyone"));
  action->setCheckable(true);
  action->setChecked(!gui_options->gui_qt_allied_chat_only);
  connect(action, &QAction::triggered, cb, [=] {
    gui_options->gui_qt_allied_chat_only = false;
    cb->setIcon(current_icon());
  });
  group->addAction(action);

  chat_line = new chat_input;
  chat_line->installEventFilter(this);

  remove_links = new QPushButton();
  remove_links->setIconSize(QSize(24, 24));
  remove_links->setFixedWidth(25);
  remove_links->setFixedHeight(25);
  remove_links->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("erase")));
  remove_links->setToolTip(_("Clear links"));

  show_hide = new QPushButton();
  show_hide->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("expand-down")));
  show_hide->setIconSize(QSize(24, 24));
  show_hide->setFixedWidth(25);
  show_hide->setFixedHeight(25);
  show_hide->setToolTip(_("Show/hide chat"));
  show_hide->setCheckable(true);
  show_hide->setChecked(true);

  mw = new move_widget(this);
  mw->put_to_corner();

  chat_output = new text_browser_dblclck(this);
  chat_output->setFont(fcFont::instance()->getFont(fonts::chatline));
  chat_output->setReadOnly(true);
  chat_output->setVisible(true);
  chat_output->setAcceptRichText(true);
  chat_output->setOpenLinks(false);
  chat_output->setReadOnly(true);

  auto title = new QLabel(_("Chat"));
  title->setAlignment(Qt::AlignCenter);
  title->setMouseTracking(true);

  gl->addWidget(mw, 0, 0, Qt::AlignLeft | Qt::AlignTop);
  gl->addWidget(title, 0, 1, 1, 2);
  gl->setColumnStretch(1, 100);
  gl->addWidget(show_hide, 0, 3);
  gl->addWidget(chat_output, 1, 0, 1, 4);
  gl->addWidget(chat_line, 2, 0, 1, 2);
  gl->addWidget(cb, 2, 2);
  gl->addWidget(remove_links, 2, 3);

  connect(chat_output, &QTextBrowser::anchorClicked, this,
          &chat_widget::anchor_clicked);
  connect(remove_links, &QAbstractButton::clicked, this,
          &chat_widget::rm_links);
  connect(show_hide, &QAbstractButton::toggled, this,
          &chat_widget::set_chat_visible);
  setMouseTracking(true);

  chat_listener::listen();
}

/**
 * Destructor
 */
chat_widget::~chat_widget()
{
  // Just to be sure.
  delete cb_menu;
}

/**
 * Manages toggling minimization.
 */
void chat_widget::set_chat_visible(bool visible)
{
  // Don't update chat_fheight
  m_chat_visible = false;

  // Save the geometry before setting the minimum size
  auto geo = geometry();

  if (visible) {
    setMinimumSize(200, 100);
    setResizable(Qt::LeftEdge | Qt::RightEdge | Qt::TopEdge
                 | Qt::BottomEdge);
  } else {
    setMinimumSize(200, 0);
    setResizable({});
  }

  chat_line->setVisible(visible);
  chat_output->setVisible(visible);
  cb->setVisible(visible && !is_server_running());
  remove_links->setVisible(visible);

  int h = visible ? qRound(parentWidget()->size().height()
                           * king()->qt_settings.chat_fheight)
                  : sizeHint().height();

  // Heuristic that more or less works
  bool expand_up =
      (y() > parentWidget()->height() - y() - (visible ? h : height()));

  QString icon_name = (expand_up ^ visible) ? QLatin1String("expand-up")
                                            : QLatin1String("expand-down");
  show_hide->setIcon(fcIcons::instance()->getIcon(icon_name));

  if (expand_up) {
    geo.setTop(std::max(geo.bottom() - h, 0));
    geo.setHeight(h);
    // Prevent it from going out of screen
    if (geo.bottom() > parentWidget()->height()) {
      geo.translate(0, parentWidget()->height() - geo.bottom());
    }
  } else {
    geo.setBottom(std::min(geo.top() + h, parentWidget()->height()));
  }
  setGeometry(geo);

  m_chat_visible = visible;
}

/**
 * Shows the chat and ensures the chat line has focus
 */
void chat_widget::take_focus()
{
  show_hide->setChecked(true); // Make sure we're visible
  chat_line->setFocus(Qt::ShortcutFocusReason);
}

/**
 * Updates font for chat_widget
 */
void chat_widget::update_font()
{
  chat_output->setFont(fcFont::instance()->getFont(fonts::chatline));
}

/**
 * User clicked clear links button
 */
void chat_widget::rm_links() { link_marks_clear_all(); }

/**
 * User clicked some custom link
 */
void chat_widget::anchor_clicked(const QUrl &link)
{
  int n;
  QStringList sl;
  int id;
  enum text_link_type type;
  sl = link.toString().split(QStringLiteral(","));
  n = sl.at(0).toInt();
  id = sl.at(1).toInt();

  type = static_cast<text_link_type>(n);
  struct tile *ptile = nullptr;
  switch (type) {
  case TLT_CITY: {
    struct city *pcity = game_city_by_number(id);

    if (pcity) {
      ptile = client_city_tile(pcity);
    } else {
      output_window_append(ftc_client, _("This city isn't known!"));
    }
  } break;
  case TLT_TILE:
    ptile = index_to_tile(&(wld.map), id);

    if (!ptile) {
      output_window_append(ftc_client,
                           _("This tile doesn't exist in this game!"));
    }
    break;
  case TLT_UNIT: {
    struct unit *punit = game_unit_by_number(id);

    if (punit) {
      ptile = unit_tile(punit);
    } else {
      output_window_append(ftc_client, _("This unit isn't known!"));
    }
  }
  case TLT_INVALID:
    break;
  }
  if (ptile) {
    queen()->mapview_wdg->center_on_tile(ptile);
    link_mark_restore(type, id);
  }
}

/**
 * Adds news string to chat_widget (from chat_listener interface)
 */
void chat_widget::chat_message_received(const QString &message,
                                        const struct text_tag_list *tags)
{
  QColor col = chat_output->palette().color(QPalette::Text);
  append(apply_tags(message, tags, col));
}

/**
 * Adds news string to chat_widget
 */
void chat_widget::append(const QString &str)
{
  chat_output->append(str);
  chat_output->verticalScrollBar()->setSliderPosition(
      chat_output->verticalScrollBar()->maximum());
}

/**
 * Paint event for chat_widget
 */
void chat_widget::paintEvent(QPaintEvent *event)
{
  QStyleOption opt;
  opt.init(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void text_browser_dblclck::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
  emit dbl_clicked();
}

/**
 * Processess history for chat
 */
bool chat_widget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == chat_line) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Escape) {
        queen()->mapview_wdg->setFocus();
        return true;
      }
    }
    if (event->type() == QEvent::ShortcutOverride) {
      event->setAccepted(true);
    }
  }
  return QObject::eventFilter(obj, event);
}

/**
 * Hides allies and links button for local game
 */
void chat_widget::update_widgets()
{
  if (is_server_running()) {
    cb->hide();
  } else {
    cb->show();
  }
}

/**
 * Returns how much space chatline of given number of lines would require,
 * or zero if it can't be determined.
 */
int chat_widget::default_size(int lines)
{
  int line_count = 0;
  int line_height;
  int size;
  QTextBlock qtb;

  qtb = chat_output->document()->firstBlock();
  /* Count all lines in all text blocks layouts
   * document()->lineCount returns number of lines without wordwrap */

  while (qtb.isValid()) {
    line_count = line_count + qtb.layout()->lineCount();
    qtb = qtb.next();
  }

  if (line_count == 0) {
    return 0;
  }

  line_height = (chat_output->document()->size().height()
                 - 2 * chat_output->document()->documentMargin())
                / line_count;

  size = lines * line_height + chat_line->size().height()
         + chat_output->document()->documentMargin();
  size = qMax(0, size);

  return size;
}

/**
 * Makes link to tile/unit or city
 */
void chat_widget::make_link(struct tile *ptile)
{
  struct unit *punit;
  QString buf;

  punit = find_visible_unit(ptile);
  if (tile_city(ptile)) {
    buf = city_link(tile_city(ptile));
  } else if (punit) {
    buf = unit_link(punit);
  } else {
    buf = tile_link(ptile);
  }
  chat_line->insert(buf);
  chat_line->setFocus();
}

/**
 * Applies tags to text
 */
QString apply_tags(QString str, const struct text_tag_list *tags,
                   QColor bg_color)
{
  if (tags == nullptr) {
    return str;
  }

  // Tag offsets are relative to bytes.
  const auto qba = str.toUtf8();

  // Tags to insert into the text
  std::map<int, QString> html_tags;

  text_tag_list_iterate(tags, ptag)
  {
    int start = text_tag_start_offset(ptag);
    if (start == FT_OFFSET_UNSET) {
      start = 0;
    }

    int stop = text_tag_stop_offset(ptag);
    if (stop == FT_OFFSET_UNSET) {
      stop = qba.length();
    }

    if (start == stop) {
      // Get rid of empty tags
      continue;
    }

    // We always append opening tags and prepend closing tags
    // This corresponds to increasing depth in the tag tree
    switch (text_tag_type(ptag)) {
    case TTT_BOLD:
      html_tags[start] += QStringLiteral("<b>");
      html_tags[stop] = html_tags[stop].prepend(QStringLiteral("</b>"));
      break;
    case TTT_ITALIC:
      html_tags[start] += QStringLiteral("<i>");
      html_tags[stop] = html_tags[stop].prepend(QStringLiteral("</i>"));
      break;
    case TTT_STRIKE:
      html_tags[start] += QStringLiteral("<s>");
      html_tags[stop] = html_tags[stop].prepend(QStringLiteral("</s>"));
      break;
    case TTT_UNDERLINE:
      html_tags[start] += QStringLiteral("<u>");
      html_tags[stop] = html_tags[stop].prepend(QStringLiteral("</u>"));
      break;
    case TTT_COLOR: {
      QString style;
      if (!text_tag_color_foreground(ptag).isEmpty()) {
        auto color = text_tag_color_foreground(ptag);
        if (color_mapping.find(color) != color_mapping.end()) {
          color = color_mapping[color];
        }
        if (QColor::isValidColor(color)) {
          style += QStringLiteral("color:%1;").arg(color);
        }
      }
      if (!text_tag_color_background(ptag).isEmpty()) {
        auto color = text_tag_color_background(ptag);
        if (color_mapping.find(color) != color_mapping.end()) {
          color = color_mapping[color];
        }
        if (QColor::isValidColor(color)) {
          style += QStringLiteral("background-color:%1;").arg(color);
        }
      }
      html_tags[start] += QStringLiteral("<span style=\"%1\">").arg(style);
      html_tags[stop] = html_tags[stop].prepend(QStringLiteral("</span>"));
      break;
    }
    case TTT_LINK: {
      QColor pcolor;

      switch (text_tag_link_type(ptag)) {
      case TLT_CITY:
        pcolor = get_color(tileset, COLOR_MAPVIEW_CITY_LINK);
        break;
      case TLT_TILE:
        pcolor = get_color(tileset, COLOR_MAPVIEW_TILE_LINK);
        break;
      case TLT_UNIT:
        pcolor = get_color(tileset, COLOR_MAPVIEW_UNIT_LINK);
        break;
      case TLT_INVALID:
        break;
      }

      const auto color = pcolor.name(QColor::HexRgb);
      html_tags[start] += QStringLiteral("<font color=\"%1\">").arg(color);
      html_tags[start] += QString("<a href=%1,%2>")
                              .arg(QString::number(text_tag_link_type(ptag)),
                                   QString::number(text_tag_link_id(ptag)));
      html_tags[stop] =
          html_tags[stop].prepend(QStringLiteral("</a></font>"));
    } break;
    case TTT_INVALID:
      break;
    }
  }
  text_tag_list_iterate_end;

  // Insert the HTML markup in the text
  auto html = QStringLiteral();
  int last_position = 0;
  for (auto &[position, tags_to_insert] : html_tags) {
    html += QString(qba.mid(last_position, position - last_position))
                .toHtmlEscaped();
    html += tags_to_insert;
    last_position = position;
  }
  html += QString(qba.mid(last_position)).toHtmlEscaped();

  return html;
}

/**
 * Helper function to determine if a given client input line is intended as
 * a "plain" public message.
 */
static bool is_plain_public_message(const QString &s)
{
  QString s1, str;
  int i;

  str = s.trimmed();
  if (str.at(0) == SERVER_COMMAND_PREFIX || str.at(0) == CHAT_ALLIES_PREFIX
      || str.at(0) == CHAT_DIRECT_PREFIX) {
    return false;
  }

  // Search for private message
  if (!str.contains(CHAT_DIRECT_PREFIX)) {
    return true;
  }
  i = str.indexOf(CHAT_DIRECT_PREFIX);
  str = str.left(i);

  // Compare all players and connections looking for match
  conn_list_iterate(game.all_connections, pconn)
  {
    s1 = pconn->username;
    if (s1.length() < i) {
      continue;
    }
    if (!QString::compare(s1.left(i), str, Qt::CaseInsensitive)) {
      return false;
    }
  }
  conn_list_iterate_end;
  players_iterate(pplayer)
  {
    s1 = pplayer->name;
    if (s1.length() < i) {
      continue;
    }
    if (!QString::compare(s1.left(i), str, Qt::CaseInsensitive)) {
      return false;
    }
  }
  players_iterate_end;

  return true;
}

/**
 * Appends the string to the chat output window.  The string should be
 * inserted on its own line, although it will have no newline.
 */
void real_output_window_append(const QString &astring,
                               const text_tag_list *tags)
{
  king()->set_status_bar(astring);

  if (astring.contains(client.conn.username)) {
    qApp->alert(king()->central_wdg);
  }

  chat_listener::invoke(&chat_listener::chat_message_received, astring,
                        tags);
}

/**
 * Got version message from metaserver
 */
void version_message(const QString &vertext)
{
  king()->set_status_bar(vertext);
}
