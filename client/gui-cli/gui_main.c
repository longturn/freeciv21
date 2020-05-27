/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdio.h>

/* common */
#include "ai.h"
#include "capstr.h"
#include "fc_interface.h"
#include "game.h"
#include "netintf.h"

/* utility */
#include "dataio.h"
#include "fcbacktrace.h"
#include "fc_cmdline.h"
#include "fciconv.h"
#include "log.h"
#include "rand.h"

/* gui main header */
#include "gui_stub.h"

/* client */
#include "gui_cbsetter.h"
#include "chatline_g.h"
#include "client_main.h"
#include "clinet.h"
#include "editgui_g.h"
#include "helpdata.h"           /* boot_help_texts() */
#include "options.h"
#include "plrdlg_g.h"
#include "tilespec.h"
#include "update_queue.h"

/* client/include */
#include "colors_g.h"

/* client/luascript */
#include "script_client.h"

#include "gui_main.h"

const char *client_string = "gui-stub";

const char * const gui_character_encoding = "UTF-8";
const bool gui_use_transliteration = FALSE;

/****************************************************************************
  Called by the tileset code to set the font size that should be used to
  draw the city names and productions.
****************************************************************************/
void gui_set_city_names_font_sizes(int my_city_names_font_size,
                                   int my_city_productions_font_size)
{
  log_error("Unimplemented set_city_names_font_sizes.");
  /* PORTME. Optional to pay any attention to this. */
}

/**********************************************************************//**
  Do any necessary pre-initialization of the UI, if necessary.
**************************************************************************/
void gui_ui_init(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Convert a text string from the data to the internal encoding when it is
  first read from the network.  Returns FALSE if the destination isn't
  large enough or the source was bad.
**************************************************************************/
static bool get_conv(char *dst, size_t ndst,
		     const char *src, size_t nsrc)
{
  char *out = data_to_internal_string_malloc(src);
  bool ret = TRUE;
  size_t len;

  if (!out) {
    dst[0] = '\0';
    return FALSE;
  }

  len = strlen(out);
  if (ndst > 0 && len >= ndst) {
    ret = FALSE;
    len = ndst - 1;
  }

  memcpy(dst, out, len);
  dst[len] = '\0';
  free(out);

  return ret;
}

/**********************************************************************//**
  Convert a text string from the internal to the data encoding, when it
  is written to the network.
**************************************************************************/
static char *put_conv(const char *src, size_t *length)
{
  char *out = internal_to_data_string_malloc(src);

  if (out) {
    *length = strlen(out);
    return out;
  } else {
    *length = 0;
    return NULL;
  }
}

/**********************************************************************//**
  Set up charsets for the client.
**************************************************************************/
static void charsets_init(void)
{
  dio_set_put_conv_callback(put_conv);
  dio_set_get_conv_callback(get_conv);
}

/**********************************************************************//**
  Return the vision of the player on a tile. Client version of
  ./server/maphand/map_is_known_and_seen().
**************************************************************************/
static bool client_map_is_known_and_seen(const struct tile *ptile,
                                         const struct player *pplayer,
                                         enum vision_layer vlayer)
{
  return dbv_isset(&pplayer->client.tile_vision[vlayer], tile_index(ptile));
}

/***************************************************************
  Initialize client specific functions.
***************************************************************/
static void fc_interface_init_client(void)
{
  struct functions *funcs = fc_interface_funcs();

  funcs->create_extra = NULL;
  funcs->destroy_extra = NULL;
  funcs->player_tile_vision_get = client_map_is_known_and_seen;
  funcs->gui_color_free = color_free;

  /* Keep this function call at the end. It checks if all required functions
     are defined. */
  fc_interface_init();
}

/**********************************************************************//**
  Entry point for whole freeciv client program.
**************************************************************************/
int main(int argc, char **argv)
{
  setup_gui_funcs();


  i_am_client(); /* Tell to libfreeciv that we are client */

  fc_interface_init_client();

  game.client.ruleset_init = FALSE;

  /* Ensure that all AIs are initialized to unused state
   * Not using ai_type_iterate as it would stop at
   * current ai type count, ai_type_get_count(), i.e., 0 */
  for (int aii = 0; aii < FREECIV_AI_MOD_LAST; aii++) {
    struct ai_type *ai = get_ai_type(aii);

    init_ai(ai);
  }

  init_nls();

  registry_module_init();
  init_character_encodings(gui_character_encoding, gui_use_transliteration);

  log_init(logfile, LOG_NORMAL, NULL, NULL, -1);
  backtrace_init();

  game.all_connections = conn_list_new();
  game.est_connections = conn_list_new();

  ui_init();
  charsets_init();
  fc_init_network();
  update_queue_init();

  fc_init_ow_mutex();

  init_our_capability();

  init_player_dlg_common();

  options_init();
  options_load();

  script_client_init();

  /* This seed is not saved anywhere; randoms in the client should
     have cosmetic effects only (eg city name suggestions).  --dwp */
  fc_srand(time(NULL));
  helpdata_init();
  boot_help_texts();

  gui_options.sound_enable_effects = FALSE;

  tileset = tileset_new();

  gui_ui_main(argc, argv);

  /* termination */
}

/**********************************************************************//**
  Print extra usage information, including one line help on each option,
  to stderr.
**************************************************************************/
static void print_usage(const char *argv0)
{
  /* PORTME */
  /* add client-specific usage information here */
  fc_fprintf(stderr,
             _("This client has no special command line options\n\n"));

  /* TRANS: No full stop after the URL, could cause confusion. */
  fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
}

/**********************************************************************//**
  Parse and enact any client-specific options.
**************************************************************************/
static void parse_options(int argc, char **argv)
{
  int i = 1;
  char *option = NULL;

  while (i < argc) {
    if (is_option("--help", argv[i])) {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    } else if ((option = get_option_malloc("--name", argv, &i, argc, FALSE))) {
      sz_strlcpy(user_name, option);
      free(option);
    } else if ((option = get_option_malloc("--port", argv, &i, argc, FALSE))) {
      if (!str_to_int(option, &server_port)) {
        fc_fprintf(stderr,
                   _("Invalid port \"%s\" specified with --port option.\n"),
                   option);
        fc_fprintf(stderr, _("Try using --help.\n"));
        exit(EXIT_FAILURE);
      }
      free(option);
    } else if ((option = get_option_malloc("--server", argv, &i, argc, FALSE))) {
      sz_strlcpy(server_host, option);
      free(option);
    } else if ((option = get_option_malloc("--dump", argv, &i, argc, FALSE))) {
      pdump_file = fopen(option, "w");
      free(option);
    } else if ((option = get_option_malloc("--Word", argv, &i, argc, FALSE))) {
      sz_strlcpy(password, option);
      free(option);
    } else {
      fc_fprintf(stderr, _("Unrecognized option: \"%s\"\n"), argv[i]);
      exit(EXIT_FAILURE);
    }

    i++;
  }
}

/**********************************************************************//**
  The main loop for the UI.  This is called from main(), and when it
  exits the client will exit.
**************************************************************************/
void gui_ui_main(int argc, char *argv[])
{
  char errbuf[512];

  parse_options(argc, argv);

  /* Main loop here */
  if (connect_to_server(user_name, server_host, server_port,
                        errbuf, sizeof(errbuf)) == -1) {
    log_error("Connection failed: %s", errbuf);
    return;
  }


  while (client.conn.used) {
    input_from_server(client.conn.sock);
  }

  start_quitting();
}

/**********************************************************************//**
  Extra initializers for client options.
**************************************************************************/
void gui_options_extra_init(void)
{
  /* Nothing to do. */
}

/**********************************************************************//**
  Do any necessary UI-specific cleanup
**************************************************************************/
void gui_ui_exit()
{
  /* PORTME */
}

/**********************************************************************//**
  Return our GUI type
**************************************************************************/
enum gui_type gui_get_gui_type(void)
{
  return GUI_STUB;
}

/**********************************************************************//**
  Update the connected users list at pregame state.
**************************************************************************/
void gui_real_conn_list_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Make a bell noise (beep).  This provides low-level sound alerts even
  if there is no real sound support.
**************************************************************************/
void gui_sound_bell(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Wait for data on the given socket.  Call input_from_server() when data
  is ready to be read.

  This function is called after the client succesfully has connected
  to the server.
**************************************************************************/
void gui_add_net_input(int sock)
{
  /* PORTME */
}

/**********************************************************************//**
  Stop waiting for any server network data.  See add_net_input().

  This function is called if the client disconnects from the server.
**************************************************************************/
void gui_remove_net_input(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Set one of the unit icons (specified by idx) in the information area
  based on punit.

  punit is the unit the information should be taken from. Use NULL to
  clear the icon.

  idx specified which icon should be modified. Use idx == -1 to indicate
  the icon for the active unit. Or idx in [0..num_units_below - 1] for
  secondary (inactive) units on the same tile.
**************************************************************************/
void gui_set_unit_icon(int idx, struct unit *punit)
{
  /* PORTME */
}

/**********************************************************************//**
  Most clients use an arrow (e.g., sprites.right_arrow) to indicate when
  the units_below will not fit. This function is called to activate or
  deactivate the arrow.

  Is disabled by default.
**************************************************************************/
void gui_set_unit_icons_more_arrow(bool onoff)
{
  /* PORTME */
}

/**********************************************************************//**
  Called when the set of units in focus (get_units_in_focus()) changes.
  Standard updates like update_unit_info_label() are handled in the platform-
  independent code, so some clients will not need to do anything here.
**************************************************************************/
void gui_real_focus_units_changed(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Enqueue a callback to be called during an idle moment.  The 'callback'
  function should be called sometimes soon, and passed the 'data' pointer
  as its data.
**************************************************************************/
void gui_add_idle_callback(void (callback)(void *), void *data)
{
  /* PORTME */

  /* This is a reasonable fallback if it's not ported. */
  (callback)(data);
}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_tileset_changed(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_refresh(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_popup_properties(const struct tile_list *tiles, int objtype)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_popdown_all(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_notify_object_changed(int objtype, int object_id,
                                       bool removal)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_notify_object_created(int tag, int id)
{}

/**********************************************************************//**
  Updates a gui font style.
**************************************************************************/
void gui_gui_update_font(const char *font_name, const char *font_value)
{
  /* PORTME */
}

/**********************************************************************//**
  Insert build information to help
**************************************************************************/
void gui_insert_client_build_info(char *outbuf, size_t outlen)
{
  /* PORTME */
}
