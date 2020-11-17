/*****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

/*****************************************************************************
  Signals implementation.

  New signal types can be declared with script_signal_create. Each
  signal should have a unique name string.
  All signal declarations are in signals_create, for convenience.

  A signal may have any number of Lua callback functions connected to it
  at any given time.

  A signal emission invokes all associated callbacks in the order they were
  connected:

  * A callback can stop the current signal emission, preventing the callbacks
    connected after it from being invoked.

  * A callback can detach itself from its associated signal.

  Lua callbacks functions are able to do these via their return values.

  All Lua callback functions can return a value. Example:
    return false

  If the value is 'true' the current signal emission will be stopped.
*****************************************************************************/
/* utility */
#include "deprecations.h"

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"

#include "luascript_signal.h"

static struct signal_callback *signal_callback_new(const char *name);
static void signal_callback_destroy(struct signal_callback *pcallback);
static struct signal *signal_new(int nargs, enum api_types *parg_types);
static void signal_destroy(struct signal *psignal);

/*************************************************************************/ /**
   Create a new signal callback.
 *****************************************************************************/
static struct signal_callback *signal_callback_new(const char *name)
{
  auto pcallback = new signal_callback;

  pcallback->name = fc_strdup(name);
  return pcallback;
}

/*************************************************************************/ /**
   Free a signal callback.
 *****************************************************************************/
static void signal_callback_destroy(struct signal_callback *pcallback)
{
  delete[] pcallback->name;
  delete pcallback;
}

/*************************************************************************/ /**
   Create a new signal.
 *****************************************************************************/
static struct signal *signal_new(int nargs, enum api_types *parg_types)
{
  auto psignal = new struct signal;

  psignal->nargs = nargs;
  psignal->arg_types = parg_types;
  psignal->callbacks = new QList<signal_callback *>;
  psignal->depr_msg = nullptr;

  return psignal;
}

/*************************************************************************/ /**
   Free a signal.
 *****************************************************************************/
static void signal_destroy(struct signal *psignal)
{
  if (psignal->arg_types) {
    delete[] psignal->arg_types;
  }
  if (psignal->depr_msg) {
    delete[] psignal->depr_msg;
  }
  while (!psignal->callbacks->isEmpty()) {
    signal_callback_destroy(psignal->callbacks->takeFirst());
  }

  delete psignal->callbacks;
  delete psignal;
}

/*************************************************************************/ /**
   Invoke all the callback functions attached to a given signal.
 *****************************************************************************/
void luascript_signal_emit_valist(struct fc_lua *fcl,
                                  const char *signal_name, va_list args)
{
  struct signal *psignal;

  fc_assert_ret(fcl);
  fc_assert_ret(fcl->signals_hash);

  psignal = fcl->signals_hash->value(signal_name, nullptr);
  if (psignal) {
    for (auto pcallback : qAsConst(*psignal->callbacks)) {
      va_list args_cb;

      va_copy(args_cb, args);
      if (luascript_callback_invoke(fcl, pcallback->name, psignal->nargs,
                                    psignal->arg_types, args_cb)) {
        va_end(args_cb);
        break;
      }
      va_end(args_cb);
    }
  } else {
    luascript_log(fcl, LOG_ERROR,
                  "Signal \"%s\" does not exist, so cannot "
                  "be invoked.",
                  signal_name);
  }
}

/*************************************************************************/ /**
   Invoke all the callback functions attached to a given signal.
 *****************************************************************************/
void luascript_signal_emit(struct fc_lua *fcl, const char *signal_name, ...)
{
  va_list args;

  va_start(args, signal_name);
  luascript_signal_emit_valist(fcl, signal_name, args);
  va_end(args);
}

/*************************************************************************/ /**
   Create a new signal type.
 *****************************************************************************/
static struct signal *luascript_signal_create_valist(struct fc_lua *fcl,
                                                     const char *signal_name,
                                                     int nargs, va_list args)
{
  struct signal *psignal;

  fc_assert_ret_val(fcl, NULL);
  fc_assert_ret_val(fcl->signals_hash, NULL);

  psignal = fcl->signals_hash->value(signal_name, nullptr);
  if (psignal) {
    luascript_log(fcl, LOG_ERROR, "Signal \"%s\" was already created.",
                  signal_name);
    return NULL;
  } else {
    enum api_types *parg_types = new api_types[nargs]();
    int i;
    QString sn = QString(signal_name);
    struct signal *created;

    for (i = 0; i < nargs; i++) {
      *(parg_types + i) = api_types(va_arg(args, int));
    }
    created = signal_new(nargs, parg_types);
    fcl->signals_hash->insert(signal_name, created);
    fcl->signal_names->append(sn);

    return created;
  }
}

/*************************************************************************/ /**
   Create a new signal type.
 *****************************************************************************/
signal_deprecator *luascript_signal_create(struct fc_lua *fcl,
                                           const char *signal_name,
                                           int nargs, ...)
{
  va_list args;
  struct signal *created;

  va_start(args, nargs);
  created = luascript_signal_create_valist(fcl, signal_name, nargs, args);
  va_end(args);

  if (created != NULL) {
    return &(created->depr_msg);
  }

  return NULL;
}

/*************************************************************************/ /**
   Mark signal deprecated.
 *****************************************************************************/
void deprecate_signal(signal_deprecator *deprecator, char *signal_name,
                      char *replacement, char *deprecated_since)
{
  if (deprecator != NULL) {
    char buffer[1024];

    if (deprecated_since != NULL && replacement != NULL) {
      fc_snprintf(
          buffer, sizeof(buffer),
          "Deprecated: lua signal \"%s\", deprecated since \"%s\", used. "
          "Use \"%s\" instead",
          signal_name, deprecated_since, replacement);
    } else if (replacement != NULL) {
      fc_snprintf(buffer, sizeof(buffer),
                  "Deprecated: lua signal \"%s\" used. Use \"%s\" instead",
                  signal_name, replacement);
    } else {
      fc_snprintf(buffer, sizeof(buffer),
                  "Deprecated: lua signal \"%s\" used.", signal_name);
    }

    *deprecator = fc_strdup(buffer);
  }
}

/*************************************************************************/ /**
   Connects a callback function to a certain signal.
 *****************************************************************************/
void luascript_signal_callback(struct fc_lua *fcl, const char *signal_name,
                               const char *callback_name, bool create)
{
  struct signal *psignal;
  struct signal_callback *pcallback_found = NULL;

  fc_assert_ret(fcl != NULL);
  fc_assert_ret(fcl->signals_hash != NULL);

  psignal = fcl->signals_hash->value(signal_name, nullptr);
  if (psignal) {
    /* check for a duplicate callback */
    for (auto pcallback : qAsConst(*psignal->callbacks)) {
      if (!strcmp(pcallback->name, callback_name)) {
        pcallback_found = pcallback;
        break;
      }
    }

    if (psignal->depr_msg != NULL) {
      log_deprecation("%s", psignal->depr_msg);
    }

    if (create) {
      if (pcallback_found) {
        luascript_error(fcl->state,
                        "Signal \"%s\" already has a callback "
                        "called \"%s\".",
                        signal_name, callback_name);
      } else {
        psignal->callbacks->append(signal_callback_new(callback_name));
      }
    } else {
      if (pcallback_found) {
        psignal->callbacks->removeAll(pcallback_found);
      }
    }
  } else {
    luascript_error(fcl->state, "Signal \"%s\" does not exist.",
                    signal_name);
  }
}

/*************************************************************************/ /**
   Returns if a callback function to a certain signal is defined.
 *****************************************************************************/
bool luascript_signal_callback_defined(struct fc_lua *fcl,
                                       const char *signal_name,
                                       const char *callback_name)
{
  struct signal *psignal;

  fc_assert_ret_val(fcl != NULL, FALSE);
  fc_assert_ret_val(fcl->signals_hash != NULL, FALSE);

  psignal = fcl->signals_hash->value(signal_name, nullptr);
  if (psignal) {
    /* check for a duplicate callback */
    for (auto pcallback : qAsConst(*psignal->callbacks)) {
      if (!strcmp(pcallback->name, callback_name)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/*************************************************************************/ /**
   Initialize script signals and callbacks.
 *****************************************************************************/
void luascript_signal_init(struct fc_lua *fcl)
{
  fc_assert_ret(fcl != NULL);

  if (NULL == fcl->signals_hash) {
    fcl->signals_hash = new QHash<QString, struct signal *>;
    fcl->signal_names = new QVector<QString>;
  }
}

/*************************************************************************/ /**
   Free script signals and callbacks.
 *****************************************************************************/
void luascript_signal_free(struct fc_lua *fcl)
{
  if (!fcl || !fcl->signals_hash) return;
  for (auto nissan : fcl->signals_hash->values()) {
    signal_destroy(nissan);
  }
  NFC_FREE(fcl->signals_hash);
  fcl->signals_hash = nullptr;
  NFC_FREE(fcl->signal_names);
  fcl->signal_names = nullptr;
}

/*************************************************************************/ /**
   Return the name of the signal with the given index.
 *****************************************************************************/
const QString &luascript_signal_by_index(struct fc_lua *fcl, int sindex)
{
  fc_assert_ret_val(fcl != NULL, NULL);
  fc_assert_ret_val(fcl->signal_names != NULL, NULL);

  return fcl->signal_names->at(sindex);
}

/*************************************************************************/ /**
   Return the name of the 'index' callback function of the signal with the
   name 'signal_name'.
 *****************************************************************************/
const char *luascript_signal_callback_by_index(struct fc_lua *fcl,
                                               const char *signal_name,
                                               int sindex)
{
  struct signal *psignal;

  fc_assert_ret_val(fcl != NULL, NULL);
  fc_assert_ret_val(fcl->signals_hash != NULL, NULL);

  psignal = fcl->signals_hash->value(signal_name, nullptr);
  if (psignal) {
    struct signal_callback *pcallback = psignal->callbacks->at(sindex);
    if (pcallback) {
      return pcallback->name;
    }
  }
  return NULL;
}
