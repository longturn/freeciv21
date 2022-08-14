/*
   //           Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors.
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
#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QGridLayout>
#include <QHash>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
// common
#include "chat.h"
#include "chatline_common.h"
// client
#include "audio.h"
#include "client_main.h"
#include "climap.h"
#include "colors_common.h"
#include "connectdlg_common.h"
#include "control.h"
#include "game.h"
#include "mapview_common.h"
// gui-qt
#include "dialogs.h"
#include "fc_client.h"
#include "fonts.h"
#include "icons.h"
#include "mapview.h"
#include "messagewin.h"
#include "page_game.h"

static bool is_plain_public_message(const QString &s);

FC_CPP_DECLARE_LISTENER(chat_listener)
QStringList chat_listener::history = QStringList();
QStringList chat_listener::word_list = QStringList();

namespace {

QHash<QString, QString> color_mapping;

} // namespace

/**
   Sets color substitution map.
 */
void set_chat_colors(const QHash<QString, QString> &colors)
{
  color_mapping = colors;
}

/**
   Updates the chat completion word list.
 */
void chat_listener::update_word_list()
{
  QString str;

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

  players_iterate(pplayer)
  {
    str = pplayer->name;
    if (!word_list.contains(str)) {
      word_list << str;
    }
  }
  players_iterate_end

      invoke(&chat_listener::chat_word_list_changed, word_list);
}

/**
   Constructor.
 */
chat_listener::chat_listener() : position(HISTORY_END) {}

/**
   Called whenever a message is received. Default implementation does
   nothing.
 */
void chat_listener::chat_message_received(const QString &,
                                          const struct text_tag_list *)
{
}

/**
   Called whenever the completion word list changes. Default implementation
   does nothing.
 */
void chat_listener::chat_word_list_changed(const QStringList &) {}

/**
   Sends commands to server, but first searches for custom keys, if it finds
   then it makes custom action.

   The history position is reset to HISTORY_END.
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
    if (client_state() >= C_S_RUNNING && gui_options.gui_qt_allied_chat_only
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
   Goes back one position in history, and returns the message at the new
   position.
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
   Goes forward one position in history, and returns the message at the new
   position. An empty string is returned if the new position is HISTORY_END.
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
   Go to the end of the history.
 */
void chat_listener::reset_history_position() { position = HISTORY_END; }

/**
   Constructor
 */
chat_input::chat_input(QWidget *parent) : QLineEdit(parent)
{
  connect(this, &QLineEdit::returnPressed, this, &chat_input::send);
  chat_word_list_changed(current_word_list());
  chat_listener::listen();
}

chat_input::~chat_input() { delete cmplt; }
/**
   Sends the content of the input box
 */
void chat_input::send()
{
  send_chat_message(text());
  clear();
}

/**
   Called whenever the completion word list changes.
 */
void chat_input::chat_word_list_changed(const QStringList &word_list)
{
  cmplt = completer();
  delete cmplt;
  cmplt = new QCompleter(word_list);
  cmplt->setCaseSensitivity(Qt::CaseInsensitive);
  cmplt->setCompletionMode(QCompleter::InlineCompletion);
  setCompleter(cmplt);
}

/**
   Event handler for chat_input, used for history
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
   Constructor for chat_widget
 */
chat_widget::chat_widget(QWidget *parent)
{
  QGridLayout *gl;
  setParent(parent);
  setMinimumSize(200, 100);
  setResizable(resizable_flag::top | resizable_flag::topLeft
               | resizable_flag::topRight | resizable_flag::bottom
               | resizable_flag::bottomLeft | resizable_flag::bottomRight
               | resizable_flag::left | resizable_flag::right);

  gl = new QGridLayout;
  gl->setVerticalSpacing(0);
  gl->setMargin(0);
  setLayout(gl);

  cb = new QPushButton();
  cb->setIconSize(QSize(24, 24));
  cb->setFixedWidth(25);
  cb->setFixedHeight(25);
  cb->setIcon(fcIcons::instance()->getIcon("private"));
  cb->setToolTip(_("Allies only"));
  cb->setCheckable(true);

  chat_line = new chat_input;
  chat_line->installEventFilter(this);

  remove_links = new QPushButton();
  remove_links->setIconSize(QSize(24, 24));
  remove_links->setFixedWidth(25);
  remove_links->setFixedHeight(25);
  remove_links->setIcon(fcIcons::instance()->getIcon("erase"));
  remove_links->setToolTip(_("Clear links"));

  show_hide = new QPushButton();
  show_hide->setIcon(fcIcons::instance()->getIcon("expand-down"));
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

  connect(cb, &QAbstractButton::toggled, [=](bool priv) {
    QString icon_name =
        priv ? QLatin1String("public") : QLatin1String("private");
    cb->setIcon(fcIcons::instance()->getIcon(icon_name));
    gui_options.gui_qt_allied_chat_only = priv;
  });
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
   Manages toggling minimization.
 */
void chat_widget::set_chat_visible(bool visible)
{
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

  auto geo = geometry();
  if (expand_up) {
    geo.setTop(std::max(geo.bottom() - h, 0));
    geo.setHeight(h);
  } else {
    geo.setBottom(std::min(geo.top() + h, parentWidget()->height()));
  }
  setGeometry(geo);
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
   Updates font for chat_widget
 */
void chat_widget::update_font()
{
  chat_output->setFont(fcFont::instance()->getFont(fonts::chatline));
}

/**
   User clicked clear links button
 */
void chat_widget::rm_links() { link_marks_clear_all(); }

/**
   User clicked some custom link
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
   Adds news string to chat_widget (from chat_listener interface)
 */
void chat_widget::chat_message_received(const QString &message,
                                        const struct text_tag_list *tags)
{
  QColor col = chat_output->palette().color(QPalette::Text);
  append(apply_tags(message, tags, col));
}

/**
   Adds news string to chat_widget
 */
void chat_widget::append(const QString &str)
{
  chat_output->append(str);
  chat_output->verticalScrollBar()->setSliderPosition(
      chat_output->verticalScrollBar()->maximum());
}

/**
   Paint event for chat_widget
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
   Processess history for chat
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
   Hides allies and links button for local game
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
   Returns how much space chatline of given number of lines would require,
   or zero if it can't be determined.
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
   Makes link to tile/unit or city
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
   Applies tags to text
 */
QString apply_tags(QString str, const struct text_tag_list *tags,
                   QColor bg_color)
{
  int start, stop, last_i;
  QString str_col;
  QString color;
  QString final_string;
  QString style;
  QByteArray qba;
  QMultiMap<int, QString> mm;
  QByteArray str_bytes;

  if (tags == nullptr) {
    return str;
  }
  str_bytes = str.toLocal8Bit();
  qba = str_bytes.data();

  text_tag_list_iterate(tags, ptag)
  {
    if ((text_tag_stop_offset(ptag) == FT_OFFSET_UNSET)) {
      stop = qba.count();
    } else {
      stop = text_tag_stop_offset(ptag);
    }

    if ((text_tag_start_offset(ptag) == FT_OFFSET_UNSET)) {
      start = 0;
    } else {
      start = text_tag_start_offset(ptag);
    }
    switch (text_tag_type(ptag)) {
    case TTT_BOLD:
      mm.insert(stop, QStringLiteral("</b>"));
      mm.insert(start, QStringLiteral("<b>"));
      break;
    case TTT_ITALIC:
      mm.insert(stop, QStringLiteral("</i>"));
      mm.insert(start, QStringLiteral("<i>"));
      break;
    case TTT_STRIKE:
      mm.insert(stop, QStringLiteral("</s>"));
      mm.insert(start, QStringLiteral("<s>"));
      break;
    case TTT_UNDERLINE:
      mm.insert(stop, QStringLiteral("</u>"));
      mm.insert(start, QStringLiteral("<u>"));
      break;
    case TTT_COLOR:
      if (text_tag_color_foreground(ptag)) {
        color = text_tag_color_foreground(ptag);
        if (color_mapping.find(color) != color_mapping.end()) {
          color = color_mapping[color];
        }
        if (QColor::isValidColor(color)) {
          style += QStringLiteral("color:%1;").arg(color);
        }
      }
      if (text_tag_color_background(ptag)) {
        color = text_tag_color_background(ptag);
        if (color_mapping.find(color) != color_mapping.end()) {
          color = color_mapping[color];
        }
        if (QColor::isValidColor(color)) {
          style += QStringLiteral("background-color:%1;").arg(color);
        }
      }
      mm.insert(stop, QStringLiteral("</span>"));
      mm.insert(start, QStringLiteral("<span style=\"%1\">").arg(style));

      break;
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

      color = pcolor.name(QColor::HexRgb);
      str_col = QStringLiteral("<font color=\"%1\">").arg(color);
      mm.insert(stop, QStringLiteral("</a></font>"));

      color = QString(str_col + "<a href=%1,%2>")
                  .arg(QString::number(text_tag_link_type(ptag)),
                       QString::number(text_tag_link_id(ptag)));
      mm.insert(start, color);
    } break;
    case TTT_INVALID:
      break;
    }
  }
  text_tag_list_iterate_end;

  // insert html starting from last items
  last_i = str.count();
  QMultiMap<int, QString>::const_iterator i = mm.constEnd();
  QMultiMap<int, QString>::const_iterator j = mm.constEnd();
  while (i != mm.constBegin()) {
    --i;
    if (i.key() < last_i) {
      final_string = final_string.prepend(
          QString(qba.mid(i.key(), last_i - i.key())).toHtmlEscaped());
    }
    last_i = i.key();
    j = i;
    if (i != mm.constBegin()) {
      --j;
    }
    if (j.key() == i.key() && i != j) {
      final_string = final_string.prepend(j.value());
      final_string = final_string.prepend(i.value());
      --i;
    } else {
      final_string = final_string.prepend(i.value());
    }
  }
  if (last_i == str.count()) {
    return str;
  }

  return final_string;
}

/**
   Helper function to determine if a given client input line is intended as
   a "plain" public message.
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
   Appends the string to the chat output window.  The string should be
   inserted on its own line, although it will have no newline.
 */
void real_output_window_append(const QString &astring,
                               const text_tag_list *tags)
{
  king()->set_status_bar(astring);

  if (astring.contains(client.conn.username)) {
    qApp->alert(king()->central_wdg);
  }

  chat_listener::update_word_list();
  chat_listener::invoke(&chat_listener::chat_message_received, astring,
                        tags);
}

/**
   Got version message from metaserver
 */
void version_message(const QString &vertext)
{
  king()->set_status_bar(vertext);
}
