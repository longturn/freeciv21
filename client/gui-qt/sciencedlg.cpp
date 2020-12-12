/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QComboBox>
#include <QGridLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QToolTip>
// common
#include "game.h"
#include "government.h"
#include "research.h"
// client
#include "client_main.h"
#include "helpdata.h"
#include "reqtree.h"
#include "sprite.h"
#include "text.h"
// gui-qt
#include "canvas.h"
#include "citydlg.h"
#include "fc_client.h"
#include "page_game.h"
#include "sciencedlg.h"
#include "sidebar.h"
#include "tooltips.h"

extern QString split_text(const QString &text, bool cut);
extern QString cut_helptext(const QString &text);
extern QString get_tooltip_improvement(impr_type *building,
                                       struct city *pcity, bool ext);
extern QString get_tooltip_unit(struct unit_type *unit, bool ext);
extern QApplication *qapp;

/************************************************************************/ /**
   Compare unit_items (used for techs) by name
 ****************************************************************************/
bool comp_less_than(const qlist_item &q1, const qlist_item &q2)
{
  return (q1.tech_str < q2.tech_str);
}

/************************************************************************/ /**
   Constructor for research diagram
 ****************************************************************************/
research_diagram::research_diagram(QWidget *parent) : QWidget(parent)
{
  pcanvas = NULL;
  req = NULL;
  reset();
  setMouseTracking(true);
}

/************************************************************************/ /**
   Destructor for research diagram
 ****************************************************************************/
research_diagram::~research_diagram()
{
  canvas_free(pcanvas);
  destroy_reqtree(req);
  qDeleteAll(tt_help);
  tt_help.clear();
}

/************************************************************************/ /**
   Constructor for req_tooltip_help
 ****************************************************************************/
req_tooltip_help::req_tooltip_help()
    : tech_id(-1), tunit(nullptr), timpr(nullptr), tgov(nullptr)
{
}

/************************************************************************/ /**
   Create list of rectangles for showing tooltips
 ****************************************************************************/
void research_diagram::create_tooltip_help()
{
  int i, j;
  int swidth, sheight;
  struct sprite *sprite;
  reqtree *tree;
  req_tooltip_help *rttp;

  qDeleteAll(tt_help);
  tt_help.clear();
  if (req == nullptr) {
    return;
  } else {
    tree = req;
  }

  for (i = 0; i < tree->num_layers; i++) {
    for (j = 0; j < tree->layer_size[i]; j++) {
      struct tree_node *node = tree->layers[i][j];
      int startx, starty, nwidth, nheight;

      startx = node->node_x;
      starty = node->node_y;
      nwidth = node->node_width;
      nheight = node->node_height;

      if (!node->is_dummy) {
        QString text = QString(research_advance_name_translation(
            research_get(client_player()), node->tech));
        int text_w, text_h;
        int icon_startx;

        get_text_size(&text_w, &text_h, FONT_REQTREE_TEXT, text);
        rttp = new req_tooltip_help();
        rttp->rect = QRect(startx + (nwidth - text_w) / 2, starty + 4,
                           text_w, text_h);
        rttp->tech_id = node->tech;
        tt_help.append(rttp);
        icon_startx = startx + 5;

        if (gui_options.reqtree_show_icons) {
          unit_type_iterate(unit)
          {
            if (advance_number(unit->require_advance) != node->tech) {
              continue;
            }
            sprite =
                get_unittype_sprite(tileset, unit, direction8_invalid());
            get_sprite_dimensions(sprite, &swidth, &sheight);
            rttp = new req_tooltip_help();
            rttp->rect = QRect(icon_startx,
                               starty + text_h + 4
                                   + (nheight - text_h - 4 - sheight) / 2,
                               swidth, sheight);
            rttp->tunit = unit;
            tt_help.append(rttp);
            icon_startx += swidth + 2;
          }
          unit_type_iterate_end;

          improvement_iterate(pimprove)
          {
            requirement_vector_iterate(&(pimprove->reqs), preq)
            {
              if (VUT_ADVANCE == preq->source.kind
                  && advance_number(preq->source.value.advance)
                         == node->tech) {
                sprite = get_building_sprite(tileset, pimprove);
                if (sprite) {
                  get_sprite_dimensions(sprite, &swidth, &sheight);
                  rttp = new req_tooltip_help();
                  rttp->rect =
                      QRect(icon_startx,
                            starty + text_h + 4
                                + (nheight - text_h - 4 - sheight) / 2,
                            swidth, sheight);
                  rttp->timpr = pimprove;
                  tt_help.append(rttp);
                  icon_startx += swidth + 2;
                }
              }
            }
            requirement_vector_iterate_end;
          }
          improvement_iterate_end;

          governments_iterate(gov)
          {
            requirement_vector_iterate(&(gov->reqs), preq)
            {
              if (VUT_ADVANCE == preq->source.kind
                  && advance_number(preq->source.value.advance)
                         == node->tech) {
                sprite = get_government_sprite(tileset, gov);
                get_sprite_dimensions(sprite, &swidth, &sheight);
                rttp = new req_tooltip_help();
                rttp->rect =
                    QRect(icon_startx,
                          starty + text_h + 4
                              + (nheight - text_h - 4 - sheight) / 2,
                          swidth, sheight);
                rttp->tgov = gov;
                tt_help.append(rttp);
                icon_startx += swidth + 2;
              }
            }
            requirement_vector_iterate_end;
          }
          governments_iterate_end;
        }
      }
    }
  }
}

/************************************************************************/ /**
   Recreates whole diagram and schedules update
 ****************************************************************************/
void research_diagram::update_reqtree()
{
  reset();
  draw_reqtree(req, pcanvas, 0, 0, 0, 0, width, height);
  create_tooltip_help();
  update();
}

/************************************************************************/ /**
   Initializes research diagram
 ****************************************************************************/
void research_diagram::reset()
{
  timer_active = false;
  if (req != NULL) {
    destroy_reqtree(req);
  }
  if (pcanvas != NULL) {
    canvas_free(pcanvas);
  }
  req = create_reqtree(client_player(), true);
  get_reqtree_dimensions(req, &width, &height);
  pcanvas = qtg_canvas_create(width, height);
  pcanvas->map_pixmap.fill(Qt::transparent);
  resize(width, height);
}

/************************************************************************/ /**
   Mouse handler for research_diagram
 ****************************************************************************/
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
    for (i = 0; i < tt_help.count(); i++) {
      rttp = tt_help.at(i);
      if (rttp->rect.contains(event->pos())) {
        if (rttp->tech_id != -1) {
          popup_help_dialog_typed(
              research_advance_name_translation(
                  research_get(client_player()), rttp->tech_id),
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

/************************************************************************/ /**
   Mouse move handler for research_diagram - for showing tooltips
 ****************************************************************************/
void research_diagram::mouseMoveEvent(QMouseEvent *event)
{
  req_tooltip_help *rttp;
  int i;
  QString tt_text;
  QString def_str;
  char buffer[8192];
  char buf2[1];

  buf2[0] = '\0';
  for (i = 0; i < tt_help.count(); i++) {
    rttp = tt_help.at(i);
    if (rttp->rect.contains(event->pos())) {
      if (rttp->tech_id != -1) {
        helptext_advance(buffer, sizeof(buffer), client.conn.playing, buf2,
                         rttp->tech_id);
        tt_text = QString(buffer);
        def_str = "<p style='white-space:pre'><b>"
                  + QString(advance_name_translation(
                                advance_by_number(rttp->tech_id)))
                        .toHtmlEscaped()
                  + "</b>\n";
      } else if (rttp->timpr != nullptr) {
        def_str = get_tooltip_improvement(rttp->timpr, nullptr);
        tt_text = helptext_building(buffer, sizeof(buffer),
                                    client.conn.playing, NULL, rttp->timpr);
        tt_text = cut_helptext(tt_text);
      } else if (rttp->tunit != nullptr) {
        def_str = get_tooltip_unit(rttp->tunit);
        tt_text += helptext_unit(buffer, sizeof(buffer), client.conn.playing,
                                 buf2, rttp->tunit);
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
      if (!QToolTip::isVisible() && !timer_active) {
        timer_active = true;
        QTimer::singleShot(500, this, &research_diagram::show_tooltip);
      }
    }
  }
}

/************************************************************************/ /**
   Slot for timer used to show tooltip
 ****************************************************************************/
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

/************************************************************************/ /**
   Paint event for research_diagram
 ****************************************************************************/
void research_diagram::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)
  QPainter painter;

  painter.begin(this);
  painter.drawPixmap(0, 0, width, height, pcanvas->map_pixmap);
  painter.end();
}

/************************************************************************/ /**
   Returns size of research_diagram
 ****************************************************************************/
QSize research_diagram::size()
{
  QSize s;

  s.setWidth(width);
  ;
  s.setHeight(height);
  return s;
}

/************************************************************************/ /**
   Consctructor for science_report
 ****************************************************************************/
science_report::science_report()
    : QWidget(), curr_list(nullptr), goal_list(nullptr), index(0)
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
  sci_layout = new QGridLayout();
  res_diag = new research_diagram();
  scroll = new QScrollArea();

  progress->setTextVisible(true);
  progress_label->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(progress_label, 0, 0, 1, 8);
  sci_layout->addWidget(researching_combo, 1, 0, 1, 4);
  researching_combo->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(progress, 1, 5, 1, 4);
  progress->setSizePolicy(size_fixed_policy);
  sci_layout->addWidget(goal_combo, 2, 0, 1, 4);
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

  QObject::connect(researching_combo, SIGNAL(currentIndexChanged(int)),
                   SLOT(current_tech_changed(int)));

  QObject::connect(goal_combo, SIGNAL(currentIndexChanged(int)),
                   SLOT(goal_tech_changed(int)));

  setLayout(sci_layout);
}

/************************************************************************/ /**
   Destructor for science report
   Removes "SCI" string marking it as closed
   And frees given index on list marking it as ready for new widget
 ****************************************************************************/
science_report::~science_report()
{
  if (curr_list) {
    delete curr_list;
  }

  if (goal_list) {
    delete goal_list;
  }
  queen()->removeRepoDlg(QStringLiteral("SCI"));
}

/************************************************************************/ /**
   Updates science_report and marks it as opened
   It has to be called soon after constructor.
   It could be in constructor but compiler will yell about not used variable
 ****************************************************************************/
void science_report::init(bool raise)
{
  Q_UNUSED(raise)
  queen()->gimmePlace(this, QStringLiteral("SCI"));
  index = queen()->addGameTab(this);
  queen()->game_tab_widget->setCurrentIndex(index);
  update_report();
}

/************************************************************************/ /**
   Schedules paint event in some qt queue
 ****************************************************************************/
void science_report::redraw() { update(); }

/************************************************************************/ /**
   Recalculates research diagram again and updates science report
 ****************************************************************************/
void science_report::reset_tree()
{
  QSize size;
  res_diag->reset();
  size = res_diag->size();
  res_diag->setMinimumSize(size);
  update();
}

/************************************************************************/ /**
   Updates all important widgets on science_report
 ****************************************************************************/
void science_report::update_report()
{

  struct research *research = research_get(client_player());
  const char *text;
  int total;
  int done = research->bulbs_researched;
  QVariant qvar, qres;
  double not_used;
  QString str;
  qlist_item item;
  struct sprite *sp;

  fc_assert_ret(NULL != research);

  if (curr_list) {
    delete curr_list;
  }

  if (goal_list) {
    delete goal_list;
  }

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
  text = get_science_target_text(&not_used);
  str = QString::fromUtf8(text);
  progress->setFormat(str);
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

    sp = get_tech_sprite(tileset, curr_list->at(i).id);
    if (sp) {
      ic = QIcon(*sp->pm);
    }
    qvar = curr_list->at(i).id;
    researching_combo->insertItem(i, ic, curr_list->at(i).tech_str, qvar);
  }

  for (int i = 0; i < goal_list->count(); i++) {
    QIcon ic;

    sp = get_tech_sprite(tileset, goal_list->at(i).id);
    if (sp) {
      ic = QIcon(*sp->pm);
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
  update_reqtree();
}

/************************************************************************/ /**
   Calls update for research_diagram
 ****************************************************************************/
void science_report::update_reqtree() { res_diag->update_reqtree(); }

/************************************************************************/ /**
   Slot used when combo box with current tech changes
 ****************************************************************************/
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

/************************************************************************/ /**
   Slot used when combo box with goal have been changed
 ****************************************************************************/
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

/************************************************************************/ /**
   Update the science report.
 ****************************************************************************/
void real_science_report_dialog_update(void *unused)
{
  Q_UNUSED(unused)
  int i;
  int percent;
  science_report *sci_rep;
  bool blk = false;
  QWidget *w;
  QString str;

  if (NULL != client.conn.playing) {
    struct research *research = research_get(client_player());
    if (research->researching == A_UNSET) {
      str = QString(_("none"));
    } else if (research->client.researching_cost != 0) {
      str =
          research_advance_name_translation(research, research->researching);
      percent = 100 * research->bulbs_researched
                / research->client.researching_cost;
      str = str + "\n (" + QString::number(percent) + "%)";
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
    queen()->sw_science->updateFinalPixmap();
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

/************************************************************************/ /**
   Closes science report
 ****************************************************************************/
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

/************************************************************************/ /**
   Resize and redraw the requirement tree.
 ****************************************************************************/
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

/************************************************************************/ /**
   Display the science report.  Optionally raise it.
   Typically triggered by F6.
 ****************************************************************************/
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
