/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- Network link manager
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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2009 Red Hat, Inc.
 */

#include "bm-settings-connection-interface.h"
#include "bm-dbus-glib-types.h"

/**
 * bm_settings_connection_interface_update:
 * @connection: an object implementing #BMSettingsConnectionInterface
 * @callback: a function to be called when the update completes
 * @user_data: caller-specific data to be passed to @callback
 *
 * Update the connection with current settings and properties.
 *
 * Returns: TRUE on success, FALSE on failure
 **/
gboolean
bm_settings_connection_interface_update (BMSettingsConnectionInterface *connection,
                                         BMSettingsConnectionInterfaceUpdateFunc callback,
                                         gpointer user_data)
{
	g_return_val_if_fail (connection != NULL, FALSE);
	g_return_val_if_fail (BM_IS_SETTINGS_CONNECTION_INTERFACE (connection), FALSE);
	g_return_val_if_fail (callback != NULL, FALSE);

	if (BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->update) {
		return BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->update (connection,
		                                                                            callback,
		                                                                            user_data);
	}
	return FALSE;
}

/**
 * bm_settings_connection_interface_delete:
 * @connection: a objecting implementing #BMSettingsConnectionInterface
 * @callback: a function to be called when the delete completes
 * @user_data: caller-specific data to be passed to @callback
 *
 * Delete the connection.
 *
 * Returns: TRUE on success, FALSE on failure
 **/
gboolean
bm_settings_connection_interface_delete (BMSettingsConnectionInterface *connection,
                                         BMSettingsConnectionInterfaceDeleteFunc callback,
                                         gpointer user_data)
{
	g_return_val_if_fail (connection != NULL, FALSE);
	g_return_val_if_fail (BM_IS_SETTINGS_CONNECTION_INTERFACE (connection), FALSE);
	g_return_val_if_fail (callback != NULL, FALSE);

	if (BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->delete) {
		return BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->delete (connection,
		                                                                            callback,
		                                                                            user_data);
	}
	return FALSE;
}

/**
 * bm_settings_connection_interface_get_secrets:
 * @connection: a object implementing #BMSettingsConnectionInterface
 * @setting_name: the #BMSetting object name to get secrets for
 * @hints: #BMSetting key names to get secrets for (optional)
 * @request_new: hint that new secrets (instead of cached or stored secrets) 
 *  should be returned
 * @callback: a function to be called when the update completes
 * @user_data: caller-specific data to be passed to @callback
 *
 * Request the connection's secrets.
 *
 * Returns: TRUE on success, FALSE on failure
 **/
gboolean
bm_settings_connection_interface_get_secrets (BMSettingsConnectionInterface *connection,
                                              const char *setting_name,
                                              const char **hints,
                                              gboolean request_new,
                                              BMSettingsConnectionInterfaceGetSecretsFunc callback,
                                              gpointer user_data)
{
	g_return_val_if_fail (connection != NULL, FALSE);
	g_return_val_if_fail (BM_IS_SETTINGS_CONNECTION_INTERFACE (connection), FALSE);
	g_return_val_if_fail (callback != NULL, FALSE);

	if (BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->get_secrets) {
		return BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->get_secrets (connection,
		                                                                                 setting_name,
		                                                                                 hints,
		                                                                                 request_new,
		                                                                                 callback,
		                                                                                 user_data);
	}
	return FALSE;
}

void
bm_settings_connection_interface_emit_updated (BMSettingsConnectionInterface *connection)
{
	if (BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->emit_updated)
		BM_SETTINGS_CONNECTION_INTERFACE_GET_INTERFACE (connection)->emit_updated (connection);
	else {
		BMConnection *tmp;
		GHashTable *settings;

		tmp = bm_connection_duplicate (BM_CONNECTION (connection));
		bm_connection_clear_secrets (tmp);
		settings = bm_connection_to_hash (tmp);
		g_object_unref (tmp);

		g_signal_emit_by_name (connection, BM_SETTINGS_CONNECTION_INTERFACE_UPDATED, settings);
		g_hash_table_destroy (settings);
	}
}

static void
bm_settings_connection_interface_init (gpointer g_iface)
{
	GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	/* Signals */
	g_signal_new (BM_SETTINGS_CONNECTION_INTERFACE_UPDATED,
				  iface_type,
				  G_SIGNAL_RUN_FIRST,
				  G_STRUCT_OFFSET (BMSettingsConnectionInterface, updated),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__BOXED,
				  G_TYPE_NONE, 1, DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT);

	g_signal_new (BM_SETTINGS_CONNECTION_INTERFACE_REMOVED,
				  iface_type,
				  G_SIGNAL_RUN_FIRST,
				  G_STRUCT_OFFSET (BMSettingsConnectionInterface, removed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__VOID,
				  G_TYPE_NONE, 0);

	initialized = TRUE;
}

GType
bm_settings_connection_interface_get_type (void)
{
	static GType itype = 0;

	if (!itype) {
		const GTypeInfo iinfo = {
			sizeof (BMSettingsConnectionInterface), /* class_size */
			bm_settings_connection_interface_init,   /* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			0,
			0,              /* n_preallocs */
			NULL
		};

		itype = g_type_register_static (G_TYPE_INTERFACE,
		                                "BMSettingsConnectionInterface",
		                                &iinfo, 0);

		g_type_interface_add_prerequisite (itype, BM_TYPE_CONNECTION);
	}

	return itype;
}

