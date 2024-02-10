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
// Qt
#include <QDialog>
#include <QHash>
#include <QList>
// common
#include "extras.h"
// client
#include "dialogs.h"
#include "helpdata.h"

class QCloseEvent;
class QFrame;
class QHideEvent;
class QLabel;
class QLayout;
class QObject;
class QPixmap;
class QPushButton;
class QShowEvent;
class QSplitter;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class help_widget;
struct help_item;

class help_dialog : public qfc_dialog {
  Q_OBJECT
  QPushButton *prev_butt;
  QPushButton *next_butt;
  QTreeWidget *tree_wdg;
  help_widget *help_wdg;
  QSplitter *splitter;
  QList<QTreeWidgetItem *> item_history;
  QHash<QTreeWidgetItem *, const help_item *> topics_map;
  int history_pos;
  void make_tree();

public:
  help_dialog(QWidget *parent = 0);
  void update_fonts();
  bool update_history;

public slots:
  void set_topic(const help_item *item);
  void history_forward();
  void history_back();

protected:
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private slots:
  void item_changed(QTreeWidgetItem *item, QTreeWidgetItem *prev);

private:
  void update_buttons();
};

class help_widget : public QWidget {
  Q_OBJECT
  QFrame *box_wdg;
  QLabel *title_label;

  QWidget *main_widget;
  QTextBrowser *text_browser;
  QWidget *bottom_panel;
  QWidget *info_panel;
  QSplitter *splitter;
  QVBoxLayout *info_layout;
  QList<int> splitter_sizes;

  void setup_ui();

  void do_layout();
  void undo_layout();

  void show_info_panel();
  void add_info_pixmap(const QPixmap *pm, bool shadow = false);
  void add_info_label(const QString &text);
  void add_info_progress(const QString &label, int progress, int min,
                         int max, const QString &value = QString());
  void add_extras_of_act_for_terrain(struct terrain *pterr,
                                     enum unit_activity act,
                                     const char *label);
  void add_info_separator();
  void info_panel_done();

  void set_bottom_panel(QWidget *widget);

  QLayout *create_terrain_widget(const QString &title, const QPixmap *image,
                                 const int &food, const int &sh,
                                 const int &eco,
                                 const QString &tooltip = QString());

  void set_topic_other(const help_item *item, const char *title);

  void set_topic_unit(const help_item *item, const char *title);
  void set_topic_building(const help_item *item, const char *title);
  void set_topic_tech(const help_item *item, const char *title);
  void set_topic_terrain(const help_item *item, const char *title);
  void set_topic_extra(const help_item *item, const char *title);
  void set_topic_specialist(const help_item *item, const char *title);
  void set_topic_government(const help_item *item, const char *title);
  void set_topic_nation(const help_item *item, const char *title);
  void set_topic_goods(const help_item *item, const char *title);
  void make_terrain_lab(QString &str);

public:
  help_widget(QWidget *parent = 0);
  help_widget(const help_item *item, QWidget *parent = 0);
  ~help_widget() override;
  void update_fonts();

private:
  QString link_me(const char *str, help_page_type hpt);

public slots:
  void set_topic(const help_item *item);
private slots:
  void anchor_clicked(const QString &link);

public:
  struct terrain *terrain_max_values();
  struct unit_type *uclass_max_values(struct unit_class *uclass);
};

void update_help_fonts();
void popup_help_dialog_typed(const char *item, help_page_type htype);
void popdown_help_dialog();
