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

#include "manager-proxy.h"

#include <gio/gio.h>

#include "dbus-shared-names.h"
#include "indicator-barcode-service-xml.h"

typedef struct _ManagerProxyPrivate ManagerProxyPrivate;

struct _ManagerProxyPrivate
{
  GDBusNodeInfo *node_info;
  GDBusConnection *connection;

  guint owner_id;
  guint registration_id;

  char *icon_name;
  char *accessible_desc;
  Manager *manager;
};

#define MANAGER_PROXY_GET_PRIVATE(o)			\
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MANAGER_PROXY_TYPE, \
				ManagerProxyPrivate))

static void manager_proxy_dispose    (GObject *object);
static void manager_proxy_finalize   (GObject *object);

G_DEFINE_TYPE (ManagerProxy, manager_proxy, G_TYPE_OBJECT);

static void emit_icon_changed(ManagerProxy *self)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);
  GVariant *parameters;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  if (priv->icon_name == NULL)
    return;

  parameters = g_variant_new("(s)", priv->icon_name);

  g_dbus_connection_emit_signal(priv->connection, NULL,
				INDICATOR_BARCODE_BACKEND_MANAGER_PATH,
				INDICATOR_BARCODE_BACKEND_MANAGER_INTERFACE,
				INDICATOR_BARCODE_SIGNAL_ICON_CHANGED,
				parameters,
				&error);

  if (error != NULL) {
    g_warning("Failed to emit " INDICATOR_BARCODE_SIGNAL_ICON_CHANGED
	      "signal: %s", error->message);
    g_error_free(error);
  }
}

void manager_proxy_set_icon(ManagerProxy* self, const gchar *icon_name)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);

  g_return_if_fail(IS_MANAGER_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (priv->icon_name && g_str_equal(icon_name, priv->icon_name))
    return;

  g_free(priv->icon_name);

  priv->icon_name = g_strdup(icon_name);
  if (!priv->icon_name)
    return;

  /* if dbus connection is not ready, postpone sending of signal */
  if (priv->connection == NULL)
    return;

  emit_icon_changed(self);
}

static void emit_accessible_desc_changed(ManagerProxy *self)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);
  GVariant *parameters;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  if (priv->accessible_desc == NULL)
    return;

  parameters = g_variant_new("(s)", priv->accessible_desc);

  g_dbus_connection_emit_signal(priv->connection, NULL,
                                INDICATOR_BARCODE_BACKEND_MANAGER_PATH,
                                INDICATOR_BARCODE_BACKEND_MANAGER_INTERFACE,
                                INDICATOR_BARCODE_SIGNAL_ACCESSIBLE_DESC_CHANGED,
                                parameters,
                                &error);

  if (error != NULL) {
    g_warning("Failed to emit " INDICATOR_BARCODE_SIGNAL_ACCESSIBLE_DESC_CHANGED
              "signal: %s", error->message);
    g_error_free(error);
  }
}

void manager_proxy_set_accessible_desc(ManagerProxy* self, const gchar *accessible_desc)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);

  g_return_if_fail(IS_MANAGER_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (priv->accessible_desc && g_strcmp0(accessible_desc, priv->accessible_desc) == 0)
    return;

  g_free(priv->accessible_desc);

  priv->accessible_desc = g_strdup(accessible_desc);
  if (!priv->accessible_desc)
    return;

  /* if dbus connection is not ready, postpone sending of signal */
  if (priv->connection == NULL)
    return;

  emit_accessible_desc_changed(self);
}

static void method_get_icon(ManagerProxy *self, GVariant *parameters,
			    GDBusMethodInvocation *invocation)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);
  GVariant *result;

  g_return_if_fail(priv != NULL);

  result = g_variant_new("(s)", priv->icon_name);

  g_dbus_method_invocation_return_value(invocation, result);
}

static void method_request_scan(ManagerProxy *self, GVariant *parameters,
				GDBusMethodInvocation *invocation)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  manager_request_scan(priv->manager);

  g_dbus_method_invocation_return_value(invocation, NULL);
}

static void method_set_debug(ManagerProxy *self, GVariant *parameters,
			     GDBusMethodInvocation *invocation)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);
  gint level;

  g_return_if_fail(priv != NULL);

  g_variant_get(parameters, "(i)", &level);

  manager_set_debug_level(priv->manager, level);

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
  ManagerProxy *self = MANAGER_PROXY(user_data);

  if (g_strcmp0 (method_name, "GetIcon") == 0)
    method_get_icon(self, parameters, invocation);
  else if (g_strcmp0 (method_name, "RequestScan") == 0)
    method_request_scan(self, parameters, invocation);
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
  ManagerProxy *self = MANAGER_PROXY(user_data);
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);
  guint id;

  g_debug("%s()", __func__);

  g_return_if_fail(priv->node_info != NULL);

  if (priv->connection != NULL)
    g_object_unref(priv->connection);

  priv->connection = g_object_ref(connection);

  id = g_dbus_connection_register_object(priv->connection,
					 INDICATOR_BARCODE_BACKEND_MANAGER_PATH,
					 priv->node_info->interfaces[0],
					 &interface_vtable,
					 self,
					 NULL,  /* user_data_free_func */
					 NULL); /* GError** */

  /* FIXME: check if priv->registration_id != 0 and act accordingly */
  g_return_if_fail(id != 0);

  priv->registration_id = id;

  emit_icon_changed(self);
  emit_accessible_desc_changed(self);
}

static void on_name_acquired (GDBusConnection *connection, const gchar *name,
			      gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
			 gpointer user_data)
{
}

static void manager_proxy_class_init(ManagerProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (object_class, sizeof(ManagerProxyPrivate));

  object_class->dispose = manager_proxy_dispose;
  object_class->finalize = manager_proxy_finalize;

  g_assert(klass != NULL);
}

static void manager_proxy_init (ManagerProxy *self)
{
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);

  priv->icon_name = g_strdup("");
  priv->manager = NULL;
  priv->accessible_desc = NULL;

  /* FIXME: node info could be created in class_init() */
  priv->node_info = g_dbus_node_info_new_for_xml(indicator_barcode_service_xml,
						 NULL);

  g_return_if_fail(priv->node_info != NULL);

  priv->owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
				  INDICATOR_BARCODE_BACKEND_NAME,
				  G_BUS_NAME_OWNER_FLAGS_NONE,
				  on_bus_acquired,
				  on_name_acquired,
				  on_name_lost,
				  self,
				  NULL);

  g_return_if_fail(priv->owner_id != 0);

  priv->registration_id = 0;

  return;
}

static void manager_proxy_dispose(GObject *object)
{
  ManagerProxy *self = MANAGER_PROXY(object);
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);

  if (priv->node_info != NULL) {
    g_dbus_node_info_unref(priv->node_info);
    priv->node_info = NULL;
  }

  if (priv->manager != NULL) {
    g_object_unref(priv->manager);
    priv->manager = NULL;
  }

  if (priv->registration_id != 0 && priv->connection != NULL) {
    g_dbus_connection_unregister_object(priv->connection,
					priv->registration_id);
    priv->registration_id = 0;
  }

  if (priv->owner_id != 0) {
    g_bus_unown_name(priv->owner_id);
    priv->owner_id = 0;
  }

  if (priv->connection != NULL) {
    g_object_unref(priv->connection);
    priv->connection = NULL;
  }

  G_OBJECT_CLASS(manager_proxy_parent_class)->dispose(object);
}

static void manager_proxy_finalize(GObject *object)
{
  ManagerProxy *self = MANAGER_PROXY(object);
  ManagerProxyPrivate *priv = MANAGER_PROXY_GET_PRIVATE(self);

  g_free(priv->icon_name);
  priv->icon_name = NULL;
  g_free(priv->accessible_desc);
  priv->accessible_desc = NULL;

  G_OBJECT_CLASS(manager_proxy_parent_class)->finalize(object);
}

ManagerProxy *manager_proxy_new(Manager *manager)
{
  ManagerProxy *self;
  ManagerProxyPrivate *priv;

  g_return_val_if_fail(IS_MANAGER(manager), NULL);

  self = g_object_new(MANAGER_PROXY_TYPE, NULL);

  if (self == NULL)
    return NULL;

  priv = MANAGER_PROXY_GET_PRIVATE(self);

  g_return_val_if_fail(priv != NULL, NULL);

  g_object_ref(manager);

  priv->manager = manager;

  return self;
}
