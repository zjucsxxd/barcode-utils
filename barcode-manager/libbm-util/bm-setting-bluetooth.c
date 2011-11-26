/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * Jakob Flierl <jakob.flierl@gmail.com>
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
 * (C) Copyright 2011 Jakob Flierl
 */

#include <string.h>
#include <ctype.h>

#include "bm-param-spec-specialized.h"
#include "bm-dbus-glib-types.h"
#include "bm-setting-bluetooth.h"

GQuark
bm_setting_bluetooth_error_quark (void)
{
	static GQuark quark;

	if (G_UNLIKELY (!quark))
		quark = g_quark_from_static_string ("bm-setting-bluetooth-error-quark");
	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
bm_setting_bluetooth_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (BM_SETTING_BLUETOOTH_ERROR_UNKNOWN, "UnknownError"),
			ENUM_ENTRY (BM_SETTING_BLUETOOTH_ERROR_INVALID_PROPERTY, "InvalidProperty"),
			ENUM_ENTRY (BM_SETTING_BLUETOOTH_ERROR_MISSING_PROPERTY, "MissingProperty"),
			ENUM_ENTRY (BM_SETTING_BLUETOOTH_ERROR_TYPE_SETTING_NOT_FOUND, "TypeSettingNotFound"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("BMSettingBluetoothError", values);
	}
	return etype;
}


G_DEFINE_TYPE (BMSettingBluetooth, bm_setting_bluetooth, BM_TYPE_SETTING)

#define BM_SETTING_BLUETOOTH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_SETTING_BLUETOOTH, BMSettingBluetoothPrivate))

typedef struct {
	GByteArray *bdaddr;
	char *type;
} BMSettingBluetoothPrivate;

enum {
	PROP_0,
	PROP_BDADDR,
	PROP_TYPE,

	LAST_PROP
};

BMSetting *bm_setting_bluetooth_new (void)
{
	return (BMSetting *) g_object_new (BM_TYPE_SETTING_BLUETOOTH, NULL);
}

const char *
bm_setting_bluetooth_get_connection_type (BMSettingBluetooth *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_BLUETOOTH (setting), 0);

	return BM_SETTING_BLUETOOTH_GET_PRIVATE (setting)->type;
}

const GByteArray *
bm_setting_bluetooth_get_bdaddr (BMSettingBluetooth *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_BLUETOOTH (setting), NULL);

	return BM_SETTING_BLUETOOTH_GET_PRIVATE (setting)->bdaddr;
}

static gint
find_setting_by_name (gconstpointer a, gconstpointer b)
{
	BMSetting *setting = BM_SETTING (a);
	const char *str = (const char *) b;

	return strcmp (bm_setting_get_name (setting), str);
}

static gboolean
verify (BMSetting *setting, GSList *all_settings, GError **error)
{
	BMSettingBluetoothPrivate *priv = BM_SETTING_BLUETOOTH_GET_PRIVATE (setting);

	if (!priv->bdaddr) {
		g_set_error (error,
		             BM_SETTING_BLUETOOTH_ERROR,
		             BM_SETTING_BLUETOOTH_ERROR_MISSING_PROPERTY,
		             BM_SETTING_BLUETOOTH_BDADDR);
		return FALSE;
	}

	// FIXME check bt settings here

	if (!priv->type) {
		g_set_error (error,
		             BM_SETTING_BLUETOOTH_ERROR,
		             BM_SETTING_BLUETOOTH_ERROR_MISSING_PROPERTY,
		             BM_SETTING_BLUETOOTH_TYPE);
		return FALSE;
	} else if (!g_str_equal (priv->type, BM_SETTING_BLUETOOTH_TYPE_DUN) &&
		   !g_str_equal (priv->type, BM_SETTING_BLUETOOTH_TYPE_PANU)) {
		g_set_error (error,
		             BM_SETTING_BLUETOOTH_ERROR,
		             BM_SETTING_BLUETOOTH_ERROR_INVALID_PROPERTY,
		             BM_SETTING_BLUETOOTH_TYPE);
		return FALSE;
	}

	return TRUE;
}

static void
bm_setting_bluetooth_init (BMSettingBluetooth *setting)
{
	g_object_set (setting, BM_SETTING_NAME, BM_SETTING_BLUETOOTH_SETTING_NAME, NULL);
}

static void
finalize (GObject *object)
{
	BMSettingBluetoothPrivate *priv = BM_SETTING_BLUETOOTH_GET_PRIVATE (object);

	if (priv->bdaddr)
		g_byte_array_free (priv->bdaddr, TRUE);

	G_OBJECT_CLASS (bm_setting_bluetooth_parent_class)->finalize (object);
}

static void
set_property (GObject *object, guint prop_id,
		    const GValue *value, GParamSpec *pspec)
{
	BMSettingBluetoothPrivate *priv = BM_SETTING_BLUETOOTH_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_BDADDR:
		if (priv->bdaddr)
			g_byte_array_free (priv->bdaddr, TRUE);
		priv->bdaddr = g_value_dup_boxed (value);
		break;
	case PROP_TYPE:
		g_free (priv->type);
		priv->type = g_value_dup_string (value);
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
	BMSettingBluetooth *setting = BM_SETTING_BLUETOOTH (object);

	switch (prop_id) {
	case PROP_BDADDR:
		g_value_set_boxed (value, bm_setting_bluetooth_get_bdaddr (setting));
		break;
	case PROP_TYPE:
		g_value_set_string (value, bm_setting_bluetooth_get_connection_type (setting));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_setting_bluetooth_class_init (BMSettingBluetoothClass *setting_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (setting_class);
	BMSettingClass *parent_class = BM_SETTING_CLASS (setting_class);

	g_type_class_add_private (setting_class, sizeof (BMSettingBluetoothPrivate));

	/* virtual methods */
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->finalize     = finalize;
	parent_class->verify       = verify;

	/* Properties */

	/**
	 * BMSettingBluetooth:bdaddr:
	 *
	 * The Bluetooth address of the device.
	 **/
	g_object_class_install_property
		(object_class, PROP_BDADDR,
		 _bm_param_spec_specialized (BM_SETTING_BLUETOOTH_BDADDR,
		                             "Bluetooth address",
		                             "The Bluetooth address of the device",
		                             DBUS_TYPE_G_UCHAR_ARRAY,
		                             G_PARAM_READWRITE | BM_SETTING_PARAM_SERIALIZE));

	/**
	 * BMSettingBluetooth:type:
	 *
	 * Either 'dun' for Dial-Up Networking connections (not yet supported) or
	 * 'panu' for Personal Area Networking connections.
	 **/
	g_object_class_install_property
		(object_class, PROP_TYPE,
		 g_param_spec_string (BM_SETTING_BLUETOOTH_TYPE,
						  "Connection type",
						  "Either '" BM_SETTING_BLUETOOTH_TYPE_DUN "' for "
						  "Dial-Up Networking connections or "
						  "'" BM_SETTING_BLUETOOTH_TYPE_PANU "' for "
						  "Personal Area Networking connections.",
						  NULL,
						  G_PARAM_READWRITE | BM_SETTING_PARAM_SERIALIZE));
}
