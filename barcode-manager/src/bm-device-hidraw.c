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

#include <stdio.h>
#include <string.h>
#include <net/ethernet.h>

#include "bm-glib-compat.h"
#include "bm-dbus-manager.h"
#include "bm-device-hidraw.h"
#include "bm-device-interface.h"
#include "bm-device-private.h"
#include "bm-logging.h"
#include "bm-marshal.h"
#include "bm-properties-changed-signal.h"
#include "bm-setting-connection.h"
#include "bm-setting-bluetooth.h"
#include "bm-device-hidraw-glue.h"
#include "BarcodeManagerUtils.h"

G_DEFINE_TYPE (BMDeviceHidraw, bm_device_hidraw, BM_TYPE_DEVICE)

#define BM_DEVICE_HIDRAW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawPrivate))

typedef struct {
	gboolean            disposed;

	char *bdaddr;
	char *name;
	guint32 capabilities;

	gboolean connected;
	gboolean have_iface;

	DBusGProxy *type_proxy;
	DBusGProxy *dev_proxy;

	char *rfcomm_iface;
	guint32 timeout_id;

	guint32 hidraw_type;  /* HIDRAW type of the current connection */

	
} BMDeviceHidrawPrivate;

enum {
	PROP_0,
	LAST_PROP
};

enum {
	PROPERTIES_CHANGED,

	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };


typedef enum {
	BM_HIDRAW_ERROR_CONNECTION_NOT_HIDRAW = 0,
	BM_HIDRAW_ERROR_CONNECTION_INVALID,
	BM_HIDRAW_ERROR_CONNECTION_INCOMPATIBLE,
} NMHidrawError;

#define BM_HIDRAW_ERROR (bm_hidraw_error_quark ())
#define BM_TYPE_HIDRAW_ERROR (bm_hidraw_error_get_type ())

static GQuark
bm_hidraw_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("bm-hidraw-error");
	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

static GType
bm_hidraw_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			/* Connection was not a HIDRAW connection. */
			ENUM_ENTRY (BM_HIDRAW_ERROR_CONNECTION_NOT_HIDRAW, "ConnectionNotHidraw"),
			/* Connection was not a valid HIDRAW connection. */
			ENUM_ENTRY (BM_HIDRAW_ERROR_CONNECTION_INVALID, "ConnectionInvalid"),
			/* Connection does not apply to this device. */
			ENUM_ENTRY (BM_HIDRAW_ERROR_CONNECTION_INCOMPATIBLE, "ConnectionIncompatible"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("NMHidrawError", values);
	}
	return etype;
}


guint32 bm_device_hidraw_get_capabilities (BMDeviceHidraw *self)
{
	g_return_val_if_fail (self != NULL, BM_HIDRAW_CAPABILITY_NONE);
	g_return_val_if_fail (BM_IS_DEVICE_HIDRAW (self), BM_HIDRAW_CAPABILITY_NONE);

	return BM_DEVICE_HIDRAW_GET_PRIVATE (self)->capabilities;
}

const char *bm_device_hidraw_get_hw_address (BMDeviceHidraw *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (BM_IS_DEVICE_HIDRAW (self), NULL);

	return BM_DEVICE_HIDRAW_GET_PRIVATE (self)->bdaddr;
}

static guint32
get_connection_hidraw_type (BMConnection *connection)
{
	BMSettingBluetooth *s_hidraw;
	const char *hidraw_type;

	s_hidraw = (BMSettingBluetooth *) bm_connection_get_setting (connection, BM_TYPE_SETTING_BLUETOOTH);
	if (!s_hidraw)
		return BM_HIDRAW_CAPABILITY_NONE;

	hidraw_type = bm_setting_bluetooth_get_connection_type (s_hidraw);
	g_assert (hidraw_type);

	if (!strcmp (hidraw_type, BM_SETTING_BLUETOOTH_TYPE_DUN))
		return BM_HIDRAW_CAPABILITY_DUN;
	else if (!strcmp (hidraw_type, BM_SETTING_BLUETOOTH_TYPE_PANU))
		return BM_HIDRAW_CAPABILITY_NAP;

	return BM_HIDRAW_CAPABILITY_NONE;
}

static BMConnection *
real_get_best_auto_connection (BMDevice *device,
                               GSList *connections,
                               char **specific_object)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (device);
	GSList *iter;

	for (iter = connections; iter; iter = g_slist_next (iter)) {
		BMConnection *connection = BM_CONNECTION (iter->data);
		BMSettingConnection *s_con;
		guint32 hidraw_type;

		s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
		g_assert (s_con);

		if (!bm_setting_connection_get_autoconnect (s_con))
			continue;

		if (strcmp (bm_setting_connection_get_connection_type (s_con), BM_SETTING_BLUETOOTH_SETTING_NAME))
			continue;

		hidraw_type = get_connection_hidraw_type (connection);
		if (!(hidraw_type & priv->capabilities))
			continue;

		return connection;
	}
	return NULL;
}

static gboolean
real_check_connection_compatible (BMDevice *device,
                                  BMConnection *connection,
                                  GError **error)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (device);
	BMSettingConnection *s_con;
	BMSettingBluetooth *s_hidraw;
	const GByteArray *array;
	char *str;
	int addr_match = FALSE;
	guint32 hidraw_type;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	if (strcmp (bm_setting_connection_get_connection_type (s_con), BM_SETTING_BLUETOOTH_SETTING_NAME)) {
		g_set_error (error,
		             BM_HIDRAW_ERROR, BM_HIDRAW_ERROR_CONNECTION_NOT_HIDRAW,
		             "The connection was not a Bluetooth connection.");
		return FALSE;
	}

	s_hidraw = BM_SETTING_BLUETOOTH (bm_connection_get_setting (connection, BM_TYPE_SETTING_BLUETOOTH));
	if (!s_hidraw) {
		g_set_error (error,
		             BM_HIDRAW_ERROR, BM_HIDRAW_ERROR_CONNECTION_INVALID,
		             "The connection was not a valid Bluetooth connection.");
		return FALSE;
	}

	array = bm_setting_bluetooth_get_bdaddr (s_hidraw);
	if (!array || (array->len != ETH_ALEN)) {
		g_set_error (error,
		             BM_HIDRAW_ERROR, BM_HIDRAW_ERROR_CONNECTION_INVALID,
		             "The connection did not contain a valid Bluetooth address.");
		return FALSE;
	}

	hidraw_type = get_connection_hidraw_type (connection);
	if (!(hidraw_type & priv->capabilities)) {
		g_set_error (error,
		             BM_HIDRAW_ERROR, BM_HIDRAW_ERROR_CONNECTION_INCOMPATIBLE,
		             "The connection was not compatible with the device's capabilities.");
		return FALSE;
	}

	str = g_strdup_printf ("%02X:%02X:%02X:%02X:%02X:%02X",
	                       array->data[0], array->data[1], array->data[2],
	                       array->data[3], array->data[4], array->data[5]);
	addr_match = !strcmp (priv->bdaddr, str);
	g_free (str);

	return addr_match;
}

static guint32
real_get_generic_capabilities (BMDevice *dev)
{
	return BM_DEVICE_CAP_BM_SUPPORTED;
}

static void
device_state_changed (BMDevice *device,
                      BMDeviceState new_state,
                      BMDeviceState old_state,
                      BMDeviceStateReason reason,
                      gpointer user_data)
{

	switch (new_state) {
	case BM_DEVICE_STATE_ACTIVATED:
	case BM_DEVICE_STATE_FAILED:
	case BM_DEVICE_STATE_DISCONNECTED:
		// clear_secrets_tries (device);
		break;
	default:
		break;
	}
}

BMDevice *
bm_device_hidraw_new (const char *udi,
					  const char *bdaddr,
					  const char *name)
{
	BMDevice *device;

	bm_log_dbg (LOGD_CORE, "udi = %s", udi);
	bm_log_dbg (LOGD_CORE, "bdaddr = %s", bdaddr);
	bm_log_dbg (LOGD_CORE, "name = %s", name);


	g_return_val_if_fail (udi != NULL, NULL);
	g_return_val_if_fail (bdaddr != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	device = (BMDevice *) g_object_new (BM_TYPE_DEVICE_HIDRAW,
										BM_DEVICE_INTERFACE_UDI, udi,
										BM_DEVICE_INTERFACE_IFACE, bdaddr,
										BM_DEVICE_INTERFACE_DRIVER, "hidraw",
										BM_DEVICE_INTERFACE_TYPE_DESC, "Hidraw",
										BM_DEVICE_INTERFACE_DEVICE_TYPE, BM_DEVICE_TYPE_HIDRAW,
										NULL);
	return device;
}

static void
bm_device_hidraw_init (BMDeviceHidraw *self)
{
	g_signal_connect (self, "state-changed", G_CALLBACK (device_state_changed), NULL);
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
finalize (GObject *object)
{
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	if (priv->type_proxy)
		g_object_unref (priv->type_proxy);

	if (priv->dev_proxy)
		g_object_unref (priv->dev_proxy);

	g_free (priv->rfcomm_iface);
	g_free (priv->bdaddr);
	g_free (priv->name);

	G_OBJECT_CLASS (bm_device_hidraw_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	BMDeviceHidraw *self = BM_DEVICE_HIDRAW (object);
	BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (self);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_device_hidraw_parent_class)->dispose (object);
		return;
	}

	priv->disposed = TRUE;

	G_OBJECT_CLASS (bm_device_hidraw_parent_class)->dispose (object);
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	GObject *object;
	BMDeviceHidrawPrivate *priv;
	BMDevice *self;
	guint32 caps;

	object = G_OBJECT_CLASS (bm_device_hidraw_parent_class)->constructor (type,
																		  n_construct_params,
																		  construct_params);
	if (!object)
		return NULL;

	self = BM_DEVICE (object);
	priv = BM_DEVICE_HIDRAW_GET_PRIVATE (self);

	bm_log_dbg (LOGD_HW, "(%s)", bm_device_get_iface (BM_DEVICE (self)));

	return object;
}

static void
bm_device_hidraw_class_init (BMDeviceHidrawClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BMDeviceClass *device_class = BM_DEVICE_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (BMDeviceHidrawPrivate));

	object_class->constructor = constructor;
	object_class->dispose = dispose;

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	device_class->get_generic_capabilities = real_get_generic_capabilities;

	/* Properties */

	/* Signals */
	signals[PROPERTIES_CHANGED] = 
		bm_properties_changed_signal_new (object_class,
		                                  G_STRUCT_OFFSET (BMDeviceHidrawClass, properties_changed));

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
	                                 &dbus_glib_bm_device_hidraw_object_info);

	dbus_g_error_domain_register (BM_HIDRAW_ERROR, NULL, BM_TYPE_HIDRAW_ERROR);
}
