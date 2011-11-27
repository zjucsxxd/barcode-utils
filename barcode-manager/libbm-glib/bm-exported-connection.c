/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager system settings service
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2008 Novell, Inc.
 * (C) Copyright 2008 - 2011 Red Hat, Inc.
 */

#include <BarcodeManager.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <bm-setting-connection.h>

#include "bm-exported-connection.h"
#include "bm-settings-interface.h"
#include "bm-settings-connection-interface.h"

static gboolean impl_exported_connection_get_settings (BMExportedConnection *connection,
                                                       GHashTable **settings,
                                                       GError **error);

static void impl_exported_connection_update (BMExportedConnection *connection,
                                             GHashTable *new_settings,
                                             DBusGMethodInvocation *context);

static void impl_exported_connection_delete (BMExportedConnection *connection,
                                             DBusGMethodInvocation *context);

static void impl_exported_connection_get_secrets (BMExportedConnection *connection,
                                                  const gchar *setting_name,
                                                  const gchar **hints,
                                                  gboolean request_new,
                                                  DBusGMethodInvocation *context);

#include "bm-exported-connection-glue.h"

static void settings_connection_interface_init (BMSettingsConnectionInterface *class);

G_DEFINE_TYPE_EXTENDED (BMExportedConnection, bm_exported_connection, BM_TYPE_CONNECTION, 0,
                        G_IMPLEMENT_INTERFACE (BM_TYPE_SETTINGS_CONNECTION_INTERFACE,
                                               settings_connection_interface_init))

#define BM_EXPORTED_CONNECTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                               BM_TYPE_EXPORTED_CONNECTION, \
                                               BMExportedConnectionPrivate))

typedef struct {
	gboolean foo;
} BMExportedConnectionPrivate;


/**************************************************************/

/* Has to be the same as BM_SYSCONFIG_SETTINGS_TIMESTAMP_TAG in bm-sysconfig-settings.h */
#define CONNECTION_TIMESTAMP_TAG "timestamp-tag"

static GHashTable *
real_get_settings (BMExportedConnection *self, GError **error)
{
	BMConnection *no_secrets;
	GHashTable *settings;
	BMSettingConnection *s_con;
	guint64 *timestamp;

	/* Secrets should *never* be returned by the GetSettings method, they
	 * get returned by the GetSecrets method which can be better
	 * protected against leakage of secrets to unprivileged callers.
	 */
	no_secrets = bm_connection_duplicate (BM_CONNECTION (self));
	g_assert (no_secrets);
	bm_connection_clear_secrets (no_secrets);

	/* Timestamp is not updated internally in connection's 'timestamp'
	 * property, because it would force updating the connection and in turn
	 * writing to /etc periodically, which we want to avoid. Rather real
	 * timestamps are kept track of in data associated with GObject.
	 * Here we substitute connection's timestamp with the real one.
	 */
	timestamp = (guint64 *) g_object_get_data (G_OBJECT (self), CONNECTION_TIMESTAMP_TAG);
	if (timestamp) {
		s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (BM_CONNECTION (no_secrets), BM_TYPE_SETTING_CONNECTION));
		g_assert (s_con);
		g_object_set (s_con, BM_SETTING_CONNECTION_TIMESTAMP, *timestamp, NULL);
	}

	settings = bm_connection_to_hash (no_secrets);
	g_assert (settings);
	g_object_unref (no_secrets);

	return settings;
}

/**************************************************************/

static gboolean
check_writable (BMConnection *connection, GError **error)
{
	BMSettingConnection *s_con;

	g_return_val_if_fail (connection != NULL, FALSE);
	g_return_val_if_fail (BM_IS_CONNECTION (connection), FALSE);

	s_con = (BMSettingConnection *) bm_connection_get_setting (connection,
	                                                           BM_TYPE_SETTING_CONNECTION);
	if (!s_con) {
		g_set_error_literal (error,
		                     BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_INVALID_CONNECTION,
		                     "Connection did not have required 'connection' setting");
		return FALSE;
	}

	/* If the connection is read-only, that has to be changed at the source of
	 * the problem (ex a system settings plugin that can't write connections out)
	 * instead of over D-Bus.
	 */
	if (bm_setting_connection_get_read_only (s_con)) {
		g_set_error_literal (error,
		                     BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_READ_ONLY_CONNECTION,
		                     "Connection is read-only");
		return FALSE;
	}

	return TRUE;
}

static gboolean
impl_exported_connection_get_settings (BMExportedConnection *self,
                                       GHashTable **settings,
                                       GError **error)
{
	/* Must always be implemented */
	g_assert (BM_EXPORTED_CONNECTION_GET_CLASS (self)->get_settings);
	*settings = BM_EXPORTED_CONNECTION_GET_CLASS (self)->get_settings (self, error);
	return *settings ? TRUE : FALSE;
}

static gboolean
update (BMSettingsConnectionInterface *connection,
	    BMSettingsConnectionInterfaceUpdateFunc callback,
	    gpointer user_data)
{
	g_object_ref (connection);
	bm_settings_connection_interface_emit_updated (connection);
	callback (connection, NULL, user_data);
	g_object_unref (connection);
	return TRUE;
}

static void
impl_exported_connection_update (BMExportedConnection *self,
                                 GHashTable *new_settings,
                                 DBusGMethodInvocation *context)
{
	BMConnection *tmp;
	GError *error = NULL;

	/* If the connection is read-only, that has to be changed at the source of
	 * the problem (ex a system settings plugin that can't write connections out)
	 * instead of over D-Bus.
	 */
	if (!check_writable (BM_CONNECTION (self), &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	/* Check if the settings are valid first */
	tmp = bm_connection_new_from_hash (new_settings, &error);
	if (!tmp) {
		g_assert (error);
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}
	g_object_unref (tmp);

	if (BM_EXPORTED_CONNECTION_GET_CLASS (self)->update)
		BM_EXPORTED_CONNECTION_GET_CLASS (self)->update (self, new_settings, context);
	else {
		error = g_error_new (BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		                     "%s: %s:%d update() unimplemented", __func__, __FILE__, __LINE__);
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}
}

static gboolean
do_delete (BMSettingsConnectionInterface *connection,
	       BMSettingsConnectionInterfaceDeleteFunc callback,
	       gpointer user_data)
{
	g_object_ref (connection);
	g_signal_emit_by_name (connection, "removed");
	callback (connection, NULL, user_data);
	g_object_unref (connection);
	return TRUE;
}

static void
impl_exported_connection_delete (BMExportedConnection *self,
                                 DBusGMethodInvocation *context)
{
	GError *error = NULL;

	if (!check_writable (BM_CONNECTION (self), &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	if (BM_EXPORTED_CONNECTION_GET_CLASS (self)->delete)
		BM_EXPORTED_CONNECTION_GET_CLASS (self)->delete (self, context);
	else {
		error = g_error_new (BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		                     "%s: %s:%d delete() unimplemented", __func__, __FILE__, __LINE__);
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}
}

static void
impl_exported_connection_get_secrets (BMExportedConnection *self,
                                      const gchar *setting_name,
                                      const gchar **hints,
                                      gboolean request_new,
                                      DBusGMethodInvocation *context)
{
	GError *error = NULL;

	if (BM_EXPORTED_CONNECTION_GET_CLASS (self)->get_secrets)
		BM_EXPORTED_CONNECTION_GET_CLASS (self)->get_secrets (self, setting_name, hints, request_new, context);
	else {
		error = g_error_new (BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		                     "%s: %s:%d get_secrets() unimplemented", __func__, __FILE__, __LINE__);
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}
}

/**************************************************************/

static void
settings_connection_interface_init (BMSettingsConnectionInterface *iface)
{
	iface->update = update;
	iface->delete = do_delete;
}

/**
 * bm_exported_connection_new:
 * @scope: the Connection scope (either user or system)
 *
 * Creates a new object representing the remote connection.
 *
 * Returns: the new exported connection object on success, or %NULL on failure
 **/
BMExportedConnection *
bm_exported_connection_new (BMConnectionScope scope)
{
	g_return_val_if_fail (scope != BM_CONNECTION_SCOPE_UNKNOWN, NULL);

	return (BMExportedConnection *) g_object_new (BM_TYPE_EXPORTED_CONNECTION,
	                                              BM_CONNECTION_SCOPE, scope,
	                                              NULL);
}

static void
bm_exported_connection_init (BMExportedConnection *self)
{
}

static void
bm_exported_connection_class_init (BMExportedConnectionClass *class)
{
	g_type_class_add_private (class, sizeof (BMExportedConnectionPrivate));

	/* Virtual methods */
	class->get_settings = real_get_settings;

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (class),
	                                 &dbus_glib_bm_exported_connection_object_info);
}
