/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * Dan Williams <dcbw@redhat.com>
 * Tambet Ingo <tambet@gmail.com>
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
 * (C) Copyright 2007 - 2008 Red Hat, Inc.
 * (C) Copyright 2007 - 2008 Novell, Inc.
 */

#include <string.h>
#include <ctype.h>
#include "bm-setting-connection.h"

/**
 * SECTION:bm-setting-connection
 * @short_description: Describes general connection properties
 * @include: bm-setting-connection.h
 *
 * The #BMSettingConnection object is a #BMSetting subclass that describes
 * properties that apply to all #BMConnection objects, regardless of what type
 * of network connection they describe.  Each #BMConnection object must contain
 * a #BMSettingConnection setting.
 **/

/**
 * bm_setting_connection_error_quark:
 *
 * Registers an error quark for #BMSettingConnection if necessary.
 *
 * Returns: the error quark used for #BMSettingConnection errors.
 **/
GQuark
bm_setting_connection_error_quark (void)
{
	static GQuark quark;

	if (G_UNLIKELY (!quark))
		quark = g_quark_from_static_string ("bm-setting-connection-error-quark");
	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
bm_setting_connection_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (BM_SETTING_CONNECTION_ERROR_UNKNOWN, "UnknownError"),
			ENUM_ENTRY (BM_SETTING_CONNECTION_ERROR_INVALID_PROPERTY, "InvalidProperty"),
			ENUM_ENTRY (BM_SETTING_CONNECTION_ERROR_MISSING_PROPERTY, "MissingProperty"),
			ENUM_ENTRY (BM_SETTING_CONNECTION_ERROR_TYPE_SETTING_NOT_FOUND, "TypeSettingNotFound"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("BMSettingConnectionError", values);
	}
	return etype;
}


G_DEFINE_TYPE (BMSettingConnection, bm_setting_connection, BM_TYPE_SETTING)

#define BM_SETTING_CONNECTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_SETTING_CONNECTION, BMSettingConnectionPrivate))

typedef struct {
	char *id;
	char *uuid;
	char *type;
	gboolean autoconnect;
	guint64 timestamp;
	gboolean read_only;
} BMSettingConnectionPrivate;

enum {
	PROP_0,
	PROP_ID,
	PROP_UUID,
	PROP_TYPE,
	PROP_AUTOCONNECT,
	PROP_TIMESTAMP,
	PROP_READ_ONLY,

	LAST_PROP
};

/**
 * bm_setting_connection_new:
 *
 * Creates a new #BMSettingConnection object with default values.
 *
 * Returns: the new empty #BMSettingConnection object
 **/
BMSetting *bm_setting_connection_new (void)
{
	return (BMSetting *) g_object_new (BM_TYPE_SETTING_CONNECTION, NULL);
}

/**
 * bm_setting_connection_get_id:
 * @setting: the #BMSettingConnection
 *
 * Returns the #BMSettingConnection:id property of the connection.
 *
 * Returns: the connection ID
 **/
const char *
bm_setting_connection_get_id (BMSettingConnection *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), NULL);

	return BM_SETTING_CONNECTION_GET_PRIVATE (setting)->id;
}

/**
 * bm_setting_connection_get_uuid:
 * @setting: the #BMSettingConnection
 *
 * Returns the #BMSettingConnection:uuid property of the connection.
 *
 * Returns: the connection UUID
 **/
const char *
bm_setting_connection_get_uuid (BMSettingConnection *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), NULL);

	return BM_SETTING_CONNECTION_GET_PRIVATE (setting)->uuid;
}

/**
 * bm_setting_connection_get_connection_type:
 * @setting: the #BMSettingConnection
 *
 * Returns the #BMSettingConnection:type property of the connection.
 *
 * Returns: the connection type
 **/
const char *
bm_setting_connection_get_connection_type (BMSettingConnection *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), NULL);

	return BM_SETTING_CONNECTION_GET_PRIVATE (setting)->type;
}

/**
 * bm_setting_connection_get_autoconnect:
 * @setting: the #BMSettingConnection
 *
 * Returns the #BMSettingConnection:autoconnect property of the connection.
 *
 * Returns: the connection's autoconnect behavior
 **/
gboolean
bm_setting_connection_get_autoconnect (BMSettingConnection *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), FALSE);

	return BM_SETTING_CONNECTION_GET_PRIVATE (setting)->autoconnect;
}

/**
 * bm_setting_connection_get_timestamp:
 * @setting: the #BMSettingConnection
 *
 * Returns the #BMSettingConnection:timestamp property of the connection.
 *
 * Returns: the connection's timestamp
 **/
guint64
bm_setting_connection_get_timestamp (BMSettingConnection *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), 0);

	return BM_SETTING_CONNECTION_GET_PRIVATE (setting)->timestamp;
}

/**
 * bm_setting_connection_get_read_only:
 * @setting: the #BMSettingConnection
 *
 * Returns the #BMSettingConnection:read-only property of the connection.
 *
 * Returns: %TRUE if the connection is read-only, %FALSE if it is not
 **/
gboolean
bm_setting_connection_get_read_only (BMSettingConnection *setting)
{
	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), TRUE);

	return BM_SETTING_CONNECTION_GET_PRIVATE (setting)->read_only;
}

static gint
find_setting_by_name (gconstpointer a, gconstpointer b)
{
	BMSetting *setting = BM_SETTING (a);
	const char *str = (const char *) b;

	return strcmp (bm_setting_get_name (setting), str);
}

static gboolean
validate_uuid (const char *uuid)
{
	int i;

	if (!uuid || !strlen (uuid))
		return FALSE;

	for (i = 0; i < strlen (uuid); i++) {
		if (!isxdigit (uuid[i]) && (uuid[i] != '-'))
			return FALSE;
	}

	return TRUE;
}

static gboolean
verify (BMSetting *setting, GSList *all_settings, GError **error)
{
	BMSettingConnectionPrivate *priv = BM_SETTING_CONNECTION_GET_PRIVATE (setting);

	if (!priv->id) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_MISSING_PROPERTY,
		             BM_SETTING_CONNECTION_ID);
		return FALSE;
	} else if (!strlen (priv->id)) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_INVALID_PROPERTY,
		             BM_SETTING_CONNECTION_ID);
		return FALSE;
	}

	if (!priv->uuid) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_MISSING_PROPERTY,
		             BM_SETTING_CONNECTION_UUID);
		return FALSE;
	} else if (!validate_uuid (priv->uuid)) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_INVALID_PROPERTY,
		             BM_SETTING_CONNECTION_UUID);
		return FALSE;
	}

	if (!priv->type) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_MISSING_PROPERTY,
		             BM_SETTING_CONNECTION_TYPE);
		return FALSE;
	} else if (!strlen (priv->type)) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_INVALID_PROPERTY,
		             BM_SETTING_CONNECTION_TYPE);
		return FALSE;
	}

	/* Make sure the corresponding 'type' item is present */
	if (all_settings && !g_slist_find_custom (all_settings, priv->type, find_setting_by_name)) {
		g_set_error (error,
		             BM_SETTING_CONNECTION_ERROR,
		             BM_SETTING_CONNECTION_ERROR_TYPE_SETTING_NOT_FOUND,
		             BM_SETTING_CONNECTION_TYPE);
		return FALSE;
	}

	return TRUE;
}

static void
bm_setting_connection_init (BMSettingConnection *setting)
{
	g_object_set (setting, BM_SETTING_NAME, BM_SETTING_CONNECTION_SETTING_NAME, NULL);
}

static void
finalize (GObject *object)
{
	BMSettingConnectionPrivate *priv = BM_SETTING_CONNECTION_GET_PRIVATE (object);

	g_free (priv->id);
	g_free (priv->uuid);
	g_free (priv->type);

	G_OBJECT_CLASS (bm_setting_connection_parent_class)->finalize (object);
}

static void
set_property (GObject *object, guint prop_id,
		    const GValue *value, GParamSpec *pspec)
{
	BMSettingConnectionPrivate *priv = BM_SETTING_CONNECTION_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_ID:
		g_free (priv->id);
		priv->id = g_value_dup_string (value);
		break;
	case PROP_UUID:
		g_free (priv->uuid);
		priv->uuid = g_value_dup_string (value);
		break;
	case PROP_TYPE:
		g_free (priv->type);
		priv->type = g_value_dup_string (value);
		break;
	case PROP_AUTOCONNECT:
		priv->autoconnect = g_value_get_boolean (value);
		break;
	case PROP_TIMESTAMP:
		priv->timestamp = g_value_get_uint64 (value);
		break;
	case PROP_READ_ONLY:
		priv->read_only = g_value_get_boolean (value);
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
	BMSettingConnection *setting = BM_SETTING_CONNECTION (object);

	switch (prop_id) {
	case PROP_ID:
		g_value_set_string (value, bm_setting_connection_get_id (setting));
		break;
	case PROP_UUID:
		g_value_set_string (value, bm_setting_connection_get_uuid (setting));
		break;
	case PROP_TYPE:
		g_value_set_string (value, bm_setting_connection_get_connection_type (setting));
		break;
	case PROP_AUTOCONNECT:
		g_value_set_boolean (value, bm_setting_connection_get_autoconnect (setting));
		break;
	case PROP_TIMESTAMP:
		g_value_set_uint64 (value, bm_setting_connection_get_timestamp (setting));
		break;
	case PROP_READ_ONLY:
		g_value_set_boolean (value, bm_setting_connection_get_read_only (setting));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_setting_connection_class_init (BMSettingConnectionClass *setting_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (setting_class);
	BMSettingClass *parent_class = BM_SETTING_CLASS (setting_class);

	g_type_class_add_private (setting_class, sizeof (BMSettingConnectionPrivate));

	/* virtual methods */
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->finalize     = finalize;
	parent_class->verify       = verify;

	/* Properties */

	/**
	 * BMSettingConnection:id:
	 *
	 * A human readable unique idenfier for the connection, like "Work WiFi" or
	 * "T-Mobile 3G".
	 **/
	g_object_class_install_property
		(object_class, PROP_ID,
		 g_param_spec_string (BM_SETTING_CONNECTION_ID,
						  "ID",
						  "User-readable connection identifier/name.  Must be "
						  "one or more characters and may change over the lifetime "
						  "of the connection if the user decides to rename it.",
						  NULL,
						  G_PARAM_READWRITE | BM_SETTING_PARAM_SERIALIZE | BM_SETTING_PARAM_FUZZY_IGNORE));

	/**
	 * BMSettingConnection:uuid:
	 *
	 * A universally unique idenfier for the connection, for example generated
	 * with libuuid.  Should be assigned when the connection is created, and
	 * never changed as long as the connection still applies to the same
	 * network.  For example, should not be changed when the
	 * #BMSettingConnection:id or #BMSettingIP4Config changes, but might need
	 * to be re-created when the WiFi SSID, mobile broadband network provider,
	 * or #BMSettingConnection:type changes.
	 *
	 * The UUID must be in the format '2815492f-7e56-435e-b2e9-246bd7cdc664'
	 * (ie, contains only hexadecimal characters and '-').  A suitable UUID may
	 * be generated by bm_utils_uuid_generate() or
	 * bm_utils_uuid_generate_from_string().
	 **/
	g_object_class_install_property
		(object_class, PROP_UUID,
		 g_param_spec_string (BM_SETTING_CONNECTION_UUID,
						  "UUID",
						  "Universally unique connection identifier.  Must be "
						  "in the format '2815492f-7e56-435e-b2e9-246bd7cdc664' "
						  "(ie, contains only hexadecimal characters and '-'). "
						  "The UUID should be assigned when the connection is "
						  "created and never changed as long as the connection "
						  "still applies to the same network.  For example, "
						  "it should not be changed when the user changes the "
						  "connection's 'id', but should be recreated when the "
						  "WiFi SSID, mobile broadband network provider, or the "
						  "connection type changes.",
						  NULL,
						  G_PARAM_READWRITE | BM_SETTING_PARAM_SERIALIZE | BM_SETTING_PARAM_FUZZY_IGNORE));

	/**
	 * BMSettingConnection:type:
	 *
	 * The general hardware type of the device used for the network connection,
	 * contains the name of the #BMSetting object that describes that hardware
	 * type's parameters.  For example, for WiFi devices, the name of the
	 * #BMSettingWireless setting.
	 **/
	g_object_class_install_property
		(object_class, PROP_TYPE,
		 g_param_spec_string (BM_SETTING_CONNECTION_TYPE,
						  "Type",
						  "Base type of the connection.  For hardware-dependent "
						  "connections, should contain the setting name of the "
						  "hardware-type specific setting (ie, '802-3-ethernet' "
						  "or '802-11-wireless' or 'bluetooth', etc), and for "
						  "non-hardware dependent connections like VPN or "
						  "otherwise, should contain the setting name of that "
						  "setting type (ie, 'vpn' or 'bridge', etc).",
						  NULL,
						  G_PARAM_READWRITE | BM_SETTING_PARAM_SERIALIZE));

	/**
	 * BMSettingConnection:autoconnect:
	 *
	 * Whether or not the connection should be automatically connected by
	 * BarcodeManager when the resources for the connection are available.
	 * %TRUE to automatically activate the connection, %FALSE to require manual
	 * intervention to activate the connection.  Defaults to %TRUE.
	 **/
	g_object_class_install_property
		(object_class, PROP_AUTOCONNECT,
		 g_param_spec_boolean (BM_SETTING_CONNECTION_AUTOCONNECT,
						   "Autoconnect",
						   "If TRUE, BarcodeManager will activate this connection "
						   "when its network resources are available.  If FALSE, "
						   "the connection must be manually activated by the user "
						   "or some other mechanism.",
						   TRUE,
						   G_PARAM_READWRITE | G_PARAM_CONSTRUCT | BM_SETTING_PARAM_SERIALIZE | BM_SETTING_PARAM_FUZZY_IGNORE));

	/**
	 * BMSettingConnection:timestamp:
	 *
	 * The time, in seconds since the Unix Epoch, that the connection was last
	 * _successfully_ fully activated.
	 **/
	g_object_class_install_property
		(object_class, PROP_TIMESTAMP,
		 g_param_spec_uint64 (BM_SETTING_CONNECTION_TIMESTAMP,
						  "Timestamp",
						  "Timestamp (in seconds since the Unix Epoch) that the "
						  "connection was last successfully activated.  Settings "
						  "services should update the connection timestamp "
						  "periodically when the connection is active to ensure "
						  "that an active connection has the latest timestamp.",
						  0, G_MAXUINT64, 0,
						  G_PARAM_READWRITE | G_PARAM_CONSTRUCT | BM_SETTING_PARAM_SERIALIZE | BM_SETTING_PARAM_FUZZY_IGNORE));

	/**
	 * BMSettingConnection:read-only:
	 *
	 * %TRUE if the connection can be modified using the providing settings
	 * service's D-Bus interface with the right privileges, or %FALSE
	 * if the connection is read-only and cannot be modified.
	 **/
	g_object_class_install_property
	    (object_class, PROP_READ_ONLY,
	     g_param_spec_boolean (BM_SETTING_CONNECTION_READ_ONLY,
	                      "Read-Only",
	                      "If TRUE, the connection is read-only and cannot be "
	                      "changed by the user or any other mechanism.  This is "
	                      "normally set for system connections whose plugin "
	                      "cannot yet write updated connections back out.",
	                      FALSE,
	                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | BM_SETTING_PARAM_SERIALIZE | BM_SETTING_PARAM_FUZZY_IGNORE));
}
