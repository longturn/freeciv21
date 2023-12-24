/*
 * SPDX-License-Identifier: GPLv3-or-later
 * SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
 */

#pragma once

#include <QCollator>
#include <QSortFilterProxyModel>

namespace freeciv {

class collated_sort_filter_proxy_model : public QSortFilterProxyModel {
  QCollator m_collator;

public:
  collated_sort_filter_proxy_model(QObject *parent = nullptr);

  /// Retrieves the string collator currently in use.
  QCollator collator() const { return m_collator; }
  void set_collator(const QCollator &coll);

protected:
  bool lessThan(const QModelIndex &source_left,
                const QModelIndex &source_right) const override;
};

} // namespace freeciv
