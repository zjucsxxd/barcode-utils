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
 * (C) Copyright 2008 - 2009 Red Hat, Inc.
 */

#include <string.h>
#include <BarcodeManager.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "bm-settings-service.h"
#include "bm-settings-interface.h"
#include "bm-exported-connection.h"

static gboolean impl_settings_list_connections (BMSettingsService *self,
                                                GPtrArray **connections,
                                                GError **error);

static void impl_settings_add_connection (BMSettingsService *self,
                                          GHashTable *settings,
                                          DBusGMethodInvocation *context);

#include "bm-settings-glue.h"

static void settings_interface_init (BMSettingsInterface *class);

G_DEFINE_TYPE_EXTENDED (BMSettingsService, bm_settings_service, G_TYPE_OBJECT, G_TYPE_FLAG_ABSTRACT,
                        G_IMPLEMENT_INTERFACE (BM_TYPE_SETTINGS_INTERFACE, settings_interface_init))

#define BM_SETTINGS_SERVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                            BM_TYPE_SETTINGS_SERVICE, \
                                            BMSettingsServicePrivate))

typedef struct {
	DBusGConnection *bus;
	BMConnectionScope scope;
	gboolean exported;

	gboolean disposed;
} BMSettingsServicePrivate;

enum {
	PROP_0,
	PROP_BUS,
	PROP_SCOPE,

	LAST_PROP
};


/**************************************************************/

void
bm_settings_service_export (BMSettingsService *self)
{
	BMSettingsServicePrivate *priv;

	g_return_if_fail (self != NULL);
	g_return_if_fail (BM_IS_SETTINGS_SERVICE (self));

	priv = BM_SETTINGS_SERVICE_GET_PRIVATE (self);

	g_return_if_fail (priv->bus != NULL);

	/* Don't allow exporting twice */
	g_return_if_fail (priv->exported == FALSE);

	dbus_g_connection_register_g_object (priv->bus,
	                                     BM_DBUS_PATH_SETTINGS,
	                                     G_OBJECT (self));
	priv->exported = TRUE;
}

/**************************************************************/

static GSList *
list_connections (BMSettingsInterface *settings)
{
	/* Must always be implemented */
	g_assert (BM_SETTINGS_SERVICE_GET_CLASS (settings)->list_connections);
	return BM_SETTINGS_SERVICE_GET_CLASS (settings)->list_connections (BM_SETTINGS_SERVICE (settings));
}

static gboolean
impl_settings_list_connections (BMSettingsService *self,
                                GPtrArray **connections,
                                GError **error)
{
	GSList *list = NULL, *iter;

	list = list_connections (BM_SETTINGS_INTERFACE (self));
	*connections = g_ptr_array_sized_new (g_slist_length (list) + 1);
	for (iter = list; iter; iter = g_slist_next (iter)) {
		g_ptr_array_add (*connections,
		                 g_strdup (bm_connection_get_path (BM_CONNECTION (iter->data))));
	}
	g_slist_free (list);
	return TRUE;
}

static BMSettingsConnectionInterface *
get_connection_by_path (BMSettingsInterface *settings, const char *path)
{
	NMExportedConnection *connection = NULL;
	GSList *list = NULL, *iter;

	list = list_connections (settings);
	for (iter = list; iter; iter = g_slist_next (iter)) {
		if (!strcmp (bm_connection_get_path (BM_CONNECTION (iter->data)), path)) {
			connection = BM_EXPORTED_CONNECTION (iter->data);
			break;
		}
	}
	g_slist_free (list);

	return (BMSettingsConnectionInterface *) connection;
}

NMExportedConnection *
bm_settings_service_get_connection_by_path (BMSettingsService *self,
                                            const char *path)
{
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (BM_IS_SETTINGS_SERVICE (self), NULL);

	return (NMExportedConnection *) get_connection_by_path (BM_SETTINGS_INTERFACE (self), path);
}

static gboolean
add_connection (BMSettingsInterface *settings,
                BMConnection *connection,
                BMSettingsAddConnectionFunc callback,
                gpointer user_data)
{
	BMSettingsService *self = BM_SETTINGS_SERVICE (settings);
	GError *error = NULL;
	gboolean success = FALSE;

	if (BM_SETTINGS_SERVICE_GET_CLASS (self)->add_connection) {
		BM_SETTINGS_SERVICE_GET_CLASS (self)->add_connection (BM_SETTINGS_SERVICE (self),
		                                                      connection,
		                                                      NULL,
		                                                      callback,
		                                                      user_data);
		success = TRUE;
	} else {
		error = g_error_new (BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		                     "%s: %s:%d add_connection() not implemented",
		                     __func__, __FILE__, __LINE__);
		callback (settings, error, user_data);
		g_error_free (error);
	}

	return success;
}

static void
dbus_add_connection_cb (BMSettingsInterface *settings,
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
impl_settings_add_connection (BMSettingsService *self,
                              GHashTable *settings,
                              DBusGMethodInvocation *context)
{
	BMConnection *tmp;
	GError *error = NULL;

	/* Check if the settings are valid first */
	tmp = bm_connection_new_from_hash (settings, &error);
	if (!tmp) {
		g_assert (error);
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	if (BM_SETTINGS_SERVICE_GET_CLASS (self)->add_connection) {
		BM_SETTINGS_SERVICE_GET_CLASS (self)->add_connection (BM_SETTINGS_SERVICE (self),
		                                                      tmp,
		                                                      context,
		                                                      dbus_add_connection_cb,
		                                                      context);
	} else {
		error = g_error_new (BM_SETTINGS_INTERFACE_ERROR,
		                     BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		                     "%s: %s:%d add_connection() not implemented",
		                     __func__, __FILE__, __LINE__);
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}

	g_object_unref (tmp);
}

void
bm_settings_service_export_connection (BMSettingsService *self,
                                       BMSettingsConnectionInterface *connection)
{
	BMSettingsServicePrivate *priv = BM_SETTINGS_SERVICE_GET_PRIVATE (self);
	static guint32 ec_counter = 0;
	char *path;

	g_return_if_fail (connection != NULL);
	g_return_if_fail (BM_IS_SETTINGS_CONNECTION_INTERFACE (connection));
	g_return_if_fail (priv->bus != NULL);

	/* Don't allow exporting twice */
	g_return_if_fail (bm_connection_get_path (BM_CONNECTION (connection)) == NULL);

	path = g_strdup_printf ("%s/%u", BM_DBUS_PATH_SETTINGS, ec_counter++);
	bm_connection_set_path (BM_CONNECTION (connection), path);
	bm_connection_set_scope (BM_CONNECTION (connection), priv->scope);

	dbus_g_connection_register_g_object (priv->bus, path, G_OBJECT (connection));
	g_free (path);
}

/**************************************************************/

static void
settings_interface_init (BMSettingsInterface *iface)
{
	/* interface implementation */
	iface->list_connections = list_connections;
	iface->get_connection_by_path = get_connection_by_path;
	iface->add_connection = add_connection;

	dbus_g_object_type_install_info (G_TYPE_FROM_INTERFACE (iface),
	                                 &dbus_glib_bm_settings_object_info);
}

static GObject *
constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
	GObject *object;

	object = G_OBJECT_CLASS (bm_settings_service_parent_class)->constructor (type, n_construct_params, construct_params);
	if (object) {
		g_assert (BM_SETTINGS_SERVICE_GET_PRIVATE (object)->scope != BM_CONNECTION_SCOPE_UNKNOWN);
	}
	return object;
}

static void
bm_settings_service_init (BMSettingsService *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	BMSettingsServicePrivate *priv = BM_SETTINGS_SERVICE_GET_PRIVATE (object);
	DBusGConnection *bus;

	switch (prop_id) {
	case PROP_BUS:
		/* Construct only */
		bus = g_value_get_boxed (value);
		if (bus)
			priv->bus = dbus_g_connection_ref (bus);
		break;
	case PROP_SCOPE:
		/* Construct only */
		priv->scope = g_value_get_uint (value);
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
	BMSettingsServicePrivate *priv = BM_SETTINGS_SERVICE_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_BUS:
		g_value_set_boxed (value, priv->bus);
		break;
	case PROP_SCOPE:
		g_value_set_uint (value, priv->scope);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
dispose (GObject *object)
{
	BMSettingsServicePrivate *priv = BM_SETTINGS_SERVICE_GET_PRIVATE (object);

	if (!priv->disposed) {
		priv->disposed = TRUE;
		if (priv->bus)
			dbus_g_connection_unref (priv->bus);
	}

	G_OBJECT_CLASS (bm_settings_service_parent_class)->dispose (object);
}

static void
bm_settings_service_class_init (BMSettingsServiceClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	g_type_class_add_private (class, sizeof (BMSettingsServicePrivate));

	/* Virtual methods */
	object_class->dispose = dispose;
	object_class->constructor = constructor;
	object_class->get_property = get_property;
	object_class->set_property = set_property;

	/**
	 * BMSettingsService:bus:
	 *
	 * The %DBusGConnection which this object is exported on
	 **/
	g_object_class_install_property (object_class, PROP_BUS,
	                                 g_param_spec_boxed (BM_SETTINGS_SERVICE_BUS,
	                                                     "Bus",
	                                                     "Bus",
	                                                     DBUS_TYPE_G_CONNECTION,
	                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	/**
	 * BMSettingsService:scope:
	 *
	 * The capabilities of the device.
	 **/
	g_object_class_install_property (object_class, PROP_SCOPE,
	                                 g_param_spec_uint (BM_SETTINGS_SERVICE_SCOPE,
	                                                    "Scope",
	                                                    "Scope",
	                                                    BM_CONNECTION_SCOPE_SYSTEM,
	                                                    BM_CONNECTION_SCOPE_USER,
	                                                    BM_CONNECTION_SCOPE_USER,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
