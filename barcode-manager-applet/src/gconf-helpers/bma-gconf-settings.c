/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Applet -- Display barcode scanners and allow user control
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

#include <string.h>
#include <stdio.h>

#include "bma-gconf-settings.h"
#include "gconf-helpers.h"
#include "bma-marshal.h"
#include "bm-utils.h"

G_DEFINE_TYPE (BMAGConfSettings, bma_gconf_settings, BM_TYPE_SETTINGS_SERVICE)

#define BMA_GCONF_SETTINGS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BMA_TYPE_GCONF_SETTINGS, BMAGConfSettingsPrivate))

typedef struct {
	GConfClient *client;
	guint conf_notify_id;	GSList *connections;
	guint read_connections_id;
	GHashTable *pending_changes;

	gboolean disposed;
} BMAGConfSettingsPrivate;

enum {
	NEW_SECRETS_REQUESTED,

	LAST_SIGNAL
};

//static guint signals[LAST_SIGNAL] = { 0 };


BMAGConfSettings *
bma_gconf_settings_new (DBusGConnection *bus)
{
	return (BMAGConfSettings *) g_object_new (BMA_TYPE_GCONF_SETTINGS,
	                                          BM_SETTINGS_SERVICE_SCOPE, BM_CONNECTION_SCOPE_USER,
	                                          BM_SETTINGS_SERVICE_BUS, bus,
	                                          NULL);
}

static void
connection_removed (BMExportedConnection *connection, gpointer user_data)
{
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (user_data);

	priv->connections = g_slist_remove (priv->connections, connection);
	g_object_unref (connection);
}

static void
internal_add_connection (BMAGConfSettings *self, BMAGConfConnection *connection)
{
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (self);
	DBusGConnection *bus = NULL;

	g_return_if_fail (connection != NULL);
	g_return_if_fail (BMA_IS_GCONF_CONNECTION (connection));

	priv->connections = g_slist_prepend (priv->connections, connection);

	g_signal_connect (connection, "removed", G_CALLBACK (connection_removed), self);

	g_object_get (G_OBJECT (self), BM_SETTINGS_SERVICE_BUS, &bus, NULL);
	if (bus) {
		bm_settings_service_export_connection (BM_SETTINGS_SERVICE (self),
		                                       BM_SETTINGS_CONNECTION_INTERFACE (connection));
		dbus_g_connection_unref (bus);
	}

	g_signal_emit_by_name (self, BM_SETTINGS_INTERFACE_NEW_CONNECTION, BM_CONNECTION (connection));
	g_return_if_fail (BMA_IS_GCONF_CONNECTION (priv->connections->data));
}

static void
update_cb (BMSettingsConnectionInterface *connection,
           GError *error,
           gpointer user_data)
{
	if (error) {
		g_warning ("%s: %s:%d error updating connection %s: (%d) %s",
		           __func__, __FILE__, __LINE__,
		           bma_gconf_connection_get_gconf_path (BMA_GCONF_CONNECTION (connection)),
		           error ? error->code : -1,
		           (error && error->message) ? error->message : "(unknown)");
	}
}

BMAGConfConnection *
bma_gconf_settings_add_connection (BMAGConfSettings *self, BMConnection *connection)
{
	BMAGConfSettingsPrivate *priv;
	BMAGConfConnection *exported;
	guint32 i = 0;
	char *path = NULL;

	g_return_val_if_fail (BMA_IS_GCONF_SETTINGS (self), NULL);
	g_return_val_if_fail (BM_IS_CONNECTION (connection), NULL);

	priv = BMA_GCONF_SETTINGS_GET_PRIVATE (self);

	/* Find free GConf directory */
	while (i++ < G_MAXUINT32) {
		char buf[255];

		snprintf (&buf[0], 255, GCONF_PATH_CONNECTIONS"/%d", i);
		if (!gconf_client_dir_exists (priv->client, buf, NULL)) {
			path = g_strdup (buf);
			break;
		}
	}

	if (path == NULL) {
		bm_warning ("Couldn't find free GConf directory for new connection.");
		return NULL;
	}

	exported = bma_gconf_connection_new_from_connection (priv->client, path, connection);
	g_free (path);
	if (exported) {
		internal_add_connection (self, exported);

		/* Must save connection to GConf _after_ adding it to the connections
		 * list to avoid races with GConf notifications.
		 */
		bm_settings_connection_interface_update (BM_SETTINGS_CONNECTION_INTERFACE (exported), update_cb, NULL);
	}

	return exported;
}

static void
add_connection (BMSettingsService *settings,
                BMConnection *connection,
                DBusGMethodInvocation *context, /* Only present for D-Bus calls */
                BMSettingsAddConnectionFunc callback,
                gpointer user_data)
{
	BMAGConfSettings *self = BMA_GCONF_SETTINGS (settings);

	/* For now, we don't support additions via D-Bus until we figure out
	 * the security implications.
	 */
	if (context) {
		GError *error;
		GQuark domain = g_quark_from_string("org.freedesktop.BarcodeManagerSettings.AddFailed");

		error = g_error_new (domain, 0, "%s: adding connections via D-Bus is not (yet) supported", __func__);
		callback (BM_SETTINGS_INTERFACE (settings), error, user_data);
		g_error_free (error);
		return;
	}

	bma_gconf_settings_add_connection (self, connection);
	callback (BM_SETTINGS_INTERFACE (settings), NULL, user_data);
}

static BMAGConfConnection *
get_connection_by_gconf_path (BMAGConfSettings *self, const char *path)
{
	BMAGConfSettingsPrivate *priv;
	GSList *iter;

	g_return_val_if_fail (BMA_IS_GCONF_SETTINGS (self), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	priv = BMA_GCONF_SETTINGS_GET_PRIVATE (self);
	for (iter = priv->connections; iter; iter = iter->next) {
		BMAGConfConnection *connection = BMA_GCONF_CONNECTION (iter->data);
		const char *gconf_path;

		gconf_path = bma_gconf_connection_get_gconf_path (connection);
		if (gconf_path && !strcmp (gconf_path, path))
			return connection;
	}

	return NULL;
}

static void
read_connections (BMAGConfSettings *settings)
{
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (settings);
	GSList *dir_list;
	GSList *iter;

	dir_list = bm_gconf_get_all_connections (priv->client);
	if (!dir_list)
		return;

	for (iter = dir_list; iter; iter = iter->next) {
		char *dir = (char *) iter->data;
		BMAGConfConnection *connection;

		connection = bma_gconf_connection_new (priv->client, dir);
		if (connection)
			internal_add_connection (settings, connection);
		g_free (dir);
	}

	g_slist_free (dir_list);
	priv->connections = g_slist_reverse (priv->connections);
}

static gboolean
read_connections_cb (gpointer data)
{
	BMA_GCONF_SETTINGS_GET_PRIVATE (data)->read_connections_id = 0;
	read_connections (BMA_GCONF_SETTINGS (data));

	return FALSE;
}

static GSList *
list_connections (BMSettingsService *settings)
{
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (settings);

	if (priv->read_connections_id) {
		g_source_remove (priv->read_connections_id);
		priv->read_connections_id = 0;

		read_connections (BMA_GCONF_SETTINGS (settings));
	}

	return g_slist_copy (priv->connections);
}

typedef struct {
	BMAGConfSettings *settings;
	char *path;
} ConnectionChangedInfo;

static void
connection_changed_info_destroy (gpointer data)
{
	ConnectionChangedInfo *info = (ConnectionChangedInfo *) data;

	g_free (info->path);
	g_free (info);
}

static gboolean
connection_changes_done (gpointer data)
{
	ConnectionChangedInfo *info = (ConnectionChangedInfo *) data;
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (info->settings);
	BMAGConfConnection *connection;

	connection = get_connection_by_gconf_path (info->settings, info->path);
	if (!connection) {
		/* New connection */
		connection = bma_gconf_connection_new (priv->client, info->path);
		if (connection)
			internal_add_connection (info->settings, connection);
	} else {
		/* Updated or removed/invalid connection */
		if (!bma_gconf_connection_gconf_changed (connection))
			priv->connections = g_slist_remove (priv->connections, connection);
	}

	g_hash_table_remove (priv->pending_changes, info->path);

	return FALSE;
}

static void
connections_changed_cb (GConfClient *conf_client,
                        guint cnxn_id,
                        GConfEntry *entry,
                        gpointer user_data)
{
	BMAGConfSettings *self = BMA_GCONF_SETTINGS (user_data);
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (self);
	char **dirs = NULL;
	guint len;
	char *path = NULL;

	dirs = g_strsplit (gconf_entry_get_key (entry), "/", -1);
	len = g_strv_length (dirs);
	if (len < 5)
		goto out;

	if (   strcmp (dirs[0], "")
	    || strcmp (dirs[1], "system")
	    || strcmp (dirs[2], "networking")
	    || strcmp (dirs[3], "connections"))
		goto out;

	path = g_strconcat ("/", dirs[1], "/", dirs[2], "/", dirs[3], "/", dirs[4], NULL);

	if (!g_hash_table_lookup (priv->pending_changes, path)) {
		ConnectionChangedInfo *info;
		guint id;

		info = g_new (ConnectionChangedInfo, 1);
		info->settings = self;
		info->path = path;
		path = NULL;

		id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, connection_changes_done, 
						  info, connection_changed_info_destroy);
		g_hash_table_insert (priv->pending_changes, info->path, GUINT_TO_POINTER (id));
	}

out:
	g_free (path);
	g_strfreev (dirs);
}

static void
remove_pending_change (gpointer data)
{
	g_source_remove (GPOINTER_TO_UINT (data));
}

/************************************************************/

static void
bma_gconf_settings_init (BMAGConfSettings *self)
{
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (self);

	priv->client = gconf_client_get_default ();
	priv->pending_changes = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, remove_pending_change);

	gconf_client_add_dir (priv->client,
	                      GCONF_PATH_CONNECTIONS,
	                      GCONF_CLIENT_PRELOAD_NONE,
	                      NULL);

	priv->conf_notify_id = gconf_client_notify_add (priv->client,
	                                                GCONF_PATH_CONNECTIONS,
	                                                (GConfClientNotifyFunc) connections_changed_cb,
	                                                self,
	                                                NULL, NULL);
}

static GObject *
constructor (GType type,
		   guint n_construct_params,
		   GObjectConstructParam *construct_params)
{
	GObject *object;

	object = G_OBJECT_CLASS (bma_gconf_settings_parent_class)->constructor (type, n_construct_params, construct_params);
	if (object)
		BMA_GCONF_SETTINGS_GET_PRIVATE (object)->read_connections_id = g_idle_add (read_connections_cb, object);
	return object;
}

static void
dispose (GObject *object)
{
	BMAGConfSettingsPrivate *priv = BMA_GCONF_SETTINGS_GET_PRIVATE (object);

	if (priv->disposed)
		return;

	priv->disposed = TRUE;

	g_hash_table_destroy (priv->pending_changes);

	if (priv->read_connections_id) {
		g_source_remove (priv->read_connections_id);
		priv->read_connections_id = 0;
	}

	gconf_client_notify_remove (priv->client, priv->conf_notify_id);
	gconf_client_remove_dir (priv->client, GCONF_PATH_CONNECTIONS, NULL);

	g_slist_foreach (priv->connections, (GFunc) g_object_unref, NULL);
	g_slist_free (priv->connections);

	g_object_unref (priv->client);

	G_OBJECT_CLASS (bma_gconf_settings_parent_class)->dispose (object);
}

static void
bma_gconf_settings_class_init (BMAGConfSettingsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	BMSettingsServiceClass *settings_class = BM_SETTINGS_SERVICE_CLASS (class);

	g_type_class_add_private (class, sizeof (BMAGConfSettingsPrivate));

	/* Virtual methods */
	object_class->constructor = constructor;
	object_class->dispose = dispose;

	settings_class->list_connections = list_connections;
	settings_class->add_connection = add_connection;
}
