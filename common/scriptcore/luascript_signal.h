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
// utility
#include "support.h"

#include "luascript_types.h"
struct fc_lua;

typedef char *signal_deprecator;

// Signal callback datastructure.
struct signal_callback {
  char *name; // callback function name
};

// Signal datastructure.
struct signal {
  int nargs;                           // number of arguments to pass
  enum api_types *arg_types;           // argument types
  QList<signal_callback *> *callbacks; // connected callbacks
  char *depr_msg; // deprecation message to show if handler added
};

void luascript_signal_init(struct fc_lua *fcl);
void luascript_signal_free(struct fc_lua *fcl);

void luascript_signal_emit_valist(struct fc_lua *fcl,
                                  const char *signal_name, va_list args);
void luascript_signal_emit(struct fc_lua *fcl, const char *signal_name, ...);
signal_deprecator *luascript_signal_create(struct fc_lua *fcl,
                                           const char *signal_name,
                                           int nargs, ...);
void deprecate_signal(signal_deprecator *deprecator, const char *signal_name,
                      const char *replacement, const char *deprecated_since);
void luascript_signal_callback(struct fc_lua *fcl, const char *signal_name,
                               const char *callback_name, bool create);
bool luascript_signal_callback_defined(struct fc_lua *fcl,
                                       const char *signal_name,
                                       const char *callback_name);

QString luascript_signal_by_index(struct fc_lua *fcl, int sindex);
const char *luascript_signal_callback_by_index(struct fc_lua *fcl,
                                               const char *signal_name,
                                               int sindex);
