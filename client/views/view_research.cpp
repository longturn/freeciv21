/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

/*
 * This file contains functions to generate the GUI for the research view
 * (formally known as the science dialog).
 */

// Qt
#include <QComboBox>
#include <QGridLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QToolTip>

// common
#include "game.h"
#include "government.h"
#include "research.h"

// client
#include "citydlg.h"
#include "client_main.h"
#include "climisc.h"
#include "fc_client.h"
#include "helpdlg.h"
#include "page_game.h"
#include "text.h"
#include "tileset/sprite.h"
#include "tooltips.h"
#include "top_bar.h"
#include "views/view_research.h"
#include "views/view_research_reqtree.h"

// server
#include "../server/techtools.h"

extern QString split_text(const QString &text, bool cut);
extern QString cut_helptext(const QString &text);
extern QString get_tooltip_improvement(impr_type *building,
                                       struct city *pcity, bool ext);
extern QString get_tooltip_unit(struct unit_type *unit, bool ext);

/**
   Compare unit_items (used for techs) by name
 */
bool comp_less_than(const qlist_item &q1, const qlist_item &q2)
{
  return (q1.tech_str < q2.tech_str);
}

/**
   Constructor for research diagram
 */
research_diagram::research_diagram(QWidget *parent) : QWidget(parent)
{
  pcanvas = nullptr;
  req = nullptr;
  reset();
  setMouseTracking(true);
  setAttribute(Qt::WA_StyledBackground);
}

/**
   Destructor for research diagram
 */
research_diagram::~research_diagram()
{
  delete tt_help;
  delete pcanvas;
  destroy_reqtree(req);
}

/**
   Recreates whole diagram and schedules update
 */
void research_diagram::update_reqtree()
{
  reset();
  delete tt_help;
  tt_help = draw_reqtree(req, pcanvas, 0, 0, 0, 0, width, height);
  update();
}

/**
   Initializes research diagram
 */
void research_diagram::reset()
{
  timer_active = false;
  if (req != nullptr) {
    destroy_reqtree(req);
  }

  delete pcanvas;
  req = create_reqtree(client_player(), true);
  get_reqtree_dimensions(req, &width, &height);
  pcanvas = new QPixmap(width, height);
  pcanvas->fill(Qt::transparent);
  resize(width, height);
}

/**
   Mouse handler for research_diagram
 */
void research_diagram::mousePressEvent(QMouseEvent *event)
{
  Tech_type_id tech = get_tech_on_reqtree(req, event->x(), event->y());
  req_tooltip_help *rttp;
  int i;

  if (event->button() == Qt::LeftButton && can_client_issue_orders()) {
    switch (research_invention_state(research_get(client_player()), tech)) {
    case TECH_PREREQS_KNOWN:
      dsend_packet_player_research(&client.conn, tech);
      break;
    case TECH_UNKNOWN:
      dsend_packet_player_tech_goal(&client.conn, tech);
      break;
    case TECH_KNOWN:
      break;
    }
  } else if (event->button() == Qt::RightButton) {
    for (i = 0; i < tt_help->count(); i++) {
      rttp = tt_help->at(i);
      if (rttp->rect.contains(event->pos())) {
        if (rttp->tech_id != -1) {
          popup_help_dialog_typed(
              qUtf8Printable(research_advance_name_translation(
                  research_get(client_player()), rttp->tech_id)),
              HELP_TECH);
        } else if (rttp->timpr != nullptr) {
          popup_help_dialog_typed(
              improvement_name_translation(rttp->timpr),
              is_great_wonder(rttp->timpr) ? HELP_WONDER : HELP_IMPROVEMENT);
        } else if (rttp->tunit != nullptr) {
          popup_help_dialog_typed(utype_name_translation(rttp->tunit),
                                  HELP_UNIT);
        } else if (rttp->tgov != nullptr) {
          popup_help_dialog_typed(government_name_translation(rttp->tgov),
                                  HELP_GOVERNMENT);
        } else {
          return;
        }
      }
    }
  }
}

/**
   Mouse move handler for research_diagram - for showing tooltips
 */
void research_diagram::mouseMoveEvent(QMouseEvent *event)
{
  req_tooltip_help *rttp;
  int i;
  QString tt_text;
  QString def_str;
  char buffer[8192];
  char buf2[1];

  buf2[0] = '\0';
  for (i = 0; i < tt_help->count(); i++) {
    rttp = tt_help->at(i);
    if (rttp->rect.contains(event->pos())) {
      if (rttp->tech_id != -1) {
        helptext_advance(buffer, sizeof(buffer), client.conn.playing, buf2,
                         rttp->tech_id, client_current_nation_set());
        tt_text = QString(buffer);
        def_str = "<p style='white-space:pre'><b>"
                  + QString(advance_name_translation(
                                advance_by_number(rttp->tech_id)))
                        .toHtmlEscaped()
                  + "</b>\n";
      } else if (rttp->timpr != nullptr) {
        def_str = get_tooltip_improvement(rttp->timpr, nullptr);
        tt_text = helptext_building(
            buffer, sizeof(buffer), client.conn.playing, nullptr,
            rttp->timpr, client_current_nation_set());
        tt_text = cut_helptext(tt_text);
      } else if (rttp->tunit != nullptr) {
        def_str = get_tooltip_unit(rttp->tunit);
        tt_text +=
            helptext_unit(buffer, sizeof(buffer), client.conn.playing, buf2,
                          rttp->tunit, client_current_nation_set());
        tt_text = cut_helptext(tt_text);
      } else if (rttp->tgov != nullptr) {
        helptext_government(buffer, sizeof(buffer), client.conn.playing,
                            buf2, rttp->tgov);
        tt_text = QString(buffer);
        tt_text = cut_helptext(tt_text);
        def_str = "<p style='white-space:pre'><b>"
                  + QString(government_name_translation(rttp->tgov))
                        .toHtmlEscaped()
                  + "</b>\n";
      } else {
        return;
      }
      tt_text = split_text(tt_text, true);
      tt_text = def_str + tt_text.toHtmlEscaped();
      tooltip_text = tt_text.trimmed();
      tooltip_rect = rttp->rect;
      tooltip_pos = event->globalPos();
      if (!timer_active) {
        timer_active = true;
        QTimer::singleShot(500, this, &research_diagram::show_tooltip);
      }
    }
  }
}

/**
   Slot for timer used to show tooltip
 */
void research_diagram::show_tooltip()
{
  QPoint cp;

  timer_active = false;
  cp = QCursor::pos();
  if (qAbs(cp.x() - tooltip_pos.x()) < 4
      && qAbs(cp.y() - tooltip_pos.y()) < 4) {
    QToolTip::showText(cp, tooltip_text, this, tooltip_rect);
  }
}

/**
   Paint event for research_diagram
 */
void research_diagram::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  QPainter painter;
  painter.begin(this);
  painter.drawPixmap(0, 0, width, height, *pcanvas);
  painter.end();
}

/**
   Returns size of research_diagram
 */
QSize research_diagram::size()
{
  QSize s;

  s.setWidth(width);
  ;
  s.setHeight(height);
  return s;
}

/**
   Consctructor for science_report
 */
science_report::science_report() : QWidget()
{
  QSize size;
  QSizePolicy size_expanding_policy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);
  QSizePolicy size_fixed_policy(QSizePolicy::Minimum, QSizePolicy::Minimum);

  goal_combo = new QComboBox();
  info_label = new QLabel();
  progress = new progress_bar(this);
  progress_label = new QLabel();
  researching_combo = new QComboBox();
  auto sci_layout = new QGridLayout();
  res_diag = new research_diagram();
  auto scroll = new QScrollArea();
  refresh_but = new QPushButton();
  refresh_but->setText(_("Refresh"));
  refresh_but->setToolTip(_("Press to refresh currently researched "
                            "technology calculation again."));
  refresh_but->setVisible(false);

  progress->setTextVisible(true);
  progress_label->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(progress_label, 0, 0, 1, 8);
  sci_layout->addWidget(researching_combo, 1, 0, 1, 3);
  sci_layout->addWidget(refresh_but, 1, 3, 1, 1);
  researching_combo->setSizePolicy(size_fixed_policy);
  refresh_but->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(progress, 1, 5, 1, 4);
  progress->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(goal_combo, 2, 0, 1, 3);
  goal_combo->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(info_label, 2, 5, 1, 4);
  info_label->setSizePolicy(size_fixed_policy);

  size = res_diag->size();
  res_diag->setMinimumSize(size);
  scroll->setAutoFillBackground(true);
  scroll->setPalette(QPalette(QColor(215, 215, 215)));
  scroll->setWidget(res_diag);
  scroll->setSizePolicy(size_expanding_policy);
  sci_layout->addWidget(scroll, 4, 0, 1, 10);

  QObject::connect(researching_combo,
                   QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &science_report::current_tech_changed);

  QObject::connect(refresh_but, &QAbstractButton::pressed, this,
                   &science_report::push_research);

  QObject::connect(goal_combo,
                   QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &science_report::goal_tech_changed);

  setLayout(sci_layout);
}

/**
   Destructor for science report
   Removes "SCI" string marking it as closed
   And frees given index on list marking it as ready for new widget
 */
science_report::~science_report()
{
  delete curr_list;
  delete goal_list;
  queen()->removeRepoDlg(QStringLiteral("SCI"));
}

/**
   Updates science_report and marks it as opened
   It has to be called soon after constructor.
   It could be in constructor but compiler will yell about not used variable
 */
void science_report::init(bool raise)
{
  Q_UNUSED(raise)
  queen()->gimmePlace(this, QStringLiteral("SCI"));
  index = queen()->addGameTab(this);
  queen()->game_tab_widget->setCurrentIndex(index);
  update_report();
}

/**
   Schedules paint event in some qt queue
 */
void science_report::redraw() { update(); }

/**
   Recalculates research diagram again and updates science report
 */
void science_report::reset_tree()
{
  QSize size;
  res_diag->reset();
  size = res_diag->size();
  res_diag->setMinimumSize(size);
  update();
}

/**
   Updates all important widgets on science_report
 */
void science_report::update_report()
{
  delete curr_list;
  delete goal_list;
  curr_list = nullptr;
  goal_list = nullptr;

  auto research = research_get(client_player());
  if (!research) {
    // Global observer
    goal_combo->clear();
    researching_combo->clear();
    info_label->setText(QString());
    progress_label->setText(QString());
    res_diag->reset();
    return;
  }

  int total;
  int done = research->bulbs_researched;
  QVariant qvar, qres;
  qlist_item item;

  if (research->researching != A_UNSET) {
    total = research->client.researching_cost;
  } else {
    total = -1;
  }

  curr_list = new QList<qlist_item>;
  goal_list = new QList<qlist_item>;
  progress_label->setText(science_dialog_text());
  progress_label->setAlignment(Qt::AlignHCenter);
  info_label->setAlignment(Qt::AlignHCenter);
  info_label->setText(get_science_goal_text(research->tech_goal));
  progress->setFormat(get_science_target_text());
  progress->setMinimum(0);
  progress->setMaximum(total);
  progress->set_pixmap(static_cast<int>(research->researching));

  if (done <= total) {
    progress->setValue(done);
  } else {
    done = total;
    progress->setValue(done);
  }

  /** Collect all techs which are reachable in the next step. */
  advance_index_iterate(A_FIRST, i)
  {
    if (TECH_PREREQS_KNOWN == research->inventions[i].state) {
      item.tech_str =
          QString::fromUtf8(advance_name_translation(advance_by_number(i)));
      item.id = i;
      curr_list->append(item);
    }
  }
  advance_index_iterate_end;

  /** Collect all techs which are reachable in next 10 steps. */
  advance_index_iterate(A_FIRST, i)
  {
    if (research_invention_reachable(research, i)
        && TECH_KNOWN != research->inventions[i].state
        && (i == research->tech_goal
            || 10 >= research->inventions[i].num_required_techs)) {
      item.tech_str =
          QString::fromUtf8(advance_name_translation(advance_by_number(i)));
      item.id = i;
      goal_list->append(item);
    }
  }
  advance_index_iterate_end;

  /** sort both lists */
  std::sort(goal_list->begin(), goal_list->end(), comp_less_than);
  std::sort(curr_list->begin(), curr_list->end(), comp_less_than);

  /** fill combo boxes */
  researching_combo->blockSignals(true);
  goal_combo->blockSignals(true);

  researching_combo->clear();
  goal_combo->clear();
  for (int i = 0; i < curr_list->count(); i++) {
    QIcon ic;

    auto sp = get_tech_sprite(tileset, curr_list->at(i).id);
    if (sp) {
      ic = QIcon(*sp);
    }
    qvar = curr_list->at(i).id;
    researching_combo->insertItem(i, ic, curr_list->at(i).tech_str, qvar);
  }

  for (int i = 0; i < goal_list->count(); i++) {
    QIcon ic;

    auto sp = get_tech_sprite(tileset, goal_list->at(i).id);
    if (sp) {
      ic = QIcon(*sp);
    }
    qvar = goal_list->at(i).id;
    goal_combo->insertItem(i, ic, goal_list->at(i).tech_str, qvar);
  }

  /** set current tech and goal */
  qres = research->researching;
  if (qres == A_UNSET || is_future_tech(research->researching)) {
    researching_combo->insertItem(
        0,
        research_advance_name_translation(research, research->researching),
        A_UNSET);
    researching_combo->setCurrentIndex(0);
  } else {
    for (int i = 0; i < researching_combo->count(); i++) {
      qvar = researching_combo->itemData(i);

      if (qvar == qres) {
        researching_combo->setCurrentIndex(i);
      }
    }
  }

  qres = research->tech_goal;

  if (qres == A_UNSET) {
    goal_combo->insertItem(0, Q_("?tech:None"), A_UNSET);
    goal_combo->setCurrentIndex(0);
  } else {
    for (int i = 0; i < goal_combo->count(); i++) {
      qvar = goal_combo->itemData(i);

      if (qvar == qres) {
        goal_combo->setCurrentIndex(i);
      }
    }
  }

  researching_combo->blockSignals(false);
  goal_combo->blockSignals(false);

  if (client_is_observer()) {
    researching_combo->setDisabled(true);
    goal_combo->setDisabled(true);
  } else {
    researching_combo->setDisabled(false);
    goal_combo->setDisabled(false);
  }

  // If tech leak happens we enable a button to force/push a refresh.
  if (done >= total) {
    refresh_but->setVisible(true);
  }

  update_reqtree();
}

/**
   Calls update for research_diagram
 */
void science_report::update_reqtree() { res_diag->update_reqtree(); }

/**
   Slot used when combo box with current tech changes
 */
void science_report::current_tech_changed(int changed_index)
{
  QVariant qvar;

  qvar = researching_combo->itemData(changed_index);

  if (researching_combo->hasFocus()) {
    if (can_client_issue_orders()) {
      dsend_packet_player_research(&client.conn, qvar.toInt());
    }
  }
}

/**
   Slot used when combo box with goal have been changed
 */
void science_report::goal_tech_changed(int changed_index)
{
  QVariant qvar;

  qvar = goal_combo->itemData(changed_index);

  if (goal_combo->hasFocus()) {
    if (can_client_issue_orders()) {
      dsend_packet_player_tech_goal(&client.conn, qvar.toInt());
    }
  }
}

/**
 * Push (redo) research when qty bulbs researched is
 * greater than the number needed
 */
void science_report::push_research()
{
  if (can_client_issue_orders()) {
    auto research = research_get(client_player());
    dsend_packet_player_research(&client.conn, research->researching);
  }
}

/**
   Update the science report.
 */
void real_science_report_dialog_update(void *unused)
{
  Q_UNUSED(unused)
  int i;
  science_report *sci_rep;
  bool blk = false;
  QWidget *w;
  QString str;

  if (nullptr != client.conn.playing) {
    struct research *research = research_get(client_player());
    if (research->researching == A_UNSET) {
      str = QString(_("none"));
    } else if (research->client.researching_cost != 0) {
      str =
          research_advance_name_translation(research, research->researching);
      str += QStringLiteral("\n");

      const int per_turn = get_bulbs_per_turn(nullptr, nullptr, nullptr);
      const int when = turns_to_research_done(research, per_turn);
      if (when >= 0) {
        // TRANS: current(+surplus)/total  (number of turns)
        str += QString(PL_("%1(+%2)/%3  (%4 turn)", "%1(+%2)/%3  (%4 turns)",
                           when))
                   .arg(research->bulbs_researched)
                   .arg(per_turn)
                   .arg(research->client.researching_cost)
                   .arg(when);
      } else {
        // TRANS: current(+surplus)/total  (number of turns)
        str += QString(_("%1(+%2)/%3  (never)"))
                   .arg(research->bulbs_researched)
                   .arg(per_turn)
                   .arg(research->client.researching_cost);
      }
    }
    if (research->researching == A_UNSET && research->tech_goal == A_UNSET
        && research->techs_researched < game.control.num_tech_types) {
      blk = true;
    }
  } else {
    str = QStringLiteral(" ");
  }

  if (blk) {
    queen()->sw_science->keep_blinking = true;
    queen()->sw_science->setCustomLabels(str);
    queen()->sw_science->sblink();
  } else {
    queen()->sw_science->keep_blinking = false;
    queen()->sw_science->setCustomLabels(str);
    queen()->sw_science->update();
  }
  queen()->updateSidebarTooltips();

  if (queen()->isRepoDlgOpen(QStringLiteral("SCI"))
      && !client_is_global_observer()) {
    i = queen()->gimmeIndexOf(QStringLiteral("SCI"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    sci_rep = reinterpret_cast<science_report *>(w);
    sci_rep->update_report();
  }
}

/**
   Closes science report
 */
void popdown_science_report()
{
  int i;
  science_report *sci_rep;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("SCI"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("SCI"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    sci_rep = reinterpret_cast<science_report *>(w);
    sci_rep->deleteLater();
  }
}

/**
   Resize and redraw the requirement tree.
 */
void science_report_dialog_redraw(void)
{
  int i;
  science_report *sci_rep;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("SCI"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("SCI"));
    if (queen()->game_tab_widget->currentIndex() == i) {
      w = queen()->game_tab_widget->widget(i);
      sci_rep = reinterpret_cast<science_report *>(w);
      sci_rep->redraw();
    }
  }
}

/**
   Display the science report.  Optionally raise it.
   Typically triggered by F6.
 */
void science_report_dialog_popup(bool raise)
{
  science_report *sci_rep;
  int i;
  QWidget *w;

  if (client_is_global_observer()) {
    return;
  }
  if (!queen()->isRepoDlgOpen(QStringLiteral("SCI"))) {
    sci_rep = new science_report;
    sci_rep->init(raise);
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("SCI"));
    w = queen()->game_tab_widget->widget(i);
    sci_rep = reinterpret_cast<science_report *>(w);
    if (queen()->game_tab_widget->currentIndex() == i) {
      sci_rep->redraw();
    } else if (raise) {
      queen()->game_tab_widget->setCurrentWidget(sci_rep);
    }
  }
}
