/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include <QGlobalStatic>
#include <cstdarg>

// utility
#include "fcintl.h"
#include "fcthread.h"

// common
#include "featured_text.h"
#include "packets.h"

// client
#include "chatline.h"
#include "chatline_common.h"
#include "client_main.h"

Q_GLOBAL_STATIC(QMutex, ow_mutex);

/**
   Send the message as a chat to the server.
 */
int send_chat(const char *message)
{
  return dsend_packet_chat_msg_req(&client.conn, message);
}

/**
   Send the message as a chat to the server. Message is constructed
   in printf style.
 */
int send_chat_printf(const char *format, ...)
{
  struct packet_chat_msg_req packet;
  va_list args;
  QByteArray ba;

  va_start(args, format);
  auto str = QString::vasprintf(format, args);
  va_end(args);

  ba = str.toLocal8Bit();
  qstrncpy(packet.message, ba.data(), sizeof(packet.message));

  return send_packet_chat_msg_req(&client.conn, &packet);
}

/**
   Allocate output window mutex
 */
void fc_allocate_ow_mutex() { ow_mutex->lock(); }

/**
   Release output window mutex
 */
void fc_release_ow_mutex() { ow_mutex->unlock(); }

/**
   Initialize output window mutex
 */
void fc_init_ow_mutex() {}

/**
   Destroy output window mutex
 */
void fc_destroy_ow_mutex() {}

/**
   Add a line of text to the output ("chatline") window, like puts() would
   do it in the console.
 */
void output_window_append(const struct ft_color color,
                          const char *featured_text)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags;

  // Separate the text and the tags.
  featured_text_to_plain_text(featured_text, plain_text, sizeof(plain_text),
                              &tags, false);

  if (ft_color_requested(color)) {
    // A color is requested.
    struct text_tag *ptag =
        text_tag_new(TTT_COLOR, 0, FT_OFFSET_UNSET, color);

    if (ptag) {
      // Prepends to the list, to avoid to overwrite inside colors.
      text_tag_list_prepend(tags, ptag);
    } else {
      qCritical(
          "Failed to create a color text tag (fg = %s, bg = %s).",
          (nullptr != color.foreground ? color.foreground : "nullptr"),
          (nullptr != color.background ? color.background : "nullptr"));
    }
  }

  fc_allocate_ow_mutex();
  real_output_window_append(plain_text, tags);
  fc_release_ow_mutex();
  text_tag_list_destroy(tags);
}

/**
   Add a line of text to the output ("chatline") window.  The text is
   constructed in printf style.
 */
void output_window_vprintf(const struct ft_color color, const char *format,
                           va_list args)
{
  char featured_text[MAX_LEN_MSG];

  fc_vsnprintf(featured_text, sizeof(featured_text), format, args);
  output_window_append(color, featured_text);
}

/**
   Add a line of text to the output ("chatline") window.  The text is
   constructed in printf style.
 */
void output_window_printf(const struct ft_color color, const char *format,
                          ...)
{
  va_list args;

  va_start(args, format);
  output_window_vprintf(color, format, args);
  va_end(args);
}

/**
   Add a line of text to the output ("chatline") window from server event.
 */
void output_window_event(const char *plain_text,
                         const struct text_tag_list *tags)
{
  fc_allocate_ow_mutex();
  real_output_window_append(plain_text, tags);
  fc_release_ow_mutex();
}

/**
   Standard welcome message.
 */
void chat_welcome_message(bool gui_has_copying_mitem)
{
  output_window_append(ftc_any, _("Freeciv21 is free software and you are "
                                  "welcome to distribute copies of it "
                                  "under certain conditions;"));
  if (gui_has_copying_mitem) {
    output_window_append(ftc_any, _("See the \"Copying\" item on the "
                                    "Help menu."));
  } else {
    output_window_append(ftc_any, _("See COPYING file distributed with "
                                    "this program."));
  }
  output_window_append(ftc_any, _("Now ... Go give 'em hell!"));
}
