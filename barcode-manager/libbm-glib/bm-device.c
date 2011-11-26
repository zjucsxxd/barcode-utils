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

#include <string.h>

#define G_UDEV_API_IS_SUBJECT_TO_CHANGE
#include <gudev/gudev.h>

#include "BarcodeManager.h"
#include "bm-device-bt.h"
#include "bm-device.h"
#include "bm-device-private.h"
#include "bm-object-private.h"
#include "bm-object-cache.h"
#include "bm-marshal.h"

#include "bm-device-bindings.h"

G_DEFINE_TYPE (BMDevice, bm_device, BM_TYPE_OBJECT)

#define BM_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_DEVICE, BMDevicePrivate))

typedef struct {
	gboolean disposed;
	DBusGProxy *proxy;

	char *iface;
	char *udi;
	char *driver;
	guint32 capabilities;
	gboolean managed;
	gboolean firmware_missing;
	BMDeviceState state;

	GUdevClient *client;
	char *product;
	char *vendor;
} BMDevicePrivate;

enum {
	PROP_0,
	PROP_INTERFACE,
	PROP_UDI,
	PROP_DRIVER,
	PROP_CAPABILITIES,
	PROP_MANAGED,
	PROP_FIRMWARE_MISSING,
	PROP_STATE,
	PROP_PRODUCT,
	PROP_VENDOR,

	LAST_PROP
};

enum {
	STATE_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


static void
bm_device_init (BMDevice *device)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (device);

	priv->state = BM_DEVICE_STATE_UNKNOWN;
}

static void
register_for_property_changed (BMDevice *device)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (device);
	const NMPropertiesChangedInfo property_changed_info[] = {
		{ BM_DEVICE_UDI,              _bm_object_demarshal_generic, &priv->udi },
		{ BM_DEVICE_INTERFACE,        _bm_object_demarshal_generic, &priv->iface },
		{ BM_DEVICE_DRIVER,           _bm_object_demarshal_generic, &priv->driver },
		{ BM_DEVICE_CAPABILITIES,     _bm_object_demarshal_generic, &priv->capabilities },
		{ BM_DEVICE_MANAGED,          _bm_object_demarshal_generic, &priv->managed },
		{ BM_DEVICE_FIRMWARE_MISSING, _bm_object_demarshal_generic, &priv->firmware_missing },
		{ NULL },
	};

	_bm_object_handle_properties_changed (BM_OBJECT (device),
	                                     priv->proxy,
	                                     property_changed_info);
}

static void
device_state_changed (DBusGProxy *proxy,
                      BMDeviceState new_state,
                      BMDeviceState old_state,
                      BMDeviceStateReason reason,
                      gpointer user_data)
{
	BMDevice *self = BM_DEVICE (user_data);
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (self);

	if (priv->state != new_state) {
		priv->state = new_state;
		g_signal_emit (self, signals[STATE_CHANGED], 0, new_state, old_state, reason);
		_bm_object_queue_notify (BM_OBJECT (self), "state");
	}
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	BMObject *object;
	BMDevicePrivate *priv;

	object = (BMObject *) G_OBJECT_CLASS (bm_device_parent_class)->constructor (type,
																				n_construct_params,
																				construct_params);
	if (!object)
		return NULL;

	priv = BM_DEVICE_GET_PRIVATE (object);

	priv->proxy = dbus_g_proxy_new_for_name (bm_object_get_connection (object),
											 BM_DBUS_SERVICE,
											 bm_object_get_path (object),
											 BM_DBUS_INTERFACE_DEVICE);

	register_for_property_changed (BM_DEVICE (object));

	dbus_g_object_register_marshaller (_bm_marshal_VOID__UINT_UINT_UINT,
									   G_TYPE_NONE,
									   G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
									   G_TYPE_INVALID);

	dbus_g_proxy_add_signal (priv->proxy,
	                         "StateChanged",
	                         G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
	                         G_TYPE_INVALID);

	dbus_g_proxy_connect_signal (priv->proxy, "StateChanged",
								 G_CALLBACK (device_state_changed),
								 BM_DEVICE (object),
								 NULL);

	return G_OBJECT (object);
}

static void
dispose (GObject *object)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_device_parent_class)->dispose (object);
		return;
	}

	priv->disposed = TRUE;

	g_object_unref (priv->proxy);
	if (priv->client)
		g_object_unref (priv->client);

	G_OBJECT_CLASS (bm_device_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (object);

	g_free (priv->iface);
	g_free (priv->udi);
	g_free (priv->driver);
	g_free (priv->product);
	g_free (priv->vendor);

	G_OBJECT_CLASS (bm_device_parent_class)->finalize (object);
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
	BMDevice *device = BM_DEVICE (object);

	switch (prop_id) {
	case PROP_UDI:
		g_value_set_string (value, bm_device_get_udi (device));
		break;
	case PROP_INTERFACE:
		g_value_set_string (value, bm_device_get_iface (device));
		break;
	case PROP_DRIVER:
		g_value_set_string (value, bm_device_get_driver (device));
		break;
	case PROP_CAPABILITIES:
		g_value_set_uint (value, bm_device_get_capabilities (device));
		break;
	case PROP_MANAGED:
		g_value_set_boolean (value, bm_device_get_managed (device));
		break;
	case PROP_FIRMWARE_MISSING:
		g_value_set_boolean (value, bm_device_get_firmware_missing (device));
		break;
	case PROP_STATE:
		g_value_set_uint (value, bm_device_get_state (device));
		break;
	case PROP_PRODUCT:
		g_value_set_string (value, bm_device_get_product (device));
		break;
	case PROP_VENDOR:
		g_value_set_string (value, bm_device_get_vendor (device));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_device_class_init (BMDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	g_type_class_add_private (device_class, sizeof (BMDevicePrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	/* properties */

	/**
	 * BMDevice:interface:
	 *
	 * The interface of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_INTERFACE,
		 g_param_spec_string (BM_DEVICE_INTERFACE,
						  "Interface",
						  "Interface name",
						  NULL,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:udi:
	 *
	 * The Unique Device Identifier of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_UDI,
		 g_param_spec_string (BM_DEVICE_UDI,
						  "UDI",
						  "Unique Device Identifier",
						  NULL,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:driver:
	 *
	 * The driver of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_DRIVER,
		 g_param_spec_string (BM_DEVICE_DRIVER,
						  "Driver",
						  "Driver",
						  NULL,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:capabilities:
	 *
	 * The capabilities of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_CAPABILITIES,
		 g_param_spec_uint (BM_DEVICE_CAPABILITIES,
						  "Capabilities",
						  "Capabilities",
						  0, G_MAXUINT32, 0,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:managed:
	 *
	 * Whether the device is managed by BarcodeManager.
	 **/
	g_object_class_install_property
		(object_class, PROP_MANAGED,
		 g_param_spec_boolean (BM_DEVICE_MANAGED,
						  "Managed",
						  "Managed",
						  FALSE,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:firmware-missing:
	 *
	 * When %TRUE indicates the device is likely missing firmware required
	 * for its operation.
	 **/
	g_object_class_install_property
		(object_class, PROP_FIRMWARE_MISSING,
		 g_param_spec_boolean (BM_DEVICE_FIRMWARE_MISSING,
						  "FirmwareMissing",
						  "Firmware missing",
						  FALSE,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:state:
	 *
	 * The state of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_STATE,
		 g_param_spec_uint (BM_DEVICE_STATE,
						  "State",
						  "State",
						  0, G_MAXUINT32, 0,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:vendor:
	 *
	 * The vendor string of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_VENDOR,
		 g_param_spec_string (BM_DEVICE_VENDOR,
						  "Vendor",
						  "Vendor string",
						  NULL,
						  G_PARAM_READABLE));

	/**
	 * BMDevice:product:
	 *
	 * The product string of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_PRODUCT,
		 g_param_spec_string (BM_DEVICE_PRODUCT,
						  "Product",
						  "Product string",
						  NULL,
						  G_PARAM_READABLE));

	/* signals */

	/**
	 * BMDevice::state-changed:
	 * @device: the client that received the signal
	 * @state: the new state of the device
	 *
	 * Notifies the state change of a #BMDevice.
	 **/
	signals[STATE_CHANGED] =
		g_signal_new ("state-changed",
				    G_OBJECT_CLASS_TYPE (object_class),
				    G_SIGNAL_RUN_FIRST,
				    G_STRUCT_OFFSET (BMDeviceClass, state_changed),
				    NULL, NULL,
				    _bm_marshal_VOID__UINT_UINT_UINT,
				    G_TYPE_NONE, 3,
				    G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
}

/**
 * bm_device_new:
 * @connection: the #DBusGConnection
 * @path: the DBus object path of the device
 *
 * Creates a new #BMDevice.
 *
 * Returns: a new device
 **/
GObject *
bm_device_new (DBusGConnection *connection, const char *path)
{
	DBusGProxy *proxy;
	GError *err = NULL;
	GValue value = {0,};
	GType dtype = 0;
	BMDevice *device = NULL;

	g_return_val_if_fail (connection != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	proxy = dbus_g_proxy_new_for_name (connection,
									   BM_DBUS_SERVICE,
									   path,
									   "org.freedesktop.DBus.Properties");
	if (!proxy) {
		g_warning ("%s: couldn't create D-Bus object proxy.", __func__);
		return NULL;
	}

	if (!dbus_g_proxy_call (proxy,
						    "Get", &err,
						    G_TYPE_STRING, BM_DBUS_INTERFACE_DEVICE,
						    G_TYPE_STRING, "DeviceType",
						    G_TYPE_INVALID,
						    G_TYPE_VALUE, &value, G_TYPE_INVALID)) {
		g_warning ("Error in get_property: %s\n", err->message);
		g_error_free (err);
		goto out;
	}

	// FIXME more types here

	switch (g_value_get_uint (&value)) {
	case BM_DEVICE_TYPE_BT:
		dtype = BM_TYPE_DEVICE_BT;
		break;
	default:
		g_warning ("Unknown device type %d", g_value_get_uint (&value));
		break;
	}

	if (dtype) {
		device = (BMDevice *) g_object_new (dtype,
											BM_OBJECT_DBUS_CONNECTION, connection,
											BM_OBJECT_DBUS_PATH, path,
											NULL);
	}

out:
	g_object_unref (proxy);
	return G_OBJECT (device);
}

/**
 * bm_device_get_iface:
 * @device: a #BMDevice
 *
 * Gets the interface name of the #BMDevice.
 *
 * Returns: the interface of the device. This is the internal string used by the
 * device, and must not be modified.
 **/
const char *
bm_device_get_iface (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), NULL);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->iface) {
		priv->iface = _bm_object_get_string_property (BM_OBJECT (device),
		                                             BM_DBUS_INTERFACE_DEVICE,
		                                             "Interface");
	}

	return priv->iface;
}

/**
 * bm_device_get_udi:
 * @device: a #BMDevice
 *
 * Gets the Unique Device Identifier of the #BMDevice.
 *
 * Returns: the Unique Device Identifier of the device.  This identifier may be
 * used to gather more information about the device from various operating
 * system services like udev or sysfs.
 **/
const char *
bm_device_get_udi (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), NULL);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->udi) {
		priv->udi = _bm_object_get_string_property (BM_OBJECT (device),
		                                           BM_DBUS_INTERFACE_DEVICE,
		                                           "Udi");
	}

	return priv->udi;
}

/**
 * bm_device_get_driver:
 * @device: a #BMDevice
 *
 * Gets the driver of the #BMDevice.
 *
 * Returns: the driver of the device. This is the internal string used by the
 * device, and must not be modified.
 **/
const char *
bm_device_get_driver (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), NULL);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->driver) {
		priv->driver = _bm_object_get_string_property (BM_OBJECT (device),
		                                              BM_DBUS_INTERFACE_DEVICE,
		                                              "Driver");
	}

	return priv->driver;
}

/**
 * bm_device_get_capabilities:
 * @device: a #BMDevice
 *
 * Gets the device' capabilities.
 *
 * Returns: the capabilities
 **/
guint32
bm_device_get_capabilities (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), 0);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->capabilities) {
		priv->capabilities = _bm_object_get_uint_property (BM_OBJECT (device),
		                                                  BM_DBUS_INTERFACE_DEVICE,
		                                                  "Capabilities");
	}

	return priv->capabilities;
}

/**
 * bm_device_get_managed:
 * @device: a #BMDevice
 *
 * Whether the #BMDevice is managed by BarcodeManager.
 *
 * Returns: %TRUE if the device is managed by BarcodeManager
 **/
gboolean
bm_device_get_managed (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), 0);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->managed) {
		priv->managed = _bm_object_get_boolean_property (BM_OBJECT (device),
		                                                BM_DBUS_INTERFACE_DEVICE,
		                                                "Managed");
	}

	return priv->managed;
}

/**
 * bm_device_get_firmware_missing:
 * @device: a #BMDevice
 *
 * Indicates that firmware required for the device's operation is likely
 * to be missing.
 *
 * Returns: %TRUE if firmware required for the device's operation is likely
 * to be missing.
 **/
gboolean
bm_device_get_firmware_missing (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), 0);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->firmware_missing) {
		priv->firmware_missing = _bm_object_get_boolean_property (BM_OBJECT (device),
		                                                          BM_DBUS_INTERFACE_DEVICE,
		                                                          "FirmwareMissing");
	}

	return priv->firmware_missing;
}


/**
 * bm_device_get_state:
 * @device: a #BMDevice
 *
 * Gets the current #BMDevice state.
 *
 * Returns: the current device state
 **/
BMDeviceState
bm_device_get_state (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), BM_DEVICE_STATE_UNKNOWN);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (priv->state == BM_DEVICE_STATE_UNKNOWN) {
		priv->state = _bm_object_get_uint_property (BM_OBJECT (device), 
		                                           BM_DBUS_INTERFACE_DEVICE,
		                                           "State");
	}

	return priv->state;
}

/* From hostap, Copyright (c) 2002-2005, Jouni Malinen <jkmaline@cc.hut.fi> */

static int hex2num (char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int hex2byte (const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

/* End from hostap */

static char *
get_decoded_property (GUdevDevice *device, const char *property)
{
	const char *orig, *p;
	char *unescaped, *n;
	guint len;

	p = orig = g_udev_device_get_property (device, property);
	if (!orig)
		return NULL;

	len = strlen (orig);
	n = unescaped = g_malloc0 (len + 1);
	while (*p) {
		if ((len >= 4) && (*p == '\\') && (*(p+1) == 'x')) {
			*n++ = (char) hex2byte (p + 2);
			p += 4;
			len -= 4;
		} else {
			*n++ = *p++;
			len--;
		}
	}

	return unescaped;
}

static void
bm_device_update_description (BMDevice *device)
{
	BMDevicePrivate *priv;
	const char *subsys[3] = { "net", "tty", NULL };
	GUdevDevice *udev_device = NULL, *tmpdev, *olddev;
	const char *ifname;
	guint32 count = 0;
	const char *vendor, *model;

	g_return_if_fail (BM_IS_DEVICE (device));
	priv = BM_DEVICE_GET_PRIVATE (device);

	if (!priv->client) {
		priv->client = g_udev_client_new (subsys);
		if (!priv->client)
			return;
	}

	ifname = bm_device_get_iface (device);
	if (!ifname)
		return;

	udev_device = g_udev_client_query_by_subsystem_and_name (priv->client, "net", ifname);
	if (!udev_device)
		udev_device = g_udev_client_query_by_subsystem_and_name (priv->client, "tty", ifname);
	if (!udev_device)
		return;

	g_free (priv->product);
	priv->product = NULL;
	g_free (priv->vendor);
	priv->vendor = NULL;

	/* Walk up the chain of the device and its parents a few steps to grab
	 * vendor and device ID information off it.
	 */

	/* Ref the device again because we have to unref it each iteration,
	 * as g_udev_device_get_parent() returns a ref-ed object.
	 */
	tmpdev = g_object_ref (udev_device);
	while ((count++ < 3) && tmpdev && (!priv->vendor || !priv->product)) {
		if (!priv->vendor)
			priv->vendor = get_decoded_property (tmpdev, "ID_VENDOR_ENC");

		if (!priv->product)
			priv->product = get_decoded_property (tmpdev, "ID_MODEL_ENC");

		olddev = tmpdev;
		tmpdev = g_udev_device_get_parent (tmpdev);
		g_object_unref (olddev);
	}

	/* Unref the last device if we found what we needed before running out
	 * of parents.
	 */
	if (tmpdev)
		g_object_unref (tmpdev);

	/* If we didn't get strings directly from the device, try database strings */

	/* Again, ref the original device as we need to unref it every iteration
	 * since g_udev_device_get_parent() returns a refed object.
	 */
	tmpdev = g_object_ref (udev_device);
	count = 0;
	while ((count++ < 3) && tmpdev && (!priv->vendor || !priv->product)) {
		if (!priv->vendor) {
			vendor = g_udev_device_get_property (tmpdev, "ID_VENDOR_FROM_DATABASE");
			if (vendor)
				priv->vendor = g_strdup (vendor);
		}

		if (!priv->product) {
			model = g_udev_device_get_property (tmpdev, "ID_MODEL_FROM_DATABASE");
			if (model)
				priv->product = g_strdup (model);
		}

		olddev = tmpdev;
		tmpdev = g_udev_device_get_parent (tmpdev);
		g_object_unref (olddev);
	}

	/* Unref the last device if we found what we needed before running out
	 * of parents.
	 */
	if (tmpdev)
		g_object_unref (tmpdev);

	/* Balance the initial g_udev_client_query_by_subsystem_and_name() */
	g_object_unref (udev_device);

	_bm_object_queue_notify (BM_OBJECT (device), BM_DEVICE_VENDOR);
	_bm_object_queue_notify (BM_OBJECT (device), BM_DEVICE_PRODUCT);
}

/**
 * bm_device_get_product:
 * @device: a #BMDevice
 *
 * Gets the product string of the #BMDevice.
 *
 * Returns: the product name of the device. This is the internal string used by the
 * device, and must not be modified.
 **/
const char *
bm_device_get_product (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), NULL);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->product)
		bm_device_update_description (device);
	return priv->product;
}

/**
 * bm_device_get_vendor:
 * @device: a #BMDevice
 *
 * Gets the vendor string of the #BMDevice.
 *
 * Returns: the vendor name of the device. This is the internal string used by the
 * device, and must not be modified.
 **/
const char *
bm_device_get_vendor (BMDevice *device)
{
	BMDevicePrivate *priv;

	g_return_val_if_fail (BM_IS_DEVICE (device), NULL);

	priv = BM_DEVICE_GET_PRIVATE (device);
	if (!priv->vendor)
		bm_device_update_description (device);
	return priv->vendor;
}

typedef struct {
	BMDevice *device;
	BMDeviceDeactivateFn fn;
	gpointer user_data;
} DeactivateInfo;

static void
deactivate_cb (DBusGProxy *proxy,
               GError *error,
               gpointer user_data)
{
	DeactivateInfo *info = user_data;

	if (info->fn)
		info->fn (info->device, error, info->user_data);
	else if (error) {
		g_warning ("%s: device %s deactivation failed: (%d) %s",
		           __func__,
		           bm_object_get_path (BM_OBJECT (info->device)),
		           error ? error->code : -1,
		           error && error->message ? error->message : "(unknown)");
	}

	g_object_unref (info->device);
	g_slice_free (DeactivateInfo, info);
}

