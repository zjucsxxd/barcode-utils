/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access network status & information from glib applications
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
 * Copyright (C) 2007 - 2010 Red Hat, Inc.
 * Copyright (C) 2008 Novell, Inc.
 */

#include <string.h>

#include "BarcodeManager.h"
#include "bm-active-connection.h"
#include "bm-object-private.h"
#include "bm-types-private.h"
#include "bm-device.h"
#include "bm-connection.h"

#include "bm-active-connection-bindings.h"

G_DEFINE_TYPE (BMActiveConnection, bm_active_connection, BM_TYPE_OBJECT)

#define BM_ACTIVE_CONNECTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_ACTIVE_CONNECTION, BMActiveConnectionPrivate))

static gboolean demarshal_devices (BMObject *object, GParamSpec *pspec, GValue *value, gpointer field);


typedef struct {
	gboolean disposed;
	DBusGProxy *proxy;

	char *service_name;
	BMConnectionScope scope;
	char *connection;
	char *specific_object;
	GPtrArray *devices;
	BMActiveConnectionState state;
	gboolean is_default;
	gboolean is_default6;
} BMActiveConnectionPrivate;

enum {
	PROP_0,
	PROP_SERVICE_NAME,
	PROP_CONNECTION,
	PROP_SPECIFIC_OBJECT,
	PROP_DEVICES,
	PROP_STATE,
	PROP_DEFAULT,
	PROP_DEFAULT6,

	LAST_PROP
};

#define DBUS_PROP_SERVICE_NAME "ServiceName"
#define DBUS_PROP_CONNECTION "Connection"
#define DBUS_PROP_SPECIFIC_OBJECT "SpecificObject"
#define DBUS_PROP_DEVICES "Devices"
#define DBUS_PROP_STATE "State"
#define DBUS_PROP_DEFAULT "Default"
#define DBUS_PROP_DEFAULT6 "Default6"

/**
 * bm_active_connection_new:
 * @connection: the #DBusGConnection
 * @path: the DBus object path of the device
 *
 * Creates a new #BMActiveConnection.
 *
 * Returns: a new active connection
 **/
GObject *
bm_active_connection_new (DBusGConnection *connection, const char *path)
{
	g_return_val_if_fail (connection != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	return g_object_new (BM_TYPE_ACTIVE_CONNECTION,
						 BM_OBJECT_DBUS_CONNECTION, connection,
						 BM_OBJECT_DBUS_PATH, path,
						 NULL);
}

static BMConnectionScope
get_scope_for_service_name (const char *service_name)
{
	if (service_name && !strcmp (service_name, BM_DBUS_SERVICE_USER_SETTINGS))
		return BM_CONNECTION_SCOPE_USER;
	else if (service_name && !strcmp (service_name, BM_DBUS_SERVICE_SYSTEM_SETTINGS))
		return BM_CONNECTION_SCOPE_SYSTEM;

	return BM_CONNECTION_SCOPE_UNKNOWN;
}

/**
 * bm_active_connection_get_service_name:
 * @connection: a #BMActiveConnection
 *
 * Gets the service name of the active connection.
 *
 * Returns: the service name. This is the internal string used by the
 * connection, and must not be modified.
 **/
const char *
bm_active_connection_get_service_name (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), NULL);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (!priv->service_name) {
		priv->service_name = _bm_object_get_string_property (BM_OBJECT (connection),
		                                                    BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
		                                                    DBUS_PROP_SERVICE_NAME);
		priv->scope = get_scope_for_service_name (priv->service_name);
	}

	return priv->service_name;
}

/**
 * bm_active_connection_get_scope:
 * @connection: a #BMActiveConnection
 *
 * Gets the scope of the active connection.
 *
 * Returns: the connection's scope
 **/
BMConnectionScope
bm_active_connection_get_scope (BMActiveConnection *connection)
{
	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), BM_CONNECTION_SCOPE_UNKNOWN);

	/* Make sure service_name and scope are up-to-date */
	bm_active_connection_get_service_name (connection);
	return BM_ACTIVE_CONNECTION_GET_PRIVATE (connection)->scope;
}

/**
 * bm_active_connection_get_connection:
 * @connection: a #BMActiveConnection
 *
 * Gets the #BMConnection<!-- -->'s DBus object path.
 *
 * Returns: the object path of the #BMConnection inside of #BMActiveConnection.
 * This is the internal string used by the connection, and must not be modified.
 **/
const char *
bm_active_connection_get_connection (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), NULL);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (!priv->connection) {
		priv->connection = _bm_object_get_string_property (BM_OBJECT (connection),
		                                                  BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
		                                                  DBUS_PROP_CONNECTION);
	}

	return priv->connection;
}

/**
 * bm_active_connection_get_specific_object:
 * @connection: a #BMActiveConnection
 *
 * Gets the "specific object" used at the activation.
 *
 * Returns: the specific object's DBus path. This is the internal string used by the
 * connection, and must not be modified.
 **/
const char *
bm_active_connection_get_specific_object (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), NULL);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (!priv->specific_object) {
		priv->specific_object = _bm_object_get_string_property (BM_OBJECT (connection),
		                                                       BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
		                                                       DBUS_PROP_SPECIFIC_OBJECT);
	}

	return priv->specific_object;
}

/**
 * bm_active_connection_get_devices:
 * @connection: a #BMActiveConnection
 *
 * Gets the #BMDevice<!-- -->s used for the active connections.
 *
 * Returns: the #GPtrArray containing #BMDevice<!-- -->s.
 * This is the internal copy used by the connection, and must not be modified.
 **/
const GPtrArray *
bm_active_connection_get_devices (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;
	GValue value = { 0, };

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), NULL);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (priv->devices)
		return handle_ptr_array_return (priv->devices);

	if (!_bm_object_get_property (BM_OBJECT (connection),
	                             BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
	                             DBUS_PROP_DEVICES,
	                             &value)) {
		return NULL;
	}

	demarshal_devices (BM_OBJECT (connection), NULL, &value, &priv->devices);
	g_value_unset (&value);

	return handle_ptr_array_return (priv->devices);
}

/**
 * bm_active_connection_get_state:
 * @connection: a #BMActiveConnection
 *
 * Gets the active connection's state.
 *
 * Returns: the state
 **/
BMActiveConnectionState
bm_active_connection_get_state (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), BM_ACTIVE_CONNECTION_STATE_UNKNOWN);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (!priv->state) {
		priv->state = _bm_object_get_uint_property (BM_OBJECT (connection),
		                                           BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
		                                           DBUS_PROP_STATE);
	}

	return priv->state;
}

/**
 * bm_active_connection_get_default:
 * @connection: a #BMActiveConnection
 *
 * Whether the active connection is the default IPv4 one (that is, is used for
 * the default IPv4 route and DNS information).
 *
 * Returns: %TRUE if the active connection is the default IPv4 connection
 **/
gboolean
bm_active_connection_get_default (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), FALSE);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (!priv->is_default) {
		priv->is_default = _bm_object_get_boolean_property (BM_OBJECT (connection),
		                                                    BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
		                                                    DBUS_PROP_DEFAULT);
	}

	return priv->is_default;
}

/**
 * bm_active_connection_get_default6:
 * @connection: a #BMActiveConnection
 *
 * Whether the active connection is the default IPv6 one (that is, is used for
 * the default IPv6 route and DNS information).
 *
 * Returns: %TRUE if the active connection is the default IPv6 connection
 **/
gboolean
bm_active_connection_get_default6 (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv;

	g_return_val_if_fail (BM_IS_ACTIVE_CONNECTION (connection), FALSE);

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	if (!priv->is_default6) {
		priv->is_default6 = _bm_object_get_boolean_property (BM_OBJECT (connection),
		                                                     BM_DBUS_INTERFACE_ACTIVE_CONNECTION,
		                                                     DBUS_PROP_DEFAULT6);
	}

	return priv->is_default6;
}

static void
bm_active_connection_init (BMActiveConnection *ap)
{
}

static void
dispose (GObject *object)
{
	BMActiveConnectionPrivate *priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_active_connection_parent_class)->dispose (object);
		return;
	}

	priv->disposed = TRUE;

	if (priv->devices) {
		g_ptr_array_foreach (priv->devices, (GFunc) g_object_unref, NULL);
		g_ptr_array_free (priv->devices, TRUE);
	}
	g_object_unref (priv->proxy);

	G_OBJECT_CLASS (bm_active_connection_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	BMActiveConnectionPrivate *priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (object);

	g_free (priv->service_name);
	g_free (priv->connection);
	g_free (priv->specific_object);

	G_OBJECT_CLASS (bm_active_connection_parent_class)->finalize (object);
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
	BMActiveConnection *self = BM_ACTIVE_CONNECTION (object);

	switch (prop_id) {
	case PROP_SERVICE_NAME:
		g_value_set_string (value, bm_active_connection_get_service_name (self));
		break;
	case PROP_CONNECTION:
		g_value_set_boxed (value, bm_active_connection_get_connection (self));
		break;
	case PROP_SPECIFIC_OBJECT:
		g_value_set_boxed (value, bm_active_connection_get_specific_object (self));
		break;
	case PROP_DEVICES:
		g_value_set_boxed (value, bm_active_connection_get_devices (self));
		break;
	case PROP_STATE:
		g_value_set_uint (value, bm_active_connection_get_state (self));
		break;
	case PROP_DEFAULT:
		g_value_set_boolean (value, bm_active_connection_get_default (self));
		break;
	case PROP_DEFAULT6:
		g_value_set_boolean (value, bm_active_connection_get_default6 (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static gboolean
demarshal_devices (BMObject *object, GParamSpec *pspec, GValue *value, gpointer field)
{
	DBusGConnection *connection;

	connection = bm_object_get_connection (object);
	if (!_bm_object_array_demarshal (value, (GPtrArray **) field, connection, bm_device_new))
		return FALSE;

	_bm_object_queue_notify (object, BM_ACTIVE_CONNECTION_DEVICES);
	return TRUE;
}

static gboolean
demarshal_service (BMObject *object, GParamSpec *pspec, GValue *value, gpointer field)
{
	BMActiveConnectionPrivate *priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (object);

	if (_bm_object_demarshal_generic (object, pspec, value, field)) {
		priv->scope = get_scope_for_service_name (priv->service_name);
		return TRUE;
	}
	return FALSE;
}

static void
register_for_property_changed (BMActiveConnection *connection)
{
	BMActiveConnectionPrivate *priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (connection);
	const NMPropertiesChangedInfo property_changed_info[] = {
		{ BM_ACTIVE_CONNECTION_SERVICE_NAME,        demarshal_service,           &priv->service_name },
		{ BM_ACTIVE_CONNECTION_CONNECTION,          _bm_object_demarshal_generic, &priv->connection },
		{ BM_ACTIVE_CONNECTION_SPECIFIC_OBJECT,     _bm_object_demarshal_generic, &priv->specific_object },
		{ BM_ACTIVE_CONNECTION_DEVICES,             demarshal_devices,           &priv->devices },
		{ BM_ACTIVE_CONNECTION_STATE,               _bm_object_demarshal_generic, &priv->state },
		{ BM_ACTIVE_CONNECTION_DEFAULT,             _bm_object_demarshal_generic, &priv->is_default },
		{ BM_ACTIVE_CONNECTION_DEFAULT6,            _bm_object_demarshal_generic, &priv->is_default6 },
		{ NULL },
	};

	_bm_object_handle_properties_changed (BM_OBJECT (connection),
	                                     priv->proxy,
	                                     property_changed_info);
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	BMObject *object;
	BMActiveConnectionPrivate *priv;

	object = (BMObject *) G_OBJECT_CLASS (bm_active_connection_parent_class)->constructor (type,
																	  n_construct_params,
																	  construct_params);
	if (!object)
		return NULL;

	priv = BM_ACTIVE_CONNECTION_GET_PRIVATE (object);

	priv->proxy = dbus_g_proxy_new_for_name (bm_object_get_connection (object),
									    BM_DBUS_SERVICE,
									    bm_object_get_path (object),
									    BM_DBUS_INTERFACE_ACTIVE_CONNECTION);

	register_for_property_changed (BM_ACTIVE_CONNECTION (object));

	return G_OBJECT (object);
}


static void
bm_active_connection_class_init (BMActiveConnectionClass *ap_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (ap_class);

	g_type_class_add_private (ap_class, sizeof (BMActiveConnectionPrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	/* properties */

	/**
	 * BMActiveConnection:service-name:
	 *
	 * The service name of the active connection.
	 **/
	g_object_class_install_property
		(object_class, PROP_SERVICE_NAME,
		 g_param_spec_string (BM_ACTIVE_CONNECTION_SERVICE_NAME,
						  "Service Name",
						  "Service Name",
						  NULL,
						  G_PARAM_READABLE));

	/**
	 * BMActiveConnection:connection:
	 *
	 * The connection's path of the active connection.
	 **/
	g_object_class_install_property
		(object_class, PROP_CONNECTION,
		 g_param_spec_string (BM_ACTIVE_CONNECTION_CONNECTION,
						      "Connection",
						      "Connection",
						      NULL,
						      G_PARAM_READABLE));

	/**
	 * BMActiveConnection:specific-object:
	 *
	 * The specific object's path of the active connection.
	 **/
	g_object_class_install_property
		(object_class, PROP_SPECIFIC_OBJECT,
		 g_param_spec_string (BM_ACTIVE_CONNECTION_SPECIFIC_OBJECT,
						      "Specific object",
						      "Specific object",
						      NULL,
						      G_PARAM_READABLE));

	/**
	 * BMActiveConnection:device:
	 *
	 * The devices (#BMDevice) of the active connection.
	 **/
	g_object_class_install_property
		(object_class, PROP_DEVICES,
		 g_param_spec_boxed (BM_ACTIVE_CONNECTION_DEVICES,
						       "Devices",
						       "Devices",
						       BM_TYPE_OBJECT_ARRAY,
						       G_PARAM_READABLE));

	/**
	 * BMActiveConnection:state:
	 *
	 * The state of the active connection.
	 **/
	g_object_class_install_property
		(object_class, PROP_STATE,
		 g_param_spec_uint (BM_ACTIVE_CONNECTION_STATE,
							  "State",
							  "State",
							  BM_ACTIVE_CONNECTION_STATE_UNKNOWN,
							  BM_ACTIVE_CONNECTION_STATE_ACTIVATED,
							  BM_ACTIVE_CONNECTION_STATE_UNKNOWN,
							  G_PARAM_READABLE));

	/**
	 * BMActiveConnection:default:
	 *
	 * Whether the active connection is the default IPv4 one.
	 **/
	g_object_class_install_property
		(object_class, PROP_DEFAULT,
		 g_param_spec_boolean (BM_ACTIVE_CONNECTION_DEFAULT,
							   "Default",
							   "Is the default IPv4 active connection",
							   FALSE,
							   G_PARAM_READABLE));

	/**
	 * BMActiveConnection:default6:
	 *
	 * Whether the active connection is the default IPv6 one.
	 **/
	g_object_class_install_property
		(object_class, PROP_DEFAULT6,
		 g_param_spec_boolean (BM_ACTIVE_CONNECTION_DEFAULT6,
							   "Default6",
							   "Is the default IPv6 active connection",
							   FALSE,
							   G_PARAM_READABLE));
}
