/**************************************************************************
 Copyright (c) 2021 Freeciv21 contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <QDialog>

class QAction;
class QLabel;
class QListWidget;
class QTreeWidget;

struct tile;
struct tileset;

namespace freeciv {

class tileset_debugger : public QDialog {
  Q_OBJECT

public:
  explicit tileset_debugger(QWidget *parent = nullptr);
  virtual ~tileset_debugger();

  void refresh(const struct tileset *t);

  const ::tile *tile() const { return m_tile; }
  void set_tile(const ::tile *t);

signals:
  void tile_picking_requested(bool active);

private slots:
  void pick_tile(bool active);

private:
  void refresh_messages(const struct tileset *t);

  const ::tile *m_tile;
  QLabel *m_label;
  QAction *m_pick_action;
  QListWidget *m_messages;
  QTreeWidget *m_content;
};

} // namespace freeciv
