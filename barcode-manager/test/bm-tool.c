/* bm-tool - information tool for BarcodeManager
 *
 * Jakob Flierl <jakob.flierl@gmail.com>
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
 * (C) Copyright 2011 Jakob Flierl
 */

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <bm-client.h>
#include <bm-device.h>
#include <bm-device-bt.h>
#include <bm-utils.h>
#include <bm-setting-connection.h>

/* Don't use bm-dbus-glib-types.h so that we can keep bm-tool
 * building standalone outside of the BM tree.
 */
#define DBUS_TYPE_G_MAP_OF_VARIANT          (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#define DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT   (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, DBUS_TYPE_G_MAP_OF_VARIANT))
#define DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH    (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))

static GHashTable *user_connections = NULL;
static GHashTable *system_connections = NULL;

static gboolean
get_bm_state (BMClient *client)
{
	BMState state;
	char *state_string;
	gboolean success = TRUE;

	state = bm_client_get_state (client);

	switch (state) {
	case BM_STATE_ASLEEP:
		state_string = "asleep";
		break;

	case BM_STATE_CONNECTING:
		state_string = "connecting";
		break;

	case BM_STATE_CONNECTED:
		state_string = "connected";
		break;

	case BM_STATE_DISCONNECTED:
		state_string = "disconnected";
		break;

	case BM_STATE_UNKNOWN:
	default:
		state_string = "unknown";
		success = FALSE;
		break;
	}

	printf ("State: %s\n\n", state_string);

	return success;
}

static void
print_header (const char *label, const char *iface, const char *connection)
{
	GString *string;

	string = g_string_sized_new (79);
	g_string_append_printf (string, "- %s: ", label);
	if (iface)
		g_string_append_printf (string, "%s ", iface);
	if (connection)
		g_string_append_printf (string, " [%s] ", connection);

	while (string->len < 80)
		g_string_append_c (string, '-');

	printf ("%s\n", string->str);

	g_string_free (string, TRUE);
}

static void
print_string (const char *label, const char *data)
{
#define SPACING 18
	int label_len = 0;
	char spaces[50];
	int i;

	g_return_if_fail (label != NULL);
	g_return_if_fail (data != NULL);

	label_len = strlen (label);
	if (label_len > SPACING)
		label_len = SPACING - 1;
	for (i = 0; i < (SPACING - label_len); i++)
		spaces[i] = 0x20;
	spaces[i] = 0x00;

	printf ("  %s:%s%s\n", label, &spaces[0], data);
}

static const char *
get_dev_state_string (BMDeviceState state)
{
	if (state == BM_DEVICE_STATE_UNMANAGED)
		return "unmanaged";
	else if (state == BM_DEVICE_STATE_UNAVAILABLE)
		return "unavailable";
	else if (state == BM_DEVICE_STATE_DISCONNECTED)
		return "disconnected";
	else if (state == BM_DEVICE_STATE_PREPARE)
		return "connecting (prepare)";
	else if (state == BM_DEVICE_STATE_CONFIG)
		return "connecting (configuring)";
	else if (state == BM_DEVICE_STATE_NEED_AUTH)
		return "connecting (need authentication)";
	else if (state == BM_DEVICE_STATE_IP_CONFIG)
		return "connecting (getting IP configuration)";
	else if (state == BM_DEVICE_STATE_ACTIVATED)
		return "connected";
	else if (state == BM_DEVICE_STATE_FAILED)
		return "connection failed";
	return "unknown";
}

static BMConnection *
get_connection_for_active (BMActiveConnection *active)
{
	BMConnectionScope scope;
	const char *path;

	g_return_val_if_fail (active != NULL, NULL);

	path = bm_active_connection_get_connection (active);
	g_return_val_if_fail (path != NULL, NULL);

	scope = bm_active_connection_get_scope (active);
	if (scope == BM_CONNECTION_SCOPE_USER)
		return (BMConnection *) g_hash_table_lookup (user_connections, path);
	else if (scope == BM_CONNECTION_SCOPE_SYSTEM)
		return (BMConnection *) g_hash_table_lookup (system_connections, path);

	g_warning ("error: unknown connection scope");
	return NULL;
}

struct cb_info {
	BMClient *client;
	const GPtrArray *active;
};

static void
detail_device (gpointer data, gpointer user_data)
{
	BMDevice *device = BM_DEVICE (data);
	struct cb_info *info = user_data;
	char *tmp;
	BMDeviceState state;
	guint32 caps;
	guint32 speed;
	const GArray *array;
	int j;
	gboolean is_default = FALSE;
	const char *id = NULL;

	state = bm_device_get_state (device);

	for (j = 0; info->active && (j < info->active->len); j++) {
		BMActiveConnection *candidate = g_ptr_array_index (info->active, j);
		const GPtrArray *devices = bm_active_connection_get_devices (candidate);
		BMDevice *candidate_dev;
		BMConnection *connection;
		BMSettingConnection *s_con;

		if (!devices || !devices->len)
			continue;
		candidate_dev = g_ptr_array_index (devices, 0);

		if (candidate_dev == device) {
			if (bm_active_connection_get_default (candidate))
				is_default = TRUE;

			connection = get_connection_for_active (candidate);
			if (!connection)
				break;

			s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
			if (s_con)
				id = bm_setting_connection_get_id (s_con);
			break;
		}
	}

	print_header ("Device", bm_device_get_iface (device), id);

	/* General information */
	if (BM_IS_DEVICE_BT (device))
		print_string ("Type", "Bluetooth");

	print_string ("Driver", bm_device_get_driver (device) ? bm_device_get_driver (device) : "(unknown)");

	print_string ("State", get_dev_state_string (state));

	if (is_default)
		print_string ("Default", "yes");
	else
		print_string ("Default", "no");

	/* Capabilities */
	caps = bm_device_get_capabilities (device);
	printf ("\n  Capabilities:\n");

	printf ("\n\n");
}

static void
get_one_connection (DBusGConnection *bus,
                    const char *path,
                    BMConnectionScope scope,
                    GHashTable *table)
{
	DBusGProxy *proxy;
	BMConnection *connection = NULL;
	const char *service;
	GError *error = NULL;
	GHashTable *settings = NULL;

	g_return_if_fail (bus != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (table != NULL);

	service = (scope == BM_CONNECTION_SCOPE_SYSTEM) ?
		BM_DBUS_SERVICE_SYSTEM_SETTINGS : BM_DBUS_SERVICE_USER_SETTINGS;

	proxy = dbus_g_proxy_new_for_name (bus, service, path, BM_DBUS_IFACE_SETTINGS_CONNECTION);
	if (!proxy)
		return;

	if (!dbus_g_proxy_call (proxy, "GetSettings", &error,
	                        G_TYPE_INVALID,
	                        DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT, &settings,
	                        G_TYPE_INVALID)) {
		bm_warning ("error: cannot retrieve connection: %s", error ? error->message : "(unknown)");
		goto out;
	}

	connection = bm_connection_new_from_hash (settings, &error);
	g_hash_table_destroy (settings);

	if (!connection) {
		bm_warning ("error: invalid connection: '%s' / '%s' invalid: %d",
		            error ? g_type_name (bm_connection_lookup_setting_type_by_quark (error->domain)) : "(unknown)",
		            error ? error->message : "(unknown)",
		            error ? error->code : -1);
		goto out;
	}

	bm_connection_set_scope (connection, scope);
	bm_connection_set_path (connection, path);
	g_hash_table_insert (table, g_strdup (path), g_object_ref (connection));

out:
	g_clear_error (&error);
	if (connection)
		g_object_unref (connection);
	g_object_unref (proxy);
}

static void
get_connections_for_service (DBusGConnection *bus,
                             BMConnectionScope scope,
                             GHashTable *table)
{
	GError *error = NULL;
	DBusGProxy *proxy;
	GPtrArray *paths = NULL;
	int i;
	const char *service;

	service = (scope == BM_CONNECTION_SCOPE_SYSTEM) ?
		BM_DBUS_SERVICE_SYSTEM_SETTINGS : BM_DBUS_SERVICE_USER_SETTINGS;

	proxy = dbus_g_proxy_new_for_name (bus,
	                                   service,
	                                   BM_DBUS_PATH_SETTINGS,
	                                   BM_DBUS_IFACE_SETTINGS);
	if (!proxy) {
		g_warning ("error: failed to create DBus proxy for %s", service);
		return;
	}

	if (!dbus_g_proxy_call (proxy, "ListConnections", &error,
                                G_TYPE_INVALID,
                                DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH, &paths,
                                G_TYPE_INVALID)) {
		/* No connections or settings service may not be running */
		g_clear_error (&error);
		goto out;
	}

	for (i = 0; paths && (i < paths->len); i++)
		get_one_connection (bus, g_ptr_array_index (paths, i), scope, table);

out:
	g_object_unref (proxy);
}

static gboolean
get_all_connections (void)
{
	DBusGConnection *bus;
	GError *error = NULL;

	bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (error || !bus) {
		g_warning ("error: could not connect to dbus");
		return FALSE;
	}

	user_connections = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	get_connections_for_service (bus, BM_CONNECTION_SCOPE_USER, user_connections);

	system_connections = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	get_connections_for_service (bus, BM_CONNECTION_SCOPE_SYSTEM, system_connections);

	dbus_g_connection_unref (bus);
	return TRUE;
}

int
main (int argc, char *argv[])
{
	BMClient *client;
	const GPtrArray *devices;
	struct cb_info info;

	g_type_init ();

	client = bm_client_new ();
	if (!client) {
		exit (1);
	}

	printf ("\nBarcodeManager Tool\n\n");

	if (!get_bm_state (client)) {
		g_warning ("error: could not connect to BarcodeManager");
		exit (1);
	}

	if (!get_all_connections ())
		exit (1);

	info.client = client;
	info.active = bm_client_get_active_connections (client);


	devices = bm_client_get_devices (client);
	if (devices)
		g_ptr_array_foreach ((GPtrArray *) devices, detail_device, &info);

	g_object_unref (client);
	g_hash_table_unref (user_connections);
	g_hash_table_unref (system_connections);

	return 0;
}
