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
#include <unistd.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gnome-keyring.h>

#include <bm-setting-connection.h>

#include "bma-gconf-connection.h"
#include "gconf-helpers.h"
#include "bm-utils.h"
#include "utils.h"
#include "bma-marshal.h"
#include "bm-settings-interface.h"

G_DEFINE_TYPE (BMAGConfConnection, bma_gconf_connection, BM_TYPE_EXPORTED_CONNECTION)

#define BMA_GCONF_CONNECTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BMA_TYPE_GCONF_CONNECTION, BMAGConfConnectionPrivate))

typedef struct {
	GConfClient *client;
	char *dir;

	gboolean disposed;
} BMAGConfConnectionPrivate;

enum {
	PROP_0,
	PROP_CLIENT,
	PROP_DIR,

	LAST_PROP
};

enum {
	NEW_SECRETS_REQUESTED,

	LAST_SIGNAL
};

BMAGConfConnection *
bma_gconf_connection_new (GConfClient *client, const char *conf_dir)
{
	BMConnection *connection;
	BMAGConfConnection *gconf_connection;

	g_return_val_if_fail (GCONF_IS_CLIENT (client), NULL);
	g_return_val_if_fail (conf_dir != NULL, NULL);

	/* retrieve GConf data */
	connection = bm_gconf_read_connection (client, conf_dir);
	if (connection) {
		gconf_connection = bma_gconf_connection_new_from_connection (client, conf_dir, connection);
		g_object_unref (connection);
	} else {
		bm_warning ("No connection read from GConf at %s.", conf_dir);
		gconf_connection = NULL;
	}
	
	return gconf_connection;
}

BMAGConfConnection *
bma_gconf_connection_new_from_connection (GConfClient *client,
                                          const char *conf_dir,
                                          BMConnection *connection)
{
	GObject *object;
	BMAGConfConnection *self;
	GError *error = NULL;
	gboolean success;
	GHashTable *settings;

	g_return_val_if_fail (GCONF_IS_CLIENT (client), NULL);
	g_return_val_if_fail (conf_dir != NULL, NULL);
	g_return_val_if_fail (BM_IS_CONNECTION (connection), NULL);

	/* Ensure the connection is valid first */
	success = bm_connection_verify (connection, &error);
	if (!success) {
		g_warning ("Invalid connection %s: '%s' / '%s' invalid: %d",
		           conf_dir,
		           g_type_name (bm_connection_lookup_setting_type_by_quark (error->domain)),
		           (error && error->message) ? error->message : "(unknown)",
		           error ? error->code : -1);
		g_clear_error (&error);
		return NULL;
	}

	object = g_object_new (BMA_TYPE_GCONF_CONNECTION,
	                       BMA_GCONF_CONNECTION_CLIENT, client,
	                       BMA_GCONF_CONNECTION_DIR, conf_dir,
	                       BM_CONNECTION_SCOPE, BM_CONNECTION_SCOPE_USER,
	                       NULL);
	if (!object)
		return NULL;

	self = BMA_GCONF_CONNECTION (object);

	/* Fill certs so that the bm_connection_replace_settings verification works */
	settings = bm_connection_to_hash (connection);
	success = bm_connection_replace_settings (BM_CONNECTION (self), settings, NULL);
	g_hash_table_destroy (settings);

	/* Already verified the settings above, they had better be OK */
	g_assert (success);

	return self;
}

const char *
bma_gconf_connection_get_gconf_path (BMAGConfConnection *self)
{
	g_return_val_if_fail (BMA_IS_GCONF_CONNECTION (self), NULL);

	return BMA_GCONF_CONNECTION_GET_PRIVATE (self)->dir;
}

gboolean
bma_gconf_connection_gconf_changed (BMAGConfConnection *self)
{
	BMAGConfConnectionPrivate *priv = BMA_GCONF_CONNECTION_GET_PRIVATE (self);
	BMConnection *new;
	GHashTable *new_settings;
	GError *error = NULL;
	gboolean success;

	new = bm_gconf_read_connection (priv->client, priv->dir);
	if (!new) {
		g_warning ("No connection read from GConf at %s.", priv->dir);
		goto invalid;
	}

	success = bm_connection_verify (new, &error);
	if (!success) {
		g_warning ("%s: Invalid connection %s: '%s' / '%s' invalid: %d",
		           __func__, priv->dir,
		           g_type_name (bm_connection_lookup_setting_type_by_quark (error->domain)),
		           error->message, error->code);
		g_object_unref (new);
		goto invalid;
	}

	/* Ignore the GConf update if nothing changed */
	if (bm_connection_compare (BM_CONNECTION (self), new, BM_SETTING_COMPARE_FLAG_EXACT)) {
		g_object_unref (new);
		return TRUE;
	}

	new_settings = bm_connection_to_hash (new);
	success = bm_connection_replace_settings (BM_CONNECTION (self), new_settings, &error);
	g_hash_table_destroy (new_settings);
	g_object_unref (new);

	if (!success) {
		g_warning ("%s: '%s' / '%s' invalid: %d",
		           __func__,
		           error ? g_type_name (bm_connection_lookup_setting_type_by_quark (error->domain)) : "(none)",
		           (error && error->message) ? error->message : "(none)",
		           error ? error->code : -1);
		goto invalid;
	}

	bm_settings_connection_interface_emit_updated (BM_SETTINGS_CONNECTION_INTERFACE (self));
	return TRUE;

invalid:
	g_clear_error (&error);
	g_signal_emit_by_name (self, BM_SETTINGS_CONNECTION_INTERFACE_REMOVED);
	return FALSE;
}

/******************************************************/

#define FILE_TAG "file://"

void
bma_gconf_connection_update (BMAGConfConnection *self,
                             gboolean ignore_secrets)
{
	BMAGConfConnectionPrivate *priv = BMA_GCONF_CONNECTION_GET_PRIVATE (self);

	bm_gconf_write_connection (BM_CONNECTION (self),
	                           priv->client,
	                           priv->dir,
	                           ignore_secrets);
	gconf_client_notify (priv->client, priv->dir);
	gconf_client_suggest_sync (priv->client, NULL);
}

/******************************************************/

static gboolean
is_dbus_request_authorized (DBusGMethodInvocation *context,
                            gboolean allow_user,
                            GError **error)
{
	DBusGConnection *bus = NULL;
	DBusConnection *connection = NULL;
	char *sender = NULL;
	gulong sender_uid = G_MAXULONG;
	DBusError dbus_error;
	gboolean success = FALSE;

	sender = dbus_g_method_get_sender (context);
	if (!sender) {
		g_set_error (error, BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		             "%s", "Could not determine D-Bus requestor");
		goto out;
	}

	bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);
	if (!bus) {
		g_set_error (error, BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		             "%s", "Could not get the system bus");
		goto out;
	}
	connection = dbus_g_connection_get_connection (bus);
	if (!connection) {
		g_set_error (error, BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		             "%s", "Could not get the D-Bus system bus");
		goto out;
	}

	dbus_error_init (&dbus_error);
	sender_uid = dbus_bus_get_unix_user (connection, sender, &dbus_error);
	if (dbus_error_is_set (&dbus_error)) {
		dbus_error_free (&dbus_error);
		g_set_error (error, BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_PERMISSION_DENIED,
		             "%s", "Could not determine the Unix user ID of the requestor");
		goto out;
	}

	/* And finally, the actual UID check */
	if (   (allow_user && (sender_uid == geteuid()))
	    || (sender_uid == 0))
		success = TRUE;
	else {
		g_set_error (error, BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_PERMISSION_DENIED,
		             "%s", "Requestor UID does not match the UID of the user settings service");
	}

out:
	if (bus)
		dbus_g_connection_unref (bus);
	g_free (sender);
	return success;
}

static void
con_update_cb (BMSettingsConnectionInterface *connection,
               GError *error,
               gpointer user_data)
{
	DBusGMethodInvocation *context = user_data;

	if (error)
		dbus_g_method_return_error (context, error);
	else
		dbus_g_method_return (context);
}

static void
dbus_update (BMExportedConnection *exported,
             GHashTable *new_settings,
             DBusGMethodInvocation *context)
{
	BMAGConfConnection *self = BMA_GCONF_CONNECTION (exported);
	BMConnection *new;
	gboolean success = FALSE;
	GError *error = NULL;

	/* Restrict Update to execution by the current user and root for DBus invocation */
	if (!is_dbus_request_authorized (context, TRUE, &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	new = bm_connection_new_from_hash (new_settings, &error);
	if (!new) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}
	g_object_unref (new);
	
	success = bm_connection_replace_settings (BM_CONNECTION (self), new_settings, NULL);
	/* Settings better be valid; we verified them above */
	g_assert (success);

	bm_settings_connection_interface_update (BM_SETTINGS_CONNECTION_INTERFACE (self),
	                                         con_update_cb,
	                                         context);
}

static void
con_delete_cb (BMSettingsConnectionInterface *connection,
               GError *error,
               gpointer user_data)
{
	DBusGMethodInvocation *context = user_data;

	if (error)
		dbus_g_method_return_error (context, error);
	else
		dbus_g_method_return (context);
}

static void
dbus_delete (BMExportedConnection *exported,
             DBusGMethodInvocation *context)
{
	BMAGConfConnection *self = BMA_GCONF_CONNECTION (exported);
	GError *error = NULL;

	/* Restrict Update to execution by the current user and root for DBus invocation */
	if (!is_dbus_request_authorized (context, TRUE, &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	bm_settings_connection_interface_delete (BM_SETTINGS_CONNECTION_INTERFACE (self),
	                                         con_delete_cb,
	                                         context);
}

/************************************************************/

static void
bma_gconf_connection_init (BMAGConfConnection *connection)
{
}

static GObject *
constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
	GObject *object;
	BMAGConfConnectionPrivate *priv;

	object = G_OBJECT_CLASS (bma_gconf_connection_parent_class)->constructor (type, n_construct_params, construct_params);

	if (!object)
		return NULL;

	priv = BMA_GCONF_CONNECTION_GET_PRIVATE (object);

	if (!priv->client) {
		bm_warning ("GConfClient not provided.");
		g_object_unref (object);
		return NULL;
	}

	if (!priv->dir) {
		bm_warning ("GConf directory not provided.");
		g_object_unref (object);
		return NULL;
	}

	return object;
}

static void
dispose (GObject *object)
{
	BMAGConfConnectionPrivate *priv = BMA_GCONF_CONNECTION_GET_PRIVATE (object);

	if (priv->disposed)
		return;
	priv->disposed = TRUE;

	g_object_unref (priv->client);

	G_OBJECT_CLASS (bma_gconf_connection_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	BMAGConfConnectionPrivate *priv = BMA_GCONF_CONNECTION_GET_PRIVATE (object);

	g_free (priv->dir);

	G_OBJECT_CLASS (bma_gconf_connection_parent_class)->finalize (object);
}

static void
set_property (GObject *object, guint prop_id,
		    const GValue *value, GParamSpec *pspec)
{
	BMAGConfConnectionPrivate *priv = BMA_GCONF_CONNECTION_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_CLIENT:
		/* Construct only */
		priv->client = GCONF_CLIENT (g_value_dup_object (value));
		break;
	case PROP_DIR:
		/* Construct only */
		priv->dir = g_value_dup_string (value);
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
	BMAGConfConnectionPrivate *priv = BMA_GCONF_CONNECTION_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_CLIENT:
		g_value_set_object (value, priv->client);
		break;
	case PROP_DIR:
		g_value_set_string (value, priv->dir);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bma_gconf_connection_class_init (BMAGConfConnectionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	BMExportedConnectionClass *ec_class = BM_EXPORTED_CONNECTION_CLASS (class);

	g_type_class_add_private (class, sizeof (BMAGConfConnectionPrivate));

	/* Virtual methods */
	object_class->constructor  = constructor;
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose      = dispose;
	object_class->finalize     = finalize;

	ec_class->update = dbus_update;
	ec_class->delete = dbus_delete;

	/* Properties */
	g_object_class_install_property
		(object_class, PROP_CLIENT,
		 g_param_spec_object (BMA_GCONF_CONNECTION_CLIENT,
						  "GConfClient",
						  "GConfClient",
						  GCONF_TYPE_CLIENT,
						  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class, PROP_DIR,
		 g_param_spec_string (BMA_GCONF_CONNECTION_DIR,
						  "GConf directory",
						  "GConf directory",
						  NULL,
						  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
