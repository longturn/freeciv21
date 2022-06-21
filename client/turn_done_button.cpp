/*
 * Code partly from QCommandLinkButton
 *
 * SPDX-FileCopyrightText: 2016 The Qt Company Ltd.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "turn_done_button.h"

#include "client_main.h"
#include "fcintl.h"
#include "fonts.h"
#include "game.h"
#include "text.h"

#include <QShortcut>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

#include <cmath>

namespace {
/**
 * Constants used when drawing the button
 */
namespace metrics {
/// By how many points the size of the title font is increased
const int font_increase = 1;
/// The margin around the contents
const int contents_margin = 10;
/// The separation between the two lines
const int line_separation = 6;
} // namespace metrics
} // namespace

/**
 * Constructor.
 */
turn_done_button::turn_done_button(QWidget *parent) : QPushButton(parent)
{
  // Set up shortcut (Shift+Enter, both main keyboard and numpad)
  setShortcut(Qt::Key_Shift + Qt::Key_Enter);
  connect(new QShortcut(Qt::Key_Shift + Qt::Key_Return, this),
          &QShortcut::activated, this, [this] { animateClick(); });

  setText(_("Turn Done"));
  setToolTip(_("End the current turn"));

  // FIXME This should come from the style...
  // Set the font for the title
  auto font = fcFont::instance()->getFont(fonts::default_font);
  font.setPointSize(font.pointSize() + metrics::font_increase);
  font.setBold(true);
  setFont(font);

  setContentsMargins(metrics::contents_margin, metrics::contents_margin,
                     metrics::contents_margin, metrics::contents_margin);
}

/**
 * Returns the size hint for this widget.
 */
QSize turn_done_button::sizeHint() const
{
  auto size = QPushButton::sizeHint(); // Accounts for title width

  const auto margins = contentsMargins();
  const auto fm_title = QFontMetrics(font());
  const auto fm_text =
      QFontMetrics(fcFont::instance()->getFont(fonts::default_font));

  size.setHeight(margins.top() + fm_title.height() + metrics::line_separation
                 + fm_text.height() + margins.bottom());

  return size;
}

/**
 * Paints the widget.
 */
void turn_done_button::paintEvent(QPaintEvent *event)
{
  if (description().isEmpty()) {
    QPushButton::paintEvent(event);
  } else {
    // The code below is adapted from QCommandLinkButton
    QStylePainter p(this);
    p.save();

    QStyleOptionButton option;
    initStyleOption(&option);
    option.text = QString();
    option.icon = QIcon();

    // Some geometry
    const int voffset =
        isDown() ? style()->pixelMetric(QStyle::PM_ButtonShiftVertical) : 0;
    const int hoffset =
        isDown() ? style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal)
                 : 0;

    const auto margins = contentsMargins();
    auto text_rect = rect().adjusted(0, margins.top(), 0, -margins.bottom());
    text_rect = text_rect.translated(hoffset, voffset);

    // Draw the background
    p.drawControl(QStyle::CE_PushButton, option);

    // Draw the title
    int title_flags = Qt::TextShowMnemonic | Qt::AlignHCenter | Qt::AlignTop;
    if (!style()->styleHint(QStyle::SH_UnderlineShortcut, &option, this)) {
      title_flags |= Qt::TextHideMnemonic;
    }
    p.setFont(font());

    p.drawItemText(text_rect, title_flags, option.palette, isEnabled(),
                   text(), QPalette::ButtonText);

    // Draw the timeout text
    p.setFont(fcFont::instance()->getFont(fonts::default_font));
    p.drawItemText(
        text_rect, Qt::AlignHCenter | Qt::AlignBottom | Qt::TextSingleLine,
        option.palette, isEnabled(), description(), QPalette::ButtonText);

    p.restore();
  }
}

namespace {
/**
 * Get the text showing the timeout.  This is generally disaplyed on the info
 * panel.
 */
const QString get_timeout_label_text()
{
  QString str;

  if (is_waiting_turn_change() && game.tinfo.last_turn_change_time >= 1.5) {
    double wt = get_seconds_to_new_turn();

    if (wt < 0.01) {
      str = Q_("?timeout:wait");
    } else {
      str = QStringLiteral("%1: %2").arg(Q_("?timeout:eta"),
                                         format_duration(wt));
    }
  } else if (current_turn_timeout() > 0) {
    str =
        QStringLiteral("%1").arg(format_duration(get_seconds_to_turndone()));
  }

  return str.trimmed();
}
}

/**
 * Updates the timeout text according to the current state of the game.
 */
void turn_done_button::update_timeout_label()
{
  setDescription(get_timeout_label_text());
}
