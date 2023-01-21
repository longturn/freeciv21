/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#pragma once

// Qt
#include <QEvent>
#include <QLineEdit>
#include <QTextBrowser>
// gui-qt
#include "listener.h"
#include "widgetdecorations.h"

class QCheckBox;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;
class QPushButton;
class QUrl;
class chat_listener;

void set_chat_colors(const QHash<QString, QString> &colors);
QString apply_tags(QString str, const struct text_tag_list *tags,
                   QColor bg_color);
template <> std::set<chat_listener *> listener<chat_listener>::instances;
/***************************************************************************
  Listener for chat. See listener<> for information about how to use it
***************************************************************************/
class chat_listener : public listener<chat_listener> {
  // History is shared among all instances...
  static QStringList history;
  // ...but each has its own position.
  int position;

public:
  // Special value meaning "end of history".
  static const int HISTORY_END = -1;

  explicit chat_listener();

  virtual void chat_message_received(const QString &,
                                     const struct text_tag_list *);

  void send_chat_message(const QString &message);

  QString back_in_history();
  QString forward_in_history();
  void reset_history_position();
};

/***************************************************************************
  Chat input widget
***************************************************************************/
class chat_input : public QLineEdit, private chat_listener {
  Q_OBJECT

private slots:
  void send();

public:
  explicit chat_input(QWidget *parent = nullptr);

protected:
  bool event(QEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;

private:
  void update_completion();
};

/***************************************************************************
  Text browser with mouse double click signal
***************************************************************************/
class text_browser_dblclck : public QTextBrowser {
  Q_OBJECT

public:
  explicit text_browser_dblclck(QWidget *parent = nullptr)
      : QTextBrowser(parent)
  {
  }
signals:
  void dbl_clicked();

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;
};

/***************************************************************************
  Class for chat widget
***************************************************************************/
class chat_widget : public resizable_widget, private chat_listener {
  Q_OBJECT

public:
  chat_widget(QWidget *parent);
  void append(const QString &str);
  chat_input *chat_line;
  void make_link(struct tile *ptile);
  void update_widgets();
  int default_size(int lines);
  void take_focus();
  void update_font();

  /// Returns whether the chat widget is currently visible.
  bool is_chat_visible() const { return m_chat_visible; }
  void set_chat_visible(bool visible);

private slots:
  void update_menu() override {}
  void rm_links();
  void anchor_clicked(const QUrl &link);

protected:
  void paintEvent(QPaintEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  void chat_message_received(const QString &message,
                             const struct text_tag_list *tags) override;

  bool m_chat_visible = true;
  QTextBrowser *chat_output;
  QPushButton *remove_links;
  QPushButton *show_hide;
  QPushButton *cb;
  move_widget *mw;
};

void real_output_window_append(const QString &astring,
                               const text_tag_list *tags);
void version_message(const QString &vertext);
