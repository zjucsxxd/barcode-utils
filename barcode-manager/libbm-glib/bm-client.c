/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access barcode scanner status & information from glib applications
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
 * Copyright (C) 2011 Jakob Flierl
 */

#include <dbus/dbus-glib.h>
#include <string.h>
#include <bm-utils.h>

#include "bm-client.h"
#include "bm-device-private.h"
#include "bm-marshal.h"
#include "bm-types-private.h"
#include "bm-object-private.h"
#include "bm-active-connection.h"
#include "bm-object-cache.h"
#include "bm-dbus-glib-types.h"

#include "bm-client-bindings.h"


G_DEFINE_TYPE (BMClient, bm_client, BM_TYPE_OBJECT)

#define BM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_CLIENT, BMClientPrivate))

typedef struct {
	gboolean disposed;

	DBusGProxy *client_proxy;
	DBusGProxy *bus_proxy;
	gboolean manager_running;
	BMState state;
	GPtrArray *devices;
	GPtrArray *active_connections;

	DBusGProxyCall *perm_call;
	GHashTable *permissions;

	gboolean have_networking_enabled;
	gboolean networking_enabled;
	gboolean wireless_enabled;
	gboolean wireless_hw_enabled;

	gboolean wwan_enabled;
	gboolean wwan_hw_enabled;
} BMClientPrivate;

enum {
	PROP_0,
	PROP_STATE,
	PROP_MANAGER_RUNNING,
	PROP_NETWORKING_ENABLED,
	PROP_WIRELESS_ENABLED,
	PROP_WIRELESS_HARDWARE_ENABLED,
	PROP_WWAN_ENABLED,
	PROP_WWAN_HARDWARE_ENABLED,
	PROP_ACTIVE_CONNECTIONS,

	LAST_PROP
};

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,
	PERMISSION_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void proxy_name_owner_changed (DBusGProxy *proxy,
									  const char *name,
									  const char *old_owner,
									  const char *new_owner,
									  gpointer user_data);

static void client_device_added_proxy (DBusGProxy *proxy, char *path, gpointer user_data);
static void client_device_removed_proxy (DBusGProxy *proxy, char *path, gpointer user_data);

static void
handle_net_enabled_changed (GObject *object,
                            GParamSpec *pspec,
                            GValue *value,
                            gpointer user_data)
{
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (object);

	/* Update the cache flag when it changes */
	priv->have_networking_enabled = TRUE;
}

static void
bm_client_init (BMClient *client)
{
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (client);

	priv->state = BM_STATE_UNKNOWN;

	priv->permissions = g_hash_table_new (g_direct_hash, g_direct_equal);

	g_signal_connect (client,
	                  "notify::" BM_CLIENT_NETWORKING_ENABLED,
	                  G_CALLBACK (handle_net_enabled_changed),
	                  client);
}

static GObject *
new_active_connection (DBusGConnection *connection, const char *path)
{
	DBusGProxy *proxy;
	GError *error = NULL;
	GValue value = {0,};
	GObject *object = NULL;

	proxy = dbus_g_proxy_new_for_name (connection,
									   BM_DBUS_SERVICE,
									   path,
									   "org.freedesktop.DBus.Properties");
	if (!proxy) {
		g_warning ("%s: couldn't create D-Bus object proxy.", __func__);
		return NULL;
	}

	object = bm_active_connection_new (connection, path);

	g_object_unref (proxy);
	return object;
}

static gboolean
demarshal_active_connections (BMObject *object,
                              GParamSpec *pspec,
                              GValue *value,
                              gpointer field)
{
	DBusGConnection *connection;

	connection = bm_object_get_connection (object);
	if (!_bm_object_array_demarshal (value, (GPtrArray **) field, connection, new_active_connection))
		return FALSE;

	_bm_object_queue_notify (object, BM_CLIENT_ACTIVE_CONNECTIONS);
	return TRUE;
}

static void
register_for_property_changed (BMClient *client)
{
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (client);
	const NMPropertiesChangedInfo property_changed_info[] = {
		{ BM_CLIENT_STATE,                     _bm_object_demarshal_generic,  &priv->state },
		{ BM_CLIENT_NETWORKING_ENABLED,        _bm_object_demarshal_generic,  &priv->networking_enabled },
		{ BM_CLIENT_WIRELESS_ENABLED,          _bm_object_demarshal_generic,  &priv->wireless_enabled },
		{ BM_CLIENT_WIRELESS_HARDWARE_ENABLED, _bm_object_demarshal_generic,  &priv->wireless_hw_enabled },
		{ BM_CLIENT_WWAN_ENABLED,              _bm_object_demarshal_generic,  &priv->wwan_enabled },
		{ BM_CLIENT_WWAN_HARDWARE_ENABLED,     _bm_object_demarshal_generic,  &priv->wwan_hw_enabled },
		{ BM_CLIENT_ACTIVE_CONNECTIONS,        demarshal_active_connections, &priv->active_connections },
		{ NULL },
	};

	_bm_object_handle_properties_changed (BM_OBJECT (client),
	                                     priv->client_proxy,
	                                     property_changed_info);
}

#define BM_AUTH_PERMISSION_ENABLE_DISABLE_NETWORK "org.freedesktop.BarcodeManager.enable-disable-network"
#define BM_AUTH_PERMISSION_USE_USER_CONNECTIONS   "org.freedesktop.BarcodeManager.use-user-connections"

static BMClientPermission
bm_permission_to_client (const char *nm)
{
	if (!strcmp (nm, BM_AUTH_PERMISSION_ENABLE_DISABLE_NETWORK))
		return BM_CLIENT_PERMISSION_ENABLE_DISABLE_NETWORK;
	else if (!strcmp (nm, BM_AUTH_PERMISSION_USE_USER_CONNECTIONS))
		return BM_CLIENT_PERMISSION_USE_USER_CONNECTIONS;
	return BM_CLIENT_PERMISSION_NONE;
}

static BMClientPermissionResult
bm_permission_result_to_client (const char *nm)
{
	if (!strcmp (nm, "yes"))
		return BM_CLIENT_PERMISSION_RESULT_YES;
	else if (!strcmp (nm, "no"))
		return BM_CLIENT_PERMISSION_RESULT_NO;
	else if (!strcmp (nm, "auth"))
		return BM_CLIENT_PERMISSION_RESULT_AUTH;
	return BM_CLIENT_PERMISSION_RESULT_UNKNOWN;
}

static void
update_permissions (BMClient *self, GHashTable *permissions)
{
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (self);
	GHashTableIter iter;
	gpointer key, value;
	BMClientPermission perm;
	BMClientPermissionResult perm_result;
	GList *keys, *keys_iter;

	/* get list of old permissions for change notification */
	keys = g_hash_table_get_keys (priv->permissions);
	g_hash_table_remove_all (priv->permissions);

	if (permissions) {
		/* Process new permissions */
		g_hash_table_iter_init (&iter, permissions);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			perm = bm_permission_to_client ((const char *) key);
			perm_result = bm_permission_result_to_client ((const char *) value);
			if (perm) {
				g_hash_table_insert (priv->permissions,
				                     GUINT_TO_POINTER (perm),
				                     GUINT_TO_POINTER (perm_result));

				/* Remove this permission from the list of previous permissions
				 * we'll be sending BM_CLIENT_PERMISSION_RESULT_UNKNOWN for
				 * in the change signal since it is still a known permission.
				 */
				keys = g_list_remove (keys, GUINT_TO_POINTER (perm));
			}
		}
	}

	/* Signal changes in all updated permissions */
	g_hash_table_iter_init (&iter, priv->permissions);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		g_signal_emit (self, signals[PERMISSION_CHANGED], 0,
		               GPOINTER_TO_UINT (key),
		               GPOINTER_TO_UINT (value));
	}

	/* And signal changes in all permissions that used to be valid but for
	 * some reason weren't received in the last request (if any).
	 */
	for (keys_iter = keys; keys_iter; keys_iter = g_list_next (keys_iter)) {
		g_signal_emit (self, signals[PERMISSION_CHANGED], 0,
		               GPOINTER_TO_UINT (keys_iter->data),
		               BM_CLIENT_PERMISSION_RESULT_UNKNOWN);
	}
	g_list_free (keys);
}

static void
get_permissions_sync (BMClient *self)
{
	gboolean success;
	GHashTable *permissions = NULL;

	success = dbus_g_proxy_call_with_timeout (BM_CLIENT_GET_PRIVATE (self)->client_proxy,
	                                          "GetPermissions", 3000, NULL,
	                                          G_TYPE_INVALID,
	                                          DBUS_TYPE_G_MAP_OF_STRING, &permissions, G_TYPE_INVALID);
	update_permissions (self, success ? permissions : NULL);
	if (permissions)
		g_hash_table_destroy (permissions);
}

static void
get_permissions_reply (DBusGProxy *proxy,
                       GHashTable *permissions,
                       GError *error,
                       gpointer user_data)
{
	BMClient *self = BM_CLIENT (user_data);

	BM_CLIENT_GET_PRIVATE (self)->perm_call = NULL;
	update_permissions (BM_CLIENT (user_data), error ? NULL : permissions);
}

static void
client_recheck_permissions (DBusGProxy *proxy, gpointer user_data)
{
	BMClient *self = BM_CLIENT (user_data);
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (self);

	if (!priv->perm_call) {
		priv->perm_call = org_freedesktop_BarcodeManager_get_permissions_async (BM_CLIENT_GET_PRIVATE (self)->client_proxy,
	                                                                            get_permissions_reply,
	                                                                            self);
	}
}

static GObject*
constructor (GType type,
		   guint n_construct_params,
		   GObjectConstructParam *construct_params)
{
	BMObject *object;
	DBusGConnection *connection;
	BMClientPrivate *priv;
	GError *err = NULL;

	object = (BMObject *) G_OBJECT_CLASS (bm_client_parent_class)->constructor (type,
																 n_construct_params,
																 construct_params);
	if (!object)
		return NULL;

	priv = BM_CLIENT_GET_PRIVATE (object);
	connection = bm_object_get_connection (object);

	priv->client_proxy = dbus_g_proxy_new_for_name (connection,
										   BM_DBUS_SERVICE,
										   bm_object_get_path (object),
										   BM_DBUS_INTERFACE);

	register_for_property_changed (BM_CLIENT (object));

	dbus_g_proxy_add_signal (priv->client_proxy, "DeviceAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->client_proxy,
						    "DeviceAdded",
						    G_CALLBACK (client_device_added_proxy),
						    object,
						    NULL);

	dbus_g_proxy_add_signal (priv->client_proxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->client_proxy,
						    "DeviceRemoved",
						    G_CALLBACK (client_device_removed_proxy),
						    object,
						    NULL);

	/* Permissions */
	dbus_g_proxy_add_signal (priv->client_proxy, "CheckPermissions", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->client_proxy,
	                             "CheckPermissions",
	                             G_CALLBACK (client_recheck_permissions),
	                             object,
	                             NULL);
	get_permissions_sync (BM_CLIENT (object));

	priv->bus_proxy = dbus_g_proxy_new_for_name (connection,
										"org.freedesktop.DBus",
										"/org/freedesktop/DBus",
										"org.freedesktop.DBus");

	dbus_g_proxy_add_signal (priv->bus_proxy, "NameOwnerChanged",
						G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
						G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->bus_proxy,
						    "NameOwnerChanged",
						    G_CALLBACK (proxy_name_owner_changed),
						    object, NULL);

	if (!dbus_g_proxy_call (priv->bus_proxy,
					    "NameHasOwner", &err,
					    G_TYPE_STRING, BM_DBUS_SERVICE,
					    G_TYPE_INVALID,
					    G_TYPE_BOOLEAN, &priv->manager_running,
					    G_TYPE_INVALID)) {
		g_warning ("Error on NameHasOwner DBUS call: %s", err->message);
		g_error_free (err);
	}

	return G_OBJECT (object);
}

static void
free_object_array (GPtrArray **array)
{
	g_return_if_fail (array != NULL);

	if (*array) {
		g_ptr_array_foreach (*array, (GFunc) g_object_unref, NULL);
		g_ptr_array_free (*array, TRUE);
		*array = NULL;
	}
}

static void
dispose (GObject *object)
{
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_client_parent_class)->dispose (object);
		return;
	}

	if (priv->perm_call)
		dbus_g_proxy_cancel_call (priv->client_proxy, priv->perm_call);

	g_object_unref (priv->client_proxy);
	g_object_unref (priv->bus_proxy);

	free_object_array (&priv->devices);
	free_object_array (&priv->active_connections);

	g_hash_table_destroy (priv->permissions);

	G_OBJECT_CLASS (bm_client_parent_class)->dispose (object);
}

static void
set_property (GObject *object, guint prop_id,
		    const GValue *value, GParamSpec *pspec)
{
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (object);
	gboolean b;

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
	BMClient *self = BM_CLIENT (object);
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_STATE:
		g_value_set_uint (value, bm_client_get_state (self));
		break;
	case PROP_MANAGER_RUNNING:
		g_value_set_boolean (value, priv->manager_running);
		break;
	case PROP_NETWORKING_ENABLED:
		g_value_set_boolean (value, priv->networking_enabled);
		break;
	case PROP_ACTIVE_CONNECTIONS:
		g_value_set_boxed (value, bm_client_get_active_connections (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_client_class_init (BMClientClass *client_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (client_class);

	g_type_class_add_private (client_class, sizeof (BMClientPrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;

	/* properties */

	/**
	 * BMClient:state:
	 *
	 * The current daemon state.
	 **/
	g_object_class_install_property
		(object_class, PROP_STATE,
		 g_param_spec_uint (BM_CLIENT_STATE,
						    "State",
						    "BarcodeManager state",
						    BM_STATE_UNKNOWN, BM_STATE_DISCONNECTED, BM_STATE_UNKNOWN,
						    G_PARAM_READABLE));

	/**
	 * BMClient::manager-running:
	 *
	 * Whether the daemon is running.
	 **/
	g_object_class_install_property
		(object_class, PROP_MANAGER_RUNNING,
		 g_param_spec_boolean (BM_CLIENT_MANAGER_RUNNING,
						       "ManagerRunning",
						       "Whether BarcodeManager is running",
						       FALSE,
						       G_PARAM_READABLE));

	/**
	 * BMClient::networking-enabled:
	 *
	 * Whether networking is enabled.
	 **/
	g_object_class_install_property
		(object_class, PROP_NETWORKING_ENABLED,
		 g_param_spec_boolean (BM_CLIENT_NETWORKING_ENABLED,
						   "NetworkingEnabled",
						   "Is networking enabled",
						   TRUE,
						   G_PARAM_READABLE));

	/**
	 * BMClient::active-connections:
	 *
	 * The active connections.
	 **/
	g_object_class_install_property
		(object_class, PROP_ACTIVE_CONNECTIONS,
		 g_param_spec_boxed (BM_CLIENT_ACTIVE_CONNECTIONS,
						   "Active connections",
						   "Active connections",
						   BM_TYPE_OBJECT_ARRAY,
						   G_PARAM_READABLE));

	/* signals */

	/**
	 * BMClient::device-added:
	 * @client: the client that received the signal
	 * @device: the new device
	 *
	 * Notifies that a #BMDevice is added.
	 **/
	signals[DEVICE_ADDED] =
		g_signal_new ("device-added",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (BMClientClass, device_added),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  G_TYPE_OBJECT);

	/**
	 * BMClient::device-removed:
	 * @widget: the client that received the signal
	 * @device: the removed device
	 *
	 * Notifies that a #BMDevice is removed.
	 **/
	signals[DEVICE_REMOVED] =
		g_signal_new ("device-removed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (BMClientClass, device_removed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  G_TYPE_OBJECT);

	/**
	 * BMClient::permission-changed:
	 * @widget: the client that received the signal
	 * @permission: a permission from #BMClientPermission
	 * @result: the permission's result, one of #BMClientPermissionResult
	 *
	 * Notifies that a permission has changed
	 **/
	signals[PERMISSION_CHANGED] =
		g_signal_new ("permission-changed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  0, NULL, NULL,
					  _bm_marshal_VOID__UINT_UINT,
					  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
}

/**
 * bm_client_new:
 *
 * Creates a new #BMClient.
 *
 * Returns: a new #BMClient
 **/
BMClient *
bm_client_new (void)
{
	DBusGConnection *connection;
	GError *err = NULL;

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!connection) {
		g_warning ("Couldn't connect to system bus: %s", err->message);
		g_error_free (err);
		return NULL;
	}

	return (BMClient *) g_object_new (BM_TYPE_CLIENT,
									  BM_OBJECT_DBUS_CONNECTION, connection,
									  BM_OBJECT_DBUS_PATH, BM_DBUS_PATH,
									  NULL);
}

static void
proxy_name_owner_changed (DBusGProxy *proxy,
						  const char *name,
						  const char *old_owner,
						  const char *new_owner,
						  gpointer user_data)
{
	BMClient *client = BM_CLIENT (user_data);
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (client);
	gboolean old_good = (old_owner && strlen (old_owner));
	gboolean new_good = (new_owner && strlen (new_owner));
	gboolean new_running = FALSE;

	if (!name || strcmp (name, BM_DBUS_SERVICE))
		return;

	if (!old_good && new_good)
		new_running = TRUE;
	else if (old_good && !new_good)
		new_running = FALSE;

	if (new_running == priv->manager_running)
		return;
}

static void
client_device_added_proxy (DBusGProxy *proxy, char *path, gpointer user_data)
{
	BMClient *client = BM_CLIENT (user_data);
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (client);
	GObject *device;

	device = G_OBJECT (bm_client_get_device_by_path (client, path));
	if (!device) {
		DBusGConnection *connection = bm_object_get_connection (BM_OBJECT (client));

		device = G_OBJECT (_bm_object_cache_get (path));
		if (device) {
			g_ptr_array_add (priv->devices, g_object_ref (device));
		} else {
			device = G_OBJECT (bm_device_new (connection, path));
			if (device)
				g_ptr_array_add (priv->devices, device);
		}
	}

	if (device)
		g_signal_emit (client, signals[DEVICE_ADDED], 0, device);
}

static void
client_device_removed_proxy (DBusGProxy *proxy, char *path, gpointer user_data)
{
	BMClient *client = BM_CLIENT (user_data);
	BMClientPrivate *priv = BM_CLIENT_GET_PRIVATE (client);
	BMDevice *device;

	device = bm_client_get_device_by_path (client, path);
	if (device) {
		g_signal_emit (client, signals[DEVICE_REMOVED], 0, device);
		g_ptr_array_remove (priv->devices, device);
		g_object_unref (device);
	}
}

/**
 * bm_client_get_devices:
 * @client: a #BMClient
 *
 * Gets all the detected devices.
 *
 * Returns: a #GPtrArray containing all the #BMDevice<!-- -->s.
 * The returned array is owned by the client and should not be modified.
 **/
const GPtrArray *
bm_client_get_devices (BMClient *client)
{
	BMClientPrivate *priv;
	DBusGConnection *connection;
	GValue value = { 0, };
	GError *error = NULL;
	GPtrArray *temp;

	g_return_val_if_fail (BM_IS_CLIENT (client), NULL);

	priv = BM_CLIENT_GET_PRIVATE (client);
	if (priv->devices)
		return handle_ptr_array_return (priv->devices);

	if (!org_freedesktop_BarcodeManager_get_devices (priv->client_proxy, &temp, &error)) {
		g_warning ("%s: error getting devices: %s\n", __func__, error->message);
		g_error_free (error);
		return NULL;
	}

	g_value_init (&value, DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH);
	g_value_take_boxed (&value, temp);
	connection = bm_object_get_connection (BM_OBJECT (client));
	_bm_object_array_demarshal (&value, &priv->devices, connection, bm_device_new);
	g_value_unset (&value);

	return handle_ptr_array_return (priv->devices);
}

/**
 * bm_client_get_device_by_path:
 * @client: a #BMClient
 * @object_path: the object path to search for
 *
 * Gets a #BMDevice from a #BMClient.
 *
 * Returns: the #BMDevice for the given @object_path or %NULL if none is found.
 **/
BMDevice *
bm_client_get_device_by_path (BMClient *client, const char *object_path)
{
	const GPtrArray *devices;
	int i;
	BMDevice *device = NULL;

	g_return_val_if_fail (BM_IS_CLIENT (client), NULL);
	g_return_val_if_fail (object_path, NULL);

	devices = bm_client_get_devices (client);
	if (!devices)
		return NULL;

	for (i = 0; i < devices->len; i++) {
		BMDevice *candidate = g_ptr_array_index (devices, i);
		if (!strcmp (bm_object_get_path (BM_OBJECT (candidate)), object_path)) {
			device = candidate;
			break;
		}
	}

	return device;
}

typedef struct {
	BMClientActivateDeviceFn fn;
	gpointer user_data;
} ActivateDeviceInfo;

static void
activate_cb (DBusGProxy *proxy,
             char *path,
             GError *error,
             gpointer user_data)
{
	ActivateDeviceInfo *info = (ActivateDeviceInfo *) user_data;

	if (info->fn)
		info->fn (info->user_data, path, error);
	else if (error)
		bm_warning ("Device activation failed: (%d) %s", error->code, error->message);

	g_slice_free (ActivateDeviceInfo, info);
}

/**
 * bm_client_activate_connection:
 * @client: a #BMClient
 * @service_name: the connection's service name
 * @connection_path: the connection's DBus path
 * @device: the #BMDevice
 * @specific_object: the device specific object (currently used only for
 * activating wireless devices and should be the #NMAccessPoint<!-- -->'s path.
 * @callback: the function to call when the call is done
 * @user_data: user data to pass to the callback function
 *
 * Activates a connection with the given #BMDevice.
 **/
void
bm_client_activate_connection (BMClient *client,
					  const char *service_name,
					  const char *connection_path,
					  BMDevice *device,
					  const char *specific_object,
					  BMClientActivateDeviceFn callback,
					  gpointer user_data)
{
	ActivateDeviceInfo *info;
	char *internal_so = (char *) specific_object;

	g_return_if_fail (BM_IS_CLIENT (client));
	g_return_if_fail (BM_IS_DEVICE (device));
	g_return_if_fail (service_name != NULL);
	g_return_if_fail (connection_path != NULL);

	/* NULL specific object must be translated into "/" because D-Bus does
	 * not have any idea of NULL object paths.
	 */
	if (internal_so == NULL)
		internal_so = "/";

	info = g_slice_new (ActivateDeviceInfo);
	info->fn = callback;
	info->user_data = user_data;

	org_freedesktop_BarcodeManager_activate_connection_async (BM_CLIENT_GET_PRIVATE (client)->client_proxy,
											    service_name,
											    connection_path,
											    bm_object_get_path (BM_OBJECT (device)),
											    internal_so,
											    activate_cb,
											    info);
}

/**
 * bm_client_deactivate_connection:
 * @client: a #BMClient
 * @active: the #BMActiveConnection to deactivate
 *
 * Deactivates an active #BMActiveConnection.
 **/
void
bm_client_deactivate_connection (BMClient *client, BMActiveConnection *active)
{
	BMClientPrivate *priv;
	const char *path;
	GError *error = NULL;

	g_return_if_fail (BM_IS_CLIENT (client));
	g_return_if_fail (BM_IS_ACTIVE_CONNECTION (active));

	// FIXME: return errors
	priv = BM_CLIENT_GET_PRIVATE (client);
	path = bm_object_get_path (BM_OBJECT (active));
	if (!org_freedesktop_BarcodeManager_deactivate_connection (priv->client_proxy, path, &error)) {
		g_warning ("Could not deactivate connection '%s': %s", path, error->message);
		g_error_free (error);
	}
}

/**
 * bm_client_get_active_connections:
 * @client: a #BMClient
 *
 * Gets the active connections.
 *
 * Returns: a #GPtrArray containing all the active #BMActiveConnection<!-- -->s.
 * The returned array is owned by the client and should not be modified.
 **/
const GPtrArray * 
bm_client_get_active_connections (BMClient *client)
{
	BMClientPrivate *priv;
	GValue value = { 0, };

	g_return_val_if_fail (BM_IS_CLIENT (client), NULL);

	priv = BM_CLIENT_GET_PRIVATE (client);
	if (priv->active_connections)
		return handle_ptr_array_return (priv->active_connections);

	if (!priv->manager_running)
		return NULL;

	if (!_bm_object_get_property (BM_OBJECT (client),
	                             "org.freedesktop.BarcodeManager",
	                             "ActiveConnections",
	                             &value)) {
		return NULL;
	}

	demarshal_active_connections (BM_OBJECT (client), NULL, &value, &priv->active_connections);	
	g_value_unset (&value);

	return handle_ptr_array_return (priv->active_connections);
}

/**
 * bm_client_get_state:
 * @client: a #BMClient
 *
 * Gets the current daemon state.
 *
 * Returns: the current %BMState
 **/
BMState
bm_client_get_state (BMClient *client)
{
	BMClientPrivate *priv;

	g_return_val_if_fail (BM_IS_CLIENT (client), BM_STATE_UNKNOWN);

	priv = BM_CLIENT_GET_PRIVATE (client);

	if (!priv->manager_running)
		return BM_STATE_UNKNOWN;

	if (priv->state == BM_STATE_UNKNOWN)
		priv->state = _bm_object_get_uint_property (BM_OBJECT (client), BM_DBUS_INTERFACE, "State");

	return priv->state;
}

/**
 * bm_client_networking_get_enabled:
 * @client: a #BMClient
 *
 * Whether networking is enabled or disabled.
 *
 * Returns: %TRUE if networking is enabled, %FALSE if networking is disabled
 **/
gboolean
bm_client_networking_get_enabled (BMClient *client)
{
	BMClientPrivate *priv;

	g_return_val_if_fail (BM_IS_CLIENT (client), FALSE);

	priv = BM_CLIENT_GET_PRIVATE (client);
	if (!priv->have_networking_enabled) {
		priv = BM_CLIENT_GET_PRIVATE (client);
		if (!priv->networking_enabled) {
			priv->networking_enabled = _bm_object_get_boolean_property (BM_OBJECT (client),
			                                                            BM_DBUS_INTERFACE,
			                                                            "NetworkingEnabled");
			priv->have_networking_enabled = TRUE;
		}
	}

	return priv->networking_enabled;
}

/**
 * bm_client_networking_set_enabled:
 * @client: a #BMClient
 * @enabled: %TRUE to set networking enabled, %FALSE to set networking disabled
 *
 * Enables or disables networking.  When networking is disabled, all controlled
 * interfaces are disconnected and deactivated.  When networking is enabled,
 * all controlled interfaces are available for activation.
 **/
void
bm_client_networking_set_enabled (BMClient *client, gboolean enable)
{
	GError *err = NULL;

	g_return_if_fail (BM_IS_CLIENT (client));

	if (!org_freedesktop_BarcodeManager_enable (BM_CLIENT_GET_PRIVATE (client)->client_proxy, enable, &err)) {
		g_warning ("Error enabling/disabling networking: %s", err->message);
		g_error_free (err);
	}
}

/**
 * bm_client_sleep:
 * @client: a #BMClient
 * @sleep: %TRUE to put the daemon to sleep
 *
 * Deprecated; use bm_client_networking_set_enabled() instead.
 **/
void
bm_client_sleep (BMClient *client, gboolean sleep)
{
	bm_client_networking_set_enabled (client, !sleep);
}

/**
 * bm_client_get_manager_running:
 * @client: a #BMClient
 *
 * Determines whether the daemon is running.
 *
 * Returns: %TRUE if the daemon is running
 **/
gboolean
bm_client_get_manager_running (BMClient *client)
{
	g_return_val_if_fail (BM_IS_CLIENT (client), FALSE);

	return BM_CLIENT_GET_PRIVATE (client)->manager_running;
}

/**
 * bm_client_get_permission_result:
 * @client: a #BMClient
 * @permission: the permission for which to return the result, one of #BMClientPermission
 *
 * Requests the result of a specific permission, which indicates whether the
 * client can or cannot perform the action the permission represents
 *
 * Returns: the permission's result, one of #BMClientPermissionResult
 **/
BMClientPermissionResult
bm_client_get_permission_result (BMClient *client, BMClientPermission permission)
{
	gpointer result;

	g_return_val_if_fail (BM_IS_CLIENT (client), BM_CLIENT_PERMISSION_RESULT_UNKNOWN);

	result = g_hash_table_lookup (BM_CLIENT_GET_PRIVATE (client)->permissions,
	                              GUINT_TO_POINTER (permission));
	return GPOINTER_TO_UINT (result);
}

