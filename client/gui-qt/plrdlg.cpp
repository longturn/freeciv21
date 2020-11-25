/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "plrdlg.h"
// Qt
#include <QMouseEvent>
#include <QPainter>
#include <QSortFilterProxyModel>
// utility
#include "fcintl.h"
// common
#include "colors_common.h"
#include "game.h"
#include "government.h"
#include "research.h"
// client
#include "chatline_common.h"
#include "client_main.h"
#include "inteldlg_g.h"
// gui-qt
#include "colors.h"
#include "fc_client.h"
#include "fonts.h"
#include "page_game.h"
#include "sprite.h"

/**********************************************************************/ /**
   Help function to draw checkbox inside delegate
 **************************************************************************/
static QRect
check_box_rect(const QStyleOptionViewItem &view_item_style_options)
{
  QStyleOptionButton cbso;
  QRect check_box_rect = QApplication::style()->subElementRect(
      QStyle::SE_CheckBoxIndicator, &cbso);
  QPoint check_box_point(view_item_style_options.rect.x()
                             + view_item_style_options.rect.width() / 2
                             - check_box_rect.width() / 2,
                         view_item_style_options.rect.y()
                             + view_item_style_options.rect.height() / 2
                             - check_box_rect.height() / 2);
  return QRect(check_box_point, check_box_rect.size());
}

/**********************************************************************/ /**
   Slighty increase deafult cell height
 **************************************************************************/
QSize plr_item_delegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
  QSize r;

  r = QItemDelegate::sizeHint(option, index);
  r.setHeight(r.height() + 4);
  return r;
}

/**********************************************************************/ /**
   Paint evenet for custom player item delegation
 **************************************************************************/
void plr_item_delegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
  QStyleOptionButton but;
  QStyleOptionButton cbso;
  bool b;
  QString str;
  QRect rct;
  QPixmap pix(16, 16);

  QStyleOptionViewItem opt = QItemDelegate::setOptions(index, option);
  painter->save();
  switch (player_dlg_columns[index.column()].type) {
  case COL_FLAG:
    QItemDelegate::drawBackground(painter, opt, index);
    QItemDelegate::drawDecoration(painter, opt, option.rect,
                                  index.data().value<QPixmap>());
    break;
  case COL_COLOR:
    pix.fill(index.data().value<QColor>());
    QItemDelegate::drawBackground(painter, opt, index);
    QItemDelegate::drawDecoration(painter, opt, option.rect, pix);
    break;
  case COL_BOOLEAN:
    b = index.data().toBool();
    QItemDelegate::drawBackground(painter, opt, index);
    cbso.state |= QStyle::State_Enabled;
    if (b) {
      cbso.state |= QStyle::State_On;
    } else {
      cbso.state |= QStyle::State_Off;
    }
    cbso.rect = check_box_rect(option);

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &cbso, painter);
    break;
  case COL_TEXT:
    QItemDelegate::paint(painter, option, index);
    break;
  case COL_RIGHT_TEXT:
    QItemDelegate::drawBackground(painter, opt, index);
    opt.displayAlignment = Qt::AlignRight;
    rct = option.rect;
    rct.setTop((rct.top() + rct.bottom()) / 2
               - opt.fontMetrics.height() / 2);
    rct.setBottom((rct.top() + rct.bottom()) / 2
                  + opt.fontMetrics.height() / 2);
    if (index.data().toInt() == -1) {
      str = "?";
    } else {
      str = index.data().toString();
    }
    QItemDelegate::drawDisplay(painter, opt, rct, str);
    break;
  default:
    QItemDelegate::paint(painter, option, index);
  }
  painter->restore();
}

/**********************************************************************/ /**
   Constructor for plr_item
 **************************************************************************/
plr_item::plr_item(struct player *pplayer) : QObject()
{
  ipplayer = pplayer;
}

/**********************************************************************/ /**
   Sets data for plr_item (not used)
 **************************************************************************/
bool plr_item::setData(int column, const QVariant &value, int role)
{
  return false;
}

/**********************************************************************/ /**
   Returns data from item
 **************************************************************************/
QVariant plr_item::data(int column, int role) const
{
  QFont f;
  QFontMetrics *fm;
  QPixmap *pix;
  QString str;
  struct player_dlg_column *pdc;

  if (role == Qt::UserRole) {
    return QVariant::fromValue((void *) ipplayer);
  }
  if (role != Qt::DisplayRole) {
    return QVariant();
  }
  pdc = &player_dlg_columns[column];
  switch (player_dlg_columns[column].type) {
  case COL_FLAG:
    pix = get_nation_flag_sprite(tileset, nation_of_player(ipplayer))->pm;
    f = *fc_font::instance()->get_font(fonts::default_font);
    fm = new QFontMetrics(f);
    *pix = pix->scaledToHeight(fm->height());
    delete fm;
    return *pix;
    break;
  case COL_COLOR:
    return get_player_color(tileset, ipplayer)->qcolor;
    break;
  case COL_BOOLEAN:
    return pdc->bool_func(ipplayer);
    break;
  case COL_TEXT:
    return pdc->func(ipplayer);
    break;
  case COL_RIGHT_TEXT:
    str = pdc->func(ipplayer);
    if (str.toInt() != 0) {
      return str.toInt();
    } else if (str == "?") {
      return -1;
    }
    return str;
  default:
    return QVariant();
  }
}

/**********************************************************************/ /**
   Constructor for player model
 **************************************************************************/
plr_model::plr_model(QObject *parent) : QAbstractListModel(parent)
{
  populate();
}

/**********************************************************************/ /**
   Destructor for player model
 **************************************************************************/
plr_model::~plr_model()
{
  qDeleteAll(plr_list);
  plr_list.clear();
}

/**********************************************************************/ /**
   Returns data from model
 **************************************************************************/
QVariant plr_model::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount())
    return plr_list[index.row()]->data(index.column(), role);
  return QVariant();
}

/**********************************************************************/ /**
   Returns header data from model
 **************************************************************************/
QVariant plr_model::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
  struct player_dlg_column *pcol;
  if (orientation == Qt::Horizontal && section < num_player_dlg_columns) {
    if (role == Qt::DisplayRole) {
      pcol = &player_dlg_columns[section];
      return pcol->title;
    }
  }
  return QVariant();
}

/**********************************************************************/ /**
   Sets data in model
 **************************************************************************/
bool plr_model::setData(const QModelIndex &index, const QVariant &value,
                        int role)
{
  if (!index.isValid() || role != Qt::DisplayRole)
    return false;
  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount()) {
    bool change =
        plr_list[index.row()]->setData(index.column(), value, role);
    if (change) {
      notify_plr_changed(index.row());
    }
    return change;
  }
  return false;
}

/**********************************************************************/ /**
   Notifies that row has been changed
 **************************************************************************/
void plr_model::notify_plr_changed(int row)
{
  emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

/**********************************************************************/ /**
   Fills model with data
 **************************************************************************/
void plr_model::populate()
{
  plr_item *pi;

  qDeleteAll(plr_list);
  plr_list.clear();
  beginResetModel();
  players_iterate(pplayer)
  {
    if ((is_barbarian(pplayer))) {
      continue;
    }
    pi = new plr_item(pplayer);
    plr_list << pi;
  }
  players_iterate_end;
  endResetModel();
}

/**********************************************************************/ /**
   Constructor for plr_widget
 **************************************************************************/
plr_widget::plr_widget(QWidget *widget) : QTreeView(widget)
{
  other_player = NULL;
  selected_player = nullptr;
  pid = new plr_item_delegate(this);
  setItemDelegate(pid);
  list_model = new plr_model(this);
  filter_model = new QSortFilterProxyModel();
  filter_model->setDynamicSortFilter(true);
  filter_model->setSourceModel(list_model);
  filter_model->setFilterRole(Qt::DisplayRole);
  setModel(filter_model);
  setRootIsDecorated(false);
  setAllColumnsShowFocus(true);
  setSortingEnabled(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setItemsExpandable(false);
  setAutoScroll(true);
  setAlternatingRowColors(true);
  header()->setContextMenuPolicy(Qt::CustomContextMenu);
  hide_columns();
  connect(header(), &QWidget::customContextMenuRequested, this,
          &plr_widget::display_header_menu);

  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &plr_widget::nation_selected);
}

void plr_widget::set_pr_rep(plr_report *pr) { plr = pr; }
/**********************************************************************/ /**
   Restores selection of previously selected nation
 **************************************************************************/
void plr_widget::restore_selection()
{
  QItemSelection selection;
  QModelIndex i;
  struct player *pplayer;
  QVariant qvar;

  if (selected_player == nullptr) {
    return;
  }
  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pplayer = reinterpret_cast<struct player *>(qvar.value<void *>());
    if (selected_player == pplayer) {
      selection.append(QItemSelectionRange(i));
    }
  }
  selectionModel()->select(selection,
                           QItemSelectionModel::Rows
                               | QItemSelectionModel::SelectCurrent);
}

/**********************************************************************/ /**
   Displays menu on header by right clicking
 **************************************************************************/
void plr_widget::display_header_menu(const QPoint)
{
  QMenu *hideshow_column = new QMenu(this);
  hideshow_column->setTitle(_("Column visibility"));
  QList<QAction *> actions;
  for (int i = 0; i < list_model->columnCount(); ++i) {
    QAction *myAct = hideshow_column->addAction(
        list_model->headerData(i, Qt::Horizontal, Qt::DisplayRole)
            .toString());
    myAct->setCheckable(true);
    myAct->setChecked(!isColumnHidden(i));
    actions.append(myAct);
  }

  hideshow_column->setAttribute(Qt::WA_DeleteOnClose);
  connect(hideshow_column, &QMenu::triggered, this, [=](QAction *act) {
    int col;
    struct player_dlg_column *pcol;

    if (!act) {
      return;
    }

    col = actions.indexOf(act);
    fc_assert_ret(col >= 0);
    pcol = &player_dlg_columns[col];
    pcol->show = !pcol->show;
    setColumnHidden(col, !isColumnHidden(col));
    if (!isColumnHidden(col) && columnWidth(col) <= 5)
      setColumnWidth(col, 100);
  });

  hideshow_column->popup(QCursor::pos());
}

/**********************************************************************/ /**
   Returns information about column if hidden
 **************************************************************************/
QVariant plr_model::hide_data(int section) const
{
  struct player_dlg_column *pcol;
  pcol = &player_dlg_columns[section];
  return pcol->show;
}

/**********************************************************************/ /**
   Hides columns in plr widget, depending on info from plr_list
 **************************************************************************/
void plr_widget::hide_columns()
{
  int col;

  for (col = 0; col < list_model->columnCount(); col++) {
    if (!list_model->hide_data(col).toBool()) {
      setColumnHidden(col, !isColumnHidden(col));
    }
  }
}

/**********************************************************************/ /**
   Slot for selecting player/nation
 **************************************************************************/
void plr_widget::nation_selected(const QItemSelection &sl,
                                 const QItemSelection &ds)
{
  QModelIndex index;
  QVariant qvar;
  QModelIndexList indexes = sl.indexes();
  struct city *pcity;
  const struct player_diplstate *state;
  struct research *my_research, *research;
  char tbuf[256];
  QString res;
  QString sp = " ";
  QString etax, esci, elux, egold, egov;
  QString cult;
  QString nl = "<br>";
  QStringList sorted_list_a;
  QStringList sorted_list_b;
  struct player *pplayer;
  int a, b;
  bool added;
  bool entry_exist = false;
  struct player *me;
  Tech_type_id tech_id;
  bool global_observer = client_is_global_observer();

  other_player = NULL;
  intel_str.clear();
  tech_str.clear();
  ally_str.clear();
  if (indexes.isEmpty()) {
    selected_player = nullptr;
    plr->update_report(false);
    return;
  }
  index = indexes.at(0);
  qvar = index.data(Qt::UserRole);
  pplayer = reinterpret_cast<player *>(qvar.value<void *>());
  selected_player = pplayer;
  other_player = pplayer;
  if (!pplayer->is_alive) {
    plr->update_report(false);
    return;
  }
  me = client_player();
  pcity = player_primary_capital(pplayer);
  research = research_get(pplayer);

  switch (research->researching) {
  case A_UNKNOWN:
    res = _("(Unknown)");
    break;
  case A_UNSET:
    if (global_observer || player_has_embassy(me, pplayer)) {
      res = _("(none)");
    } else {
      res = _("(Unknown)");
    }
    break;
  default:
    res = QString(research_advance_name_translation(research,
                                                    research->researching))
          + sp + "(" + QString::number(research->bulbs_researched) + "/"
          + QString::number(research->client.researching_cost) + ")";
    break;
  }
  if (global_observer || player_has_embassy(me, pplayer)) {
    etax = QString::number(pplayer->economic.tax) + "%";
    esci = QString::number(pplayer->economic.science) + "%";
    elux = QString::number(pplayer->economic.luxury) + "%";
    cult = QString::number(pplayer->client.culture);
  } else {
    etax = _("(Unknown)");
    esci = _("(Unknown)");
    elux = _("(Unknown)");
    cult = _("(Unknown)");
  }
  if (global_observer || pplayer == me
      || could_intel_with_player(me, pplayer)) {
    egold = QString::number(pplayer->economic.gold);
    egov = QString(government_name_for_player(pplayer));
  } else {
    egold = _("(Unknown)");
    egov = _("(Unknown)");
  }

  intel_str = "<table>";
  QString line = "<tr><td><b>%1</b></td><td>%2</td></tr>";

  intel_str +=
      line.arg(_("Nation"))
          .arg(
              QString(nation_adjective_for_player(pplayer)).toHtmlEscaped());
  intel_str +=
      line.arg(_("Ruler"))
          .arg(QString(ruler_title_for_player(pplayer, tbuf, sizeof(tbuf)))
                   .toHtmlEscaped());
  intel_str += line.arg(_("Government")).arg(egov.toHtmlEscaped());
  intel_str +=
      line.arg(_("Capital"))
          .arg(QString(((!pcity) ? _("(Unknown)") : city_name_get(pcity)))
                   .toHtmlEscaped());
  intel_str += line.arg(_("Gold")).arg(egold.toHtmlEscaped());
  intel_str += line.arg(_("Tax")).arg(etax.toHtmlEscaped());
  intel_str += line.arg(_("Science")).arg(esci.toHtmlEscaped());
  intel_str += line.arg(_("Luxury")).arg(elux.toHtmlEscaped());
  intel_str += line.arg(_("Researching")).arg(res.toHtmlEscaped());
  intel_str += line.arg(_("Culture:")).arg(cult.toHtmlEscaped());
  intel_str += "</table>";

  for (int i = 0; i < static_cast<int>(DS_LAST); i++) {
    added = false;
    if (entry_exist) {
      ally_str += "<br>";
    }
    entry_exist = false;
    players_iterate_alive(other)
    {
      if (other == pplayer || is_barbarian(other)) {
        continue;
      }
      state = player_diplstate_get(pplayer, other);
      if (static_cast<int>(state->type) == i
          && (global_observer || could_intel_with_player(me, pplayer))) {
        if (!added) {
          ally_str = ally_str + QString("<b>")
                     + QString(diplstate_type_translated_name(
                                   static_cast<diplstate_type>(i)))
                           .toHtmlEscaped()
                     + ": " + QString("</b>") + nl;
          added = true;
        }
        if (gives_shared_vision(pplayer, other)) {
          ally_str = ally_str + "(◐‿◑)";
        }
        ally_str = ally_str
                   + QString(nation_plural_for_player(other)).toHtmlEscaped()
                   + ", ";
        entry_exist = true;
      }
    }
    players_iterate_alive_end;
    if (entry_exist) {
      ally_str.replace(ally_str.lastIndexOf(","), 1, ".");
    }
  }
  my_research = research_get(me);
  if (!global_observer) {
    if (player_has_embassy(me, pplayer) && me != pplayer) {
      a = 0;
      b = 0;
      techs_known = QString(_("<b>Techs unknown by %1:</b>"))
                        .arg(QString(nation_plural_for_player(pplayer))
                                 .toHtmlEscaped());
      techs_unknown = QString(_("<b>Techs unknown by you :</b>"));

      advance_iterate(A_FIRST, padvance)
      {
        tech_id = advance_number(padvance);
        if (research_invention_state(my_research, tech_id) == TECH_KNOWN
            && (research_invention_state(research, tech_id) != TECH_KNOWN)) {
          a++;
          sorted_list_a << research_advance_name_translation(research,
                                                             tech_id);
        }
        if (research_invention_state(my_research, tech_id) != TECH_KNOWN
            && (research_invention_state(research, tech_id) == TECH_KNOWN)) {
          b++;
          sorted_list_b << research_advance_name_translation(research,
                                                             tech_id);
        }
      }
      advance_iterate_end;
      sorted_list_a.sort(Qt::CaseInsensitive);
      sorted_list_b.sort(Qt::CaseInsensitive);
      for (auto const &res : qAsConst(sorted_list_a)) {
        techs_known = techs_known + QString("<i>") + res.toHtmlEscaped()
                      + "," + QString("</i>") + sp;
      }
      for (auto const &res : qAsConst(sorted_list_b)) {
        techs_unknown = techs_unknown + QString("<i>") + res.toHtmlEscaped()
                        + "," + QString("</i>") + sp;
      }
      if (a == 0) {
        techs_known = techs_known + QString("<i>") + sp
                      + QString(Q_("?tech:None")) + QString("</i>");
      } else {
        techs_known.replace(techs_known.lastIndexOf(","), 1, ".");
      }
      if (b == 0) {
        techs_unknown = techs_unknown + QString("<i>") + sp
                        + QString(Q_("?tech:None")) + QString("</i>");
      } else {
        techs_unknown.replace(techs_unknown.lastIndexOf(","), 1, ".");
      }
      tech_str = techs_known + nl + techs_unknown;
    }
  } else {
    tech_str =
        QString(_("<b>Techs known by %1:</b>"))
            .arg(QString(nation_plural_for_player(pplayer)).toHtmlEscaped());
    advance_iterate(A_FIRST, padvance)
    {
      tech_id = advance_number(padvance);
      if (research_invention_state(research, tech_id) == TECH_KNOWN) {
        sorted_list_a << research_advance_name_translation(research,
                                                           tech_id);
      }
    }
    advance_iterate_end;
    sorted_list_a.sort(Qt::CaseInsensitive);
    for (auto const &res : qAsConst(sorted_list_a)) {
      tech_str = tech_str + QString("<i>") + res.toHtmlEscaped() + ","
                 + QString("</i>") + sp;
    }
  }
  plr->update_report(false);
}

/**********************************************************************/ /**
   Returns model used in widget
 **************************************************************************/
plr_model *plr_widget::get_model() const { return list_model; }

/**********************************************************************/ /**
   Destructor for player widget
 **************************************************************************/
plr_widget::~plr_widget()
{
  delete pid;
  delete list_model;
  delete filter_model;
  king()->qt_settings.player_repo_sort_col =
      header()->sortIndicatorSection();
  king()->qt_settings.player_report_sort = header()->sortIndicatorOrder();
}

/**********************************************************************/ /**
   Constructor for plr_report
 **************************************************************************/
plr_report::plr_report() : QWidget()
{
  ui.setupUi(this);

  ui.meet_but->setText(_("Meet"));
  ui.cancel_but->setText(_("Cancel Treaty"));
  ui.withdraw_but->setText(_("Withdraw Vision"));
  ui.toggle_ai_but->setText(_("Toggle AI Mode"));
  connect(ui.meet_but, &QAbstractButton::pressed, this,
          &plr_report::req_meeeting);
  connect(ui.cancel_but, &QAbstractButton::pressed, this,
          &plr_report::plr_cancel_threaty);
  connect(ui.withdraw_but, &QAbstractButton::pressed, this,
          &plr_report::plr_withdraw_vision);
  connect(ui.toggle_ai_but, &QAbstractButton::pressed, this,
          &plr_report::toggle_ai_mode);
  setLayout(ui.layout);
  other_player = nullptr;
  index = 0;
  if (king()->qt_settings.player_repo_sort_col != -1) {
    ui.plr_wdg->sortByColumn(king()->qt_settings.player_repo_sort_col,
                             king()->qt_settings.player_report_sort);
  }
  ui.plr_wdg->set_pr_rep(this);
}

/**********************************************************************/ /**
   Destructor for plr_report
 **************************************************************************/
plr_report::~plr_report() { queen()->remove_repo_dlg("PLR"); }

/**********************************************************************/ /**
   Adds plr_report to tab widget
 **************************************************************************/
void plr_report::init()
{
  queen()->gimme_place(this, "PLR");
  index = queen()->add_game_tab(this);
  queen()->game_tab_widget->setCurrentIndex(index);
}

/**********************************************************************/ /**
   Public function to call meeting
 **************************************************************************/
void plr_report::call_meeting()
{
  if (ui.meet_but->isEnabled()) {
    req_meeeting();
  }
}

/**********************************************************************/ /**
   Slot for canceling threaty
 **************************************************************************/
void plr_report::plr_cancel_threaty()
{
  dsend_packet_diplomacy_cancel_pact(
      &client.conn, player_number(other_player), CLAUSE_CEASEFIRE);
}

/**********************************************************************/ /**
   Slot for meeting request
 **************************************************************************/
void plr_report::req_meeeting()
{
  dsend_packet_diplomacy_init_meeting_req(&client.conn,
                                          player_number(other_player));
}

/**********************************************************************/ /**
   Slot for withdrawing vision
 **************************************************************************/
void plr_report::plr_withdraw_vision()
{
  dsend_packet_diplomacy_cancel_pact(
      &client.conn, player_number(other_player), CLAUSE_VISION);
}

/**********************************************************************/ /**
   Slot for changing AI mode
 **************************************************************************/
void plr_report::toggle_ai_mode()
{
  QAction *toggle_ai_act;
  QAction *ai_level_act;
  QMenu *ai_menu = new QMenu(this);
  int level;

  toggle_ai_act = new QAction(_("Toggle AI Mode"), nullptr);
  ai_menu->addAction(toggle_ai_act);
  ai_menu->addSeparator();
  for (level = 0; level < AI_LEVEL_COUNT; level++) {
    if (is_settable_ai_level(static_cast<ai_level>(level))) {
      QString ln = ai_level_translated_name(static_cast<ai_level>(level));
      ai_level_act = new QAction(ln, nullptr);
      ai_level_act->setData(QVariant::fromValue(level));
      ai_menu->addAction(ai_level_act);
    }
  }
  ai_menu->setAttribute(Qt::WA_DeleteOnClose);
  connect(ai_menu, &QMenu::triggered, [=](QAction *act) {
    int level;
    if (act == toggle_ai_act) {
      send_chat_printf("/aitoggle \"%s\"",
                       player_name(ui.plr_wdg->other_player));
      return;
    }
    if (act && act->isVisible()) {
      level = act->data().toInt();
      if (is_human(ui.plr_wdg->other_player)) {
        send_chat_printf("/aitoggle \"%s\"",
                         player_name(ui.plr_wdg->other_player));
      }
      send_chat_printf("/%s %s", ai_level_cmd(static_cast<ai_level>(level)),
                       player_name(ui.plr_wdg->other_player));
    }
  });

  ai_menu->popup(QCursor::pos());
}

/**********************************************************************/ /**
   Handle mouse click
 **************************************************************************/
void plr_widget::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = this->indexAt(event->pos());
  if (index.isValid() && event->button() == Qt::RightButton
      && can_client_issue_orders()) {
    plr->call_meeting();
  }
  QTreeView::mousePressEvent(event);
}

/**********************************************************************/ /**
   Updates widget
 **************************************************************************/
void plr_report::update_report(bool update_selection)
{
  QModelIndex qmi;
  int player_count = 0;

  /* Force updating selected player information */
  if (update_selection) {
    qmi = ui.plr_wdg->currentIndex();
    if (qmi.isValid()) {
      ui.plr_wdg->clearSelection();
      ui.plr_wdg->setCurrentIndex(qmi);
    }
  }

  players_iterate(pplayer)
  {
    if ((is_barbarian(pplayer))) {
      continue;
    }
    player_count++;
  }
  players_iterate_end;

  if (player_count != ui.plr_wdg->get_model()->rowCount()) {
    ui.plr_wdg->get_model()->populate();
  }

  ui.plr_wdg->header()->resizeSections(QHeaderView::ResizeToContents);
  ui.meet_but->setDisabled(true);
  ui.cancel_but->setDisabled(true);
  ui.withdraw_but->setDisabled(true);
  ui.toggle_ai_but->setDisabled(true);
  ui.plr_label->setText(ui.plr_wdg->intel_str);
  ui.ally_label->setText(ui.plr_wdg->ally_str);
  ui.tech_label->setText(ui.plr_wdg->tech_str);
  other_player = ui.plr_wdg->other_player;
  if (other_player == NULL || !can_client_issue_orders()) {
    return;
  }
  if (NULL != client.conn.playing && other_player != client.conn.playing) {

    // We keep button sensitive in case of DIPL_SENATE_BLOCKING, so that
    // player can request server side to check requirements of those effects
    // with omniscience
    if (pplayer_can_cancel_treaty(client_player(), other_player)
        != DIPL_ERROR) {
      ui.cancel_but->setEnabled(true);
    }
    ui.toggle_ai_but->setEnabled(true);
  }
  if (gives_shared_vision(client_player(), other_player)
      && !players_on_same_team(client_player(), other_player)) {
    ui.withdraw_but->setEnabled(true);
  }
  if (can_meet_with_player(other_player)) {
    ui.meet_but->setEnabled(true);
  }
  ui.plr_wdg->restore_selection();
}

/**********************************************************************/ /**
   Display the player list dialog.  Optionally raise it.
 **************************************************************************/
void popup_players_dialog(bool raise)
{
  int i;
  QWidget *w;

  if (!queen()->is_repo_dlg_open("PLR")) {
    plr_report *pr = new plr_report;

    pr->init();
    pr->update_report();
  } else {
    plr_report *pr;

    i = queen()->gimme_index_of("PLR");
    w = queen()->game_tab_widget->widget(i);
    if (w->isVisible()) {
      queen()->game_tab_widget->setCurrentIndex(0);
      return;
    }
    pr = reinterpret_cast<plr_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(pr);
    pr->update_report();
  }
}

/**********************************************************************/ /**
   Update all information in the player list dialog.
 **************************************************************************/
void real_players_dialog_update(void *unused)
{
  int i;
  plr_report *pr;
  QWidget *w;

  if (queen()->is_repo_dlg_open("PLR")) {
    i = queen()->gimme_index_of("PLR");
    if (queen()->game_tab_widget->currentIndex() == i) {
      w = queen()->game_tab_widget->widget(i);
      pr = reinterpret_cast<plr_report *>(w);
      pr->update_report();
    }
  }
}

/**********************************************************************/ /**
   Closes players report
 **************************************************************************/
void popdown_players_report()
{
  int i;
  plr_report *pr;
  QWidget *w;

  if (queen()->is_repo_dlg_open("PLR")) {
    i = queen()->gimme_index_of("PLR");
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    pr = reinterpret_cast<plr_report *>(w);
    pr->deleteLater();
  }
}
/**********************************************************************/ /**
   Update the intelligence dialog for the given player.  This is called by
   the core client code when that player's information changes.
 **************************************************************************/
void update_intel_dialog(struct player *p) { real_players_dialog_update(p); }
/**********************************************************************/ /**
   Close an intelligence dialog for the given player.
 **************************************************************************/
void close_intel_dialog(struct player *p) { real_players_dialog_update(p); }
