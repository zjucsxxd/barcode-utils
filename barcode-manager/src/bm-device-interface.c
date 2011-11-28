/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- Network link manager
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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2010 Red Hat, Inc.
 */

#include <dbus/dbus-glib.h>

#include "bm-marshal.h"
#include "bm-setting-connection.h"
#include "bm-device-interface.h"
#include "bm-logging.h"
#include "bm-properties-changed-signal.h"

#include "bm-device-interface-glue.h"

GQuark
bm_device_interface_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("bm-device-interface-error");
	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
bm_device_interface_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			/* Connection is already activating. */
			ENUM_ENTRY (BM_DEVICE_INTERFACE_ERROR_CONNECTION_ACTIVATING, "ConnectionActivating"),
			/* Connection is invalid for this device. */
			ENUM_ENTRY (BM_DEVICE_INTERFACE_ERROR_CONNECTION_INVALID, "ConnectionInvalid"),
			/* Operation could not be performed because the device is not active. */
			ENUM_ENTRY (BM_DEVICE_INTERFACE_ERROR_NOT_ACTIVE, "NotActive"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("BMDeviceInterfaceError", values);
	}
	return etype;
}


static void
bm_device_interface_init (gpointer g_iface)
{
	GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	/* Properties */
	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (BM_DEVICE_INTERFACE_UDI,
							  "UDI",
							  "Unique Device Identifier",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (BM_DEVICE_INTERFACE_IFACE,
							  "Interface",
							  "Interface",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (BM_DEVICE_INTERFACE_DRIVER,
							  "Driver",
							  "Driver",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	
	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (BM_DEVICE_INTERFACE_CAPABILITIES,
							"Capabilities",
							"Capabilities",
							0, G_MAXUINT32, BM_DEVICE_CAP_NONE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (BM_DEVICE_INTERFACE_STATE,
							"State",
							"State",
							0, G_MAXUINT32, BM_DEVICE_STATE_UNKNOWN,
							G_PARAM_READABLE));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (BM_DEVICE_INTERFACE_DEVICE_TYPE,
							"DeviceType",
							"DeviceType",
							0, G_MAXUINT32, BM_DEVICE_TYPE_UNKNOWN,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | BM_PROPERTY_PARAM_NO_EXPORT));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (BM_DEVICE_INTERFACE_TYPE_DESC,
							  "Type Description",
							  "Device type description",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | BM_PROPERTY_PARAM_NO_EXPORT));

	/* Signals */
	g_signal_new ("state-changed",
				  iface_type,
				  G_SIGNAL_RUN_FIRST,
				  G_STRUCT_OFFSET (BMDeviceInterface, state_changed),
				  NULL, NULL,
				  _bm_marshal_VOID__UINT_UINT_UINT,
				  G_TYPE_NONE, 3,
				  G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);

	g_signal_new (BM_DEVICE_INTERFACE_DISCONNECT_REQUEST,
	              iface_type,
	              G_SIGNAL_RUN_FIRST,
	              0, NULL, NULL,
	              g_cclosure_marshal_VOID__POINTER,
	              G_TYPE_NONE, 1, G_TYPE_POINTER);

	dbus_g_object_type_install_info (iface_type,
									 &dbus_glib_bm_device_interface_object_info);

	dbus_g_error_domain_register (BM_DEVICE_INTERFACE_ERROR,
	                              NULL,
	                              BM_TYPE_DEVICE_INTERFACE_ERROR);

	initialized = TRUE;
}


GType
bm_device_interface_get_type (void)
{
	static GType device_interface_type = 0;

	if (!device_interface_type) {
		const GTypeInfo device_interface_info = {
			sizeof (BMDeviceInterface), /* class_size */
			bm_device_interface_init,   /* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			0,
			0,              /* n_preallocs */
			NULL
		};

		device_interface_type = g_type_register_static (G_TYPE_INTERFACE,
														"BMDeviceInterface",
														&device_interface_info, 0);

		g_type_interface_add_prerequisite (device_interface_type, G_TYPE_OBJECT);
	}

	return device_interface_type;
}

/* FIXME: This should be public and bm_device_get_iface() should be removed. */
static char *
bm_device_interface_get_iface (BMDeviceInterface *device)
{
	char *iface = NULL;

	g_return_val_if_fail (BM_IS_DEVICE_INTERFACE (device), NULL);

	g_object_get (device, BM_DEVICE_INTERFACE_IFACE, &iface, NULL);

	return iface;
}

gboolean
bm_device_interface_check_connection_compatible (BMDeviceInterface *device,
                                                 BMConnection *connection,
                                                 GError **error)
{
	g_return_val_if_fail (BM_IS_DEVICE_INTERFACE (device), FALSE);
	g_return_val_if_fail (BM_IS_CONNECTION (connection), FALSE);
	g_return_val_if_fail (error != NULL, FALSE);
	g_return_val_if_fail (*error == NULL, FALSE);

	if (BM_DEVICE_INTERFACE_GET_INTERFACE (device)->check_connection_compatible)
		return BM_DEVICE_INTERFACE_GET_INTERFACE (device)->check_connection_compatible (device, connection, error);
	return TRUE;
}

BMDeviceState
bm_device_interface_get_state (BMDeviceInterface *device)
{
	BMDeviceState state;

	g_object_get (G_OBJECT (device), "state", &state, NULL);
	return state;
}

BMConnection *
bm_device_interface_connection_match_config (BMDeviceInterface *device,
                                             const GSList *connections)
{
	g_return_val_if_fail (BM_IS_DEVICE_INTERFACE (device), NULL);

	if (BM_DEVICE_INTERFACE_GET_INTERFACE (device)->connection_match_config)
		return BM_DEVICE_INTERFACE_GET_INTERFACE (device)->connection_match_config (device, connections);
	return NULL;
}

gboolean
bm_device_interface_can_assume_connections (BMDeviceInterface *device)
{
	g_return_val_if_fail (BM_IS_DEVICE_INTERFACE (device), FALSE);

	if (BM_DEVICE_INTERFACE_GET_INTERFACE (device)->can_assume_connections)
		return BM_DEVICE_INTERFACE_GET_INTERFACE (device)->can_assume_connections (device);
	return FALSE;
}

void
bm_device_interface_set_enabled (BMDeviceInterface *device, gboolean enabled)
{
	g_return_if_fail (BM_IS_DEVICE_INTERFACE (device));

	if (BM_DEVICE_INTERFACE_GET_INTERFACE (device)->set_enabled)
		BM_DEVICE_INTERFACE_GET_INTERFACE (device)->set_enabled (device, enabled);
}

gboolean
bm_device_interface_get_enabled (BMDeviceInterface *device)
{
	g_return_val_if_fail (BM_IS_DEVICE_INTERFACE (device), FALSE);

	if (BM_DEVICE_INTERFACE_GET_INTERFACE (device)->get_enabled)
		return BM_DEVICE_INTERFACE_GET_INTERFACE (device)->get_enabled (device);
	return TRUE;
}

