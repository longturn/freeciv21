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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#ifdef AI_MODULES
#include <ltdl.h>
#endif

// utility
#include "support.h"

// common
#include "ai.h"
#include "player.h"

/* server/advisors */
#include "autosettlers.h"

/* ai/classic */
#include "classicai.h"

#include "aiiface.h"

#ifdef AI_MOD_STATIC_THREADED
bool fc_ai_threaded_setup(struct ai_type *ai);
#endif

#ifdef AI_MOD_STATIC_TEX
bool fc_ai_tex_setup(struct ai_type *ai);
#endif

#ifdef AI_MOD_STATIC_STUB
bool fc_ai_stub_setup(struct ai_type *ai);
#endif

static struct ai_type *default_ai = NULL;

#ifdef AI_MODULES
/**
   Return string describing module loading error. Never returns NULL.
 */
static const char *fc_module_error(void)
{
  static char def_err[] = "Unknown error";
  const char *errtxt = lt_dlerror();

  if (errtxt == NULL) {
    return def_err;
  }

  return errtxt;
}

/**
   Load ai module from file.
 */
bool load_ai_module(const char *modname)
{
  struct ai_type *ai = ai_type_alloc();
  bool setup_success;
  lt_dlhandle handle;
  bool (*setup_func)(struct ai_type * ai);
  const char *(*capstr_func)(void);
  const char *capstr;
  char buffer[2048];
  char filename[1024];

  if (ai == NULL) {
    return false;
  }

  init_ai(ai);

  fc_snprintf(filename, sizeof(filename), "fc_ai_%s", modname);
  fc_snprintf(buffer, sizeof(buffer), "%s", filename);
  handle = lt_dlopenext(buffer);
  if (handle == NULL) {
    qCritical(_("Cannot open AI module %s (%s)"), filename,
              fc_module_error());
    return false;
  }

  fc_snprintf(buffer, sizeof(buffer), "%s_capstr", filename);
  capstr_func = lt_dlsym(handle, buffer);
  if (capstr_func == NULL) {
    qCritical(_("Cannot find capstr function from ai module %s (%s)"),
              filename, fc_module_error());
    return false;
  }

  capstr = capstr_func();
  if (strcmp(FC_AI_MOD_CAPSTR, capstr)) {
    qCritical(_("Incompatible ai module %s:"), filename);
    qCritical(_("  Module options:    %s"), capstr);
    qCritical(_("  Supported options: %s"), FC_AI_MOD_CAPSTR);

    return false;
  }

  fc_snprintf(buffer, sizeof(buffer), "%s_setup", filename);
  setup_func = lt_dlsym(handle, buffer);
  if (setup_func == NULL) {
    qCritical(_("Cannot find setup function from ai module %s (%s)"),
              filename, fc_module_error());
    return false;
  }
  setup_success = setup_func(ai);

  if (!setup_success) {
    qCritical(_("Setup of ai module %s failed."), filename);
    return false;
  }

  return TRUE;
}
#endif // AI_MODULES

/**
   Initialize ai stuff
 */
void ai_init()
{
  bool failure = false;
#if !defined(AI_MODULES) || defined(AI_MOD_STATIC_CLASSIC)                  \
    || defined(AI_MOD_STATIC_THREADED) || defined(AI_MOD_STATIC_TEX)        \
    || defined(AI_MOD_STATIC_STUB)
  // First !defined(AI_MODULES) case is for default ai support.
  struct ai_type *ai;
#endif

#ifdef AI_MODULES
  if (lt_dlinit()) {
    failure = TRUE;
  }
  if (!failure) {
#ifdef FREECIV_DEBUG
    /* First search ai modules under directory ai/<module> under
       current directory. This allows us to run freeciv without
       installing it. */
    const char *moduledirs[] = {"classic", "threaded", "tex", "stub", NULL};
    int i;

    for (i = 0; moduledirs[i] != NULL; i++) {
      char buf[2048];

      fc_snprintf(buf, sizeof(buf), "ai/%s", moduledirs[i]);
      lt_dladdsearchdir(buf);
    }
#endif // FREECIV_DEBUG

    // Then search ai modules from their installation directory.
    lt_dladdsearchdir(AI_MODULEDIR);
  }
#endif // AI_MODULES

#ifdef AI_MOD_STATIC_CLASSIC
  ai = ai_type_alloc();
  if (ai != NULL) {
    init_ai(ai);
    if (!fc_ai_classic_setup(ai)) {
      qCritical(_("Failed to setup \"%s\" AI module"), "classic");
      ai_type_dealloc();
    }
  }
#endif // AI_MOD_STATIC_CLASSIC

#ifdef AI_MOD_STATIC_THREADED
  ai = ai_type_alloc();
  if (ai != NULL) {
    init_ai(ai);
    if (!fc_ai_threaded_setup(ai)) {
      qCritical(_("Failed to setup \"%s\" AI module"), "threaded");
      ai_type_dealloc();
    }
  }
#endif // AI_MOD_STATIC_THREADED

#ifdef AI_MOD_STATIC_TEX
  ai = ai_type_alloc();
  if (ai != NULL) {
    init_ai(ai);
    if (!fc_ai_tex_setup(ai)) {
      qCritical(_("Failed to setup \"%s\" AI module"), "tex");
      ai_type_dealloc();
    }
  }
#endif // AI_MOD_STATIC_TEX

#ifdef AI_MOD_STATIC_STUB
  ai = ai_type_alloc();
  if (ai != NULL) {
    init_ai(ai);
    if (!fc_ai_stub_setup(ai)) {
      qCritical(_("Failed to setup \"%s\" AI module"), "stub");
      ai_type_dealloc();
    }
  }
#endif // AI_MOD_STATIC_STUB

  default_ai = ai_type_by_name(AI_MOD_DEFAULT);
#ifdef AI_MODULES
  if (default_ai == NULL) {
    // Wasn't among statically linked. Try to load dynamic module.
    if (!failure && !load_ai_module(AI_MOD_DEFAULT)) {
      failure = TRUE;
    }
    if (!failure) {
      default_ai = ai_type_by_name(AI_MOD_DEFAULT);
    }
  }
#endif // AI_MODULES
  if (default_ai == NULL || failure) {
    qCritical(
        _("Failed to setup default AI module \"%s\", cannot continue."),
        AI_MOD_DEFAULT);
    exit(EXIT_FAILURE);
  }
}

/**
   Call incident function of victim.
 */
void call_incident(enum incident_type type, enum casus_belli_range scope,
                   const struct action *paction, struct player *violator,
                   struct player *victim)
{
  if (scope == CBR_VICTIM_ONLY) {
    CALL_PLR_AI_FUNC(incident, victim, type, scope, paction, victim,
                     violator, victim);
  } else {
    fc_assert(scope == CBR_INTERNATIONAL_OUTRAGE);
    players_iterate(receiver)
    {
      CALL_PLR_AI_FUNC(incident, receiver, type, scope, paction, receiver,
                       violator, victim);
    }
  }
  players_iterate_end;
}

/**
   Call ai refresh() callback for all players.
 */
void call_ai_refresh()
{
  players_iterate(pplayer) { CALL_PLR_AI_FUNC(refresh, pplayer, pplayer); }
  players_iterate_end;
}

/**
   Return name of default ai type.
 */
const char *default_ai_type_name() { return default_ai->name; }
