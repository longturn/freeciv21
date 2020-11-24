/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "fonts.h"
// Qt
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
// client
#include "gui_main.h"
#include "options.h"

/************************************************************************/ /**
   Font provider constructor
 ****************************************************************************/
fc_font::fc_font() : city_fontsize(12), prod_fontsize(12) {}

/************************************************************************/ /**
   Returns instance of fc_font
 ****************************************************************************/
fc_font *fc_font::instance()
{
  if (!m_instance)
    m_instance = new fc_font;
  return m_instance;
}

/************************************************************************/ /**
   Deletes fc_icons instance
 ****************************************************************************/
void fc_font::drop()
{
  if (m_instance) {
    m_instance->release_fonts();
    delete m_instance;
    m_instance = 0;
  }
}

/************************************************************************/ /**
   Returns desired font
 ****************************************************************************/
QFont *fc_font::get_font(const QString &name)
{
  /**
   * example: get_font("gui_qt_font_notify_label")
   */

  if (font_map.contains(name)) {
    return font_map.value(name);
  } else {
    return nullptr;
  }
}

/************************************************************************/ /**
   Initiazlizes fonts from client options
 ****************************************************************************/
void fc_font::init_fonts()
{
  QFont *f;
  QString s;

  /**
   * default font names are:
   * gui_qt_font_notify_label and so on.
   * (full list is in options.c in client dir)
   */

  options_iterate(client_optset, poption)
  {
    if (option_type(poption) == OT_FONT) {
      f = new QFont;
      s = option_font_get(poption);
      if (f->fromString(s)) {
        s = option_name(poption);
        set_font(s, f);
      } else {
        delete f;
      }
    }
  }
  options_iterate_end;
  get_mapfont_size();
}

/*****************************************************************************
   Increases/decreases all fonts sizes
 ****************************************************************************/
void fc_font::set_size_all(int new_size)
{
  options_iterate(client_optset, poption)
  {
    if (option_type(poption) == OT_FONT) {
      QFont font;
      font.fromString(option_font_get(poption));
      int old_size = font.pointSize();
      font.setPointSize(old_size + (new_size * old_size) / 100);
      QString s = font.toString();
      QByteArray ba = s.toLocal8Bit();
      option_font_set(poption, ba.data());
      gui_qt_apply_font(poption);
    }
  }
  options_iterate_end;
}

/************************************************************************/ /**
   Deletes all fonts
 ****************************************************************************/
void fc_font::release_fonts()
{
  for (QFont *f : qAsConst(font_map)) {
    delete f;
  }
}

/************************************************************************/ /**
   Stores default font sizes
 ****************************************************************************/
void fc_font::get_mapfont_size()
{
  city_fontsize = get_font(fonts::city_names)->pointSize();
  prod_fontsize = get_font(fonts::city_productions)->pointSize();
}

/************************************************************************/ /**
   Adds new font or overwrite old one
 ****************************************************************************/
void fc_font::set_font(const QString &name, QFont *qf)
{
  font_map.insert(name, qf);
}

/************************************************************************/ /**
   Tries to choose good fonts for freeciv-qt
 ****************************************************************************/
void configure_fonts()
{
  int max, smaller, default_size;
  QStringList sl;
  QString font_name;
  const QList<QScreen *> screens = QGuiApplication::screens();
  const QScreen *screen = screens.at(0);
  qreal logical_dpi = screen->logicalDotsPerInchX();
  qreal physical_dpi = screen->physicalDotsPerInchX();
  qreal screen_size = screen->geometry().width() / physical_dpi + 5;
  qreal scale = (physical_dpi * screen_size / (logical_dpi * 27))
                / screen->devicePixelRatio();
  QByteArray fn_bytes;

  max = qRound(scale * 16);
  smaller = qRound(scale * 12);
  default_size = qRound(scale * 14);

  /* default and help label*/
  sl << "Segoe UI"
     << "Cousine"
     << "Liberation Sans"
     << "Droid Sans"
     << "Ubuntu"
     << "Noto Sans"
     << "DejaVu Sans"
     << "Luxi Sans"
     << "Lucida Sans"
     << "Trebuchet MS"
     << "Times New Roman";
  font_name = configure_font(fonts::default_font, sl, max);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_default, fn_bytes.data(), 512);
  }
  font_name = configure_font(fonts::city_names, sl, smaller, true);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_city_names, fn_bytes.data(), 512);
  }
  /* default for help text */
  font_name = configure_font(fonts::help_text, sl, default_size);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_help_text, fn_bytes.data(), 512);
  }
  sl.clear();

  /* notify */
  sl << "Cousine"
     << "Liberation Mono"
     << "Source Code Pro"
     << "Source Code Pro [ADBO]"
     << "Noto Mono"
     << "Ubuntu Mono"
     << "Courier New";
  font_name = configure_font(fonts::notify_label, sl, default_size);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_notify_label, fn_bytes.data(), 512);
  }

  /* standard for chat */
  font_name = configure_font(fonts::chatline, sl, default_size);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_chatline, fn_bytes.data(), 512);
  }

  /* City production */
  sl.clear();
  sl << "Arimo"
     << "Play"
     << "Tinos"
     << "Ubuntu"
     << "Times New Roman"
     << "Droid Sans"
     << "Noto Sans";
  font_name =
      configure_font(fonts::city_productions, sl, default_size, true);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_city_productions, fn_bytes.data(),
               512);
  }
  /* Reqtree */
  sl.clear();
  sl << "Papyrus"
     << "Segoe Script"
     << "Comic Sans MS"
     << "Droid Sans"
     << "Noto Sans"
     << "Ubuntu";
  font_name = configure_font(fonts::reqtree_text, sl, max, true);
  if (!font_name.isEmpty()) {
    fn_bytes = font_name.toLocal8Bit();
    fc_strlcpy(gui_options.gui_qt_font_reqtree_text, fn_bytes.data(), 512);
  }
}

/************************************************************************/ /**
   Returns long font name, sets given for for use
 ****************************************************************************/
QString configure_font(const QString &font_name, const QStringList &sl,
                       int size, bool bold)
{
  QFontDatabase database;
  QFont *f;

  for (auto const &str : sl) {
    if (database.families().contains(str)) {
      QByteArray fn_bytes;

      f = new QFont(str, size);
      if (bold) {
        f->setBold(true);
      }
      fc_font::instance()->set_font(font_name, f);
      fn_bytes = f->toString().toLocal8Bit();

      return fn_bytes.data();
    }
  }
  return QString();
}
