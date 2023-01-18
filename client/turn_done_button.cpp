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

  // FIXME This should come from the style...
  // Set the font for the title
  auto font = fcFont::instance()->getFont(fonts::default_font);
  font.setPointSize(font.pointSize() + metrics::font_increase);
  font.setBold(true);
  setFont(font);

  setContentsMargins(metrics::contents_margin, metrics::contents_margin,
                     metrics::contents_margin, metrics::contents_margin);

  update_timeout_label();
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
 * Formats a duration without switching to "until hh::mm" when more than one
 * hour in the future.
 */
QString format_simple_duration(int seconds)
{
  if (seconds < 0) {
    seconds = 0;
  }

  if (seconds < 60) {
    return QString(Q_("?seconds:%1s")).arg(seconds, 2);
  } else if (seconds < 5 * 60) { // < 5 minutes
    return QString(Q_("?mins/secs:%1min %2s"))
        .arg(seconds / 60, 2)
        .arg(seconds % 60, 2);
  } else if (seconds < 3600) { // < one hour
    // TRANS: Used in "Time left: 10 minutes". Always at least 5 minutes
    return QString(_("%1 minutes")).arg(seconds / 60);
  } else {
    // Hours and minutes
    const auto minutes = seconds / 60;
    return QString(_("%1h %2min")).arg(minutes / 60).arg(minutes % 60);
  }
}

/**
 * Format the duration until TC, given in seconds, so it comes up in minutes
 * or hours if that would be more meaningful. If allow_date is true, switches
 * to displaying the date and time of TC when it's too far in the future.
 */
QString format_duration(int duration)
{
  if (duration < 0) {
    duration = 0;
  }

  const auto now = QDateTime::currentDateTime();
  const auto turn_change = now.addSecs(duration);
  const auto days_left = now.daysTo(turn_change);

  if (duration < 3600) { // < one hour
    return format_simple_duration(duration);
  } else if (days_left == 0) { // Same day
    const auto time =
        QLocale().toString(turn_change, QStringLiteral("hh:mm"));
    // TRANS: Used in "Time left: until 17:59", %1 is hours and minutes
    return QString(_("until %1")).arg(time);
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

  // TRANS: Used to indicate a fuzzy duration, always more than 7 days
  return QString(PL_("%1 day", "%1 days", days_left)).arg(days_left);
}
} // anonymous namespace

/**
 * Updates the timeout text according to the current state of the game.
 */
void turn_done_button::update_timeout_label()
{
  QString tooltip = _("End the current turn");

  if (is_waiting_turn_change() && game.tinfo.last_turn_change_time >= 1.5) {
    // TRANS: Processing turn change
    m_timeout_label = QString(_("Processing... %1"))
                          .arg(format_duration(get_seconds_to_new_turn()));
  } else if (current_turn_timeout() > 0) {
    m_timeout_label = QString(_("Time left: %1"))
                          .arg(format_duration(get_seconds_to_turndone()));
    tooltip += QStringLiteral("\n");
    // TRANS: Time until turn change in the (). %1 is "5s", "4min", or "6h
    //        7min".
    tooltip = QString(_("End the current turn (%1 remaining)"))
                  .arg(format_simple_duration(get_seconds_to_turndone()));
  } else {
    m_timeout_label = QString();
  }

  setToolTip(tooltip);

  // Redraw
  update();
}
