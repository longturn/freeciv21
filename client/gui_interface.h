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

#pragma once

// Forward declarations
class QTcpSocket;

// common
#include "fc_types.h"
#include "featured_text.h"
#include "tile.h"

/* client/include */
#include "canvas_g.h"
#include "pages_g.h"

// client
#include "tilespec.h"

struct gui_funcs {
  void (*free_intro_radar_sprites)();
  QPixmap *(*load_gfxfile)(const char *filename);
  QPixmap *(*create_sprite)(int width, int height, const QColor *pcolor);
  void (*get_sprite_dimensions)(const QPixmap *sprite, int *width,
                                int *height);
  QPixmap *(*crop_sprite)(const QPixmap *source, int x, int y, int width,
                          int height, const QPixmap *mask, int mask_offset_x,
                          int mask_offset_y);
  void (*free_sprite)(QPixmap *s);

  QColor *(*color_alloc)(int r, int g, int b);
  void (*color_free)(QColor *pcolor);

  QPixmap *(*canvas_create)(int width, int height);
  void (*canvas_free)(QPixmap *store);
  void (*canvas_copy)(QPixmap *dest, const QPixmap *src, int src_x,
                      int src_y, int dest_x, int dest_y, int width,
                      int height);
  void (*canvas_put_sprite)(QPixmap *pcanvas, int canvas_x, int canvas_y,
                            const QPixmap *psprite, int offset_x,
                            int offset_y, int width, int height);
  void (*canvas_put_sprite_full)(QPixmap *pcanvas, int canvas_x,
                                 int canvas_y, const QPixmap *psprite);
  void (*canvas_put_sprite_fogged)(QPixmap *pcanvas, int canvas_x,
                                   int canvas_y, const QPixmap *psprite,
                                   bool fog, int fog_x, int fog_y);
  void (*canvas_put_sprite_citymode)(QPixmap *pcanvas, int canvas_x,
                                     int canvas_y, const QPixmap *psprite,
                                     bool fog, int fog_x, int fog_y);
  void (*canvas_put_rectangle)(QPixmap *pcanvas, const QColor *pcolor,
                               int canvas_x, int canvas_y, int width,
                               int height);
  void (*canvas_fill_sprite_area)(QPixmap *pcanvas, const QPixmap *psprite,
                                  const QColor *pcolor, int canvas_x,
                                  int canvas_y);
  void (*canvas_put_line)(QPixmap *pcanvas, const QColor *pcolor,
                          enum line_type ltype, int start_x, int start_y,
                          int dx, int dy);
  void (*canvas_put_curved_line)(QPixmap *pcanvas, const QColor *pcolor,
                                 enum line_type ltype, int start_x,
                                 int start_y, int dx, int dy);
  void (*get_text_size)(int *width, int *height, enum client_font font,
                        const QString &text);
  void (*canvas_put_text)(QPixmap *pcanvas, int canvas_x, int canvas_y,
                          enum client_font font, const QColor *pcolor,
                          const QString &);

  void (*set_rulesets)(int num_rulesets, QStringList rulesets);
  void (*options_extra_init)();
  void (*add_net_input)(QTcpSocket *sock);
  void (*remove_net_input)();
  void (*real_conn_list_dialog_update)(void *unused);
  void (*close_connection_dialog)();
  void (*add_idle_callback)(void(callback)(void *), void *data);
  void (*sound_bell)();

  void (*real_set_client_page)(enum client_pages page);
  enum client_pages (*get_current_client_page)();

  void (*set_unit_icon)(int idx, struct unit *punit);
  void (*set_unit_icons_more_arrow)(bool onoff);
  void (*real_focus_units_changed)();
  void (*gui_update_font)(const QString &font_name,
                          const QString &font_value);

  void (*editgui_refresh)();
  void (*editgui_notify_object_created)(int tag, int id);
  void (*editgui_notify_object_changed)(int objtype, int object_id,
                                        bool removal);
  void (*editgui_popup_properties)(const struct tile_list *tiles,
                                   int objtype);
  void (*editgui_tileset_changed)();
  void (*editgui_popdown_all)();

  void (*popup_combat_info)(int attacker_unit_id, int defender_unit_id,
                            int attacker_hp, int defender_hp,
                            bool make_att_veteran, bool make_def_veteran);
  void (*update_timeout_label)();
  void (*start_turn)();
  void (*real_city_dialog_popup)(struct city *pcity);
  void (*real_city_dialog_refresh)(struct city *pcity);
  void (*popdown_city_dialog)(struct city *pcity);
  void (*popdown_all_city_dialogs)();
  bool (*handmade_scenario_warning)();
  void (*refresh_unit_city_dialogs)(struct unit *punit);
  bool (*city_dialog_is_open)(struct city *pcity);

  bool (*request_transport)(struct unit *pcargo, struct tile *ptile);

  void (*update_infra_dialog)();

  void (*gui_load_theme)(QString &directory, QString &theme_name);
  void (*gui_clear_theme)();
  QStringList (*get_gui_specific_themes_directories)(int *count);
  QStringList (*get_useable_themes_in_directory)(QString &directory,
                                                 int *count);
};

struct gui_funcs *get_gui_funcs();
