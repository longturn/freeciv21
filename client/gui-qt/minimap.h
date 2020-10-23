/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

#ifndef FC__MINIMAP_H
#define FC__MINIMAP_H

#include "mapview_g.h"

// Qt
#include <QFrame>
#include <QLabel>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QTimer>

#include "widgetdecorations.h"  // for fcwidget, resize_widget (ptr only)
class QImage;
class QMouseEvent;
class QMoveEvent;
class QObject;
class QPaintEvent;
class QPainter;
class QPixmap;  // lines 26-26
class QResizeEvent;
class QShowEvent;
class QWheelEvent;
class QWidget;

/**************************************************************************
  Thread helper for drawing minimap
**************************************************************************/
class minimap_thread : public QThread {
  Q_OBJECT
public:
  minimap_thread(QObject *parent = 0);
  ~minimap_thread();
  void render(double scale_factor, int width, int height);

signals:
  void rendered_image(const QImage &image);

protected:
  void run() Q_DECL_OVERRIDE;

private:
  int mini_width, mini_height;
  double scale;
  QMutex mutex;
};

/**************************************************************************
  Widget used for displaying overview (minimap)
**************************************************************************/
class minimap_view : public fcwidget {
  Q_OBJECT
public:
  minimap_view(QWidget *parent);
  ~minimap_view();
  void paint(QPainter *painter, QPaintEvent *event);
  virtual void update_menu();
  void update_image();
  void reset();

protected:
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void moveEvent(QMoveEvent *event);
  void showEvent(QShowEvent *event);

private slots:
  void update_pixmap(const QImage &image);
  void zoom_in();
  void zoom_out();

private:
  void draw_viewport(QPainter *painter);
  void scale(double factor);
  void scale_point(int &x, int &y);
  double scale_factor;
  float w_ratio, h_ratio;
  minimap_thread thread;
  QBrush background;
  QPixmap *pix;
  QPoint cursor;
  QPoint position;
  resize_widget *rw;
};

#endif /* FC__MINIMAP_H */
