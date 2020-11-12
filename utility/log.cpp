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

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <vector>

// Qt
#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "fcthread.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "deprecations.h"
#include "log.h"

#define MAX_LEN_LOG_LINE 5120

static char *log_filename = NULL;
static log_pre_callback_fn log_pre_callback = nullptr;
static log_callback_fn log_callback = NULL;
static log_prefix_fn log_prefix = NULL;

static fc_mutex logfile_mutex;

#ifdef FREECIV_DEBUG
static const QtMsgType max_level = LOG_DEBUG;
#else
static const QtMsgType max_level = LOG_VERBOSE;
#endif /* FREECIV_DEBUG */

static QtMsgType fc_QtMsgType = LOG_NORMAL;
static int fc_fatal_assertions = -1;

#ifdef FREECIV_DEBUG
struct log_fileinfo {
  char *name;
  QtMsgType level;
  unsigned int min;
  unsigned int max;
};
static std::vector<log_fileinfo> log_files;
#endif /* FREECIV_DEBUG */

static const char *QtMsgType_names[] = {
    "Fatal", "Error", "Warning", "Normal", "Verbose", "Debug", NULL};

/**********************************************************************/ /**
  Parses a log level string as provided by the user on the command line, and
  installs the corresponding Qt log filters. Prints a warning and returns
  false if the log level name isn't known.
 **************************************************************************/
bool log_init(const QString &level_str)
{
  // Create default filter rules to pass to Qt. We do it this way so the user
  // can override our simplistic rules with environment variables.
  if (level_str == QStringLiteral("fatal")) {
    // Level "fatal" cannot be disabled, so we omit it below.
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = false\n"
                                                    "*.warning = false\n"
                                                    "*.info = false\n"
                                                    "*.debug = false\n"));
    return true;
  } else if (level_str == QStringLiteral("critical")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = false\n"
                                                    "*.info = false\n"
                                                    "*.debug = false\n"));
    return true;
  } else if (level_str == QStringLiteral("warning")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = true\n"
                                                    "*.info = false\n"
                                                    "*.debug = false\n"));
    return true;
  } else if (level_str == QStringLiteral("info")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = true\n"
                                                    "*.info = true\n"
                                                    "*.debug = false\n"
                                                    "qt.*.info = false\n"));
    return true;
  } else if (level_str == QStringLiteral("debug")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = true\n"
                                                    "*.info = true\n"
                                                    "*.debug = true\n"
                                                    "qt.*.info = false\n"
                                                    "qt.*.debug = false\n"));
    return true;
  } else {
    // Not a known name
    // TRANS: Do not translate "fatal", "critical", "warning", "info" or
    //        "debug". It's exactly what the user must type.
    qCritical(_("\"%s\" is not a valid log level name (valid names are "
                "fatal/critical/warning/info/debug)"),
              qPrintable(level_str));
    return false;
  }
}

/**********************************************************************/ /**
   Initialise the log module. Either 'filename' or 'callback' may be NULL.
   If both are NULL, print to stderr. If both are non-NULL, both callback,
   and fprintf to file.  Pass -1 for fatal_assertions to don't raise any
   signal on failed assertion.
 **************************************************************************/
void log_init(const char *filename, log_callback_fn callback,
              log_prefix_fn prefix, int fatal_assertions)
{
  if (log_filename) {
    free(log_filename);
    log_filename = NULL;
  }
  if (filename && strlen(filename) > 0) {
    log_filename = fc_strdup(filename);
  } else {
    log_filename = NULL;
  }
  log_callback = callback;
  log_prefix = prefix;
  fc_fatal_assertions = fatal_assertions;
  fc_init_mutex(&logfile_mutex);
  qDebug("log started");
  log_debug("LOG_DEBUG test");
}

/**********************************************************************/ /**
    Deinitialize logging module.
 **************************************************************************/
void log_close(void) { fc_destroy_mutex(&logfile_mutex); }

/**********************************************************************/ /**
   Adjust the log preparation callback function.
 **************************************************************************/
log_pre_callback_fn log_set_pre_callback(log_pre_callback_fn precallback)
{
  log_pre_callback_fn old = log_pre_callback;

  log_pre_callback = precallback;

  return old;
}

/**********************************************************************/ /**
   Adjust the callback function after initial log_init().
 **************************************************************************/
log_callback_fn log_set_callback(log_callback_fn callback)
{
  log_callback_fn old = log_callback;

  log_callback = callback;

  return old;
}

/**********************************************************************/ /**
   Adjust the prefix callback function after initial log_init().
 **************************************************************************/
log_prefix_fn log_set_prefix(log_prefix_fn prefix)
{
  log_prefix_fn old = log_prefix;

  log_prefix = prefix;

  return old;
}

/**********************************************************************/ /**
   Adjust the logging level after initial log_init().
 **************************************************************************/
void log_set_level(QtMsgType level) { fc_QtMsgType = level; }

/**********************************************************************/ /**
   Returns the current log level.
 **************************************************************************/
QtMsgType log_get_level(void) { return fc_QtMsgType; }

#ifdef FREECIV_DEBUG
/**********************************************************************/ /**
   Returns wether we should do an output for this level, in this file,
   at this line.
 **************************************************************************/
bool log_do_output_for_level_at_location(QtMsgType level, const char *file,
                                         int line)
{
  auto name = QFileInfo(file).fileName();
  for (const auto &pfile : log_files) {
    if (pfile.level >= level && name == pfile.name
        && ((0 == pfile.min && 0 == pfile.max)
            || (pfile.min <= line && pfile.max >= line))) {
      return TRUE;
    }
  }
  return (fc_QtMsgType >= level);
}
#endif /* FREECIV_DEBUG */

/**********************************************************************/ /**
   Set what signal the fc_assert* macros should raise on failed assertion
   (-1 to disable).
 **************************************************************************/
void fc_assert_set_fatal(int fatal_assertions)
{
  fc_fatal_assertions = fatal_assertions;
}

/**********************************************************************/ /**
   Checks whether the fc_assert* macros should raise on failed assertion.
 **************************************************************************/
bool fc_assert_are_fatal() { return fc_fatal_assertions >= 0; }

void log_time(QString msg, bool log)
{
  static bool logging;
  if (log) {
    logging = true;
  }
  if (logging) {
    qInfo() << qPrintable(msg);
  }
}
