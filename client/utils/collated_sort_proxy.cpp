/*
 * SPDX-License-Identifier: GPLv3-or-later
 * SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
 */

#include "collated_sort_proxy.h"

namespace freeciv {

/**
 * \class collated_sort_filter_proxy_model
 * \brief A sort and filter proxy model supporting string collation.
 *
 * This model can be used when strings in a model should use a "natural" sort
 * order.
 */

/**
 * \brief Constructor.
 */
collated_sort_filter_proxy_model::collated_sort_filter_proxy_model(
    QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

/**
 * \brief Changes the collator currently in use.
 */
void collated_sort_filter_proxy_model::set_collator(const QCollator &coll)
{
  m_collator = coll;
  invalidate();
}

/**
 * \brief Reimplemented protected function.
 *
 * This overrides @c QSortFilterProxyModel::lessThan to use the collator when
 * comparing strings.
 */
bool collated_sort_filter_proxy_model::lessThan(
    const QModelIndex &source_left, const QModelIndex &source_right) const
{
  // Copied from QSortFilterProxyModel
  QVariant l = (source_left.model()
                    ? source_left.model()->data(source_left, sortRole())
                    : QVariant());
  QVariant r = (source_right.model()
                    ? source_right.model()->data(source_right, sortRole())
                    : QVariant());

  if (l.typeId() == QMetaType::QString && r.typeId() == QMetaType::QString) {
    return m_collator(l.toString(), r.toString());
  }

  return QSortFilterProxyModel::lessThan(source_left, source_right);
}

} // namespace freeciv
