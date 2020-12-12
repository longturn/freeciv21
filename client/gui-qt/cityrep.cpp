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
#include <QApplication>
#include <QHeaderView>
#include <QVBoxLayout>
// utility
#include "fcintl.h"
// common
#include "citydlg_common.h"
#include "game.h"
#include "global_worklist.h"
// client
#include "cityrep_g.h"
#include "client_main.h"
#include "governor.h"
#include "mapview_common.h"
// gui-qt
#include "cityrep.h"
#include "fc_client.h"
#include "hudwidget.h"
#include "icons.h"
#include "page_game.h"
#include "qtg_cxxside.h"

// header city icons
class hIcon {
  Q_DISABLE_COPY(hIcon);

private:
  explicit hIcon(){};
  static hIcon *m_instance;
  QHash<QString, QIcon> hash;

public:
  static hIcon *i();
  static void drop();
  void createIcons();
  QIcon get(const QString &id);
};

hIcon *hIcon::i()
{
  if (!m_instance) {
    m_instance = new hIcon;
    m_instance->createIcons();
  }
  return m_instance;
}

void hIcon::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = nullptr;
  }
}

void hIcon::createIcons()
{
  hash.insert("prodplus",
              fcIcons::instance()->getIcon(QStringLiteral("hprod")));
  hash.insert("foodplus",
              fcIcons::instance()->getIcon(QStringLiteral("hfood")));
  hash.insert("tradeplus",
              fcIcons::instance()->getIcon(QStringLiteral("htrade")));
}

QIcon hIcon::get(const QString &id) { return hash.value(id, QIcon()); }

hIcon *hIcon::m_instance = nullptr;

/***********************************************************************/ /**
   Overriden compare for sorting items
 ***************************************************************************/
bool city_sort_model::lessThan(const QModelIndex &left,
                               const QModelIndex &right) const
{
  QVariant qleft;
  QVariant qright;
  int i;
  QByteArray l_bytes;
  QByteArray r_bytes;

  qleft = sourceModel()->data(left);
  qright = sourceModel()->data(right);
  l_bytes = qleft.toString().toLocal8Bit();
  r_bytes = qright.toString().toLocal8Bit();
  i = cityrepfield_compare(l_bytes.data(), r_bytes.data());

  if (i >= 0) {
    return true;
  } else {
    return false;
  }
}

/***********************************************************************/ /**
   City item delegate constructor
 ***************************************************************************/
city_item_delegate::city_item_delegate(QObject *parent)
    : QItemDelegate(parent)
{
  QFont f = QApplication::font();
  QFontMetrics fm(f);

  item_height = fm.height() + 4;
}

/***********************************************************************/ /**
   City item delgate paint event
 ***************************************************************************/
void city_item_delegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
  QStyleOptionViewItem opt = QItemDelegate::setOptions(index, option);
  QString txt;
  QFont font;
  QPalette palette;
  struct city_report_spec *spec;
  spec = city_report_specs + index.column();
  txt = spec->tagname;
  if (txt == QLatin1String("cityname")) {
    font.setCapitalization(QFont::SmallCaps);
    font.setBold(true);
    opt.font = font;
  }
  if (txt == QLatin1String("hstate_verbose")) {
    font.setItalic(true);
    opt.font = font;
  }
  if (txt == QLatin1String("prodplus")) {
    txt = index.data().toString();
    if (txt.toInt() < 0) {
      font.setBold(true);
      palette.setColor(QPalette::Text, QColor(255, 0, 0));
      opt.font = font;
      opt.palette = palette;
    }
  }

  QItemDelegate::paint(painter, opt, index);
}

/***********************************************************************/ /**
   Size hint for city item delegate
 ***************************************************************************/
QSize city_item_delegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
  QSize s = QItemDelegate::sizeHint(option, index);

  s.setHeight(item_height + 4);
  return s;
}

/***********************************************************************/ /**
   Constructor for city item
 ***************************************************************************/
city_item::city_item(city *pcity) : QObject() { i_city = pcity; }

/***********************************************************************/ /**
   Returns used city pointer for city item creation
 ***************************************************************************/
city *city_item::get_city() { return i_city; }

/***********************************************************************/ /**
   Sets nothing, but must be declared
 ***************************************************************************/
bool city_item::setData(int column, const QVariant &value, int role)
{
  Q_UNUSED(role)
  Q_UNUSED(column)
  Q_UNUSED(value)
  return false;
}

/***********************************************************************/ /**
   Returns data from city item (or city pointer from Qt::UserRole)
 ***************************************************************************/
QVariant city_item::data(int column, int role) const
{
  struct city_report_spec *spec;

  if (role == Qt::UserRole && column == 0) {
    return QVariant::fromValue((void *) i_city);
  }
  if (role != Qt::DisplayRole) {
    return QVariant();
  }
  spec = city_report_specs + column;
  QString buf = QString("%1").arg(spec->func(i_city, spec->data));
  return buf.trimmed();
}

/***********************************************************************/ /**
   Constructor for city model
 ***************************************************************************/
city_model::city_model(QObject *parent) : QAbstractListModel(parent)
{
  populate();
}

/***********************************************************************/ /**
   Destructor for city model
 ***************************************************************************/
city_model::~city_model()
{
  qDeleteAll(city_list);
  city_list.clear();
}

/***********************************************************************/ /**
   Notifies about changed row
 ***************************************************************************/
void city_model::notify_city_changed(int row)
{
  emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

/***********************************************************************/ /**
   Returns stored data in index
 ***************************************************************************/
QVariant city_model::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount()) {
    return city_list[index.row()]->data(index.column(), role);
  }
  return QVariant();
}

/***********************************************************************/ /**
   Sets data in model under index
 ***************************************************************************/
bool city_model::setData(const QModelIndex &index, const QVariant &value,
                         int role)
{
  if (!index.isValid() || role != Qt::DisplayRole) {
    return false;
  }
  if (index.row() >= 0 && index.row() < rowCount() && index.column() >= 0
      && index.column() < columnCount()) {
    bool change =
        city_list[index.row()]->setData(index.column(), value, role);

    if (change) {
      notify_city_changed(index.row());
    }
    return change;
  }
  return false;
}

/***********************************************************************/ /**
   Returns header data for given section(column)
 ***************************************************************************/
QVariant city_model::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
  struct city_report_spec *spec = city_report_specs + section;

  if (orientation == Qt::Horizontal && section < NUM_CREPORT_COLS) {
    if (role == Qt::DisplayRole) {
      QString buf = QString("%1\n%2").arg(spec->title1 ? spec->title1 : "",
                                          spec->title2 ? spec->title2 : "");
      QIcon i = hIcon::i()->get(spec->tagname);
      if (!i.isNull()) { // icon exists for that header
        return QString();
      }
      return buf.trimmed();
    }
    if (role == Qt::ToolTipRole) {
      return QString(spec->explanation);
    }
    if (role == Qt::DecorationRole) {
      QIcon i = hIcon::i()->get(spec->tagname);
      if (!i.isNull()) {
        return i;
      }
    }
  }
  return QVariant();
}

/***********************************************************************/ /**
   Returns header information about section
 ***************************************************************************/
QVariant city_model::menu_data(int section) const
{
  struct city_report_spec *spec;

  if (section < NUM_CREPORT_COLS) {
    spec = city_report_specs + section;
    return QString(spec->explanation);
  }
  return QVariant();
}

/***********************************************************************/ /**
   Hides given column if show is false
 ***************************************************************************/
QVariant city_model::hide_data(int section) const
{
  struct city_report_spec *spec;

  if (section < NUM_CREPORT_COLS) {
    spec = city_report_specs + section;
    return spec->show;
  }
  return QVariant();
}

/***********************************************************************/ /**
   Creates city model
 ***************************************************************************/
void city_model::populate()
{
  city_item *ci;

  if (client_has_player()) {
    city_list_iterate(client_player()->cities, pcity)
    {
      ci = new city_item(pcity);
      city_list << ci;
    }
    city_list_iterate_end;
  } else {
    cities_iterate(pcity)
    {
      ci = new city_item(pcity);
      city_list << ci;
    }
    cities_iterate_end;
  }
}

/***********************************************************************/ /**
   Notifies about changed item
 ***************************************************************************/
void city_model::city_changed(struct city *pcity)
{
  city_item *item;

  beginResetModel();
  endResetModel();
  for (int i = 0; i < city_list.count(); i++) {
    item = city_list.at(i);
    if (pcity == item->get_city()) {
      notify_city_changed(i);
    }
  }
}

/***********************************************************************/ /**
   Notifies about whole model changed
 ***************************************************************************/
void city_model::all_changed()
{
  city_list.clear();
  beginResetModel();
  populate();
  endResetModel();
}

/***********************************************************************/ /**
   Restores last selection
 ***************************************************************************/
void city_widget::restore_selection()
{
  QItemSelection selection;
  QModelIndex i;
  struct city *pcity;
  QVariant qvar;

  if (selected_cities.isEmpty()) {
    return;
  }
  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    if (selected_cities.contains(pcity)) {
      selection.append(QItemSelectionRange(i));
    }
  }
  selectionModel()->select(selection,
                           QItemSelectionModel::Rows
                               | QItemSelectionModel::SelectCurrent);
}

/***********************************************************************/ /**
   Constructor for city widget
 ***************************************************************************/
city_widget::city_widget(city_report *ctr) : QTreeView()
{
  cr = ctr;
  c_i_d = new city_item_delegate(this);
  setItemDelegate(c_i_d);
  list_model = new city_model(this);
  filter_model = new city_sort_model();
  filter_model->setDynamicSortFilter(true);
  filter_model->setSourceModel(list_model);
  filter_model->setFilterRole(Qt::DisplayRole);
  setModel(filter_model);
  setRootIsDecorated(false);
  setAllColumnsShowFocus(true);
  setSortingEnabled(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setItemsExpandable(false);
  setAutoScroll(true);
  setProperty("uniformRowHeights", "true");
  setAlternatingRowColors(true);
  header()->setContextMenuPolicy(Qt::CustomContextMenu);
  header()->setMinimumSectionSize(10);
  setContextMenuPolicy(Qt::CustomContextMenu);
  hide_columns();
  connect(header(), &QWidget::customContextMenuRequested, this,
          &city_widget::display_header_menu);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &city_widget::cities_selected);
  connect(this, &QAbstractItemView::doubleClicked, this,
          &city_widget::city_doubleclick);
  connect(this, &QWidget::customContextMenuRequested, this,
          &city_widget::display_list_menu);
}

/***********************************************************************/ /**
   Slot for double clicking row
 ***************************************************************************/
void city_widget::city_doubleclick(const QModelIndex &index)
{
  Q_UNUSED(index);
  city_view();
}

/***********************************************************************/ /**
   Shows first selected city
 ***************************************************************************/
void city_widget::city_view()
{
  struct city *pcity;

  if (selected_cities.isEmpty()) {
    return;
  }
  pcity = selected_cities[0];

  Q_ASSERT(pcity != NULL);
  if (gui_options.center_when_popup_city) {
    center_tile_mapcanvas(pcity->tile);
  }
  qtg_real_city_dialog_popup(pcity);
}

/***********************************************************************/ /**
   Clears worklist for selected cities
 ***************************************************************************/
void city_widget::clear_worlist()
{
  struct worklist empty;
  worklist_init(&empty);

  for (auto pcity : qAsConst(selected_cities)) {
    Q_ASSERT(pcity != NULL);
    city_set_worklist(pcity, &empty);
  }
}

/***********************************************************************/ /**
   Buys current item in city
 ***************************************************************************/
void city_widget::buy()
{
  for (auto pcity : qAsConst(selected_cities)) {
    Q_ASSERT(pcity != NULL);
    cityrep_buy(pcity);
  }
}

/***********************************************************************/ /**
   Centers map on city
 ***************************************************************************/
void city_widget::center()
{
  struct city *pcity;

  if (selected_cities.isEmpty()) {
    return;
  }
  pcity = selected_cities[0];
  Q_ASSERT(pcity != NULL);
  center_tile_mapcanvas(pcity->tile);
  queen()->game_tab_widget->setCurrentIndex(0);
}

/***********************************************************************/ /**
   Displays right click menu on city row
 ***************************************************************************/
void city_widget::display_list_menu(const QPoint)
{
  QMap<QString, cid> custom_labels;
  QMap<QString, int> cma_labels;
  QMenu *some_menu;
  QMenu *tmp2_menu;
  QMenu *tmp_menu;
  bool select_only = false;
  int sell_gold;
  QMenu *list_menu;
  QAction cty_view(style()->standardIcon(QStyle::SP_CommandLink),
                   Q_("?verb:View"), 0);
  sell_gold = 0;
  if (selected_cities.isEmpty()) {
    select_only = true;
  }
  for (auto pcity : qAsConst(selected_cities)) {
    sell_gold = sell_gold + pcity->client.buy_cost;
  }
  if (!can_client_issue_orders()) {
    return;
  }
  list_menu = new QMenu(this);
  QString buf =
      QString(_("Buy ( Cost: %1 )")).arg(QString::number(sell_gold));

  QAction *cty_buy = new QAction(QString(buf), list_menu);
  QAction *cty_center = new QAction(
      style()->standardIcon(QStyle::SP_ArrowRight), _("Center"), list_menu);
  QAction *wl_clear = new QAction(_("Clear"), list_menu);
  QAction *wl_empty = new QAction(_("(no worklists defined)"), list_menu);
  bool worklist_defined = true;

  if (!select_only) {
    some_menu = list_menu->addMenu(_("Production"));
    tmp_menu = some_menu->addMenu(_("Change"));
    fill_production_menus(CHANGE_PROD_NOW, custom_labels, can_city_build_now,
                          tmp_menu);
    tmp_menu = some_menu->addMenu(_("Add next"));
    fill_production_menus(CHANGE_PROD_NEXT, custom_labels,
                          can_city_build_now, tmp_menu);
    tmp_menu = some_menu->addMenu(_("Add before last"));
    fill_production_menus(CHANGE_PROD_BEF_LAST, custom_labels,
                          can_city_build_now, tmp_menu);
    tmp_menu = some_menu->addMenu(_("Add last"));
    fill_production_menus(CHANGE_PROD_LAST, custom_labels,
                          can_city_build_now, tmp_menu);

    tmp_menu = some_menu->addMenu(_("Worklist"));
    tmp_menu->addAction(wl_clear);
    connect(wl_clear, &QAction::triggered, this,
            &city_widget::clear_worlist);
    tmp2_menu = tmp_menu->addMenu(_("Add"));
    gen_worklist_labels(cma_labels);
    if (cma_labels.count() == 0) {
      tmp2_menu->addAction(wl_empty);
      worklist_defined = false;
    }
    fill_data(WORKLIST_ADD, cma_labels, tmp2_menu);
    tmp2_menu = tmp_menu->addMenu(_("Change"));
    if (cma_labels.count() == 0) {
      tmp2_menu->addAction(wl_empty);
      worklist_defined = false;
    }
    fill_data(WORKLIST_CHANGE, cma_labels, tmp2_menu);
    some_menu = list_menu->addMenu(_("Governor"));
    gen_cma_labels(cma_labels);
    fill_data(CMA, cma_labels, some_menu);
    some_menu = list_menu->addMenu(_("Sell"));
    gen_production_labels(SELL, custom_labels, false, false,
                          can_city_sell_universal);
    fill_data(SELL, custom_labels, some_menu);
  }
  some_menu = list_menu->addMenu(_("Select"));
  gen_select_labels(some_menu);
  if (!select_only) {
    list_menu->addAction(&cty_view);
    connect(&cty_view, &QAction::triggered, this, &city_widget::city_view);
    list_menu->addAction(cty_buy);
    connect(cty_buy, &QAction::triggered, this, &city_widget::buy);
    list_menu->addAction(cty_center);
    connect(cty_center, &QAction::triggered, this, &city_widget::center);
  }
  sell_gold = 0;

  list_menu->setAttribute(Qt::WA_DeleteOnClose);
  connect(list_menu, &QMenu::triggered, this, [=](QAction *act) {
    QVariant qvar, qvar2;
    enum menu_labels m_state;
    cid id;
    struct universal target;
    QString imprname;
    const struct impr_type *building;
    Impr_type_id impr_id;
    int city_id;
    bool need_clear = true;
    bool sell_ask = true;

    if (!act) {
      return;
    }

    qvar2 = act->property("FC");
    m_state = static_cast<menu_labels>(qvar2.toInt());
    qvar = act->data();
    id = qvar.toInt();
    target = cid_decode(id);

    city_list_iterate(client_player()->cities, iter_city)
    {
      if (NULL != iter_city) {
        switch (m_state) {
        case SELECT_IMPR:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (city_building_present(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        case SELECT_WONDERS:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (city_building_present(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        case SELECT_SUPP_UNITS:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (city_unit_supported(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        case SELECT_PRES_UNITS:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (city_unit_present(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        case SELECT_AVAIL_UNITS:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (can_city_build_now(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        case SELECT_AVAIL_IMPR:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (can_city_build_now(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        case SELECT_AVAIL_WONDERS:
          if (need_clear) {
            clearSelection();
          }
          need_clear = false;
          if (can_city_build_now(iter_city, &target)) {
            select_city(iter_city);
          }
          break;
        default:
          break;
        }
      }
    }
    city_list_iterate_end;

    for (auto pcity : qAsConst(selected_cities)) {
      if (nullptr != pcity) {
        switch (m_state) {
        case CHANGE_PROD_NOW:
          city_change_production(pcity, &target);
          break;
        case CHANGE_PROD_NEXT:
          city_queue_insert(pcity, 1, &target);
          break;
        case CHANGE_PROD_BEF_LAST:
          city_queue_insert(pcity, worklist_length(&pcity->worklist),
                            &target);
          break;
        case CHANGE_PROD_LAST:
          city_queue_insert(pcity, -1, &target);
          break;
        case SELL:
          building = target.value.building;
          if (sell_ask) {
            hud_message_box *ask = new hud_message_box(king()->central_wdg);
            imprname = improvement_name_translation(building);
            QString buf =
                QString(_("Are you sure you want to sell those %1?"))
                    .arg(imprname);
            sell_ask = false;
            ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
            ask->setDefaultButton(QMessageBox::Cancel);
            ask->set_text_title(buf, _("Sell?"));
            ask->setAttribute(Qt::WA_DeleteOnClose);
            city_id = pcity->id;
            impr_id = improvement_number(building);
            connect(ask, &hud_message_box::accepted, this, [=]() {
              struct city *pcity = game_city_by_number(city_id);
              struct impr_type *building = improvement_by_number(impr_id);
              if (!pcity || !building) {
                return;
              }
              if (!pcity->did_sell && city_has_building(pcity, building)) {
                city_sell_improvement(pcity, impr_id);
              }
            });
            ask->show();
          }
          break;
        case CMA:
          if (NULL != pcity) {
            if (CMA_NONE == id) {
              cma_release_city(pcity);
            } else {
              cma_put_city_under_agent(pcity,
                                       cmafec_preset_get_parameter(id));
            }
          }

          break;
        case WORKLIST_ADD:
          if (worklist_defined) {
            city_queue_insert_worklist(
                pcity, -1, global_worklist_get(global_worklist_by_id(id)));
          }
          break;

        case WORKLIST_CHANGE:
          if (worklist_defined) {
            city_set_queue(pcity,
                           global_worklist_get(global_worklist_by_id(id)));
          }
          break;
        default:
          break;
        }
      }
    }
  });
  list_menu->popup(QCursor::pos());
}

/***********************************************************************/ /**
   Fills menu items that can be produced or sold
 ***************************************************************************/
void city_widget::fill_production_menus(city_widget::menu_labels what,
                                        QMap<QString, cid> &custom_labels,
                                        TestCityFunc test_func, QMenu *menu)
{
  QMenu *m1, *m2, *m3;

  m1 = menu->addMenu(_("Buildings"));
  m2 = menu->addMenu(_("Units"));
  m3 = menu->addMenu(_("Wonders"));
  gen_production_labels(what, custom_labels, false, false, test_func);
  fill_data(what, custom_labels, m1);
  gen_production_labels(what, custom_labels, true, false, test_func);
  fill_data(what, custom_labels, m2);
  gen_production_labels(what, custom_labels, false, true, test_func);
  fill_data(what, custom_labels, m3);
}

/***********************************************************************/ /**
   Fills menu actions
 ***************************************************************************/
void city_widget::fill_data(menu_labels which,
                            QMap<QString, cid> &custom_labels, QMenu *menu)
{
  QAction *action;
  QMap<QString, cid>::const_iterator map_iter;

  map_iter = custom_labels.constBegin();
  while (map_iter != custom_labels.constEnd()) {
    action = menu->addAction(map_iter.key());
    action->setData(map_iter.value());
    action->setProperty("FC", which);
    ++map_iter;
  }
  if (custom_labels.isEmpty()) {
    menu->setDisabled(true);
  }
}

/***********************************************************************/ /**
   Selects all cities on report
 ***************************************************************************/
void city_widget::select_all() { selectAll(); }

/***********************************************************************/ /**
   Selects no cities on report
 ***************************************************************************/
void city_widget::select_none() { clearSelection(); }

/***********************************************************************/ /**
   Inverts selection on report
 ***************************************************************************/
void city_widget::invert_selection()
{
  QItemSelection selection;
  QModelIndex i;
  struct city *pcity;
  QVariant qvar;

  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    if (!selected_cities.contains(pcity)) {
      selection.append(QItemSelectionRange(i));
    }
  }
  clearSelection();
  selectionModel()->select(selection,
                           QItemSelectionModel::Rows
                               | QItemSelectionModel::SelectCurrent);
}

/***********************************************************************/ /**
   Marks given city selected
 ***************************************************************************/
void city_widget::select_city(city *spcity)
{
  QItemSelection selection;
  QModelIndex i;
  struct city *pcity;
  QVariant qvar;

  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    if (pcity == spcity) {
      selection.append(QItemSelectionRange(i));
    }
  }
  selectionModel()->select(selection, QItemSelectionModel::Rows
                                          | QItemSelectionModel::Select);
}

/***********************************************************************/ /**
   Selects coastal cities on report
 ***************************************************************************/
void city_widget::select_coastal()
{
  QItemSelection selection;
  QModelIndex i;
  struct city *pcity;
  QVariant qvar;

  clearSelection();
  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    if (NULL != pcity && is_terrain_class_near_tile(pcity->tile, TC_OCEAN)) {
      selection.append(QItemSelectionRange(i));
    }
  }
  selectionModel()->select(selection,
                           QItemSelectionModel::Rows
                               | QItemSelectionModel::SelectCurrent);
}

/***********************************************************************/ /**
   Selects same cities on the same island
 ***************************************************************************/
void city_widget::select_same_island()
{
  QItemSelection selection;
  QModelIndex i;
  struct city *pcity;
  QVariant qvar;

  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    for (auto pscity : qAsConst(selected_cities)) {
      if (NULL != pcity
          && (tile_continent(pcity->tile) == tile_continent(pscity->tile))) {
        selection.append(QItemSelectionRange(i));
      }
    }
  }
  selectionModel()->select(selection,
                           QItemSelectionModel::Rows
                               | QItemSelectionModel::SelectCurrent);
}

/***********************************************************************/ /**
   Selects cities building units or buildings or wonders
   depending on data stored in QAction
 ***************************************************************************/
void city_widget::select_building_something()
{
  QItemSelection selection;
  QModelIndex i;
  struct city *pcity;
  QVariant qvar;
  QAction *act;
  QString str;

  clearSelection();
  for (int j = 0; j < filter_model->rowCount(); j++) {
    i = filter_model->index(j, 0);
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    act = qobject_cast<QAction *>(sender());
    qvar = act->data();
    str = qvar.toString();
    if (NULL != pcity) {
      if (str == QLatin1String("impr")
          && VUT_IMPROVEMENT == pcity->production.kind
          && !is_wonder(pcity->production.value.building)
          && !improvement_has_flag(pcity->production.value.building,
                                   IF_GOLD)) {
        selection.append(QItemSelectionRange(i));
      } else if (str == QLatin1String("unit")
                 && VUT_UTYPE == pcity->production.kind) {
        selection.append(QItemSelectionRange(i));
      } else if (str == QLatin1String("wonder")
                 && VUT_IMPROVEMENT == pcity->production.kind
                 && is_wonder(pcity->production.value.building)) {
        selection.append(QItemSelectionRange(i));
      }
    }
  }
  selectionModel()->select(selection,
                           QItemSelectionModel::Rows
                               | QItemSelectionModel::SelectCurrent);
}

/***********************************************************************/ /**
   Creates menu labels and id of available cma, stored in list
 ***************************************************************************/
void city_widget::gen_cma_labels(QMap<QString, int> &list)
{
  list.clear();
  for (int i = 0; i < cmafec_preset_num(); i++) {
    list.insert(cmafec_preset_get_descr(i), i);
  }
}

/***********************************************************************/ /**
   Creates menu labels for selecting cities
 ***************************************************************************/
void city_widget::gen_select_labels(QMenu *menu)
{
  QAction *act;
  QMenu *tmp_menu;
  QMap<QString, cid> custom_labels;

  act = menu->addAction(_("All Cities"));
  connect(act, &QAction::triggered, this, &city_widget::select_all);
  act = menu->addAction(_("No Cities"));
  connect(act, &QAction::triggered, this, &city_widget::select_none);
  act = menu->addAction(_("Invert Selection"));
  connect(act, &QAction::triggered, this, &city_widget::invert_selection);
  menu->addSeparator();
  act = menu->addAction(_("Coastal Cities"));
  connect(act, &QAction::triggered, this, &city_widget::select_coastal);
  act = menu->addAction(_("Same Island"));
  connect(act, &QAction::triggered, this, &city_widget::select_same_island);
  if (selected_cities.isEmpty()) {
    act->setDisabled(true);
  }
  menu->addSeparator();
  act = menu->addAction(_("Building Units"));
  act->setData("unit");
  connect(act, &QAction::triggered, this,
          &city_widget::select_building_something);
  act = menu->addAction(_("Building Improvements"));
  act->setData("impr");
  connect(act, &QAction::triggered, this,
          &city_widget::select_building_something);
  act = menu->addAction(_("Building Wonders"));
  act->setData("wonder");
  connect(act, &QAction::triggered, this,
          &city_widget::select_building_something);
  menu->addSeparator();
  tmp_menu = menu->addMenu(_("Improvements in City"));
  gen_production_labels(SELECT_IMPR, custom_labels, false, false,
                        city_building_present, true);
  fill_data(SELECT_IMPR, custom_labels, tmp_menu);
  tmp_menu = menu->addMenu(_("Wonders in City"));
  gen_production_labels(SELECT_WONDERS, custom_labels, false, true,
                        city_building_present, true);
  fill_data(SELECT_WONDERS, custom_labels, tmp_menu);
  menu->addSeparator();
  tmp_menu = menu->addMenu(_("Supported Units"));
  gen_production_labels(SELECT_SUPP_UNITS, custom_labels, true, false,
                        city_unit_supported, true);
  fill_data(SELECT_SUPP_UNITS, custom_labels, tmp_menu);
  tmp_menu = menu->addMenu(_("Units Present"));
  gen_production_labels(SELECT_PRES_UNITS, custom_labels, true, false,
                        city_unit_present, true);
  fill_data(SELECT_PRES_UNITS, custom_labels, tmp_menu);
  menu->addSeparator();
  tmp_menu = menu->addMenu(_("Available Units"));
  gen_production_labels(SELECT_AVAIL_UNITS, custom_labels, true, false,
                        can_city_build_now, true);
  fill_data(SELECT_AVAIL_UNITS, custom_labels, tmp_menu);
  tmp_menu = menu->addMenu(_("Available Improvements"));
  gen_production_labels(SELECT_AVAIL_IMPR, custom_labels, false, false,
                        can_city_build_now, true);
  fill_data(SELECT_AVAIL_IMPR, custom_labels, tmp_menu);
  tmp_menu = menu->addMenu(_("Available Wonders"));
  gen_production_labels(SELECT_AVAIL_WONDERS, custom_labels, false, true,
                        can_city_build_now, true);
  fill_data(SELECT_AVAIL_WONDERS, custom_labels, tmp_menu);
}

/***********************************************************************/ /**
   Creates menu labels and info of available worklists, stored in list
 ***************************************************************************/
void city_widget::gen_worklist_labels(QMap<QString, int> &list)
{
  list.clear();
  global_worklists_iterate(pgwl)
  {
    list.insert(global_worklist_name(pgwl), global_worklist_id(pgwl));
  }
  global_worklists_iterate_end;
}

/***********************************************************************/ /**
   Creates menu labels and id about available production targets
 ***************************************************************************/
void city_widget::gen_production_labels(city_widget::menu_labels what,
                                        QMap<QString, cid> &list,
                                        bool append_units,
                                        bool append_wonders,
                                        TestCityFunc test_func, bool global)
{
  Q_UNUSED(what)
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  int i, item, targets_used;
  QString str;
  char buf[64];
  struct city **city_data;
  int num_sel = 0;

  if (global) {
    num_sel = list_model->rowCount();
  } else {
    num_sel = selected_cities.count();
  }
  std::vector<struct city *> array;
  array.reserve(num_sel);

  if (global) {
    i = 0;
    city_list_iterate(client.conn.playing->cities, pcity)
    {
      array[i] = pcity;
      i++;
    }
    city_list_iterate_end;
  } else {
    for (i = 0; i < num_sel; i++) {
      array[i] = selected_cities.at(i);
    }
  }
  city_data = &array[0];
  targets_used =
      collect_production_targets(targets, city_data, num_sel, append_units,
                                 append_wonders, true, test_func);
  name_and_sort_items(targets, targets_used, items, true, NULL);
  list.clear();
  for (item = 0; item < targets_used; item++) {
    struct universal target = items[item].item;

    str.clear();
    universal_name_translation(&target, buf, sizeof(buf));
    QString txt = QString("%1 ").arg(buf);
    str = str + txt;
    list.insert(str, cid_encode(target));
  }
}

/***********************************************************************/ /**
   Updates single city
 ***************************************************************************/
void city_widget::update_city(city *pcity)
{
  list_model->city_changed(pcity);
  restore_selection();
}

/***********************************************************************/ /**
   Updates whole model
 ***************************************************************************/
void city_widget::update_model()
{
  setUpdatesEnabled(false);
  list_model->all_changed();
  restore_selection();
  header()->resizeSections(QHeaderView::ResizeToContents);
  setUpdatesEnabled(true);
}

/***********************************************************************/ /**
   Context menu for header
 ***************************************************************************/
void city_widget::display_header_menu(const QPoint)
{
  QMenu *hideshow_column = new QMenu(this);
  QList<QAction *> actions;

  hideshow_column->setTitle(_("Column visibility"));
  for (int i = 0; i < list_model->columnCount(); i++) {
    QAction *myAct =
        hideshow_column->addAction(list_model->menu_data(i).toString());
    myAct->setCheckable(true);
    myAct->setChecked(!isColumnHidden(i));
    actions.append(myAct);
  }
  hideshow_column->setAttribute(Qt::WA_DeleteOnClose);
  connect(hideshow_column, &QMenu::triggered, this, [=](QAction *act) {
    int col;
    struct city_report_spec *spec;
    if (!act) {
      return;
    }

    col = actions.indexOf(act);
    fc_assert_ret(col >= 0);
    setColumnHidden(col, !isColumnHidden(col));
    spec = city_report_specs + col;
    spec->show = !spec->show;
    if (!isColumnHidden(col) && columnWidth(col) <= 5)
      setColumnWidth(col, 100);
  });
  hideshow_column->popup(QCursor::pos());
}

/***********************************************************************/ /**
   Hides columns for city widget, depending on stored data (bool spec->show)
 ***************************************************************************/
void city_widget::hide_columns()
{
  int col;

  for (col = 0; col < list_model->columnCount(); col++) {
    if (!list_model->hide_data(col).toBool()) {
      setColumnHidden(col, !isColumnHidden(col));
    }
  }
}

/***********************************************************************/ /**
   Slot for selecting items in city widget, they are stored in
   selected_cities until deselected
 ***************************************************************************/
void city_widget::cities_selected(const QItemSelection &sl,
                                  const QItemSelection &ds)
{
  Q_UNUSED(sl)
  Q_UNUSED(ds)
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  QVariant qvar;
  struct city *pcity;

  selected_cities.clear();

  if (indexes.isEmpty()) {
    return;
  }
  for (auto i : qAsConst(indexes)) {
    qvar = i.data(Qt::UserRole);
    if (qvar.isNull()) {
      continue;
    }
    pcity = reinterpret_cast<city *>(qvar.value<void *>());
    selected_cities << pcity;
  }
}

/***********************************************************************/ /**
   Returns used model
 ***************************************************************************/
city_model *city_widget::get_model() const { return list_model; }

/***********************************************************************/ /**
   Destructor for city widget
 ***************************************************************************/
city_widget::~city_widget()
{
  delete c_i_d;
  delete list_model;
  delete filter_model;
  king()->qt_settings.city_repo_sort_col = header()->sortIndicatorSection();
  king()->qt_settings.city_report_sort = header()->sortIndicatorOrder();
}

/***********************************************************************/ /**
   Constructor for city report
 ***************************************************************************/
city_report::city_report() : QWidget()
{
  layout = new QVBoxLayout;
  city_wdg = new city_widget(this);
  if (king()->qt_settings.city_repo_sort_col != -1) {
    city_wdg->sortByColumn(king()->qt_settings.city_repo_sort_col,
                           king()->qt_settings.city_report_sort);
  }
  layout->addWidget(city_wdg);
  setLayout(layout);
  index = 0;
}

/***********************************************************************/ /**
   Destructor for city report
 ***************************************************************************/
city_report::~city_report()
{
  queen()->removeRepoDlg(QStringLiteral("CTS"));
}

/***********************************************************************/ /**
   Inits place in game tab widget
 ***************************************************************************/
void city_report::init()
{
  queen()->gimmePlace(this, QStringLiteral("CTS"));
  index = queen()->addGameTab(this);
  queen()->game_tab_widget->setCurrentIndex(index);
}

/***********************************************************************/ /**
   Updates whole report
 ***************************************************************************/
void city_report::update_report() { city_wdg->update_model(); }

/***********************************************************************/ /**
   Updates single city
 ***************************************************************************/
void city_report::update_city(struct city *pcity)
{
  city_wdg->update_city(pcity);
}

/***********************************************************************/ /**
   Display the city report dialog.  Optionally raise it.
 ***************************************************************************/
void city_report_dialog_popup(bool raise)
{
  Q_UNUSED(raise)
  int i;
  city_report *cr;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("CTS"))) {
    cr = new city_report;
    cr->init();
    cr->update_report();
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("CTS"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    if (w->isVisible()) {
      queen()->game_tab_widget->setCurrentIndex(0);
      return;
    }
    cr = reinterpret_cast<city_report *>(w);
    queen()->game_tab_widget->setCurrentWidget(cr);
    cr->update_report();
  }
}

/***********************************************************************/ /**
   Update (refresh) the entire city report dialog.
 ***************************************************************************/
void real_city_report_dialog_update(void *unused)
{
  Q_UNUSED(unused)
  int i;
  city_report *cr;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("CTS"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("CTS"));
    if (queen()->game_tab_widget->currentIndex() == i) {
      w = queen()->game_tab_widget->widget(i);
      cr = reinterpret_cast<city_report *>(w);
      cr->update_report();
    }
  }
}

/***********************************************************************/ /**
   Update the information for a single city in the city report.
 ***************************************************************************/
void real_city_report_update_city(struct city *pcity)
{
  int i;
  city_report *cr;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("CTS"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("CTS"));
    if (queen()->game_tab_widget->currentIndex() == i) {
      w = queen()->game_tab_widget->widget(i);
      cr = reinterpret_cast<city_report *>(w);
      cr->update_city(pcity);
    }
  }
}

/***********************************************************************/ /**
   Closes city report
 ***************************************************************************/
void popdown_city_report()
{
  int i;
  city_report *cr;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("CTS"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("CTS"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    cr = reinterpret_cast<city_report *>(w);
    cr->deleteLater();
  }
  hIcon::drop();
}
