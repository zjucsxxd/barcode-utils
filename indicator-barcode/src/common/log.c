/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Jakob Flierl <jakob.flierl@gmail.com>
 *
 * Authors:
 * Jakob Flierl <jakob.flierl@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "log.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <syslog.h>

static gchar ident[100];
static GLogLevelFlags enabled_levels;

static void syslog_handler(const gchar *log_domain, GLogLevelFlags log_level,
			   const gchar *message, gpointer user_data)
{
  int priority;

  if (!(enabled_levels & log_level))
    return;

  if (log_level & G_LOG_LEVEL_DEBUG)
    priority = LOG_DEBUG;
  else
    priority = LOG_INFO;

  syslog(priority, "%s", message);
}

static void start_debug()
{
  enabled_levels |= G_LOG_LEVEL_DEBUG;

  g_debug("Starting %s version %s in debug mode", PACKAGE_NAME,
	  PACKAGE_VERSION);
}

static void stop_debug()
{
  g_debug("Stopping debug mode");

  enabled_levels &= ~G_LOG_LEVEL_DEBUG;
}

void log_set_debug(gboolean active)
{
  if (active)
    start_debug();
  else
    stop_debug();
}

void log_init(const gchar *name)
{
  g_strlcpy(ident, name, sizeof(ident));

  enabled_levels = G_LOG_LEVEL_MASK;
  enabled_levels &= ~G_LOG_LEVEL_DEBUG;

  openlog(ident, 0, LOG_USER);

  g_log_set_default_handler(syslog_handler, NULL);
}
