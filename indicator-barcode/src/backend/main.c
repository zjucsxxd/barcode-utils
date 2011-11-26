/*
 * indicator-barcode - user interface for barman
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * Kalle Valo <kalle.valo@canonical.com>
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

#include "manager.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <glib/gi18n.h>
#include <libindicator/indicator-service.h>
#include <libnotify/notify.h>

#include "marshal.h"
#include "service.h"
#include "barman.h"
#include "dbus-shared-names.h"
#include "service-manager.h"
#include "barcode-menu.h"
#include "log.h"

static GMainLoop *mainloop;

static gboolean debug = FALSE;

static GOptionEntry entries[] = {
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Enable debug mode", NULL },
  { NULL }
};

/*
 * When the service interface starts to shutdown, we should follow it. -
 */
static void service_shutdown(IndicatorService *service, gpointer user_data)
{
  if (mainloop != NULL) {
    g_debug("indicator service shutdown");
    g_main_loop_quit(mainloop);
  }

  return;
}

static void signal_handler(int signal)
{
  g_debug("signal %d received, terminating", signal);

  g_main_loop_quit(mainloop);
}

int main (int argc, char ** argv)
{
  Manager *self;
  struct sigaction sa;
  IndicatorService *service;
  GOptionContext *context;
  GError *error = NULL;

  g_type_init();

  log_init("indicator-barcode-service");

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new("- backend for Barcode Reader Menu");
  g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("option parsing failed: %s\n", error->message);
    return 1;
  }

  if (debug)
    log_set_debug(TRUE);

  /* FIXME: move to Manager */
  if (!notify_init("network-indicator")) {
    g_critical("libnotify initialisation failed");
    return 1;
  }

  self = manager_new();

  service = indicator_service_new_version(INDICATOR_BARCODE_DBUS_NAME,
										  INDICATOR_BARCODE_DBUS_VERSION);

  g_signal_connect(G_OBJECT(service),
		   INDICATOR_SERVICE_SIGNAL_SHUTDOWN,
		   G_CALLBACK(service_shutdown), self);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  /* run the loop */
  mainloop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(mainloop);

  stop_agent(self);
  g_object_unref(self);
  self = NULL;

  notify_uninit();

  return 0;
}
