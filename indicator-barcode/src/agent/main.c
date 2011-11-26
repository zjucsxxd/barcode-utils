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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "agent.h"
#include "log.h"

static gboolean debug = FALSE;

static GOptionEntry entries[] = {
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Enable debug mode", NULL },
  { NULL }
};

int main(int argc, char **argv)
{
  GOptionContext *context;
  GError *error = NULL;
  BarcodeAgent *agent;

  g_debug("%s()", __func__);

  g_type_init();

  gtk_init(&argc, &argv);

  context = g_option_context_new("- UI for Barcode Reader Menu");
  g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("option parsing failed: %s\n", error->message);
    return 1;
  }

  if (debug)
    log_set_debug(TRUE);

  log_init("indicator-barcode-agent");

  agent = g_object_new(NETWORK_AGENT_TYPE, NULL);

  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain(GETTEXT_PACKAGE);

  /* run the loop */
  gtk_main();

  g_object_unref(agent);
  agent = NULL;

  return 0;
}
