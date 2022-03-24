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

  // Chat completion word list.
  static QStringList word_list;

public:
  // Special value meaning "end of history".
  static const int HISTORY_END = -1;

  static void update_word_list();

  explicit chat_listener();

  virtual void chat_message_received(const QString &,
                                     const struct text_tag_list *);
  virtual void chat_word_list_changed(const QStringList &);

  void send_chat_message(const QString &message);

  int position_in_history() { return position; }
  QString back_in_history();
  QString forward_in_history();
  void reset_history_position();

  QStringList current_word_list() { return word_list; }
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
  ~chat_input() override;
  void chat_word_list_changed(const QStringList &) override;
  bool event(QEvent *event) override;

private:
  QCompleter *cmplt;
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
  void scroll_to_bottom();
  void take_focus();
  void update_font();
private slots:
  void set_chat_visible(bool visible);
  void update_menu() override {}
  void rm_links();
  void anchor_clicked(const QUrl &link);

protected:
  void paintEvent(QPaintEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  void chat_message_received(const QString &message,
                             const struct text_tag_list *tags) override;
  QTextBrowser *chat_output;
  QPushButton *remove_links;
  QPushButton *show_hide;
  QPushButton *cb;
  move_widget *mw;
};
