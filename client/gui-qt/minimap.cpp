/*####################################################################
###      ***                                     ***               ###
###     *****          ⒻⓇⒺⒺⒸⒾⓋ ②①         *****              ###
###      ***                                     ***               ###
#####################################################################*/

// Qt
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QToolTip>
#include <QWheelEvent>

// utility
#include "log.h"
#include "support.h"

// common
#include "calendar.h"
#include "game.h"
#include "map.h"
#include "research.h"

// client
#include "climisc.h"
#include "client_main.h"
#include "colors_common.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "menu.h" // gov_menu
#include "movement.h"
#include "overview_common.h"
#include "sprite.h"
#include "text.h"
#include "tilespec.h"

// gui-qt
#include "colors.h"
#include "fc_client.h"
#include "icons.h"
#include "mapview.h"
#include "minimap.h"
#include "sciencedlg.h"
#include "qtg_cxxside.h"
#include "sidebar.h"

/**********************************************************************/ /**
    TODO drop it
 **************************************************************************/
static void gui_to_overview(int *ovr_x, int *ovr_y, int gui_x, int gui_y)
{
  double ntl_x, ntl_y;
  const double gui_xd = gui_x, gui_yd = gui_y;
  const double W = tileset_tile_width(tileset);
  const double H = tileset_tile_height(tileset);
  double map_x, map_y;

  if (tileset_is_isometric(tileset)) {
    map_x = (gui_xd * H + gui_yd * W) / (W * H);
    map_y = (gui_yd * W - gui_xd * H) / (W * H);
  } else {
    map_x = gui_xd / W;
    map_y = gui_yd / H;
  }

  if (MAP_IS_ISOMETRIC) {
    ntl_y = map_x + map_y - wld.map.xsize;
    ntl_x = 2 * map_x - ntl_y;
  } else {
    ntl_x = map_x;
    ntl_y = map_y;
  }

  *ovr_x = floor((ntl_x - (double) gui_options.overview.map_x0)
                 * OVERVIEW_TILE_SIZE);
  *ovr_y = floor((ntl_y - (double) gui_options.overview.map_y0)
                 * OVERVIEW_TILE_SIZE);

  if (current_topo_has_flag(TF_WRAPX)) {
    *ovr_x = FC_WRAP(*ovr_x, NATURAL_WIDTH * OVERVIEW_TILE_SIZE);
  } else {
    if (MAP_IS_ISOMETRIC) {
      *ovr_x -= OVERVIEW_TILE_SIZE;
    }
  }
  if (current_topo_has_flag(TF_WRAPY)) {
    *ovr_y = FC_WRAP(*ovr_y, NATURAL_HEIGHT * OVERVIEW_TILE_SIZE);
  }
}


/**********************************************************************/ /**
   Constructor for minimap
 **************************************************************************/
minimap_view::minimap_view(QWidget *parent) : fcwidget()
{
  setParent(parent);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  w_ratio = 0.0;
  h_ratio = 0.0;
  // Dark magic: This call is required for the widget to work.
  resize(0, 0);
  background = QBrush(QColor(0, 0, 0));
  setCursor(Qt::CrossCursor);
  rw = new resize_widget(this);
  rw->put_to_corner();
  pix = new QPixmap;
  scale_factor = 1.0;
  connect(&thread, &minimap_thread::rendered_image, this,
          &minimap_view::update_pixmap);
}

/**********************************************************************/ /**
   Minimap_view destructor
 **************************************************************************/
minimap_view::~minimap_view()
{
  if (pix) {
    delete pix;
  }
}

/**********************************************************************/ /**
   Paint event for minimap
 **************************************************************************/
void minimap_view::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**********************************************************************/ /**
   Sets scaling factor for minimap
 **************************************************************************/
void minimap_view::scale(double factor)
{
  scale_factor *= factor;
  if (scale_factor < 1) {
    scale_factor = 1.0;
  };
  update_image();
}

/**********************************************************************/ /**
   Called by close widget, cause widget has been hidden. Updates menu.
 **************************************************************************/
void minimap_view::update_menu()
{
  ::gui()->menu_bar->minimap_status->setChecked(false);
}

/**********************************************************************/ /**
   Minimap is being moved, position is being remembered
 **************************************************************************/
void minimap_view::moveEvent(QMoveEvent *event) { position = event->pos(); }

/**********************************************************************/ /**
   Minimap is just unhidden, old position is restored
 **************************************************************************/
void minimap_view::showEvent(QShowEvent *event)
{
  move(position);
  event->setAccepted(true);
}

/**********************************************************************/ /**
   Draws viewport on minimap
 **************************************************************************/
void minimap_view::draw_viewport(QPainter *painter)
{
  int i, x[4], y[4];
  int src_x, src_y, dst_x, dst_y;

  if (!gui_options.overview.map) {
    return;
  }

  gui_to_overview(&x[0], &y[0], mapview.gui_x0, mapview.gui_y0);
  gui_to_overview(&x[1], &y[1], mapview.gui_x0 + mapview.width,
                  mapview.gui_y0);
  gui_to_overview(&x[2], &y[2], mapview.gui_x0 + mapview.width,
                  mapview.gui_y0 + mapview.height);
  gui_to_overview(&x[3], &y[3], mapview.gui_x0,
                  mapview.gui_y0 + mapview.height);
  painter->setPen(QColor(Qt::white));

  if (scale_factor > 1) {
    for (i = 0; i < 4; i++) {
      scale_point(x[i], y[i]);
    }
  }

  for (i = 0; i < 4; i++) {
    src_x = x[i] * w_ratio;
    src_y = y[i] * h_ratio;
    dst_x = x[(i + 1) % 4] * w_ratio;
    dst_y = y[(i + 1) % 4] * h_ratio;
    painter->drawLine(src_x, src_y, dst_x, dst_y);
  }
}

/**********************************************************************/ /**
   Scales point from real overview coords to scaled overview coords.
 **************************************************************************/
void minimap_view::scale_point(int &x, int &y)
{
  int ax, bx;
  int dx, dy;

  gui_to_overview(&ax, &bx, mapview.gui_x0 + mapview.width / 2,
                  mapview.gui_y0 + mapview.height / 2);
  x = qRound(x * scale_factor);
  y = qRound(y * scale_factor);
  dx = qRound(ax * scale_factor - gui_options.overview.width / 2);
  dy = qRound(bx * scale_factor - gui_options.overview.height / 2);
  x = x - dx;
  y = y - dy;
}

/**********************************************************************/ /**
   Scales point from scaled overview coords to real overview coords.
 **************************************************************************/
void unscale_point(double scale_factor, int &x, int &y)
{
  int ax, bx;
  int dx, dy;

  gui_to_overview(&ax, &bx, mapview.gui_x0 + mapview.width / 2,
                  mapview.gui_y0 + mapview.height / 2);
  dx = qRound(ax * scale_factor - gui_options.overview.width / 2);
  dy = qRound(bx * scale_factor - gui_options.overview.height / 2);
  x = x + dx;
  y = y + dy;
  x = qRound(x / scale_factor);
  y = qRound(y / scale_factor);
}

/**********************************************************************/ /**
   Sets minimap scale to default
 **************************************************************************/
void minimap_view::reset() { scale_factor = 1; }

/**********************************************************************/ /**
   Slot for updating pixmap from thread's image
 **************************************************************************/
void minimap_view::update_pixmap(const QImage &image)
{
  *pix = QPixmap::fromImage(image);
  update();
}


/**********************************************************************/ /**
   Minimap thread's contructor
 **************************************************************************/
minimap_thread::minimap_thread(QObject *parent) : QThread(parent) {}

/**********************************************************************/ /**
   Minimap thread's desctructor
 **************************************************************************/
minimap_thread::~minimap_thread() { wait(); }

/**********************************************************************/ /**
   Starts thread
 **************************************************************************/
void minimap_thread::render(double scale_factor, int width, int height)
{
  QMutexLocker locker(&mutex);
  mini_width = width;
  mini_height = height;
  scale = scale_factor;
  start(LowPriority);
}

/**********************************************************************/ /**
   Updates minimap's image in thread
 **************************************************************************/
void minimap_thread::run()
{
  QImage tpix;
  QImage gpix;
  QImage image(QSize(mini_width, mini_height), QImage::Format_RGB32);
  QImage bigger_pix(gui_options.overview.width * 2,
                    gui_options.overview.height * 2, QImage::Format_RGB32);
  int delta_x, delta_y;
  int x, y, ix, iy;
  float wf, hf;
  QPixmap *src, *dst;

  mutex.lock();
  if (gui_options.overview.map != nullptr) {
    if (scale > 1) {
      /* move minimap now,
         scale later and draw without looking for origin */
      src = &gui_options.overview.map->map_pixmap;
      dst = &gui_options.overview.window->map_pixmap;
      x = gui_options.overview.map_x0;
      y = gui_options.overview.map_y0;
      ix = gui_options.overview.width - x;
      iy = gui_options.overview.height - y;
      pixmap_copy(dst, src, 0, 0, ix, iy, x, y);
      pixmap_copy(dst, src, 0, y, ix, 0, x, iy);
      pixmap_copy(dst, src, x, 0, 0, iy, ix, y);
      pixmap_copy(dst, src, x, y, 0, 0, ix, iy);
      tpix = gui_options.overview.window->map_pixmap.toImage();
      wf = static_cast<float>(gui_options.overview.width) / scale;
      hf = static_cast<float>(gui_options.overview.height) / scale;
      x = 0;
      y = 0;
      unscale_point(scale, x, y);
      bigger_pix.fill(Qt::black);
      delta_x = gui_options.overview.width / 2;
      delta_y = gui_options.overview.height / 2;
      image_copy(&bigger_pix, &tpix, 0, 0, delta_x, delta_y,
                 gui_options.overview.width, gui_options.overview.height);
      gpix = bigger_pix.copy(delta_x + x, delta_y + y, wf, hf);
      image = gpix.scaled(mini_width, mini_height, Qt::IgnoreAspectRatio,
                          Qt::FastTransformation);
    } else {
      tpix = gui_options.overview.map->map_pixmap.toImage();
      image = tpix.scaled(mini_width, mini_height, Qt::IgnoreAspectRatio,
                          Qt::FastTransformation);
    }
  }
  emit rendered_image(image);
  mutex.unlock();
}

/**********************************************************************/ /**
   Updates minimap's pixmap
 **************************************************************************/
void minimap_view::update_image()
{
  if (isHidden()) {
    return;
  }
  thread.render(scale_factor, width(), height());
}

/**********************************************************************/ /**
   Redraws visible map using stored pixmap
 **************************************************************************/
void minimap_view::paint(QPainter *painter, QPaintEvent *event)
{
  int x, y, ix, iy;

  x = gui_options.overview.map_x0 * w_ratio;
  y = gui_options.overview.map_y0 * h_ratio;
  ix = pix->width() - x;
  iy = pix->height() - y;

  if (scale_factor > 1) {
    painter->drawPixmap(0, 0, *pix, 0, 0, pix->width(), pix->height());
  } else {
    painter->drawPixmap(ix, iy, *pix, 0, 0, x, y);
    painter->drawPixmap(ix, 0, *pix, 0, y, x, iy);
    painter->drawPixmap(0, iy, *pix, x, 0, ix, y);
    painter->drawPixmap(0, 0, *pix, x, y, ix, iy);
  }
  painter->setPen(QColor(palette().color(QPalette::Highlight)));
  painter->drawRect(0, 0, width() - 1, height() - 1);
  draw_viewport(painter);
  rw->put_to_corner();
}

/**********************************************************************/ /**
   Called when minimap has been resized
 **************************************************************************/
void minimap_view::resizeEvent(QResizeEvent *event)
{
  QSize size;
  size = event->size();

  if (C_S_RUNNING <= client_state() && size.width() > 0
      && size.height() > 0) {
    w_ratio = static_cast<float>(width()) / gui_options.overview.width;
    h_ratio = static_cast<float>(height()) / gui_options.overview.height;
    gui()->qt_settings.minimap_width =
        static_cast<float>(size.width()) / mapview.width;
    gui()->qt_settings.minimap_height =
        static_cast<float>(size.height()) / mapview.height;
  }
  update_image();
}

/**********************************************************************/ /**
   Wheel event for minimap - zooms it in or out
 **************************************************************************/
void minimap_view::wheelEvent(QWheelEvent *event)
{
  if (event->delta() > 0) {
    zoom_in();
  } else {
    zoom_out();
  }
  event->accept();
}

/**********************************************************************/ /**
   Sets scale factor to scale minimap 20% up
 **************************************************************************/
void minimap_view::zoom_in()
{
  if (scale_factor < gui_options.overview.width / 8) {
    scale(1.2);
  }
}

/**********************************************************************/ /**
   Sets scale factor to scale minimap 20% down
 **************************************************************************/
void minimap_view::zoom_out() { scale(0.833); }

/**********************************************************************/ /**
   Mouse Handler for minimap_view
   Left button - moves minimap
   Right button - recenters on some point
   For wheel look mouseWheelEvent
 **************************************************************************/
void minimap_view::mousePressEvent(QMouseEvent *event)
{
  int fx, fy;
  int x, y;

  if (event->button() == Qt::LeftButton) {
    if (gui()->interface_locked) {
      return;
    }
    cursor = event->globalPos() - geometry().topLeft();
  }
  if (event->button() == Qt::RightButton) {
    cursor = event->pos();
    fx = event->pos().x();
    fy = event->pos().y();
    fx = qRound(fx / w_ratio);
    fy = qRound(fy / h_ratio);
    if (scale_factor > 1) {
      unscale_point(scale_factor, fx, fy);
    }
    fx = qMax(fx, 1);
    fy = qMax(fy, 1);
    fx = qMin(fx, gui_options.overview.width - 1);
    fy = qMin(fy, gui_options.overview.height - 1);
    overview_to_map_pos(&x, &y, fx, fy);
    center_tile_mapcanvas(map_pos_to_tile(&(wld.map), x, y));
    update_image();
  }
  event->setAccepted(true);
}

/**********************************************************************/ /**
   Called when mouse button was pressed. Used to moving minimap.
 **************************************************************************/
void minimap_view::mouseMoveEvent(QMouseEvent *event)
{
  if (gui()->interface_locked) {
    return;
  }
  if (event->buttons() & Qt::LeftButton) {
    QPoint p, r;
    p = event->pos();
    r = mapTo(gui()->mapview_wdg, p);
    p = r - p;
    move(event->globalPos() - cursor);
    setCursor(Qt::SizeAllCursor);
    gui()->qt_settings.minimap_x = static_cast<float>(p.x()) / mapview.width;
    gui()->qt_settings.minimap_y =
        static_cast<float>(p.y()) / mapview.height;
  }
}

/**********************************************************************/ /**
   Called when mouse button unpressed. Restores cursor.
 **************************************************************************/
void minimap_view::mouseReleaseEvent(QMouseEvent *event)
{
  setCursor(Qt::CrossCursor);
}

/**********************************************************************/ /**
   Return a canvas that is the overview window.
 **************************************************************************/
struct canvas *get_overview_window(void)
{
  gui()->minimapview_wdg->update_image();
  return NULL;
}

/**********************************************************************/ /**
   Return the dimensions of the area (container widget; maximum size) for
   the overview.
 **************************************************************************/
void get_overview_area_dimensions(int *width, int *height)
{
  *width = 0;
  *height = 0;
}

/**********************************************************************/ /**
   Called when the map size changes. This may be used to change the
   size of the GUI element holding the overview canvas. The
   overview.width and overview.height are updated if this function is
   called.
   It's used for first creation of overview only, later overview stays the
   same size, scaled by qt-specific function.
 **************************************************************************/
void overview_size_changed(void)
{
  gui()->minimapview_wdg->resize(0, 0);
  gui()->minimapview_wdg->move(gui()->qt_settings.minimap_x * mapview.width,
                               gui()->qt_settings.minimap_y
                                   * mapview.height);
  gui()->minimapview_wdg->resize(
      gui()->qt_settings.minimap_width * mapview.width,
      gui()->qt_settings.minimap_height * mapview.height);
}

/**********************************************************************/ /**
   Sets the position of the overview scroll window based on mapview position.
 **************************************************************************/
void update_overview_scroll_window_pos(int x, int y)
{
}

