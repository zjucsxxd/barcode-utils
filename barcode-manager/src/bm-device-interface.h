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

#ifndef BM_DEVICE_INTERFACE_H
#define BM_DEVICE_INTERFACE_H

#include <glib-object.h>
#include "BarcodeManager.h"
#include "bm-connection.h"
#include "bm-activation-request.h"

#define BM_TYPE_DEVICE_INTERFACE      (bm_device_interface_get_type ())
#define BM_DEVICE_INTERFACE(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE_INTERFACE, BMDeviceInterface))
#define BM_IS_DEVICE_INTERFACE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE_INTERFACE))
#define BM_DEVICE_INTERFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BM_TYPE_DEVICE_INTERFACE, BMDeviceInterface))

#define IS_ACTIVATING_STATE(state) \
	(state > BM_DEVICE_STATE_DISCONNECTED && state < BM_DEVICE_STATE_ACTIVATED)

typedef enum
{
	BM_DEVICE_INTERFACE_ERROR_CONNECTION_ACTIVATING = 0,
	BM_DEVICE_INTERFACE_ERROR_CONNECTION_INVALID,
	BM_DEVICE_INTERFACE_ERROR_NOT_ACTIVE,
} BMDeviceInterfaceError;

#define BM_DEVICE_INTERFACE_ERROR (bm_device_interface_error_quark ())
#define BM_TYPE_DEVICE_INTERFACE_ERROR (bm_device_interface_error_get_type ()) 

#define BM_DEVICE_INTERFACE_DISCONNECT_REQUEST "disconnect-request"

#define BM_DEVICE_INTERFACE_UDI              "udi"
#define BM_DEVICE_INTERFACE_IFACE            "interface"
#define BM_DEVICE_INTERFACE_IP_IFACE         "ip-interface"
#define BM_DEVICE_INTERFACE_DRIVER           "driver"
#define BM_DEVICE_INTERFACE_CAPABILITIES     "capabilities"
#define BM_DEVICE_INTERFACE_IP4_ADDRESS      "ip4-address"
#define BM_DEVICE_INTERFACE_IP4_CONFIG       "ip4-config"
#define BM_DEVICE_INTERFACE_DHCP4_CONFIG     "dhcp4-config"
#define BM_DEVICE_INTERFACE_IP6_CONFIG       "ip6-config"
#define BM_DEVICE_INTERFACE_DHCP6_CONFIG     "dhcp6-config"
#define BM_DEVICE_INTERFACE_STATE            "state"
#define BM_DEVICE_INTERFACE_DEVICE_TYPE      "device-type" /* ugh */
#define BM_DEVICE_INTERFACE_MANAGED          "managed"
#define BM_DEVICE_INTERFACE_FIRMWARE_MISSING "firmware-missing"
#define BM_DEVICE_INTERFACE_TYPE_DESC        "type-desc"    /* Internal only */
#define BM_DEVICE_INTERFACE_RFKILL_TYPE      "rfkill-type"  /* Internal only */
#define BM_DEVICE_INTERFACE_IFINDEX          "ifindex"      /* Internal only */

typedef enum {
	BM_DEVICE_INTERFACE_PROP_FIRST = 0x1000,

	BM_DEVICE_INTERFACE_PROP_UDI = BM_DEVICE_INTERFACE_PROP_FIRST,
	BM_DEVICE_INTERFACE_PROP_IFACE,
	BM_DEVICE_INTERFACE_PROP_IP_IFACE,
	BM_DEVICE_INTERFACE_PROP_DRIVER,
	BM_DEVICE_INTERFACE_PROP_CAPABILITIES,
	BM_DEVICE_INTERFACE_PROP_IP4_ADDRESS,
	BM_DEVICE_INTERFACE_PROP_IP4_CONFIG,
	BM_DEVICE_INTERFACE_PROP_DHCP4_CONFIG,
	BM_DEVICE_INTERFACE_PROP_IP6_CONFIG,
	BM_DEVICE_INTERFACE_PROP_DHCP6_CONFIG,
	BM_DEVICE_INTERFACE_PROP_STATE,
	BM_DEVICE_INTERFACE_PROP_DEVICE_TYPE,
	BM_DEVICE_INTERFACE_PROP_MANAGED,
	BM_DEVICE_INTERFACE_PROP_FIRMWARE_MISSING,
	BM_DEVICE_INTERFACE_PROP_TYPE_DESC,
	BM_DEVICE_INTERFACE_PROP_RFKILL_TYPE,
	BM_DEVICE_INTERFACE_PROP_IFINDEX,
} BMDeviceInterfaceProp;


typedef struct _BMDeviceInterface BMDeviceInterface;

struct _BMDeviceInterface {
	GTypeInterface g_iface;

	/* Methods */
	gboolean (*check_connection_compatible) (BMDeviceInterface *device,
	                                         BMConnection *connection,
	                                         GError **error);

	gboolean (*activate) (BMDeviceInterface *device,
	                      NMActRequest *req,
	                      GError **error);

	void (*deactivate) (BMDeviceInterface *device, BMDeviceStateReason reason);
	gboolean (*disconnect) (BMDeviceInterface *device, GError **error);

	gboolean (*spec_match_list) (BMDeviceInterface *device, const GSList *specs);

	BMConnection * (*connection_match_config) (BMDeviceInterface *device, const GSList *specs);

	gboolean (*can_assume_connections) (BMDeviceInterface *device);

	void (*set_enabled) (BMDeviceInterface *device, gboolean enabled);

	gboolean (*get_enabled) (BMDeviceInterface *device);

	/* Signals */
	void (*state_changed) (BMDeviceInterface *device,
	                       BMDeviceState new_state,
	                       BMDeviceState old_state,
	                       BMDeviceStateReason reason);
};

GQuark bm_device_interface_error_quark (void);
GType bm_device_interface_error_get_type (void);

gboolean bm_device_interface_disconnect (BMDeviceInterface *device, GError **error);

GType bm_device_interface_get_type (void);

gboolean bm_device_interface_check_connection_compatible (BMDeviceInterface *device,
                                                          BMConnection *connection,
                                                          GError **error);

gboolean bm_device_interface_activate (BMDeviceInterface *device,
				       NMActRequest *req,
				       GError **error);

void bm_device_interface_deactivate (BMDeviceInterface *device, BMDeviceStateReason reason);

BMDeviceState bm_device_interface_get_state (BMDeviceInterface *device);

gboolean bm_device_interface_spec_match_list (BMDeviceInterface *device,
                                              const GSList *specs);

BMConnection * bm_device_interface_connection_match_config (BMDeviceInterface *device,
                                                            const GSList *connections);

gboolean bm_device_interface_can_assume_connections (BMDeviceInterface *device);

gboolean bm_device_interface_get_enabled (BMDeviceInterface *device);

void bm_device_interface_set_enabled (BMDeviceInterface *device, gboolean enabled);

#endif /* BM_DEVICE_INTERFACE_H */
