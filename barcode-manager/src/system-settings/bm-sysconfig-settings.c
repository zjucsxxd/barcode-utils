/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager system settings service
 *
 * Søren Sandmann <sandmann@daimi.au.dk>
 * Dan Williams <dcbw@redhat.com>
 * Tambet Ingo <tambet@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2007 - 2011 Red Hat, Inc.
 * (C) Copyright 2008 Novell, Inc.
 */

#include <unistd.h>
#include <string.h>
#include <gmodule.h>
#include <net/ethernet.h>
#include <netinet/ether.h>

#include <BarcodeManager.h>
#include <bm-connection.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <bm-settings-system-interface.h>

#include <bm-setting-bluetooth.h>
#include <bm-setting-connection.h>
#include <bm-setting-serial.h>

#include "bm-dbus-glib-types.h"
#include "bm-sysconfig-settings.h"
#include "bm-sysconfig-connection.h"
#include "bm-polkit-helpers.h"
#include "bm-system-config-error.h"
#include "bm-logging.h"

#define CONFIG_KEY_NO_AUTO_DEFAULT "no-auto-default"

/* LINKER CRACKROCK */
#define EXPORT(sym) void * __export_##sym = &sym;

#include "bm-inotify-helper.h"
EXPORT(bm_inotify_helper_get_type)
EXPORT(bm_inotify_helper_get)
EXPORT(bm_inotify_helper_add_watch)
EXPORT(bm_inotify_helper_remove_watch)

EXPORT(bm_sysconfig_connection_get_type)
EXPORT(bm_sysconfig_connection_update)
/* END LINKER CRACKROCK */

static void claim_connection (BMSysconfigSettings *self,
                              BMSettingsConnectionInterface *connection,
                              gboolean do_export);

static void impl_settings_save_hostname (BMSysconfigSettings *self,
                                         const char *hostname,
                                         DBusGMethodInvocation *context);

static void impl_settings_get_permissions (BMSysconfigSettings *self,
                                           DBusGMethodInvocation *context);

#include "bm-settings-system-glue.h"

static void unmanaged_specs_changed (NMSystemConfigInterface *config, gpointer user_data);

typedef struct {
	PolkitAuthority *authority;
	guint auth_changed_id;
	char *config_file;

	GSList *pk_calls;
	GSList *permissions_calls;

	GSList *plugins;
	gboolean connections_loaded;
	GHashTable *connections;
	GSList *unmanaged_specs;
} BMSysconfigSettingsPrivate;

static void settings_system_interface_init (BMSettingsSystemInterface *klass);

G_DEFINE_TYPE_WITH_CODE (BMSysconfigSettings, bm_sysconfig_settings, BM_TYPE_SETTINGS_SERVICE,
                         G_IMPLEMENT_INTERFACE (BM_TYPE_SETTINGS_SYSTEM_INTERFACE,
                                                settings_system_interface_init))

#define BM_SYSCONFIG_SETTINGS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_SYSCONFIG_SETTINGS, BMSysconfigSettingsPrivate))

enum {
	PROPERTIES_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
	PROP_0,
	PROP_UNMANAGED_SPECS,

	LAST_PROP
};

static void
load_connections (BMSysconfigSettings *self)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	if (priv->connections_loaded)
		return;

	for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
		NMSystemConfigInterface *plugin = BM_SYSTEM_CONFIG_INTERFACE (iter->data);
		GSList *plugin_connections;
		GSList *elt;

		plugin_connections = bm_system_config_interface_get_connections (plugin);

		// FIXME: ensure connections from plugins loaded with a lower priority
		// get rejected when they conflict with connections from a higher
		// priority plugin.

		for (elt = plugin_connections; elt; elt = g_slist_next (elt))
			claim_connection (self, BM_SETTINGS_CONNECTION_INTERFACE (elt->data), TRUE);

		g_slist_free (plugin_connections);
	}

	priv->connections_loaded = TRUE;

	/* FIXME: Bad hack */
	unmanaged_specs_changed (NULL, self);
}

static GSList *
list_connections (BMSettingsService *settings)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (settings);
	GHashTableIter iter;
	gpointer key;
	GSList *list = NULL;

	load_connections (BM_SYSCONFIG_SETTINGS (settings));

	g_hash_table_iter_init (&iter, priv->connections);
	while (g_hash_table_iter_next (&iter, &key, NULL))
		list = g_slist_prepend (list, BM_EXPORTED_CONNECTION (key));
	return g_slist_reverse (list);
}

static void
clear_unmanaged_specs (BMSysconfigSettings *self)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	g_slist_foreach (priv->unmanaged_specs, (GFunc) g_free, NULL);
	g_slist_free (priv->unmanaged_specs);
	priv->unmanaged_specs = NULL;
}

static char*
uscore_to_wincaps (const char *uscore)
{
	const char *p;
	GString *str;
	gboolean last_was_uscore;

	last_was_uscore = TRUE;
  
	str = g_string_new (NULL);
	p = uscore;
	while (p && *p) {
		if (*p == '-' || *p == '_')
			last_was_uscore = TRUE;
		else {
			if (last_was_uscore) {
				g_string_append_c (str, g_ascii_toupper (*p));
				last_was_uscore = FALSE;
			} else
				g_string_append_c (str, *p);
		}
		++p;
	}

	return g_string_free (str, FALSE);
}

static void
notify (GObject *object, GParamSpec *pspec)
{
	GValue *value;
	GHashTable *hash;

	value = g_slice_new0 (GValue);
	hash = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);

	g_value_init (value, pspec->value_type);
	g_object_get_property (object, pspec->name, value);
	g_hash_table_insert (hash, uscore_to_wincaps (pspec->name), value);
	g_signal_emit (object, signals[PROPERTIES_CHANGED], 0, hash);
	g_hash_table_destroy (hash);
	g_value_unset (value);
	g_slice_free (GValue, value);
}

const GSList *
bm_sysconfig_settings_get_unmanaged_specs (BMSysconfigSettings *self)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	load_connections (self);
	return priv->unmanaged_specs;
}

static NMSystemConfigInterface *
get_plugin (BMSysconfigSettings *self, guint32 capability)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	g_return_val_if_fail (self != NULL, NULL);

	/* Do any of the plugins support setting the hostname? */
	for (iter = priv->plugins; iter; iter = iter->next) {
		NMSystemConfigInterfaceCapabilities caps = BM_SYSTEM_CONFIG_INTERFACE_CAP_NONE;

		g_object_get (G_OBJECT (iter->data), BM_SYSTEM_CONFIG_INTERFACE_CAPABILITIES, &caps, NULL);
		if (caps & capability)
			return BM_SYSTEM_CONFIG_INTERFACE (iter->data);
	}

	return NULL;
}

/* Returns an allocated string which the caller owns and must eventually free */
char *
bm_sysconfig_settings_get_hostname (BMSysconfigSettings *self)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GSList *iter;
	char *hostname = NULL;

	/* Hostname returned is the hostname returned from the first plugin
	 * that provides one.
	 */
	for (iter = priv->plugins; iter; iter = iter->next) {
		NMSystemConfigInterfaceCapabilities caps = BM_SYSTEM_CONFIG_INTERFACE_CAP_NONE;

		g_object_get (G_OBJECT (iter->data), BM_SYSTEM_CONFIG_INTERFACE_CAPABILITIES, &caps, NULL);
		if (caps & BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_HOSTNAME) {
			g_object_get (G_OBJECT (iter->data), BM_SYSTEM_CONFIG_INTERFACE_HOSTNAME, &hostname, NULL);
			if (hostname && strlen (hostname))
				return hostname;
			g_free (hostname);
		}
	}

	return NULL;
}

static void
plugin_connection_added (NMSystemConfigInterface *config,
                         BMSettingsConnectionInterface *connection,
                         gpointer user_data)
{
	claim_connection (BM_SYSCONFIG_SETTINGS (user_data), connection, TRUE);
}

static gboolean
find_unmanaged_device (BMSysconfigSettings *self, const char *needle)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	for (iter = priv->unmanaged_specs; iter; iter = g_slist_next (iter)) {
		if (!strcmp ((const char *) iter->data, needle))
			return TRUE;
	}
	return FALSE;
}

static void
unmanaged_specs_changed (NMSystemConfigInterface *config,
                         gpointer user_data)
{
	BMSysconfigSettings *self = BM_SYSCONFIG_SETTINGS (user_data);
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	clear_unmanaged_specs (self);

	/* Ask all the plugins for their unmanaged specs */
	for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
		GSList *specs, *specs_iter;

		specs = bm_system_config_interface_get_unmanaged_specs (BM_SYSTEM_CONFIG_INTERFACE (iter->data));
		for (specs_iter = specs; specs_iter; specs_iter = specs_iter->next) {
			if (!find_unmanaged_device (self, (const char *) specs_iter->data)) {
				priv->unmanaged_specs = g_slist_prepend (priv->unmanaged_specs, specs_iter->data);
			} else
				g_free (specs_iter->data);
		}

		g_slist_free (specs);
	}

	g_object_notify (G_OBJECT (self), BM_SYSCONFIG_SETTINGS_UNMANAGED_SPECS);
}

static void
hostname_changed (NMSystemConfigInterface *config,
                  GParamSpec *pspec,
                  gpointer user_data)
{
	g_object_notify (G_OBJECT (user_data), BM_SETTINGS_SYSTEM_INTERFACE_HOSTNAME);
}

static void
add_plugin (BMSysconfigSettings *self, NMSystemConfigInterface *plugin)
{
	BMSysconfigSettingsPrivate *priv;
	char *pname = NULL;
	char *pinfo = NULL;

	g_return_if_fail (BM_IS_SYSCONFIG_SETTINGS (self));
	g_return_if_fail (BM_IS_SYSTEM_CONFIG_INTERFACE (plugin));

	priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	priv->plugins = g_slist_append (priv->plugins, g_object_ref (plugin));

	g_signal_connect (plugin, BM_SYSTEM_CONFIG_INTERFACE_CONNECTION_ADDED,
	                  G_CALLBACK (plugin_connection_added), self);
	g_signal_connect (plugin, "notify::hostname", G_CALLBACK (hostname_changed), self);

	bm_system_config_interface_init (plugin, NULL);

	g_object_get (G_OBJECT (plugin),
	              BM_SYSTEM_CONFIG_INTERFACE_NAME, &pname,
	              BM_SYSTEM_CONFIG_INTERFACE_INFO, &pinfo,
	              NULL);

	g_signal_connect (plugin, BM_SYSTEM_CONFIG_INTERFACE_UNMANAGED_SPECS_CHANGED,
	                  G_CALLBACK (unmanaged_specs_changed), self);

	bm_log_info (LOGD_SYS_SET, "Loaded plugin %s: %s", pname, pinfo);
	g_free (pname);
	g_free (pinfo);
}

static GObject *
find_plugin (GSList *list, const char *pname)
{
	GSList *iter;
	GObject *obj = NULL;

	g_return_val_if_fail (pname != NULL, FALSE);

	for (iter = list; iter && !obj; iter = g_slist_next (iter)) {
		NMSystemConfigInterface *plugin = BM_SYSTEM_CONFIG_INTERFACE (iter->data);
		char *list_pname = NULL;

		g_object_get (G_OBJECT (plugin),
		              BM_SYSTEM_CONFIG_INTERFACE_NAME,
		              &list_pname,
		              NULL);
		if (list_pname && !strcmp (pname, list_pname))
			obj = G_OBJECT (plugin);

		g_free (list_pname);
	}

	return obj;
}

static gboolean
load_plugins (BMSysconfigSettings *self, const char *plugins, GError **error)
{
	GSList *list = NULL;
	char **plist;
	char **iter;
	gboolean success = TRUE;

	plist = g_strsplit (plugins, ",", 0);
	if (!plist)
		return FALSE;

	for (iter = plist; *iter; iter++) {
		GModule *plugin;
		char *full_name, *path;
		const char *pname = g_strstrip (*iter);
		GObject *obj;
		GObject * (*factory_func) (void);

		/* ifcfg-fedora was renamed ifcfg-rh; handle old configs here */
		if (!strcmp (pname, "ifcfg-fedora"))
			pname = "ifcfg-rh";

		obj = find_plugin (list, pname);
		if (obj)
			continue;

		full_name = g_strdup_printf ("bm-settings-plugin-%s", pname);
		path = g_module_build_path (PLUGINDIR, full_name);

		plugin = g_module_open (path, G_MODULE_BIND_LOCAL);
		if (!plugin) {
			g_set_error (error, 0, 0,
			             "Could not load plugin '%s': %s",
			             pname, g_module_error ());
			g_free (full_name);
			g_free (path);
			success = FALSE;
			break;
		}

		g_free (full_name);
		g_free (path);

		if (!g_module_symbol (plugin, "bm_system_config_factory", (gpointer) (&factory_func))) {
			g_set_error (error, 0, 0,
			             "Could not find plugin '%s' factory function.",
			             pname);
			success = FALSE;
			break;
		}

		obj = (*factory_func) ();
		if (!obj || !BM_IS_SYSTEM_CONFIG_INTERFACE (obj)) {
			g_set_error (error, 0, 0,
			             "Plugin '%s' returned invalid system config object.",
			             pname);
			success = FALSE;
			break;
		}

		g_module_make_resident (plugin);
		g_object_weak_ref (obj, (GWeakNotify) g_module_close, plugin);
		add_plugin (self, BM_SYSTEM_CONFIG_INTERFACE (obj));
		list = g_slist_append (list, obj);
	}

	g_strfreev (plist);

	g_slist_foreach (list, (GFunc) g_object_unref, NULL);
	g_slist_free (list);

	return success;
}

static void
connection_removed (BMSettingsConnectionInterface *connection,
                    gpointer user_data)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (user_data);
	BMSettingConnection *s_con;
	GKeyFile *timestamps_file;
	const char *connection_uuid;
	char *data;
	gsize len;
	GError *error = NULL;

	/* Remove connection from the table */
	g_hash_table_remove (priv->connections, connection);

	/* Remove timestamp from timestamps database file */
	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (BM_CONNECTION (connection), BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);
	connection_uuid = bm_setting_connection_get_uuid (s_con);

	timestamps_file = g_key_file_new ();
	if (g_key_file_load_from_file (timestamps_file, BM_SYSCONFIG_SETTINGS_TIMESTAMPS_FILE, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
		g_key_file_remove_key (timestamps_file, "timestamps", connection_uuid, NULL);
		data = g_key_file_to_data (timestamps_file, &len, &error);
		if (data) {
			g_file_set_contents (BM_SYSCONFIG_SETTINGS_TIMESTAMPS_FILE, data, len, &error);
			g_free (data);
		}
		if (error) {
			bm_log_warn (LOGD_SYS_SET, "error writing timestamps file '%s': %s", BM_SYSCONFIG_SETTINGS_TIMESTAMPS_FILE, error->message);
			g_error_free (error);
		}
	}
	g_key_file_free (timestamps_file);
}

static void
claim_connection (BMSysconfigSettings *self,
                  BMSettingsConnectionInterface *connection,
                  gboolean do_export)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	BMSettingConnection *s_con;
	const char *connection_uuid;
	guint64 timestamp = 0;
	GKeyFile *timestamps_file;
	GError *err = NULL;
	char *tmp_str;

	g_return_if_fail (BM_IS_SYSCONFIG_SETTINGS (self));
	g_return_if_fail (BM_IS_SETTINGS_CONNECTION_INTERFACE (connection));

	if (g_hash_table_lookup (priv->connections, connection))
		/* A plugin is lying to us. */
		return;

	/* Read timestamp from database file and store it into connection's object data */
	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (BM_CONNECTION (connection), BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);
	connection_uuid = bm_setting_connection_get_uuid (s_con);

	timestamps_file = g_key_file_new ();
	g_key_file_load_from_file (timestamps_file, BM_SYSCONFIG_SETTINGS_TIMESTAMPS_FILE, G_KEY_FILE_KEEP_COMMENTS, NULL);
	tmp_str = g_key_file_get_value (timestamps_file, "timestamps", connection_uuid, &err);
	if (tmp_str) {
		timestamp = g_ascii_strtoull (tmp_str, NULL, 10);
		g_free (tmp_str);
	}

	/* Update connection's timestamp */
	if (!err) {
		guint64 *ts_ptr = g_new (guint64, 1);

		*ts_ptr = timestamp;
		g_object_set_data_full (G_OBJECT (connection), BM_SYSCONFIG_SETTINGS_TIMESTAMP_TAG, ts_ptr, g_free);
	} else {
		bm_log_dbg (LOGD_SYS_SET, "failed to read connection timestamp for '%s': (%d) %s",
		            connection_uuid, err->code, err->message);
		g_clear_error (&err);
	}
	g_key_file_free (timestamps_file);

	g_hash_table_insert (priv->connections, g_object_ref (connection), GINT_TO_POINTER (1));
	g_signal_connect (connection,
	                  BM_SETTINGS_CONNECTION_INTERFACE_REMOVED,
	                  G_CALLBACK (connection_removed),
	                  self);

	if (do_export) {
		bm_settings_service_export_connection (BM_SETTINGS_SERVICE (self), connection);
		g_signal_emit_by_name (self, BM_SETTINGS_INTERFACE_NEW_CONNECTION, connection);
	}
}

static void
remove_connection (BMSysconfigSettings *self,
                   BMSettingsConnectionInterface *connection,
                   gboolean do_signal)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	g_return_if_fail (BM_IS_SYSCONFIG_SETTINGS (self));
	g_return_if_fail (BM_IS_SETTINGS_CONNECTION_INTERFACE (connection));

	if (g_hash_table_lookup (priv->connections, connection)) {
		g_signal_emit_by_name (G_OBJECT (connection), BM_SETTINGS_CONNECTION_INTERFACE_REMOVED);
		g_hash_table_remove (priv->connections, connection);
	}
}

typedef struct {
	BMSysconfigSettings *self;
	DBusGMethodInvocation *context;
	PolkitSubject *subject;
	GCancellable *cancellable;
	gboolean disposed;

	BMConnection *connection;
	BMSettingsAddConnectionFunc callback;
	gpointer callback_data;

	char *hostname;

	BMSettingsSystemPermissions permissions;
	guint32 permissions_calls;
} PolkitCall;

#include "bm-dbus-manager.h"

static PolkitCall *
polkit_call_new (BMSysconfigSettings *self,
                 DBusGMethodInvocation *context,
                 BMConnection *connection,
                 BMSettingsAddConnectionFunc callback,
                 gpointer callback_data,
                 const char *hostname)
{
	PolkitCall *call;
	char *sender;

	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (context != NULL, NULL);

	call = g_malloc0 (sizeof (PolkitCall));
	call->self = self;
	call->cancellable = g_cancellable_new ();
	call->context = context;
	if (connection)
		call->connection = g_object_ref (connection);
	if (callback) {
		call->callback = callback;
		call->callback_data = callback_data;
	}
	if (hostname)
		call->hostname = g_strdup (hostname);

 	sender = dbus_g_method_get_sender (context);
	call->subject = polkit_system_bus_name_new (sender);
	g_free (sender);

	return call;
}

static void
polkit_call_free (PolkitCall *call)
{
	if (call->connection)
		g_object_unref (call->connection);
	g_object_unref (call->cancellable);
	g_free (call->hostname);
	g_object_unref (call->subject);
	g_free (call);
}

static gboolean
add_new_connection (BMSysconfigSettings *self,
                    BMConnection *connection,
                    GError **error)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GError *tmp_error = NULL, *last_error = NULL;
	GSList *iter;
	gboolean success = FALSE;

	/* Here's how it works:
	   1) plugin writes a connection.
	   2) plugin notices that a new connection is available for reading.
	   3) plugin reads the new connection (the one it wrote in 1) and emits 'connection-added' signal.
	   4) BMSysconfigSettings receives the signal and adds it to it's connection list.
	*/

	for (iter = priv->plugins; iter && !success; iter = iter->next) {
		success = bm_system_config_interface_add_connection (BM_SYSTEM_CONFIG_INTERFACE (iter->data),
		                                                     connection,
		                                                     &tmp_error);
		g_clear_error (&last_error);
		if (!success) {
			last_error = tmp_error;
			tmp_error = NULL;
		}
	}

	if (!success)
		*error = last_error;
	return success;
}

static void
pk_add_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
	PolkitCall *call = user_data;
	BMSysconfigSettings *self = call->self;
	BMSysconfigSettingsPrivate *priv;
	PolkitAuthorizationResult *pk_result;
	GError *error = NULL, *add_error = NULL;

	/* If BMSysconfigSettings is already gone, do nothing */
	if (call->disposed) {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_GENERAL,
		                             "Request was canceled.");
		dbus_g_method_return_error (call->context, error);
		g_error_free (error);
		polkit_call_free (call);
		return;
	}

	priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	priv->pk_calls = g_slist_remove (priv->pk_calls, call);

	pk_result = polkit_authority_check_authorization_finish (priv->authority,
	                                                         result,
	                                                         &error);
	/* Some random error happened */
	if (error) {
		call->callback (BM_SETTINGS_INTERFACE (self), error, call->callback_data);
		goto out;
	}

	/* Caller didn't successfully authenticate */
	if (!polkit_authorization_result_get_is_authorized (pk_result)) {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_NOT_PRIVILEGED,
		                             "Insufficient privileges.");
		call->callback (BM_SETTINGS_INTERFACE (self), error, call->callback_data);
		goto out;
	}

	if (add_new_connection (self, call->connection, &add_error))
		call->callback (BM_SETTINGS_INTERFACE (self), NULL, call->callback_data);
	else {
		error = g_error_new (BM_SYSCONFIG_SETTINGS_ERROR,
		                     BM_SYSCONFIG_SETTINGS_ERROR_ADD_FAILED,
		                     "Saving connection failed: (%d) %s",
		                     add_error ? add_error->code : -1,
		                     (add_error && add_error->message) ? add_error->message : "(unknown)");
		g_error_free (add_error);
		call->callback (BM_SETTINGS_INTERFACE (self), error, call->callback_data);
	}

out:
	g_clear_error (&error);
	polkit_call_free (call);
	if (pk_result)
		g_object_unref (pk_result);
}

static void
add_connection (BMSettingsService *service,
	            BMConnection *connection,
	            DBusGMethodInvocation *context, /* Only present for D-Bus calls */
	            BMSettingsAddConnectionFunc callback,
	            gpointer user_data)
{
	BMSysconfigSettings *self = BM_SYSCONFIG_SETTINGS (service);
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	PolkitCall *call;
	GError *error = NULL;

	/* Do any of the plugins support adding? */
	if (!get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_CONNECTIONS)) {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_ADD_NOT_SUPPORTED,
		                             "None of the registered plugins support add.");
		callback (BM_SETTINGS_INTERFACE (service), error, user_data);
		g_error_free (error);
		return;
	}

	call = polkit_call_new (self, context, connection, callback, user_data, NULL);
	g_assert (call);
	polkit_authority_check_authorization (priv->authority,
	                                      call->subject,
	                                      BM_SYSCONFIG_POLICY_ACTION_CONNECTION_MODIFY,
	                                      NULL,
	                                      POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
	                                      call->cancellable,
	                                      pk_add_cb,
	                                      call);
	priv->pk_calls = g_slist_append (priv->pk_calls, call);
}

static void
pk_hostname_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
	PolkitCall *call = user_data;
	BMSysconfigSettings *self = call->self;
	BMSysconfigSettingsPrivate *priv;
	PolkitAuthorizationResult *pk_result;
	GError *error = NULL;
	GSList *iter;
	gboolean success = FALSE;

	/* If our NMSysconfigConnection is already gone, do nothing */
	if (call->disposed) {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_GENERAL,
		                             "Request was canceled.");
		dbus_g_method_return_error (call->context, error);
		g_error_free (error);
		polkit_call_free (call);
		return;
	}

	priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	priv->pk_calls = g_slist_remove (priv->pk_calls, call);

	pk_result = polkit_authority_check_authorization_finish (priv->authority,
	                                                         result,
	                                                         &error);
	/* Some random error happened */
	if (error) {
		dbus_g_method_return_error (call->context, error);
		goto out;
	}

	/* Caller didn't successfully authenticate */
	if (!polkit_authorization_result_get_is_authorized (pk_result)) {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_NOT_PRIVILEGED,
		                             "Insufficient privileges.");
		dbus_g_method_return_error (call->context, error);
		goto out;
	}

	/* Set the hostname in all plugins */
	for (iter = priv->plugins; iter; iter = iter->next) {
		NMSystemConfigInterfaceCapabilities caps = BM_SYSTEM_CONFIG_INTERFACE_CAP_NONE;

		g_object_get (G_OBJECT (iter->data), BM_SYSTEM_CONFIG_INTERFACE_CAPABILITIES, &caps, NULL);
		if (caps & BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_HOSTNAME) {
			g_object_set (G_OBJECT (iter->data), BM_SYSTEM_CONFIG_INTERFACE_HOSTNAME, call->hostname, NULL);
			success = TRUE;
		}
	}

	if (success) {
		dbus_g_method_return (call->context);
	} else {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_SAVE_HOSTNAME_FAILED,
		                             "Saving the hostname failed.");
		dbus_g_method_return_error (call->context, error);
	}

out:
	g_clear_error (&error);
	polkit_call_free (call);
	if (pk_result)
		g_object_unref (pk_result);
}

static void
impl_settings_save_hostname (BMSysconfigSettings *self,
                             const char *hostname,
                             DBusGMethodInvocation *context)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	PolkitCall *call;
	GError *error = NULL;

	/* Do any of the plugins support setting the hostname? */
	if (!get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_HOSTNAME)) {
		error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
		                             BM_SYSCONFIG_SETTINGS_ERROR_SAVE_HOSTNAME_NOT_SUPPORTED,
		                             "None of the registered plugins support setting the hostname.");
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	call = polkit_call_new (self, context, NULL, NULL, NULL, hostname);
	g_assert (call);
	polkit_authority_check_authorization (priv->authority,
	                                      call->subject,
	                                      BM_SYSCONFIG_POLICY_ACTION_HOSTNAME_MODIFY,
	                                      NULL,
	                                      POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
	                                      call->cancellable,
	                                      pk_hostname_cb,
	                                      call);
	priv->pk_calls = g_slist_append (priv->pk_calls, call);
}

static void
pk_authority_changed_cb (GObject *object, gpointer user_data)
{
	/* Let clients know they should re-check their authorization */
	g_signal_emit_by_name (BM_SYSCONFIG_SETTINGS (user_data),
                           BM_SETTINGS_SYSTEM_INTERFACE_CHECK_PERMISSIONS);
}

typedef struct {
	PolkitCall *pk_call;
	const char *pk_action;
	GCancellable *cancellable;
	BMSettingsSystemPermissions permission;
	gboolean disposed;
} PermissionsCall;

static void
permission_call_done (GObject *object, GAsyncResult *result, gpointer user_data)
{
	PermissionsCall *call = user_data;
	PolkitCall *pk_call = call->pk_call;
	BMSysconfigSettings *self = pk_call->self;
	BMSysconfigSettingsPrivate *priv;
	PolkitAuthorizationResult *pk_result;
	GError *error = NULL;

	/* If BMSysconfigSettings is gone, just skip to the end */
	if (call->disposed)
		goto done;

	priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	priv->permissions_calls = g_slist_remove (priv->permissions_calls, call);

	pk_result = polkit_authority_check_authorization_finish (priv->authority,
	                                                         result,
	                                                         &error);
	/* Some random error happened */
	if (error) {
		bm_log_err (LOGD_SYS_SET, "error checking '%s' permission: (%d) %s",
		            call->pk_action,
		            error ? error->code : -1,
		            error && error->message ? error->message : "(unknown)");
		if (error)
			g_error_free (error);
	} else {
		/* If the caller is authorized, or the caller could authorize via a
		 * challenge, then authorization is possible.  Otherwise, caller is out of
		 * luck.
		 */
		if (   polkit_authorization_result_get_is_authorized (pk_result)
		    || polkit_authorization_result_get_is_challenge (pk_result))
		    pk_call->permissions |= call->permission;
	}

	g_object_unref (pk_result);

done:
	pk_call->permissions_calls--;
	if (pk_call->permissions_calls == 0) {
		if (call->disposed) {
			error = g_error_new_literal (BM_SYSCONFIG_SETTINGS_ERROR,
			                             BM_SYSCONFIG_SETTINGS_ERROR_GENERAL,
			                             "Request was canceled.");
			dbus_g_method_return_error (pk_call->context, error);
			g_error_free (error);
		} else {
			/* All the permissions calls are done, return the full permissions
			 * bitfield back to the user.
			 */
			dbus_g_method_return (pk_call->context, pk_call->permissions);
		}

		polkit_call_free (pk_call);
	}
	memset (call, 0, sizeof (PermissionsCall));
	g_free (call);
}

static void
start_permission_check (BMSysconfigSettings *self,
                        PolkitCall *pk_call,
                        const char *pk_action,
                        BMSettingsSystemPermissions permission)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	PermissionsCall *call;

	g_return_if_fail (pk_call != NULL);
	g_return_if_fail (pk_action != NULL);
	g_return_if_fail (permission != BM_SETTINGS_SYSTEM_PERMISSION_NONE);

	call = g_malloc0 (sizeof (PermissionsCall));
	call->pk_call = pk_call;
	call->pk_action = pk_action;
	call->permission = permission;
	call->cancellable = g_cancellable_new ();

	pk_call->permissions_calls++;

	polkit_authority_check_authorization (priv->authority,
	                                      pk_call->subject,
	                                      pk_action,
	                                      NULL,
	                                      0,
	                                      call->cancellable,
	                                      permission_call_done,
	                                      call);
	priv->permissions_calls = g_slist_append (priv->permissions_calls, call);
}

static void
impl_settings_get_permissions (BMSysconfigSettings *self,
                               DBusGMethodInvocation *context)
{
	PolkitCall *call;

	call = polkit_call_new (self, context, NULL, NULL, NULL, FALSE);
	g_assert (call);

	/* Start checks for the various permissions */

	/* Only check for connection-modify if one of our plugins supports it. */
	if (get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_CONNECTIONS)) {
		start_permission_check (self, call,
		                        BM_SYSCONFIG_POLICY_ACTION_CONNECTION_MODIFY,
		                        BM_SETTINGS_SYSTEM_PERMISSION_CONNECTION_MODIFY);
	}

	/* Only check for hostname-modify if one of our plugins supports it. */
	if (get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_HOSTNAME)) {
		start_permission_check (self, call,
		                        BM_SYSCONFIG_POLICY_ACTION_HOSTNAME_MODIFY,
		                        BM_SETTINGS_SYSTEM_PERMISSION_HOSTNAME_MODIFY);
	}
}

static gboolean
get_permissions (BMSettingsSystemInterface *settings,
                 BMSettingsSystemGetPermissionsFunc callback,
                 gpointer user_data)
{
	BMSysconfigSettings *self = BM_SYSCONFIG_SETTINGS (settings);
	BMSettingsSystemPermissions permissions = BM_SETTINGS_SYSTEM_PERMISSION_NONE;

	/* Local caller (ie, NM) gets full permissions by default because it doesn't
	 * need authorization.  However, permissions are still subject to plugin's
	 * restrictions.  i.e. if no plugins support connection-modify, then even
	 * the local caller won't get that permission.
	 */

	/* Only check for connection-modify if one of our plugins supports it. */
	if (get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_CONNECTIONS))
		permissions |= BM_SETTINGS_SYSTEM_PERMISSION_CONNECTION_MODIFY;

	/* Only check for hostname-modify if one of our plugins supports it. */
	if (get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_HOSTNAME))
		permissions |= BM_SETTINGS_SYSTEM_PERMISSION_HOSTNAME_MODIFY;

	// FIXME: hook these into plugin permissions like the modify permissions */
	permissions |= BM_SETTINGS_SYSTEM_PERMISSION_WIFI_SHARE_OPEN;
	permissions |= BM_SETTINGS_SYSTEM_PERMISSION_WIFI_SHARE_PROTECTED;

	callback (settings, permissions, NULL, user_data);
	return TRUE;
}

static gboolean
have_connection_for_device (BMSysconfigSettings *self, GByteArray *mac)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GHashTableIter iter;
	gpointer key;
	BMSettingConnection *s_con;
	const GByteArray *setting_mac;
	gboolean ret = FALSE;

	g_return_val_if_fail (BM_IS_SYSCONFIG_SETTINGS (self), FALSE);
	g_return_val_if_fail (mac != NULL, FALSE);

	return ret;
}

static void
delete_cb (BMSettingsConnectionInterface *connection, GError *error, gpointer user_data)
{
}

BMSysconfigSettings *
bm_sysconfig_settings_new (const char *config_file,
                           const char *plugins,
                           DBusGConnection *bus,
                           GError **error)
{
	BMSysconfigSettings *self;
	BMSysconfigSettingsPrivate *priv;

	self = g_object_new (BM_TYPE_SYSCONFIG_SETTINGS,
	                     BM_SETTINGS_SERVICE_BUS, bus,
	                     BM_SETTINGS_SERVICE_SCOPE, BM_CONNECTION_SCOPE_SYSTEM,
	                     NULL);
	if (!self)
		return NULL;

	priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	priv->config_file = g_strdup (config_file);

	if (plugins) {
		/* Load the plugins; fail if a plugin is not found. */
		if (!load_plugins (self, plugins, error)) {
			g_object_unref (self);
			return NULL;
		}
		unmanaged_specs_changed (NULL, self);
	}

	return self;
}

/***************************************************************/

static void
dispose (GObject *object)
{
	BMSysconfigSettings *self = BM_SYSCONFIG_SETTINGS (object);
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	if (priv->auth_changed_id) {
		g_signal_handler_disconnect (priv->authority, priv->auth_changed_id);
		priv->auth_changed_id = 0;
	}

	/* Cancel PolicyKit requests */
	for (iter = priv->pk_calls; iter; iter = g_slist_next (iter)) {
		PolkitCall *call = iter->data;

		call->disposed = TRUE;
		g_cancellable_cancel (call->cancellable);
	}
	g_slist_free (priv->pk_calls);
	priv->pk_calls = NULL;

	/* Cancel PolicyKit permissions requests */
	for (iter = priv->permissions_calls; iter; iter = g_slist_next (iter)) {
		PermissionsCall *call = iter->data;

		call->disposed = TRUE;
		g_cancellable_cancel (call->cancellable);
	}
	g_slist_free (priv->permissions_calls);
	priv->permissions_calls = NULL;

	G_OBJECT_CLASS (bm_sysconfig_settings_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	BMSysconfigSettings *self = BM_SYSCONFIG_SETTINGS (object);
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);

	g_hash_table_destroy (priv->connections);

	clear_unmanaged_specs (self);

	g_slist_foreach (priv->plugins, (GFunc) g_object_unref, NULL);
	g_slist_free (priv->plugins);

	g_free (priv->config_file);

	G_OBJECT_CLASS (bm_sysconfig_settings_parent_class)->finalize (object);
}

static void
settings_system_interface_init (BMSettingsSystemInterface *iface)
{
	iface->get_permissions = get_permissions;

	dbus_g_object_type_install_info (G_TYPE_FROM_INTERFACE (iface),
	                                 &dbus_glib_bm_settings_system_object_info);
}

static void
get_property (GObject *object, guint prop_id,
			  GValue *value, GParamSpec *pspec)
{
	BMSysconfigSettings *self = BM_SYSCONFIG_SETTINGS (object);
	const GSList *specs, *iter;
	GSList *copy = NULL;

	switch (prop_id) {
	case PROP_UNMANAGED_SPECS:
		specs = bm_sysconfig_settings_get_unmanaged_specs (self);
		for (iter = specs; iter; iter = g_slist_next (iter))
			copy = g_slist_append (copy, g_strdup (iter->data));
		g_value_take_boxed (value, copy);
		break;
	case BM_SETTINGS_SYSTEM_INTERFACE_PROP_HOSTNAME:
		g_value_take_string (value, bm_sysconfig_settings_get_hostname (self));

		/* Don't ever pass NULL through D-Bus */
		if (!g_value_get_string (value))
			g_value_set_static_string (value, "");
		break;
	case BM_SETTINGS_SYSTEM_INTERFACE_PROP_CAN_MODIFY:
		g_value_set_boolean (value, !!get_plugin (self, BM_SYSTEM_CONFIG_INTERFACE_CAP_MODIFY_CONNECTIONS));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_sysconfig_settings_class_init (BMSysconfigSettingsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	BMSettingsServiceClass *ss_class = BM_SETTINGS_SERVICE_CLASS (class);
	
	g_type_class_add_private (class, sizeof (BMSysconfigSettingsPrivate));

	/* virtual methods */
	object_class->notify = notify;
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;
	ss_class->list_connections = list_connections;
	ss_class->add_connection = add_connection;

	/* properties */
	g_object_class_install_property
		(object_class, PROP_UNMANAGED_SPECS,
		 g_param_spec_boxed (BM_SYSCONFIG_SETTINGS_UNMANAGED_SPECS,
							 "Unamanged device specs",
							 "Unmanaged device specs",
							 DBUS_TYPE_G_LIST_OF_STRING,
							 G_PARAM_READABLE));

	g_object_class_override_property (object_class,
									  BM_SETTINGS_SYSTEM_INTERFACE_PROP_HOSTNAME,
									  BM_SETTINGS_SYSTEM_INTERFACE_HOSTNAME);

	g_object_class_override_property (object_class,
									  BM_SETTINGS_SYSTEM_INTERFACE_PROP_CAN_MODIFY,
									  BM_SETTINGS_SYSTEM_INTERFACE_CAN_MODIFY);

	/* signals */
	signals[PROPERTIES_CHANGED] = 
	                g_signal_new ("properties-changed",
	                              G_OBJECT_CLASS_TYPE (object_class),
	                              G_SIGNAL_RUN_FIRST,
	                              G_STRUCT_OFFSET (BMSysconfigSettingsClass, properties_changed),
	                              NULL, NULL,
	                              g_cclosure_marshal_VOID__BOXED,
	                              G_TYPE_NONE, 1, DBUS_TYPE_G_MAP_OF_VARIANT);

	dbus_g_error_domain_register (BM_SYSCONFIG_SETTINGS_ERROR,
	                              BM_DBUS_IFACE_SETTINGS_SYSTEM,
	                              BM_TYPE_SYSCONFIG_SETTINGS_ERROR);

	dbus_g_error_domain_register (BM_SETTINGS_INTERFACE_ERROR,
	                              BM_DBUS_IFACE_SETTINGS,
	                              BM_TYPE_SETTINGS_INTERFACE_ERROR);

	/* And register all the settings errors with D-Bus */
	dbus_g_error_domain_register (BM_CONNECTION_ERROR, NULL, BM_TYPE_CONNECTION_ERROR);
	dbus_g_error_domain_register (BM_SETTING_BLUETOOTH_ERROR, NULL, BM_TYPE_SETTING_BLUETOOTH_ERROR);
	dbus_g_error_domain_register (BM_SETTING_CONNECTION_ERROR, NULL, BM_TYPE_SETTING_CONNECTION_ERROR);
	dbus_g_error_domain_register (BM_SETTING_SERIAL_ERROR, NULL, BM_TYPE_SETTING_SERIAL_ERROR);
	dbus_g_error_domain_register (BM_SETTING_ERROR, NULL, BM_TYPE_SETTING_ERROR);
}

static void
bm_sysconfig_settings_init (BMSysconfigSettings *self)
{
	BMSysconfigSettingsPrivate *priv = BM_SYSCONFIG_SETTINGS_GET_PRIVATE (self);
	GError *error = NULL;

	priv->connections = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

	priv->authority = polkit_authority_get_sync (NULL, &error);
	if (priv->authority) {
		priv->auth_changed_id = g_signal_connect (priv->authority,
		                                          "changed",
		                                          G_CALLBACK (pk_authority_changed_cb),
		                                          self);
	} else {
		bm_log_warn (LOGD_SYS_SET, "failed to create PolicyKit authority: (%d) %s",
		             error ? error->code : -1,
		             error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
	}
}

void
bm_sysconfig_settings_device_removed (BMSysconfigSettings *self, BMDevice *device)
{
    // BMDefaultWiredConnection *connection;

	bm_log_dbg(LOGD_SYS_SET, "");

	// FIXME remove connection here

}

void
bm_sysconfig_settings_device_added (BMSysconfigSettings *self, BMDevice *device)
{
	bm_log_dbg(LOGD_SYS_SET, "");
}
