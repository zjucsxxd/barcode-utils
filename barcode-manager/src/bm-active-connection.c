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

#include <glib.h>
#include "bm-active-connection.h"
#include "BarcodeManager.h"
#include "bm-logging.h"
#include "bm-dbus-glib-types.h"

char *
bm_active_connection_get_next_object_path (void)
{
	static guint32 counter = 0;

	return g_strdup_printf (BM_DBUS_PATH "/ActiveConnection/%d", counter++);
}

void
bm_active_connection_scope_to_value (BMConnection *connection, GValue *value)
{
	if (!connection) {
		g_value_set_string (value, "");
		return;
	}

	switch (bm_connection_get_scope (connection)) {
	case BM_CONNECTION_SCOPE_SYSTEM:
		g_value_set_string (value, BM_DBUS_SERVICE_SYSTEM_SETTINGS);
		break;
	case BM_CONNECTION_SCOPE_USER:
		g_value_set_string (value, BM_DBUS_SERVICE_USER_SETTINGS);
		break;
	default:
		bm_log_err (LOGD_CORE, "unknown connection scope!");
		break;
	}
}

void
bm_active_connection_install_properties (GObjectClass *object_class,
                                         guint prop_service_name,
                                         guint prop_connection,
                                         guint prop_specific_object,
                                         guint prop_devices,
                                         guint prop_state,
                                         guint prop_default,
                                         guint prop_default6,
                                         guint prop_vpn)
{
	g_object_class_install_property (object_class, prop_service_name,
		g_param_spec_string (BM_ACTIVE_CONNECTION_SERVICE_NAME,
		                     "Service name",
		                     "Service name",
		                     NULL,
		                     G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_connection,
		g_param_spec_boxed (BM_ACTIVE_CONNECTION_CONNECTION,
		                    "Connection",
		                    "Connection",
		                    DBUS_TYPE_G_OBJECT_PATH,
		                    G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_specific_object,
		g_param_spec_boxed (BM_ACTIVE_CONNECTION_SPECIFIC_OBJECT,
		                    "Specific object",
		                    "Specific object",
		                    DBUS_TYPE_G_OBJECT_PATH,
		                    G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_devices,
		g_param_spec_boxed (BM_ACTIVE_CONNECTION_DEVICES,
		                    "Devices",
		                    "Devices",
		                    DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH,
		                    G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_state,
		g_param_spec_uint (BM_ACTIVE_CONNECTION_STATE,
		                   "State",
		                   "State",
		                   BM_ACTIVE_CONNECTION_STATE_UNKNOWN,
		                   BM_ACTIVE_CONNECTION_STATE_ACTIVATED,
		                   BM_ACTIVE_CONNECTION_STATE_UNKNOWN,
		                   G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_default,
		g_param_spec_boolean (BM_ACTIVE_CONNECTION_DEFAULT,
		                      "Default",
		                      "Is the default IPv4 active connection",
		                      FALSE,
		                      G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_default6,
		g_param_spec_boolean (BM_ACTIVE_CONNECTION_DEFAULT6,
		                      "Default6",
		                      "Is the default IPv6 active connection",
		                      FALSE,
		                      G_PARAM_READABLE));

	g_object_class_install_property (object_class, prop_vpn,
		g_param_spec_boolean (BM_ACTIVE_CONNECTION_VPN,
		                      "VPN",
		                      "Is a VPN connection",
		                      FALSE,
		                      G_PARAM_READABLE));
}

