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

#include "agent.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "passphrase-dialog.h"
#include "connect-error-dialog.h"
#include "wireless-connect-dialog.h"

#include "indicator-barcode-agent-xml.h"

#include "dbus-shared-names.h"
#include "log.h"

typedef struct _BarcodeAgentPrivate BarcodeAgentPrivate;

struct _BarcodeAgentPrivate
{
  PassphraseDialog *passphrase_dialog;
  ConnectErrorDialog *error_dialog;
  WirelessConnectDialog *wireless_connect_dialog;

  GDBusMethodInvocation *ask_pin_invocation;

  GDBusNodeInfo *node_info;
  guint owner_id;
  guint registration_id;
};

#define NETWORK_AGENT_GET_PRIVATE(o)			 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NETWORK_AGENT_TYPE, \
				BarcodeAgentPrivate))

static void network_agent_dispose(GObject *object);
static void network_agent_finalize(GObject *object);

G_DEFINE_TYPE (BarcodeAgent, network_agent, G_TYPE_OBJECT);

static void free_dialogs(BarcodeAgent *self)
{
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  if (priv->passphrase_dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(priv->passphrase_dialog));
    priv->passphrase_dialog = NULL;
  }

  if (priv->error_dialog != NULL) {
    /* g_object_unref(priv->error_dialog); */
    priv->error_dialog = NULL;
  }

  if (priv->wireless_connect_dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(priv->wireless_connect_dialog));
    priv->wireless_connect_dialog = NULL;
  }
}

static void method_start(BarcodeAgent *self, GVariant *parameters,
			 GDBusMethodInvocation *invocation)
{
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);

  g_debug("%s()", __func__);

  g_return_if_fail(IS_NETWORK_AGENT(self));
  g_return_if_fail(priv != NULL);

  priv->passphrase_dialog = passphrase_dialog_new();
  if (priv->passphrase_dialog == NULL)
	  g_warning("Failed to create passphrase dialog");

  priv->error_dialog = connect_error_dialog_new();
  if (priv->error_dialog == NULL)
	  g_warning("Failed to create connect error dialog");

  priv->wireless_connect_dialog = wireless_connect_dialog_new();
  if (priv->wireless_connect_dialog == NULL)
	  g_warning("Failed to create wireless connect dialog");

  g_dbus_method_invocation_return_value(invocation, NULL);
}

static void method_stop(BarcodeAgent *self, GVariant *parameters,
			GDBusMethodInvocation *invocation)
{
  g_debug("%s()", __func__);

  g_return_if_fail(IS_NETWORK_AGENT(self));

  free_dialogs(self);

  g_dbus_method_invocation_return_value(invocation, NULL);

  /* FIXME: uncomment this */
  /* gtk_main_quit(); */
}

static void method_show_connect_error(BarcodeAgent *self,
				      GVariant *parameters,
				      GDBusMethodInvocation *invocation)
{
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);
  gchar *error_type;

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->error_dialog != NULL);

  g_variant_get(parameters, "s", &error_type);

  g_debug("%s() %s", __func__, error_type);

  connect_error_dialog_show_error(priv->error_dialog, error_type);

  g_dbus_method_invocation_return_value(invocation, NULL);

  g_free(error_type);
}

static void method_show_wireless_connect(BarcodeAgent *self,
					 GVariant *parameters,
					 GDBusMethodInvocation *invocation)
{
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);

  g_debug("%s()", __func__);

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->wireless_connect_dialog != NULL);

  wireless_connect_dialog_show(priv->wireless_connect_dialog);

  g_dbus_method_invocation_return_value(invocation, NULL);
}

static void method_set_debug(BarcodeAgent* self, GVariant *parameters,
			     GDBusMethodInvocation *invocation)
{
  gint32 level;

  g_variant_get(parameters, "i", &level);

  log_set_debug(level > 0);

  g_dbus_method_invocation_return_value(invocation, NULL);
}

static void handle_method_call(GDBusConnection *connection,
			       const gchar *sender,
			       const gchar *object_path,
			       const gchar *interface_name,
			       const gchar *method_name,
			       GVariant *parameters,
			       GDBusMethodInvocation *invocation,
			       gpointer user_data)
{
  BarcodeAgent *self = NETWORK_AGENT(user_data);

  if (g_strcmp0 (method_name, "Start") == 0)
    method_start(self, parameters, invocation);
  else if (g_strcmp0 (method_name, "Stop") == 0)
    method_stop(self, parameters, invocation);
  else if (g_strcmp0 (method_name, "ShowConnectError") == 0)
    method_show_connect_error(self, parameters, invocation);
  else if (g_strcmp0 (method_name, "ShowWirelessConnect") == 0)
    method_show_wireless_connect(self, parameters, invocation);
  else if (g_strcmp0 (method_name, "SetDebug") == 0)
    method_set_debug(self, parameters, invocation);
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL,
};

static void on_bus_acquired(GDBusConnection *connection,
			    const gchar *name,
			    gpointer user_data)
{
  BarcodeAgent *self = NETWORK_AGENT(user_data);
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);
  guint id;

  g_debug("%s()", __func__);

  g_return_if_fail(priv->node_info != NULL);

  id = g_dbus_connection_register_object(connection,
					 INDICATOR_BARCODE_AGENT_OBJECT,
					 priv->node_info->interfaces[0],
					 &interface_vtable,
					 self,
					 NULL,  /* user_data_free_func */
					 NULL); /* GError** */

  g_return_if_fail(id != 0);

  /* FIXME: check if priv->registration_id != 0 and act accordingly */

  priv->registration_id = id;

  /*
   * FIXME: how to unregister priv->registration_id when BarcodeAgent is
   * destroyed? In on_name_lost() or in dispose()?
   */
}

static void on_name_acquired (GDBusConnection *connection, const gchar *name,
			      gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
			 gpointer user_data)
{

}

static void network_agent_class_init(BarcodeAgentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  g_debug("%s()", __func__);

  g_type_class_add_private(object_class, sizeof(BarcodeAgentPrivate));

  object_class->dispose = network_agent_dispose;
  object_class->finalize = network_agent_finalize;

  g_assert(klass != NULL);
}

static void network_agent_init(BarcodeAgent *self)
{
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);

  g_debug("%s()", __func__);

  priv->passphrase_dialog = NULL;
  priv->error_dialog = NULL;
  priv->wireless_connect_dialog = NULL;

  priv->ask_pin_invocation = NULL;

  priv->node_info = g_dbus_node_info_new_for_xml(indicator_barcode_agent_xml,
						 NULL);

  g_return_if_fail(priv->node_info != NULL);

  priv->owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
				  INDICATOR_BARCODE_AGENT_NAME,
				  G_BUS_NAME_OWNER_FLAGS_NONE,
				  on_bus_acquired,
				  on_name_acquired,
				  on_name_lost,
				  self,
				  NULL);

  priv->registration_id = 0;
}

static void network_agent_dispose(GObject *object)
{
  BarcodeAgent *self = NETWORK_AGENT(object);
  BarcodeAgentPrivate *priv = NETWORK_AGENT_GET_PRIVATE(self);

  free_dialogs(self);

  if (priv->node_info != NULL) {
    g_dbus_node_info_unref(priv->node_info);
    priv->node_info = NULL;
  }

  if (priv->owner_id != 0) {
    g_bus_unown_name(priv->owner_id);
    priv->owner_id = 0;
  }

  G_OBJECT_CLASS(network_agent_parent_class)->dispose(object);
}

static void network_agent_finalize(GObject *object)
{
  G_OBJECT_CLASS(network_agent_parent_class)->finalize(object);
}
