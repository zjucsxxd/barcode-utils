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
#include "bm-device-bt.h"
#include "bm-device-interface.h"
#include "bm-device-private.h"
#include "bm-logging.h"
#include "bm-marshal.h"
#include "bm-properties-changed-signal.h"
#include "bm-setting-connection.h"
#include "bm-setting-bluetooth.h"
#include "bm-device-bt-glue.h"
#include "BarcodeManagerUtils.h"

G_DEFINE_TYPE (BMDeviceBt, bm_device_bt, BM_TYPE_DEVICE)

#define BM_DEVICE_BT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_DEVICE_BT, BMDeviceBtPrivate))

typedef struct {
	char *bdaddr;
	char *name;
	guint32 capabilities;

	gboolean connected;
	gboolean have_iface;

	DBusGProxy *type_proxy;
	DBusGProxy *dev_proxy;

	char *rfcomm_iface;
	guint32 timeout_id;

	guint32 bt_type;  /* BT type of the current connection */
} BMDeviceBtPrivate;

enum {
	PROP_0,
	PROP_HW_ADDRESS,
	PROP_BT_NAME,
	PROP_BT_CAPABILITIES,

	LAST_PROP
};

enum {
	PPP_STATS,
	PROPERTIES_CHANGED,

	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };


typedef enum {
	BM_BT_ERROR_CONNECTION_NOT_BT = 0,
	BM_BT_ERROR_CONNECTION_INVALID,
	BM_BT_ERROR_CONNECTION_INCOMPATIBLE,
} NMBtError;

#define BM_BT_ERROR (bm_bt_error_quark ())
#define BM_TYPE_BT_ERROR (bm_bt_error_get_type ())

static GQuark
bm_bt_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("bm-bt-error");
	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

static GType
bm_bt_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			/* Connection was not a BT connection. */
			ENUM_ENTRY (BM_BT_ERROR_CONNECTION_NOT_BT, "ConnectionNotBt"),
			/* Connection was not a valid BT connection. */
			ENUM_ENTRY (BM_BT_ERROR_CONNECTION_INVALID, "ConnectionInvalid"),
			/* Connection does not apply to this device. */
			ENUM_ENTRY (BM_BT_ERROR_CONNECTION_INCOMPATIBLE, "ConnectionIncompatible"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("NMBtError", values);
	}
	return etype;
}


guint32 bm_device_bt_get_capabilities (BMDeviceBt *self)
{
	g_return_val_if_fail (self != NULL, BM_BT_CAPABILITY_NONE);
	g_return_val_if_fail (BM_IS_DEVICE_BT (self), BM_BT_CAPABILITY_NONE);

	return BM_DEVICE_BT_GET_PRIVATE (self)->capabilities;
}

const char *bm_device_bt_get_hw_address (BMDeviceBt *self)
{
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (BM_IS_DEVICE_BT (self), NULL);

	return BM_DEVICE_BT_GET_PRIVATE (self)->bdaddr;
}

static guint32
get_connection_bt_type (BMConnection *connection)
{
	BMSettingBluetooth *s_bt;
	const char *bt_type;

	s_bt = (BMSettingBluetooth *) bm_connection_get_setting (connection, BM_TYPE_SETTING_BLUETOOTH);
	if (!s_bt)
		return BM_BT_CAPABILITY_NONE;

	bt_type = bm_setting_bluetooth_get_connection_type (s_bt);
	g_assert (bt_type);

	if (!strcmp (bt_type, BM_SETTING_BLUETOOTH_TYPE_DUN))
		return BM_BT_CAPABILITY_DUN;
	else if (!strcmp (bt_type, BM_SETTING_BLUETOOTH_TYPE_PANU))
		return BM_BT_CAPABILITY_NAP;

	return BM_BT_CAPABILITY_NONE;
}

static BMConnection *
real_get_best_auto_connection (BMDevice *device,
                               GSList *connections,
                               char **specific_object)
{
	BMDeviceBtPrivate *priv = BM_DEVICE_BT_GET_PRIVATE (device);
	GSList *iter;

	for (iter = connections; iter; iter = g_slist_next (iter)) {
		BMConnection *connection = BM_CONNECTION (iter->data);
		BMSettingConnection *s_con;
		guint32 bt_type;

		s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
		g_assert (s_con);

		if (!bm_setting_connection_get_autoconnect (s_con))
			continue;

		if (strcmp (bm_setting_connection_get_connection_type (s_con), BM_SETTING_BLUETOOTH_SETTING_NAME))
			continue;

		bt_type = get_connection_bt_type (connection);
		if (!(bt_type & priv->capabilities))
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
	BMDeviceBtPrivate *priv = BM_DEVICE_BT_GET_PRIVATE (device);
	BMSettingConnection *s_con;
	BMSettingBluetooth *s_bt;
	const GByteArray *array;
	char *str;
	int addr_match = FALSE;
	guint32 bt_type;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	if (strcmp (bm_setting_connection_get_connection_type (s_con), BM_SETTING_BLUETOOTH_SETTING_NAME)) {
		g_set_error (error,
		             BM_BT_ERROR, BM_BT_ERROR_CONNECTION_NOT_BT,
		             "The connection was not a Bluetooth connection.");
		return FALSE;
	}

	s_bt = BM_SETTING_BLUETOOTH (bm_connection_get_setting (connection, BM_TYPE_SETTING_BLUETOOTH));
	if (!s_bt) {
		g_set_error (error,
		             BM_BT_ERROR, BM_BT_ERROR_CONNECTION_INVALID,
		             "The connection was not a valid Bluetooth connection.");
		return FALSE;
	}

	array = bm_setting_bluetooth_get_bdaddr (s_bt);
	if (!array || (array->len != ETH_ALEN)) {
		g_set_error (error,
		             BM_BT_ERROR, BM_BT_ERROR_CONNECTION_INVALID,
		             "The connection did not contain a valid Bluetooth address.");
		return FALSE;
	}

	bt_type = get_connection_bt_type (connection);
	if (!(bt_type & priv->capabilities)) {
		g_set_error (error,
		             BM_BT_ERROR, BM_BT_ERROR_CONNECTION_INCOMPATIBLE,
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
bm_device_bt_init (BMDeviceBt *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	BMDeviceBtPrivate *priv = BM_DEVICE_BT_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_HW_ADDRESS:
		/* Construct only */
		priv->bdaddr = g_ascii_strup (g_value_get_string (value), -1);
		break;
	case PROP_BT_NAME:
		/* Construct only */
		priv->name = g_value_dup_string (value);
		break;
	case PROP_BT_CAPABILITIES:
		/* Construct only */
		priv->capabilities = g_value_get_uint (value);
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
	BMDeviceBtPrivate *priv = BM_DEVICE_BT_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_HW_ADDRESS:
		g_value_set_string (value, priv->bdaddr);
		break;
	case PROP_BT_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_BT_CAPABILITIES:
		g_value_set_uint (value, priv->capabilities);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
finalize (GObject *object)
{
	BMDeviceBtPrivate *priv = BM_DEVICE_BT_GET_PRIVATE (object);

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

	G_OBJECT_CLASS (bm_device_bt_parent_class)->finalize (object);
}

static void
bm_device_bt_class_init (BMDeviceBtClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BMDeviceClass *device_class = BM_DEVICE_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (BMDeviceBtPrivate));

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	device_class->get_generic_capabilities = real_get_generic_capabilities;

	/* Properties */
	g_object_class_install_property
		(object_class, PROP_HW_ADDRESS,
		 g_param_spec_string (BM_DEVICE_BT_HW_ADDRESS,
		                      "Bluetooth address",
		                      "Bluetooth address",
		                      NULL,
		                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class, PROP_BT_NAME,
		 g_param_spec_string (BM_DEVICE_BT_NAME,
		                      "Bluetooth device name",
		                      "Bluetooth device name",
		                      NULL,
		                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class, PROP_BT_CAPABILITIES,
		 g_param_spec_uint (BM_DEVICE_BT_CAPABILITIES,
		                    "Bluetooth device capabilities",
		                    "Bluetooth device capabilities",
		                    BM_BT_CAPABILITY_NONE, G_MAXUINT, BM_BT_CAPABILITY_NONE,
		                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/* Signals */
	signals[PPP_STATS] =
		g_signal_new ("ppp-stats",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMDeviceBtClass, ppp_stats),
		              NULL, NULL,
		              _bm_marshal_VOID__UINT_UINT,
		              G_TYPE_NONE, 2,
		              G_TYPE_UINT, G_TYPE_UINT);

	signals[PROPERTIES_CHANGED] = 
		bm_properties_changed_signal_new (object_class,
		                                  G_STRUCT_OFFSET (BMDeviceBtClass, properties_changed));

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
	                                 &dbus_glib_bm_device_bt_object_info);

	dbus_g_error_domain_register (BM_BT_ERROR, NULL, BM_TYPE_BT_ERROR);
}
