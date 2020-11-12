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
#ifndef FC__LOG_H
#define FC__LOG_H

#include <stdarg.h>
#include <stdlib.h>

// Qt
#include <QDebug>
#include <QString>
#include <QtGlobal>

#include "fcintl.h"

constexpr auto LOG_FATAL = QtFatalMsg;
constexpr auto LOG_ERROR = QtCriticalMsg;
constexpr auto LOG_WARN = QtWarningMsg;
constexpr auto LOG_NORMAL = QtInfoMsg;
constexpr auto LOG_VERBOSE = QtDebugMsg;
constexpr auto LOG_DEBUG = QtDebugMsg;

/* If one wants to compare autogames with lots of code changes, the line
 * numbers can cause a lot of noise. In that case set this to a fixed
 * value. */
#define __FC_LINE__ __LINE__

/* Preparation of the log message, i.e. add a backtrace. */
typedef void (*log_pre_callback_fn)(QtMsgType, bool print_from_where,
                                    const char *where, const char *msg);

/* A function type to enable custom output of log messages other than
 * via fputs(stderr).  Eg, to the server console while handling prompts,
 * rfcstyle, client notifications; Eg, to the client window output window?
 */
typedef void (*log_callback_fn)(QtMsgType, const char *, bool file_too);

/* A function type to generate a custom prefix for the log messages, e.g.
 * add the turn and/or time of the log message. */
typedef const char *(*log_prefix_fn)(void);

void log_init(const char *filename, log_callback_fn callback,
              log_prefix_fn prefix, int fatal_assertions);
void log_close(void);
bool log_init(const QString &level_str = QStringLiteral("info"));

log_pre_callback_fn log_set_pre_callback(log_pre_callback_fn precallback);
log_callback_fn log_set_callback(log_callback_fn callback);
log_prefix_fn log_set_prefix(log_prefix_fn prefix);
void log_set_level(QtMsgType level);
QtMsgType log_get_level(void);
#ifdef FREECIV_DEBUG
bool log_do_output_for_level_at_location(QtMsgType level, const char *file,
                                         int line);
#endif

#ifdef FREECIV_DEBUG
#define log_do_output_for_level(level)                                      \
  log_do_output_for_level_at_location(level, __FILE__, __FC_LINE__)
#else
#define log_do_output_for_level(level) (log_get_level() >= level)
#endif /* FREECIV_DEBUG */

/* The log macros */
#define log_base(level, message, ...)                                       \
  do {                                                                      \
    switch (level) {                                                        \
    case QtFatalMsg:                                                        \
      qFatal(message, ##__VA_ARGS__);                                       \
      break;                                                                \
    case QtCriticalMsg:                                                     \
      qCritical(message, ##__VA_ARGS__);                                    \
      break;                                                                \
    case QtWarningMsg:                                                      \
      qWarning(message, ##__VA_ARGS__);                                     \
      break;                                                                \
    case QtInfoMsg:                                                         \
      qInfo(message, ##__VA_ARGS__);                                        \
      break;                                                                \
    case QtDebugMsg:                                                        \
      qDebug(message, ##__VA_ARGS__);                                       \
      break;                                                                \
    }                                                                       \
  } while (false)

#ifdef FREECIV_DEBUG
#define log_debug(message, ...) qDebug(message, ##__VA_ARGS__)
#else
#define log_debug(message, ...) 0
#endif

#ifdef FREECIV_TESTMATIC
#define log_testmatic(message, ...) qCritical(message, ##__VA_ARGS__)
#else
#define log_testmatic(message, ...) 0
#endif

#define log_testmatic_alt(lvl, ...) log_testmatic(__VA_ARGS__)

/* Used by game debug command */
#define log_test qInfo
#define log_packet qDebug
#define log_packet_detailed log_debug
#define LOG_TEST LOG_NORMAL /* needed by citylog_*() functions */

/* Assertions. */
void fc_assert_set_fatal(int fatal_assertions);
bool fc_assert_are_fatal();

/* Like assert(). */
// The lambda below is used to allow returning a value from a multi-line
// macro. We need a macro for line number reporting to work.
#define fc_assert(condition)                                                \
  [&] {                                                                     \
    if (!(condition)) {                                                     \
      qCritical("Assertion %s failed", #condition);                         \
      qCritical().noquote()                                                 \
          /* TRANS: No full stop after the URL, could cause confusion. */   \
          << QString(_("Please report this message at %1")).arg(BUG_URL);   \
      if (fc_assert_are_fatal()) {                                          \
        qFatal("%s", _("Assertion failed"));                                \
      }                                                                     \
      return false;                                                         \
    }                                                                       \
    return true;                                                            \
  }() // Forces the usage of ';' at the end of the call.

/* Like assert() with extra message. */
#define fc_assert_msg(condition, message, ...)                              \
  [&] {                                                                     \
    if (!(condition)) {                                                     \
      qCritical("Assertion %s failed", #condition);                         \
      qCritical(message, ##__VA_ARGS__);                                    \
      qCritical().noquote()                                                 \
          /* TRANS: No full stop after the URL, could cause confusion. */   \
          << QString(_("Please report this message at %1")).arg(BUG_URL);   \
      if (fc_assert_are_fatal()) {                                          \
        qFatal("%s", _("Assertion failed"));                                \
      }                                                                     \
      return false;                                                         \
    }                                                                       \
    return true;                                                            \
  }() // Forces the usage of ';' at the end of the call.

/* Do action on failure. */
#define fc_assert_action(condition, action)                                 \
  if (!fc_assert(condition)) {                                              \
    action;                                                                 \
  }

/* Return on failure. */
#define fc_assert_ret(condition) fc_assert_action(condition, return )
/* Return a value on failure. */
#define fc_assert_ret_val(condition, val)                                   \
  fc_assert_action(condition, return val)
/* Exit on failure. */
#define fc_assert_exit(condition)                                           \
  fc_assert_action(condition, exit(EXIT_FAILURE))

/* Do action on failure with extra message. */
#define fc_assert_action_msg(condition, action, message, ...)               \
  if (!fc_assert_msg(condition, message, ##__VA_ARGS__)) {                  \
    action;                                                                 \
  }
/* Return on failure with extra message. */
#define fc_assert_ret_msg(condition, message, ...)                          \
  fc_assert_action_msg(condition, return, message, ##__VA_ARGS__)
/* Return a value on failure with extra message. */
#define fc_assert_ret_val_msg(condition, val, message, ...)                 \
  fc_assert_action_msg(condition, return val, message, ##__VA_ARGS__)
/* Exit on failure with extra message. */
#define fc_assert_exit_msg(condition, message, ...)                         \
  fc_assert_action(condition, qFatal(message, ##__VA_ARGS__);               \
                   exit(EXIT_FAILURE))

#ifdef __cplusplus
#ifdef FREECIV_CXX11_STATIC_ASSERT
#define FC_STATIC_ASSERT(cond, tag) static_assert(cond, #tag)
#endif /* FREECIV_CXX11_STATIC_ASSERT */
#else  /* __cplusplus */
#ifdef FREECIV_C11_STATIC_ASSERT
#define FC_STATIC_ASSERT(cond, tag) _Static_assert(cond, #tag)
#endif /* FREECIV_C11_STATIC_ASSERT */
#ifdef FREECIV_STATIC_STRLEN
#define FC_STATIC_STRLEN_ASSERT(cond, tag) FC_STATIC_ASSERT(cond, tag)
#else /* FREECIV_STATIC_STRLEN */
#define FC_STATIC_STRLEN_ASSERT(cond, tag)
#endif /* FREECIV_STATIC_STRLEN */
#endif /* __cplusplus */

#ifndef FC_STATIC_ASSERT
/* Static (compile-time) assertion.
 * "tag" is a semi-meaningful C identifier which will appear in the
 * compiler error message if the assertion fails. */
#define FC_STATIC_ASSERT(cond, tag)                                         \
  enum { static_assert_##tag = 1 / (!!(cond)) }
#endif

void log_time(QString msg, bool log = false);

#endif /* FC__LOG_H */
