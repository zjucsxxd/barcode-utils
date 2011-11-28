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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2009 Red Hat, Inc.
 */

#include "bm-device-hidraw.h"
#include "bm-device-private.h"
#include "bm-object-private.h"

#include "bm-device-hidraw-bindings.h"

G_DEFINE_TYPE (BMDeviceHidraw, bm_device_hidraw, BM_TYPE_DEVICE)

#define BM_DEVICE_HIDRAW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawPrivate))

typedef struct {
	DBusGProxy *proxy;

	char *hw_address;
	char *name;
	guint32 hidraw_capabilities;
	gboolean hidraw_capabilities_valid;

	gboolean disposed;
} BMDeviceHidrawPrivate;

enum {
	PROP_0,
	PROP_HW_ADDRESS,
	PROP_NAME,
	PROP_HIDRAW_CAPABILITIES,

	LAST_PROP
};

#define DBUS_PROP_HW_ADDRESS      "HwAddress"
#define DBUS_PROP_NAME            "Name"
#define DBUS_PROP_HIDRAW_CAPABILITIES "HidrawCapabilities"

/**
 * bm_device_hidraw_new:
 * @connection: the #DBusGConnection
 * @path: the DBus object path of the device
 *
 * Creates a new #BMDeviceHidraw.
 *
 * Returns: a new device
 **/
GObject *
bm_device_hidraw_new (DBusGConnection *connection, const char *path)
{
	g_return_val_if_fail (connection != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	return g_object_new (BM_TYPE_DEVICE_HIDRAW,
	                     BM_OBJECT_DBUS_CONNECTION, connection,
	                     BM_OBJECT_DBUS_PATH, path,
	                     NULL);
}

/**
 * bm_device_hidraw_get_hw_address:
 * @device: a #BMDeviceHidraw
 *
 * Gets the hardware (MAC) address of the #BMDeviceHidraw
 *
 * Returns: the hardware address. This is the internal string used by the
 * device, and must not be modified.
 **/
const char *
bm_device_hidraw_get_hw_address (BMDeviceHidraw *device)
{
	BMDeviceHidrawPrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE_HIDRAW (device), NULL);

	priv = BM_DEVICE_HIDRAW_GET_PRIVATE (device);
	if (!priv->hw_address) {
		priv->hw_address = _bm_object_get_string_property (BM_OBJECT (device),
		                                                   BM_DBUS_INTERFACE_DEVICE_HIDRAW,
		                                                   DBUS_PROP_HW_ADDRESS);
	}

	return priv->hw_address;
}

/**
 * bm_device_hidraw_get_name:
 * @device: a #BMDeviceHidraw
 *
 * Gets the name of the #BMDeviceHidraw.
 *
 * Returns: the name of the device
 **/
const char *
bm_device_hidraw_get_name (BMDeviceHidraw *device)
{
	BMDeviceHidrawPrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE_HIDRAW (device), NULL);

	priv = BM_DEVICE_HIDRAW_GET_PRIVATE (device);
	if (!priv->name) {
		priv->name = _bm_object_get_string_property (BM_OBJECT (device),
		                                             BM_DBUS_INTERFACE_DEVICE_HIDRAW,
		                                             DBUS_PROP_NAME);
	}

	return priv->name;
}

/**
 * bm_device_hidraw_get_capabilities:
 * @device: a #BMDeviceHidraw
 *
 * Returns the Bluetooth device's usable capabilities.
 *
 * Returns: a combination of #BMBluetoothCapabilities
 **/
BMBluetoothCapabilities
bm_device_hidraw_get_capabilities (BMDeviceHidraw *device)
{
	BMDeviceHidrawPrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE_HIDRAW (device), BM_HIDRAW_CAPABILITY_NONE);

	priv = BM_DEVICE_HIDRAW_GET_PRIVATE (device);
	if (!priv->hidraw_capabilities_valid) {
		priv->hidraw_capabilities = _bm_object_get_uint_property (BM_OBJECT (device),
		                                                      BM_DBUS_INTERFACE_DEVICE_HIDRAW,
		                                                      DBUS_PROP_HIDRAW_CAPABILITIES);
		priv->hidraw_capabilities_valid = TRUE;
	}

	return priv->hidraw_capabilities;
}

static void
bm_device_hidraw_init (BMDeviceHidraw *device)
{
}

static void
register_for_property_changed (BMDeviceHidraw *device)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (device);
	const NMPropertiesChangedInfo property_changed_info[] = {
		{ BM_DEVICE_HIDRAW_HW_ADDRESS,   _bm_object_demarshal_generic, &priv->hw_address },
		{ BM_DEVICE_HIDRAW_NAME,         _bm_object_demarshal_generic, &priv->name },
		{ BM_DEVICE_HIDRAW_CAPABILITIES, _bm_object_demarshal_generic, &priv->hidraw_capabilities },
		{ NULL },
	};

	_bm_object_handle_properties_changed (BM_OBJECT (device),
	                                     priv->proxy,
	                                     property_changed_info);
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	GObject *object;

	object = G_OBJECT_CLASS (bm_device_hidraw_parent_class)->constructor (type,
	                                                                  n_construct_params,
	                                                                  construct_params);
	if (object) {
			BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

		priv->proxy = dbus_g_proxy_new_for_name (bm_object_get_connection (BM_OBJECT (object)),
		                                         BM_DBUS_SERVICE,
		                                         bm_object_get_path (BM_OBJECT (object)),
		                                         BM_DBUS_INTERFACE_DEVICE_HIDRAW);

		register_for_property_changed (BM_DEVICE_HIDRAW (object));
	}

	return object;
}

static void
dispose (GObject *object)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_device_hidraw_parent_class)->dispose (object);
		return;
	}
	priv->disposed = TRUE;

	g_object_unref (priv->proxy);

	G_OBJECT_CLASS (bm_device_hidraw_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

	g_free (priv->hw_address);
	g_free (priv->name);

	G_OBJECT_CLASS (bm_device_hidraw_parent_class)->finalize (object);
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
	BMDeviceHidraw *device = BM_DEVICE_HIDRAW (object);

	switch (prop_id) {
	case PROP_HW_ADDRESS:
		g_value_set_string (value, bm_device_hidraw_get_hw_address (device));
		break;
	case PROP_NAME:
		g_value_set_string (value, bm_device_hidraw_get_name (device));
		break;
	case PROP_HIDRAW_CAPABILITIES:
		g_value_set_uint (value, bm_device_hidraw_get_capabilities (device));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_device_hidraw_class_init (BMDeviceHidrawClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	g_type_class_add_private (device_class, sizeof (BMDeviceHidrawPrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->dispose = dispose;
	object_class->finalize = finalize;
	object_class->get_property = get_property;

	/* properties */

	/**
	 * BMDeviceHidraw:hw-address:
	 *
	 * The hardware (MAC) address of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_HW_ADDRESS,
		 g_param_spec_string (BM_DEVICE_HIDRAW_HW_ADDRESS,
		                      "MAC Address",
		                      "Hardware MAC address",
		                      NULL,
		                      G_PARAM_READABLE));

	/**
	 * BMDeviceHidraw:name:
	 *
	 * The name of the bluetooth device.
	 **/
	g_object_class_install_property
		(object_class, PROP_NAME,
		 g_param_spec_string (BM_DEVICE_HIDRAW_NAME,
		                      "Name",
		                      "Device name",
		                      NULL,
		                      G_PARAM_READABLE));

	/**
	 * BMDeviceHidraw:hidraw-capabilities:
	 *
	 * The device's bluetooth capabilities, a combination of #BMBluetoothCapabilities.
	 **/
	g_object_class_install_property
		(object_class, PROP_HIDRAW_CAPABILITIES,
		 g_param_spec_uint (BM_DEVICE_HIDRAW_CAPABILITIES,
		                    "HidrawCapabilities",
		                    "Bluetooth capabilities",
		                    BM_HIDRAW_CAPABILITY_NONE, G_MAXUINT32, BM_HIDRAW_CAPABILITY_NONE,
		                    G_PARAM_READABLE));

}

