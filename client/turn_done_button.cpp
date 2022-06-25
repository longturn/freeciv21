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

#include <QDateTime>
#include <QShortcut>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

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
  if (m_timeout_label.isEmpty()) {
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
        option.palette, isEnabled(), m_timeout_label, QPalette::ButtonText);

    p.restore();
  }
}

namespace {

/**
 * Format a duration, in seconds, so it comes up in minutes or hours if
 * that would be more meaningful.
 */
QString format_duration(int duration)
{
  if (duration < 0) {
    duration = 0;
  }

  const auto now = QDateTime::currentDateTime();
  const auto turn_change = now.addSecs(duration);
  const auto days_left = now.daysTo(turn_change);

  if (duration < 60) {
    return QString(Q_("?seconds:%1s")).arg(duration, 2);
  } else if (duration < 5 * 60) { // < 5 minutes
    return QString(Q_("?mins/secs:%1min %2s"))
        .arg(duration / 60, 2)
        .arg(duration % 60, 2);
  } else if (duration < 3600) { // < one hour
    // TRANS: Used in "Time left: 10 minutes". Always at least 5 minutes
    return QString(_("%1 minutes")).arg(duration / 60);
  } else if (days_left == 0) { // Same day
    return QString(Q_("?hrs/mns:%1h %2min"))
        .arg(duration / 3600, 2)
        .arg((duration / 60) % 60, 2);
  } else if (days_left == 1) { // Tomorrow
    const auto time =
        QLocale().toString(turn_change, QStringLiteral("hh:mm"));
    // TRANS: Used in "Time left: until tomorrow 17:59"
    return QString(_("until tomorrow %1")).arg(time);
  } else if (days_left < 7) { // This week
    const auto time =
        QLocale().toString(turn_change, QStringLiteral("dddd hh:mm"));
    // TRANS: Used in "Time left: until Monday 17:59"
    return QString(_("until %1")).arg(time);
  }

  // TRANS: Used to indicate a fuzzy duration. "until tomorrow" is never used
  return QString(_("%1 days")).arg(days_left);
}
} // anonymous namespace

/**
 * Updates the timeout text according to the current state of the game.
 */
void turn_done_button::update_timeout_label()
{
  if (is_waiting_turn_change() && game.tinfo.last_turn_change_time >= 1.5) {
    // TRANS: Processing turn change
    m_timeout_label = QString(_("Processing... %1"))
                          .arg(format_duration(get_seconds_to_new_turn()));
  } else if (current_turn_timeout() > 0) {
    m_timeout_label = QString(_("Time left: %1"))
                          .arg(format_duration(get_seconds_to_turndone()));
  } else {
    m_timeout_label = QString();
  }

  // Redraw
  update();
}
