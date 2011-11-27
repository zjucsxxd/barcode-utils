/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- barcode scanner manager
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
 * Copyright (C) 2011 Jakob Flierl
 */

#include <config.h>

#include <netinet/ether.h>
#include <string.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>

#include "bm-glib-compat.h"
#include "bm-manager.h"
#include "bm-manager-auth.h"
#include "bm-logging.h"
#include "bm-dbus-manager.h"
#include "bm-device-bt.h"
#include "bm-device-hidraw.h"
#include "bm-system.h"
#include "bm-properties-changed-signal.h"
#include "bm-setting-bluetooth.h"
#include "bm-setting-connection.h"
#include "bm-marshal.h"
#include "bm-dbus-glib-types.h"
#include "bm-udev-manager.h"
#include "bm-sysconfig-settings.h"
#include "bm-secrets-provider-interface.h"
#include "bm-settings-interface.h"
#include "bm-settings-system-interface.h"

static gboolean impl_manager_get_devices (BMManager *manager, GPtrArray **devices, GError **err);

static void impl_manager_enable (BMManager *manager,
                                 gboolean enable,
                                 DBusGMethodInvocation *context);

static gboolean impl_manager_set_logging (BMManager *manager,
                                          const char *level,
                                          const char *domains,
                                          GError **error);

#include "bm-manager-glue.h"

static void udev_device_added_cb (BMUdevManager *udev_mgr,
                                  GUdevDevice *device,
                                  BMDeviceCreatorFn creator_fn,
                                  gpointer user_data);

static void udev_device_removed_cb (BMUdevManager *udev_mgr,
                                    GUdevDevice *device,
                                    gpointer user_data);

static void add_device (BMManager *self, BMDevice *device);

static const char *internal_activate_device (BMManager *manager,
                                             BMDevice *device,
                                             BMConnection *connection,
                                             const char *specific_object,
                                             gboolean user_requested,
                                             gboolean assumed,
                                             GError **error);

static GSList * remove_one_device (BMManager *manager,
                                   GSList *list,
                                   BMDevice *device,
                                   gboolean quitting);

static BMDevice *bm_manager_get_device_by_udi (BMManager *manager, const char *udi);

typedef struct {
	char *config_file;
	char *state_file;

	GSList *devices;
	BMState state;

	BMDBusManager *dbus_mgr;
	BMUdevManager *udev_mgr;

	GHashTable *user_connections;
	DBusGProxy *user_proxy;

	GHashTable *system_connections;
	BMSysconfigSettings *sys_settings;
	char *hostname;

	GSList *secrets_calls;

	GSList *pending_activations;

	gboolean sleeping;
	gboolean net_enabled;

    PolkitAuthority *authority;
    guint auth_changed_id;
    GSList *auth_chains;

	gboolean disposed;
} BMManagerPrivate;

#define BM_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_MANAGER, BMManagerPrivate))

G_DEFINE_TYPE(BMManager, bm_manager, G_TYPE_OBJECT)
//G_DEFINE_TYPE_EXTENDED (BMManager, bm_manager, G_TYPE_OBJECT, 0, 0)

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,
	STATE_CHANGED,
	STATE_CHANGE,  /* DEPRECATED */
	PROPERTIES_CHANGED,
	CONNECTIONS_ADDED,
	CONNECTION_ADDED,
	CONNECTION_UPDATED,
	CONNECTION_REMOVED,
	CHECK_PERMISSIONS,
	USER_PERMISSIONS_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
	PROP_0,
	PROP_VERSION,
	PROP_STATE,
	PROP_NETWORKING_ENABLED,
	PROP_WIRELESS_ENABLED,
	PROP_WIRELESS_HARDWARE_ENABLED,
	PROP_WWAN_ENABLED,
	PROP_WWAN_HARDWARE_ENABLED,
	PROP_ACTIVE_CONNECTIONS,

	/* Not exported */
	PROP_HOSTNAME,
	PROP_SLEEPING,

	LAST_PROP
};

typedef enum
{
	BM_MANAGER_ERROR_UNKNOWN_CONNECTION = 0,
	BM_MANAGER_ERROR_UNKNOWN_DEVICE,
	BM_MANAGER_ERROR_UNMANAGED_DEVICE,
	BM_MANAGER_ERROR_INVALID_SERVICE,
	BM_MANAGER_ERROR_SYSTEM_CONNECTION,
	BM_MANAGER_ERROR_PERMISSION_DENIED,
	BM_MANAGER_ERROR_CONNECTION_NOT_ACTIVE,
	BM_MANAGER_ERROR_ALREADY_ASLEEP_OR_AWAKE,
	BM_MANAGER_ERROR_ALREADY_ENABLED_OR_DISABLED,
} BMManagerError;

#define BM_MANAGER_ERROR (bm_manager_error_quark ())
#define BM_TYPE_MANAGER_ERROR (bm_manager_error_get_type ()) 

static GQuark
bm_manager_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("bm-manager-error");
	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

static GType
bm_manager_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			/* Connection was not provided by any known settings service. */
			ENUM_ENTRY (BM_MANAGER_ERROR_UNKNOWN_CONNECTION, "UnknownConnection"),
			/* Unknown device. */
			ENUM_ENTRY (BM_MANAGER_ERROR_UNKNOWN_DEVICE, "UnknownDevice"),
			/* Unmanaged device. */
			ENUM_ENTRY (BM_MANAGER_ERROR_UNMANAGED_DEVICE, "UnmanagedDevice"),
			/* Invalid settings service (not a recognized system or user
			 * settings service name)
			 */
			ENUM_ENTRY (BM_MANAGER_ERROR_INVALID_SERVICE, "InvalidService"),
			/* Connection was superceded by a system connection. */
			ENUM_ENTRY (BM_MANAGER_ERROR_SYSTEM_CONNECTION, "SystemConnection"),
			/* User does not have the permission to activate this connection. */
			ENUM_ENTRY (BM_MANAGER_ERROR_PERMISSION_DENIED, "PermissionDenied"),
			/* The connection was not active. */
			ENUM_ENTRY (BM_MANAGER_ERROR_CONNECTION_NOT_ACTIVE, "ConnectionNotActive"),
			/* The manager is already in the requested sleep state */
			ENUM_ENTRY (BM_MANAGER_ERROR_ALREADY_ASLEEP_OR_AWAKE, "AlreadyAsleepOrAwake"),
			/* The manager is already in the requested enabled/disabled state */
			ENUM_ENTRY (BM_MANAGER_ERROR_ALREADY_ENABLED_OR_DISABLED, "AlreadyEnabledOrDisabled"),
			{ 0, 0, 0 },
		};
		etype = g_enum_register_static ("BMManagerError", values);
	}
	return etype;
}

/* Removes a device from a device list; returns the start of the new device list */
static GSList *
remove_one_device (BMManager *manager,
                   GSList *list,
                   BMDevice *device,
                   gboolean quitting)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);

	if (bm_device_get_managed (device)) {
		/* When quitting, we want to leave up interfaces & connections
		 * that can be taken over again (ie, "assumed") when NM restarts
		 * so that '/etc/init.d/BarcodeManager restart' will not distrupt
		 * networking for interfaces that support connection assumption.
		 * All other devices get unmanaged when NM quits so that their
		 * connections get torn down and the interface is deactivated.
		 */

		if (   !bm_device_interface_can_assume_connections (BM_DEVICE_INTERFACE (device))
		    || (bm_device_get_state (device) != BM_DEVICE_STATE_ACTIVATED)
			   || !quitting) {
			// bm_device_set_managed (device, FALSE, BM_DEVICE_STATE_REASON_REMOVED);
		}
	}

	bm_sysconfig_settings_device_removed (priv->sys_settings, device);
	g_signal_emit (manager, signals[DEVICE_REMOVED], 0, device);
	g_object_unref (device);

	return g_slist_remove (list, device);
}

BMState
bm_manager_get_state (BMManager *manager)
{
	g_return_val_if_fail (BM_IS_MANAGER (manager), BM_STATE_UNKNOWN);

	return BM_MANAGER_GET_PRIVATE (manager)->state;
}

static void
emit_removed (gpointer key, gpointer value, gpointer user_data)
{
	BMManager *manager = BM_MANAGER (user_data);
	BMConnection *connection = BM_CONNECTION (value);

	g_signal_emit (manager, signals[CONNECTION_REMOVED], 0,
	               connection,
	               bm_connection_get_scope (connection));
}

/*******************************************************************/
/* User settings stuff via D-Bus                                   */
/*******************************************************************/

static void
user_proxy_cleanup (BMManager *self, gboolean resync_bt)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);

	if (priv->user_connections) {
		g_hash_table_foreach (priv->user_connections, emit_removed, self);
		g_hash_table_remove_all (priv->user_connections);
	}

	if (priv->user_proxy) {
		g_object_unref (priv->user_proxy);
		priv->user_proxy = NULL;
	}
}

typedef struct GetSettingsInfo {
	BMManager *manager;
	BMConnection *connection;
	DBusGProxy *proxy;
	guint32 *calls;
} GetSettingsInfo;

static void
free_get_settings_info (gpointer data)
{
	GetSettingsInfo *info = (GetSettingsInfo *) data;

	/* If this was the last pending call for a batch of GetSettings calls,
	 * send out the connections-added signal.
	 */
	if (info->calls) {
		(*info->calls)--;
		if (*info->calls == 0) {
			g_slice_free (guint32, (gpointer) info->calls);
			g_signal_emit (info->manager, signals[CONNECTIONS_ADDED], 0, BM_CONNECTION_SCOPE_USER);

			/* Update the Bluetooth connections for all the new connections */
			bluez_manager_resync_devices (info->manager);
		}
	}

	if (info->manager) {
		g_object_unref (info->manager);
		info->manager = NULL;
	}
	if (info->connection) {
		g_object_unref (info->connection);
		info->connection = NULL;
	}
	if (info->proxy) {
		g_object_unref (info->proxy);
		info->proxy = NULL;
	}

	g_slice_free (GetSettingsInfo, data);	
}

static void
user_connection_get_settings_cb  (DBusGProxy *proxy,
                                  DBusGProxyCall *call_id,
                                  gpointer user_data)
{
	GetSettingsInfo *info = (GetSettingsInfo *) user_data;
	GError *err = NULL;
	GHashTable *settings = NULL;
	BMConnection *connection;
	BMManager *manager;

	g_return_if_fail (info != NULL);

	if (!dbus_g_proxy_end_call (proxy, call_id, &err,
	                            DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT, &settings,
	                            G_TYPE_INVALID)) {
		bm_log_info (LOGD_USER_SET, "couldn't retrieve connection settings: %s.",
		             err && err->message ? err->message : "(unknown)");
		g_error_free (err);
		goto out;
	}

	manager = info->manager;
	connection = info->connection;
 	if (connection == NULL) {
		const char *path = dbus_g_proxy_get_path (proxy);
		BMManagerPrivate *priv;
		GError *error = NULL;
		BMConnection *existing = NULL;

		connection = bm_connection_new_from_hash (settings, &error);
		if (connection == NULL) {
			bm_log_warn (LOGD_USER_SET, "invalid connection: '%s' / '%s' invalid: %d",
			             g_type_name (bm_connection_lookup_setting_type_by_quark (error->domain)),
			             error->message, error->code);
			g_error_free (error);
			goto out;
		}

		bm_connection_set_path (connection, path);
		bm_connection_set_scope (connection, BM_CONNECTION_SCOPE_USER);

		/* Add the new connection to the internal hashes only if the same
		 * connection isn't already there.
		 */
		priv = BM_MANAGER_GET_PRIVATE (manager);

		existing = g_hash_table_lookup (priv->user_connections, path);
		if (!existing || !bm_connection_compare (existing, connection, BM_SETTING_COMPARE_FLAG_EXACT)) {
			g_hash_table_insert (priv->user_connections,
			                     g_strdup (path),
			                     connection);
			existing = NULL;

			/* Attach the D-Bus proxy representing the remote BMConnection
			 * to the local BMConnection object to ensure it stays alive to
			 * continue delivering signals.  It'll be destroyed once the
			 * BMConnection is destroyed.
			 */
			g_object_set_data_full (G_OBJECT (connection),
			                        "proxy",
			                        g_object_ref (info->proxy),
									g_object_unref);
		} else
			g_object_unref (connection);

		/* If the connection-added signal is supposed to be batched, don't
		 * emit the single connection-added here.  Also, don't emit the signal
		 * if the connection wasn't actually added to the system or user hashes.
		 */
		if (!info->calls && !existing) {
			g_signal_emit (manager, signals[CONNECTION_ADDED], 0, connection, BM_CONNECTION_SCOPE_USER);
			/* Update the Bluetooth connections for that single new connection */
			bluez_manager_resync_devices (manager);
		}
	} else {
		// FIXME: merge settings? or just replace?
		bm_log_dbg (LOGD_USER_SET, "implement merge settings");
	}

out:
	if (settings)
		g_hash_table_destroy (settings);

	return;
}

static void
user_connection_removed_cb (DBusGProxy *proxy, gpointer user_data)
{
	BMManager *manager = BM_MANAGER (user_data);
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
	BMConnection *connection = NULL;
	const char *path;

	path = dbus_g_proxy_get_path (proxy);
	if (path) {
		connection = g_hash_table_lookup (priv->user_connections, path);
		if (connection)
			remove_connection (manager, connection, priv->user_connections);
	}
}

static void
user_connection_updated_cb (DBusGProxy *proxy,
                            GHashTable *settings,
                            gpointer user_data)
{
	BMManager *manager = BM_MANAGER (user_data);
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
	BMConnection *new_connection;
	BMConnection *old_connection = NULL;
	gboolean valid = FALSE;
	GError *error = NULL;
	const char *path;

	path = dbus_g_proxy_get_path (proxy);
	if (path)
		old_connection = g_hash_table_lookup (priv->user_connections, path);

	g_return_if_fail (old_connection != NULL);

	new_connection = bm_connection_new_from_hash (settings, &error);
	if (!new_connection) {
		/* New connection invalid, remove existing connection */
		bm_log_warn (LOGD_USER_SET, "invalid connection: '%s' / '%s' invalid: %d",
		             g_type_name (bm_connection_lookup_setting_type_by_quark (error->domain)),
		             error->message, error->code);
		g_error_free (error);
		remove_connection (manager, old_connection, priv->user_connections);
		return;
	}
	g_object_unref (new_connection);

	valid = bm_connection_replace_settings (old_connection, settings, NULL);
	if (valid) {
		g_signal_emit (manager, signals[CONNECTION_UPDATED], 0,
		               old_connection,
		               bm_connection_get_scope (old_connection));

		bluez_manager_resync_devices (manager);
	} else {
		remove_connection (manager, old_connection, priv->user_connections);
	}
}

static void
user_internal_new_connection_cb (BMManager *manager,
                                 const char *path,
                                 guint32 *counter)
{
	GetSettingsInfo *info;
	DBusGProxy *con_proxy;
	DBusGConnection *g_connection;
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);

	g_connection = bm_dbus_manager_get_connection (priv->dbus_mgr);
	con_proxy = dbus_g_proxy_new_for_name (g_connection,
	                                       BM_DBUS_SERVICE_USER_SETTINGS,
	                                       path,
	                                       BM_DBUS_IFACE_SETTINGS_CONNECTION);
	if (!con_proxy) {
		bm_log_err (LOGD_USER_SET, "could not init user connection proxy");
		return;
	}

	dbus_g_proxy_add_signal (con_proxy, "Updated",
	                         DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT,
	                         G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (con_proxy, "Updated",
	                             G_CALLBACK (user_connection_updated_cb),
	                             manager,
	                             NULL);

	dbus_g_proxy_add_signal (con_proxy, "Removed", G_TYPE_INVALID, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (con_proxy, "Removed",
	                             G_CALLBACK (user_connection_removed_cb),
	                             manager,
	                             NULL);

	info = g_slice_new0 (GetSettingsInfo);
	info->manager = g_object_ref (manager);
	info->proxy = con_proxy;
	if (counter) {
		info->calls = counter;
		(*info->calls)++;
	}
	dbus_g_proxy_begin_call (con_proxy, "GetSettings",
	                         user_connection_get_settings_cb,
	                         info,
	                         free_get_settings_info,
	                         G_TYPE_INVALID);
}

static void
user_list_connections_cb  (DBusGProxy *proxy,
                           DBusGProxyCall *call_id,
                           gpointer user_data)
{
	BMManager *manager = BM_MANAGER (user_data);
	GError *err = NULL;
	GPtrArray *ops;
	guint32 *counter = NULL;
	int i;

	if (!dbus_g_proxy_end_call (proxy, call_id, &err,
	                            DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH, &ops,
	                            G_TYPE_INVALID)) {
		bm_log_warn (LOGD_USER_SET, "couldn't retrieve connections: %s",
		             err && err->message ? err->message : "(unknown)");
		g_clear_error (&err);
		return;
	}

	/* Keep track of all calls made here; don't want to emit connection-added for
	 * each one, but emit connections-added when they are all done.
	 */
	counter = g_slice_new0 (guint32);
	for (i = 0; i < ops->len; i++) {
		char *op = g_ptr_array_index (ops, i);

		user_internal_new_connection_cb (manager, op, counter);
		g_free (op);
	}
	g_ptr_array_free (ops, TRUE);
}

static void
user_proxy_destroyed_cb (DBusGProxy *proxy, BMManager *self)
{
	bm_log_dbg (LOGD_USER_SET, "Removing user connections...");

	/* At this point the user proxy is already being disposed */
	BM_MANAGER_GET_PRIVATE (self)->user_proxy = NULL;

	/* User Settings service disappeared; throw away user connections */
	user_proxy_cleanup (self, TRUE);
}

static void
user_new_connection_cb (DBusGProxy *proxy, const char *path, gpointer user_data)
{
	user_internal_new_connection_cb (BM_MANAGER (user_data), path, NULL);
}

static void
user_proxy_init (BMManager *self)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
	DBusGConnection *bus;
	GError *error = NULL;

	g_return_if_fail (self != NULL);
	g_return_if_fail (priv->user_proxy == NULL);

	/* Don't try to initialize the user settings proxy if the user
	 * settings service doesn't actually exist.
	 */
	if (!bm_dbus_manager_name_has_owner (priv->dbus_mgr, BM_DBUS_SERVICE_USER_SETTINGS))
		return;

	bus = bm_dbus_manager_get_connection (priv->dbus_mgr);
	priv->user_proxy = dbus_g_proxy_new_for_name_owner (bus,
	                                                    BM_DBUS_SERVICE_USER_SETTINGS,
	                                                    BM_DBUS_PATH_SETTINGS,
	                                                    BM_DBUS_IFACE_SETTINGS,
	                                                    &error);
	if (!priv->user_proxy) {
		bm_log_err (LOGD_USER_SET, "could not init user settings proxy: (%d) %s",
		            error ? error->code : -1,
		            error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
		return;
	}
}

/*******************************************************************/
/* System settings stuff via BMSysconfigSettings                   */
/*******************************************************************/

static void
system_connection_removed_cb (BMSettingsConnectionInterface *connection,
                              BMManager *manager)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
	const char *path;

	path = bm_connection_get_path (BM_CONNECTION (connection));

	connection = g_hash_table_lookup (priv->system_connections, path);
	if (connection)
		remove_connection (manager, BM_CONNECTION (connection), priv->system_connections);
}

static void
system_query_connections (BMManager *manager)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
	GSList *system_connections, *iter;
}

/*******************************************************************/
/* General BMManager stuff                                         */
/*******************************************************************/

static BMDevice *
bm_manager_get_device_by_path (BMManager *manager, const char *path)
{
	GSList *iter;

	for (iter = BM_MANAGER_GET_PRIVATE (manager)->devices; iter; iter = iter->next) {
		if (!strcmp (bm_device_get_path (BM_DEVICE (iter->data)), path))
			return BM_DEVICE (iter->data);
	}
	return NULL;
}

static void
bm_manager_name_owner_changed (BMDBusManager *mgr,
                               const char *name,
                               const char *old,
                               const char *new,
                               gpointer user_data)
{
	BMManager *manager = BM_MANAGER (user_data);
	gboolean old_owner_good = (old && (strlen (old) > 0));
	gboolean new_owner_good = (new && (strlen (new) > 0));

	if (strcmp (name, BM_DBUS_SERVICE_USER_SETTINGS) == 0) {
		if (!old_owner_good && new_owner_good)
			user_proxy_init (manager);
		else
			user_proxy_cleanup (manager, TRUE);
	}
}

/* Store value into key-file; supported types: boolean, int, string */
static gboolean
write_value_to_state_file (const char *filename,
                           const char *group,
                           const char *key,
                           GType value_type,
                           gpointer value,
                           GError **error)
{
	GKeyFile *key_file;
	char *data;
	gsize len = 0;
	gboolean ret = FALSE;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (group != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value_type == G_TYPE_BOOLEAN ||
	                      value_type == G_TYPE_INT ||
	                      value_type == G_TYPE_STRING,
	                      FALSE);

	key_file = g_key_file_new ();
	if (!key_file)
		return FALSE;

	g_key_file_set_list_separator (key_file, ',');
	g_key_file_load_from_file (key_file, filename, G_KEY_FILE_KEEP_COMMENTS, NULL);
	switch (value_type) {
	case G_TYPE_BOOLEAN:
		g_key_file_set_boolean (key_file, group, key, *((gboolean *) value));
		break;
	case G_TYPE_INT:
		g_key_file_set_integer (key_file, group, key, *((gint *) value));
		break;
	case G_TYPE_STRING:
		g_key_file_set_string (key_file, group, key, *((const gchar **) value));
		break;
	}

	data = g_key_file_to_data (key_file, &len, NULL);
	if (data) {
		ret = g_file_set_contents (filename, data, len, error);
		g_free (data);
	}
	g_key_file_free (key_file);

	return ret;
}

static gboolean
bdaddr_matches_connection (BMSettingBluetooth *s_bt, const char *bdaddr)
{
	const GByteArray *arr;
	gboolean ret = FALSE;

	arr = bm_setting_bluetooth_get_bdaddr (s_bt);

	if (   arr != NULL 
	       && arr->len == ETH_ALEN) {
		char *str;

		str = g_strdup_printf ("%02X:%02X:%02X:%02X:%02X:%02X",
				       arr->data[0],
				       arr->data[1],
				       arr->data[2],
				       arr->data[3],
				       arr->data[4],
				       arr->data[5]);
		ret = g_str_equal (str, bdaddr);
		g_free (str);
	}

	return ret;
}

static BMConnection *
bluez_manager_find_connection (BMManager *manager,
                               const char *bdaddr,
                               guint32 capabilities)
{
	BMConnection *found = NULL;
	GSList *connections, *l;

	connections = bm_manager_get_connections (manager, BM_CONNECTION_SCOPE_SYSTEM);
	connections = g_slist_concat (connections, bm_manager_get_connections (manager, BM_CONNECTION_SCOPE_USER));

	for (l = connections; l != NULL; l = l->next) {
		BMConnection *candidate = BM_CONNECTION (l->data);
		BMSettingConnection *s_con;
		BMSettingBluetooth *s_bt;
		const char *con_type;
		const char *bt_type;

		s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (candidate, BM_TYPE_SETTING_CONNECTION));
		g_assert (s_con);
		con_type = bm_setting_connection_get_connection_type (s_con);
		g_assert (con_type);
		if (!g_str_equal (con_type, BM_SETTING_BLUETOOTH_SETTING_NAME))
			continue;

		s_bt = (BMSettingBluetooth *) bm_connection_get_setting (candidate, BM_TYPE_SETTING_BLUETOOTH);
		if (!s_bt)
			continue;

		if (!bdaddr_matches_connection (s_bt, bdaddr))
			continue;

		bt_type = bm_setting_bluetooth_get_connection_type (s_bt);
		if (   g_str_equal (bt_type, BM_SETTING_BLUETOOTH_TYPE_DUN)
		    && !(capabilities & BM_BT_CAPABILITY_DUN))
		    	continue;
		if (   g_str_equal (bt_type, BM_SETTING_BLUETOOTH_TYPE_PANU)
		    && !(capabilities & BM_BT_CAPABILITY_NAP))
		    	continue;

		found = candidate;
		break;
	}

	g_slist_free (connections);
	return found;
}

static gboolean
impl_manager_set_logging (BMManager *manager,
                          const char *level,
                          const char *domains,
                          GError **error)
{
	if (bm_logging_setup (level, domains, error)) {
		char *new_domains = bm_logging_domains_to_string ();

		bm_log_info (LOGD_CORE, "logging: level '%s' domains '%s'",
		             bm_logging_level_to_string (),
		             new_domains);
		g_free (new_domains);
		return TRUE;
	}
	return FALSE;
}

void
bm_manager_start (BMManager *self)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
	guint i;

	/* Log overall networking status - enabled/disabled */
	bm_log_info (LOGD_CORE, "Networking is %s by state file",
	             priv->net_enabled ? "enabled" : "disabled");

	system_query_connections (self);

	/* Get user connections if the user settings service is around, otherwise
	 * they will be queried when the user settings service shows up on the
	 * bus in bm_manager_name_owner_changed().
	 */
	user_proxy_init (self);

	bm_udev_manager_query_devices (priv->udev_mgr);
}

#define PERM_DENIED_ERROR "org.freedesktop.BarcodeManager.PermissionDenied"

static DBusHandlerResult
prop_filter (DBusConnection *connection,
             DBusMessage *message,
             void *user_data)
{
	BMManager *self = BM_MANAGER (user_data);
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
	DBusMessageIter iter;
	DBusMessageIter sub;
	const char *propiface = NULL;
	const char *propname = NULL;
	const char *sender = NULL;
	const char *glib_propname = NULL, *permission = NULL;
	DBusError dbus_error;
	gulong uid = G_MAXULONG;
	DBusMessage *reply = NULL;
	gboolean set_enabled = FALSE;

	/* The sole purpose of this function is to validate property accesses
	 * on the BMManager object since dbus-glib doesn't yet give us this
	 * functionality.
	 */

	if (!dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "Set"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init (message, &iter);

	/* Get the D-Bus interface of the property to set */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_get_basic (&iter, &propiface);
	if (!propiface || strcmp (propiface, BM_DBUS_INTERFACE))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_next (&iter);

	/* Get the property name that's going to be set */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_get_basic (&iter, &propname);
	dbus_message_iter_next (&iter);

	/* Get the new value for the property */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_recurse (&iter, &sub);
	if (dbus_message_iter_get_arg_type (&sub) != DBUS_TYPE_BOOLEAN)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_get_basic (&sub, &set_enabled);

	sender = dbus_message_get_sender (message);
	if (!sender) {
		reply = dbus_message_new_error (message, PERM_DENIED_ERROR,
		                                "Could not determine D-Bus requestor");
		goto out;
	}

	dbus_error_init (&dbus_error);
	uid = dbus_bus_get_unix_user (connection, sender, &dbus_error);
	if (dbus_error_is_set (&dbus_error)) {
		reply = dbus_message_new_error (message, PERM_DENIED_ERROR,
		                                "Could not determine the user ID of the requestor");
		dbus_error_free (&dbus_error);
		goto out;
	}

	g_object_set (self, glib_propname, set_enabled, NULL);
	reply = dbus_message_new_method_return (message);
	
out:
	if (reply) {
		dbus_connection_send (connection, reply, NULL);
		dbus_message_unref (reply);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

BMManager *
bm_manager_get (const char *config_file,
                const char *plugins,
                const char *state_file,
                GError **error)
{
	static BMManager *singleton = NULL;
	BMManagerPrivate *priv;
	DBusGConnection *bus;
	DBusConnection *dbus_connection;

	if (singleton)
		return g_object_ref (singleton);

	singleton = (BMManager *) g_object_new (BM_TYPE_MANAGER, NULL);
	g_assert (singleton);

	priv = BM_MANAGER_GET_PRIVATE (singleton);

	bus = bm_dbus_manager_get_connection (priv->dbus_mgr);
	g_assert (bus);
	dbus_connection = dbus_g_connection_get_connection (bus);
	g_assert (dbus_connection);

	if (!dbus_connection_add_filter (dbus_connection, prop_filter, singleton, NULL)) {
		bm_log_err (LOGD_CORE, "failed to register DBus connection filter");
		g_object_unref (singleton);
		return NULL;
    }

	priv->sys_settings = bm_sysconfig_settings_new (config_file, plugins, bus, error);
	if (!priv->sys_settings) {
		g_object_unref (singleton);
		return NULL;
	}
	bm_settings_service_export (BM_SETTINGS_SERVICE (priv->sys_settings));

	priv->config_file = g_strdup (config_file);

	priv->state_file = g_strdup (state_file);

	dbus_g_connection_register_g_object (bm_dbus_manager_get_connection (priv->dbus_mgr),
	                                     BM_DBUS_PATH,
	                                     G_OBJECT (singleton));

	g_signal_connect (priv->dbus_mgr,
	                  "name-owner-changed",
	                  G_CALLBACK (bm_manager_name_owner_changed),
	                  singleton);

	priv->udev_mgr = bm_udev_manager_new ();

    g_signal_connect (priv->udev_mgr,
                      "device-added",
                      G_CALLBACK (udev_device_added_cb),
                      singleton);
    g_signal_connect (priv->udev_mgr,
                      "device-removed",
                      G_CALLBACK (udev_device_removed_cb),
                      singleton);

	return singleton;
}

static void
dispose (GObject *object)
{
	BMManager *manager = BM_MANAGER (object);
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
	GSList *iter;
	DBusGConnection *bus;
	DBusConnection *dbus_connection;

	if (priv->disposed) {
		// FIXME G_OBJECT_CLASS (bm_manager_parent_class)->dispose (object);
		return;
	}
	priv->disposed = TRUE;

	user_proxy_cleanup (manager, FALSE);
	g_hash_table_destroy (priv->user_connections);
	priv->user_connections = NULL;

	g_hash_table_foreach (priv->system_connections, emit_removed, manager);
	g_hash_table_remove_all (priv->system_connections);
	g_hash_table_destroy (priv->system_connections);
	priv->system_connections = NULL;

	g_free (priv->hostname);
	g_free (priv->config_file);

	if (priv->sys_settings) {
		g_object_unref (priv->sys_settings);
		priv->sys_settings = NULL;
	}

	/* Unregister property filter */
	bus = bm_dbus_manager_get_connection (priv->dbus_mgr);
	if (bus) {
		dbus_connection = dbus_g_connection_get_connection (bus);
		g_assert (dbus_connection);
		dbus_connection_remove_filter (dbus_connection, prop_filter, manager);
	}
	g_object_unref (priv->dbus_mgr);

	// FIXME G_OBJECT_CLASS (bm_manager_parent_class)->dispose (object);
}

static void
set_property (GObject *object, guint prop_id,
			  const GValue *value, GParamSpec *pspec)
{
	BMManager *self = BM_MANAGER (object);
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_NETWORKING_ENABLED:
		/* Construct only for now */
		priv->net_enabled = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object, guint prop_id,
			  GValue *value, GParamSpec *pspec)
{
	BMManager *self = BM_MANAGER (object);
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_VERSION:
		g_value_set_string (value, VERSION);
		break;
	case PROP_STATE:
		//bm_manager_update_state (self);
		g_value_set_uint (value, priv->state);
		break;
	case PROP_NETWORKING_ENABLED:
		g_value_set_boolean (value, priv->net_enabled);
		break;
	case PROP_SLEEPING:
		g_value_set_boolean (value, priv->sleeping);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_manager_init (BMManager *manager)
{
	BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
	DBusGConnection *g_connection;
	guint id, i;
	GError *error = NULL;

	priv->sleeping = FALSE;
	priv->state = BM_STATE_DISCONNECTED;

	priv->dbus_mgr = bm_dbus_manager_get ();

	priv->user_connections = g_hash_table_new_full (g_str_hash,
	                                                g_str_equal,
	                                                g_free,
	                                                g_object_unref);

	priv->system_connections = g_hash_table_new_full (g_str_hash,
	                                                  g_str_equal,
	                                                  g_free,
	                                                  g_object_unref);

	g_connection = bm_dbus_manager_get_connection (priv->dbus_mgr);
}

static void
bm_manager_class_init (BMManagerClass *manager_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (manager_class);

	g_type_class_add_private (manager_class, sizeof (BMManagerPrivate));

	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;

	/* properties */
	g_object_class_install_property
		(object_class, PROP_VERSION,
		 g_param_spec_string (BM_MANAGER_VERSION,
		                      "Version",
		                      "BarcodeManager version",
		                      NULL,
		                      G_PARAM_READABLE));

	g_object_class_install_property
		(object_class, PROP_STATE,
		 g_param_spec_uint (BM_MANAGER_STATE,
		                    "State",
		                    "Current state",
		                    0, BM_STATE_DISCONNECTED, 0,
		                    G_PARAM_READABLE));

    g_object_class_install_property
		(object_class, PROP_NETWORKING_ENABLED,
         g_param_spec_boolean (BM_MANAGER_NETWORKING_ENABLED,
                               "NetworkingEnabled",
                               "Is networking enabled",
                               TRUE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/* signals */
	signals[DEVICE_ADDED] =
		g_signal_new ("device-added",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, device_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1, G_TYPE_OBJECT);

	signals[DEVICE_REMOVED] =
		g_signal_new ("device-removed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, device_removed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1, G_TYPE_OBJECT);

	signals[STATE_CHANGED] =
		g_signal_new ("state-changed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, state_changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__UINT,
		              G_TYPE_NONE, 1, G_TYPE_UINT);

	signals[PROPERTIES_CHANGED] =
		bm_properties_changed_signal_new (object_class,
		                                  G_STRUCT_OFFSET (BMManagerClass, properties_changed));

	signals[CONNECTIONS_ADDED] =
		g_signal_new ("connections-added",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, connections_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__UINT,
		              G_TYPE_NONE, 1, G_TYPE_UINT);

	signals[CONNECTION_ADDED] =
		g_signal_new ("connection-added",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, connection_added),
		              NULL, NULL,
		              _bm_marshal_VOID__OBJECT_UINT,
		              G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_UINT);

	signals[CONNECTION_UPDATED] =
		g_signal_new ("connection-updated",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, connection_updated),
		              NULL, NULL,
		              _bm_marshal_VOID__OBJECT_UINT,
		              G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_UINT);

	signals[CONNECTION_REMOVED] =
		g_signal_new ("connection-removed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMManagerClass, connection_removed),
		              NULL, NULL,
		              _bm_marshal_VOID__OBJECT_UINT,
		              G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_UINT);

	signals[CHECK_PERMISSIONS] =
		g_signal_new ("check-permissions",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              0, NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals[USER_PERMISSIONS_CHANGED] =
		g_signal_new ("user-permissions-changed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              0, NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	/* StateChange is DEPRECATED */
	signals[STATE_CHANGE] =
		g_signal_new ("state-change",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              0, NULL, NULL,
		              g_cclosure_marshal_VOID__UINT,
		              G_TYPE_NONE, 1, G_TYPE_UINT);

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (manager_class),
	                                 &dbus_glib_bm_manager_object_info);

	dbus_g_error_domain_register (BM_MANAGER_ERROR, NULL, BM_TYPE_MANAGER_ERROR);
	dbus_g_error_domain_register (BM_LOGGING_ERROR, "org.freedesktop.BarcodeManager.Logging", BM_TYPE_LOGGING_ERROR);
}

static gboolean
manager_sleeping (BMManager *self)
{
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);

	bm_log_dbg(LOGD_CORE, "%s", (priv->sleeping || !priv->net_enabled) ? "true" : "false");

    if (priv->sleeping || !priv->net_enabled)
        return TRUE;
    return FALSE;
}



static void
bm_manager_update_state (BMManager *manager)
{
    BMManagerPrivate *priv;
    BMState new_state = BM_STATE_DISCONNECTED;

    g_return_if_fail (BM_IS_MANAGER (manager));

    priv = BM_MANAGER_GET_PRIVATE (manager);

    if (manager_sleeping (manager))
        new_state = BM_STATE_ASLEEP;
    else {
        GSList *iter;

        for (iter = priv->devices; iter; iter = iter->next) {
            BMDevice *dev = BM_DEVICE (iter->data);

            if (bm_device_get_state (dev) == BM_DEVICE_STATE_ACTIVATED) {
                new_state = BM_STATE_CONNECTED;
                break;
            } else if (bm_device_is_activating (dev)) {
                new_state = BM_STATE_CONNECTING;
            }
        }
    }

    if (priv->state != new_state) {
		priv->state = new_state;
        g_object_notify (G_OBJECT (manager), BM_MANAGER_STATE);

        g_signal_emit (manager, signals[STATE_CHANGED], 0, priv->state);

		/* Emit StateChange too for backwards compatibility */
        g_signal_emit (manager, signals[STATE_CHANGE], 0, priv->state);
    }
}

static void
do_sleep_wake (BMManager *self)
{
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
    const GSList *unmanaged_specs;
    GSList *iter;

    if (manager_sleeping (self)) {
        bm_log_info (LOGD_SUSPEND, "sleeping or disabling...");

    } else {
        bm_log_info (LOGD_SUSPEND, "waking up and re-enabling...");

        /* Re-manage managed devices */
        for (iter = priv->devices; iter; iter = iter->next) {
			//            BMDevice *device = BM_DEVICE (iter->data);
            // guint i;
        }

        /* Ask for new bluetooth devices */
        // bluez_manager_resync_devices (self);
    }

    bm_manager_update_state (self);
}

static void
_internal_enable (BMManager *self, gboolean enable)
{
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
    GError *err = NULL;

    /* Update "NetworkingEnabled" key in state file */
    if (priv->state_file) {
		if (!write_value_to_state_file (priv->state_file,
                                        "main", "NetworkingEnabled",
                                        G_TYPE_BOOLEAN, (gpointer) &enable,
										&err)) {
            /* Not a hard error */
            bm_log_warn (LOGD_SUSPEND, "writing to state file %s failed: (%d) %s.",
                         priv->state_file,
                         err ? err->code : -1,
                         (err && err->message) ? err->message : "unknown");
        }
    }

    bm_log_info (LOGD_SUSPEND, "%s requested (sleeping: %s  enabled: %s)",
                 enable ? "enable" : "disable",
				 priv->sleeping ? "yes" : "no",
                 priv->net_enabled ? "yes" : "no");

    priv->net_enabled = enable;

    do_sleep_wake (self);

    g_object_notify (G_OBJECT (self), BM_MANAGER_NETWORKING_ENABLED);
}

static void
enable_net_done_cb (BMAuthChain *chain,
                    GError *error,
                    DBusGMethodInvocation *context,
                    gpointer user_data)
{
    BMManager *self = BM_MANAGER (user_data);
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
    GError *ret_error;
    BMAuthCallResult result;
    gboolean enable;

    priv->auth_chains = g_slist_remove (priv->auth_chains, chain);

    result = GPOINTER_TO_UINT (bm_auth_chain_get_data (chain, BM_AUTH_PERMISSION_ENABLE_DISABLE_NETWORK));
    if (error) {
        bm_log_dbg (LOGD_CORE, "Enable request failed: %s", error->message);
        ret_error = g_error_new (BM_MANAGER_ERROR,
                                 BM_MANAGER_ERROR_PERMISSION_DENIED,
                                 "Enable request failed: %s",
                                 error->message);
        dbus_g_method_return_error (context, ret_error);
        g_error_free (ret_error);
    } else if (result != BM_AUTH_CALL_RESULT_YES) {
        ret_error = g_error_new_literal (BM_MANAGER_ERROR,
                                         BM_MANAGER_ERROR_PERMISSION_DENIED,
                                         "Not authorized to enable/disable networking");
        dbus_g_method_return_error (context, ret_error);
        g_error_free (ret_error);
    } else {
        /* Auth success */
        enable = GPOINTER_TO_UINT (bm_auth_chain_get_data (chain, "enable"));
        _internal_enable (self, enable);
        dbus_g_method_return (context);
    }
    bm_auth_chain_unref (chain);
}

static gboolean
return_no_pk_error (PolkitAuthority *authority,
                    const char *detail,
                    DBusGMethodInvocation *context)
{
    GError *error;

    if (!authority) {
        error = g_error_new (BM_MANAGER_ERROR,
                             BM_MANAGER_ERROR_PERMISSION_DENIED,
                             "%s request failed: PolicyKit not initialized",
                             detail);
		dbus_g_method_return_error (context, error);
        g_error_free (error);
		return FALSE;
    }
    return TRUE;
}

static void
impl_manager_enable (BMManager *self,
                     gboolean enable,
                     DBusGMethodInvocation *context)
{
    BMManagerPrivate *priv;
    BMAuthChain *chain;
    GError *error = NULL;
    gulong sender_uid = G_MAXULONG;
    const char *error_desc = NULL;

    g_return_if_fail (BM_IS_MANAGER (self));

    priv = BM_MANAGER_GET_PRIVATE (self);

    if (priv->net_enabled == enable) {
        error = g_error_new (BM_MANAGER_ERROR,
                             BM_MANAGER_ERROR_ALREADY_ENABLED_OR_DISABLED,
                             "Already %s", enable ? "enabled" : "disabled");
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    if (!bm_auth_get_caller_uid (context, priv->dbus_mgr, &sender_uid, &error_desc)) {
        error = g_error_new_literal (BM_MANAGER_ERROR,
                                     BM_MANAGER_ERROR_PERMISSION_DENIED,
                                     error_desc);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    /* Root doesn't need PK authentication */
    if (0 == sender_uid) {
        _internal_enable (self, enable);
        dbus_g_method_return (context);
        return;
    }

    if (!return_no_pk_error (priv->authority, "Enable/disable", context))
        return;

    chain = bm_auth_chain_new (priv->authority, context, NULL, enable_net_done_cb, self);
    g_assert (chain);
    priv->auth_chains = g_slist_append (priv->auth_chains, chain);

    bm_auth_chain_set_data (chain, "enable", GUINT_TO_POINTER (enable), NULL);
    bm_auth_chain_add_call (chain, BM_AUTH_PERMISSION_ENABLE_DISABLE_NETWORK, TRUE);
}



static gboolean
impl_manager_get_devices (BMManager *manager, GPtrArray **devices, GError **err)
{
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (manager);
    GSList *iter;

	bm_log_dbg (LOGD_CORE, "impl_manager_get_devices");

    *devices = g_ptr_array_sized_new (g_slist_length (priv->devices));

    for (iter = priv->devices; iter; iter = iter->next)
        g_ptr_array_add (*devices, g_strdup (bm_device_get_path (BM_DEVICE (iter->data))));

    return TRUE;
}

static BMDevice *
find_device_by_iface (BMManager *self, const gchar *iface)
{
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
    GSList *iter;

    for (iter = priv->devices; iter; iter = g_slist_next (iter)) {
        BMDevice *device = BM_DEVICE (iter->data);
        const gchar *d_iface = bm_device_get_iface (device);
        if (!strcmp (d_iface, iface))
            return device;
    }

    return NULL;
}

static void
manager_device_state_changed (BMDevice *device,
                              BMDeviceState new_state,
                              BMDeviceState old_state,
                              BMDeviceStateReason reason,
                              gpointer user_data)
{
    BMManager *manager = BM_MANAGER (user_data);

    switch (new_state) {
    case BM_DEVICE_STATE_UNMANAGED:
    case BM_DEVICE_STATE_UNAVAILABLE:
    case BM_DEVICE_STATE_DISCONNECTED:
    case BM_DEVICE_STATE_PREPARE:
    case BM_DEVICE_STATE_FAILED:
        g_object_notify (G_OBJECT (manager), BM_MANAGER_ACTIVE_CONNECTIONS);
        break;
    default:
        break;
    }

    bm_manager_update_state (manager);

    if (new_state == BM_DEVICE_STATE_ACTIVATED) {
        // TODO update_active_connection_timestamp (manager, device);
	}
}

static void
add_device (BMManager *self, BMDevice *device)
{
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
    const char *iface, *driver, *type_desc;
    char *path;
    static guint32 devcount = 0;
    const GSList *unmanaged_specs;
    BMConnection *existing = NULL;
    GHashTableIter iter;
    gpointer value;
    gboolean managed = FALSE, enabled = FALSE;

    iface = bm_device_get_iface (device);
    g_assert (iface);

    bm_log_info (LOGD_HW, "%s new device", iface);

	/* FIXME more checks here
    if (!BM_IS_DEVICE_MODEM (device) && find_device_by_iface (self, iface)) {
        g_object_unref (device);
        return;
		}*/

    priv->devices = g_slist_append (priv->devices, device);

    g_signal_connect (device, "state-changed",
                      G_CALLBACK (manager_device_state_changed),
                      self);

	/* WE dont support disconnect reqeusts right now.
    g_signal_connect (device, BM_DEVICE_INTERFACE_DISCONNECT_REQUEST,
                      G_CALLBACK (manager_device_disconnect_request),
                      self);*/

    type_desc = bm_device_get_type_desc (device);
    g_assert (type_desc);
    driver = bm_device_get_driver (device);
    if (!driver)
        driver = "unknown";

    bm_log_info (LOGD_HW, "(%s): new %s device (driver: '%s')",
                 iface, type_desc, driver);

    path = g_strdup_printf ("/org/freedesktop/BarcodeManager/Devices/%d", devcount++);
    bm_device_set_path (device, path);
    dbus_g_connection_register_g_object (bm_dbus_manager_get_connection (priv->dbus_mgr),
                                         path,
                                         G_OBJECT (device));
    bm_log_info (LOGD_CORE, "(%s): exported as %s", iface, path);
    g_free (path);

    bm_sysconfig_settings_device_added (priv->sys_settings, device);
    g_signal_emit (self, signals[DEVICE_ADDED], 0, device);

	// FIXME process states here
}

static void
udev_device_added_cb (BMUdevManager *udev_mgr,
                      GUdevDevice *udev_device,
                      BMDeviceCreatorFn creator_fn,
                      gpointer user_data)
{
    BMManager *self = BM_MANAGER (user_data);
    GObject *device;

    device = creator_fn (udev_mgr, udev_device, manager_sleeping (self));

    if (device) {
		bm_log_dbg (LOGD_HW, "processing..");
        add_device (self, BM_DEVICE (device));
	} else {
		bm_log_dbg (LOGD_HW, "skipping..");
	}
}

static void
udev_device_removed_cb (BMUdevManager *manager,
                        GUdevDevice *udev_device,
                        gpointer user_data)
{
    BMManager *self = BM_MANAGER (user_data);
    BMManagerPrivate *priv = BM_MANAGER_GET_PRIVATE (self);
    BMDevice *device;

	device = find_device_by_iface (self, g_udev_device_get_name (udev_device));

    if (device)
        priv->devices = remove_one_device (self, priv->devices, device, FALSE);
}

