/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// common
#include "fc_types.h"
// client
#include "tilespec.h"
// gui-qt
#include "canvas.h"
#include "pages_g.h"

class QTcpSocket;

void setup_gui_funcs();

void qtg_ui_init();
void qtg_ui_main();
void qtg_ui_exit();

enum gui_type qtg_get_gui_type();
void qtg_insert_client_build_info(char *outbuf, size_t outlen);
void qtg_adjust_default_options();

void qtg_version_message(const QString &vertext);
void qtg_real_output_window_append(const QString &astring,
                                   const struct text_tag_list *tags,
                                   int conn_id);

bool qtg_is_view_supported(enum ts_type type);
void qtg_tileset_type_set(enum ts_type type);
void qtg_free_intro_radar_sprites();
QPixmap *qtg_load_gfxfile(const char *filename);
QPixmap *qtg_create_sprite(int width, int height, QColor *pcolor);
void qtg_get_sprite_dimensions(QPixmap *sprite, int *width, int *height);
QPixmap *qtg_crop_sprite(QPixmap *source, int x, int y, int width,
                         int height, QPixmap *mask, int mask_offset_x,
                         int mask_offset_y, float scale, bool smooth);
void qtg_free_sprite(QPixmap *s);

QColor *qtg_color_alloc(int r, int g, int b);
void qtg_color_free(QColor *pcolor);

QPixmap *qtg_canvas_create(int width, int height);
void qtg_canvas_free(QPixmap *store);
void qtg_canvas_copy(QPixmap *dest, QPixmap *src, int src_x, int src_y,
                     int dest_x, int dest_y, int width, int height);
void qtg_canvas_put_sprite(QPixmap *pcanvas, int canvas_x, int canvas_y,
                           QPixmap *sprite, int offset_x, int offset_y,
                           int width, int height);
void qtg_canvas_put_sprite_full(QPixmap *pcanvas, int canvas_x, int canvas_y,
                                QPixmap *sprite);
void qtg_canvas_put_sprite_fogged(QPixmap *pcanvas, int canvas_x,
                                  int canvas_y, QPixmap *psprite, bool fog,
                                  int fog_x, int fog_y);
void qtg_canvas_put_sprite_citymode(QPixmap *pcanvas, int canvas_x,
                                    int canvas_y, QPixmap *psprite, bool fog,
                                    int fog_x, int fog_y);
void qtg_canvas_put_rectangle(QPixmap *pcanvas, QColor *pcolor, int canvas_x,
                              int canvas_y, int width, int height);
void qtg_canvas_fill_sprite_area(QPixmap *pcanvas, QPixmap *psprite,
                                 QColor *pcolor, int canvas_x, int canvas_y);
void qtg_canvas_put_line(QPixmap *pcanvas, QColor *pcolor,
                         enum line_type ltype, int start_x, int start_y,
                         int dx, int dy);
void qtg_canvas_put_curved_line(QPixmap *pcanvas, QColor *pcolor,
                                enum line_type ltype, int start_x,
                                int start_y, int dx, int dy);
void qtg_get_text_size(int *width, int *height, enum client_font font,
                       const QString &);
void qtg_canvas_put_text(QPixmap *pcanvas, int canvas_x, int canvas_y,
                         enum client_font font, QColor *pcolor,
                         const QString &text);

void qtg_set_rulesets(int num_rulesets, QStringList rulesets);
void qtg_options_extra_init();
void qtg_add_net_input(QTcpSocket *sock);
void qtg_remove_net_input();
void qtg_real_conn_list_dialog_update(void *unused);
void qtg_close_connection_dialog();
void qtg_add_idle_callback(void(callback)(void *), void *data);
void qtg_sound_bell();

void qtg_real_set_client_page(enum client_pages page);
enum client_pages qtg_get_current_client_page();

void qtg_popup_combat_info(int attacker_unit_id, int defender_unit_id,
                           int attacker_hp, int defender_hp,
                           bool make_att_veteran, bool make_def_veteran);
void qtg_set_unit_icon(int idx, struct unit *punit);
void qtg_set_unit_icons_more_arrow(bool onoff);
void qtg_real_focus_units_changed();
void qtg_gui_update_font(const QString &font_name,
                         const QString &font_value);

void qtg_editgui_refresh();
void qtg_editgui_notify_object_created(int tag, int id);
void qtg_editgui_notify_object_changed(int objtype, int object_id,
                                       bool removal);
void qtg_editgui_popup_properties(const struct tile_list *tiles,
                                  int objtype);
void qtg_editgui_tileset_changed();
void qtg_editgui_popdown_all();

void qtg_update_timeout_label();
void qtg_start_turn();
void qtg_real_city_dialog_popup(struct city *pcity);
void qtg_real_city_dialog_refresh(struct city *pcity);
void qtg_popdown_city_dialog(struct city *pcity);
void qtg_popdown_all_city_dialogs();
bool qtg_handmade_scenario_warning();
void qtg_refresh_unit_city_dialogs(struct unit *punit);
bool qtg_city_dialog_is_open(struct city *pcity);

bool qtg_request_transport(struct unit *pcargo, struct tile *ptile);

void qtg_update_infra_dialog();

void qtg_gui_load_theme(QString &directory, QString &theme_name);
void qtg_gui_clear_theme();
QStringList qtg_get_gui_specific_themes_directories(int *count);
QStringList qtg_get_useable_themes_in_directory(QString &directory,
                                                int *count);
