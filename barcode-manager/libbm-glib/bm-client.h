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

#ifndef BM_CLIENT_H
#define BM_CLIENT_H

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <BarcodeManager.h>
#include "bm-object.h"
#include "bm-device.h"
#include "bm-active-connection.h"

G_BEGIN_DECLS

#define BM_TYPE_CLIENT            (bm_client_get_type ())
#define BM_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_CLIENT, BMClient))
#define BM_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_CLIENT, BMClientClass))
#define BM_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_CLIENT))
#define BM_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_CLIENT))
#define BM_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_CLIENT, BMClientClass))

#define BM_CLIENT_STATE "state"
#define BM_CLIENT_MANAGER_RUNNING "manager-running"
#define BM_CLIENT_NETWORKING_ENABLED "networking-enabled"
#define BM_CLIENT_WIRELESS_ENABLED "wireless-enabled"
#define BM_CLIENT_WIRELESS_HARDWARE_ENABLED "wireless-hardware-enabled"
#define BM_CLIENT_WWAN_ENABLED "wwan-enabled"
#define BM_CLIENT_WWAN_HARDWARE_ENABLED "wwan-hardware-enabled"
#define BM_CLIENT_ACTIVE_CONNECTIONS "active-connections"

/* Permissions */
typedef enum {
	BM_CLIENT_PERMISSION_NONE = 0,
	BM_CLIENT_PERMISSION_ENABLE_DISABLE_NETWORK = 1,
	BM_CLIENT_PERMISSION_ENABLE_DISABLE_WIFI = 2,
	BM_CLIENT_PERMISSION_ENABLE_DISABLE_WWAN = 3,
	BM_CLIENT_PERMISSION_USE_USER_CONNECTIONS = 4,

	BM_CLIENT_PERMISSION_LAST = BM_CLIENT_PERMISSION_USE_USER_CONNECTIONS
} BMClientPermission;

typedef enum {
	BM_CLIENT_PERMISSION_RESULT_UNKNOWN = 0,
	BM_CLIENT_PERMISSION_RESULT_YES,
	BM_CLIENT_PERMISSION_RESULT_AUTH,
	BM_CLIENT_PERMISSION_RESULT_NO
} BMClientPermissionResult;


typedef struct {
	BMObject parent;
} BMClient;

typedef struct {
	BMObjectClass parent;

	/* Signals */
	void (*device_added) (BMClient *client, BMDevice *device);
	void (*device_removed) (BMClient *client, BMDevice *device);

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} BMClientClass;

GType bm_client_get_type (void);

BMClient *bm_client_new (void);

const GPtrArray *bm_client_get_devices    (BMClient *client);
BMDevice *bm_client_get_device_by_path    (BMClient *client, const char *object_path);

typedef void (*BMClientActivateDeviceFn) (gpointer user_data, const char *object_path, GError *error);

void bm_client_activate_connection (BMClient *client,
						  const char *service_name,
						  const char *connection_path,
						  BMDevice *device,
						  const char *specific_object,
						  BMClientActivateDeviceFn callback,
						  gpointer user_data);

void bm_client_deactivate_connection (BMClient *client, BMActiveConnection *active);

gboolean  bm_client_networking_get_enabled (BMClient *client);
void      bm_client_networking_set_enabled (BMClient *client, gboolean enabled);

gboolean  bm_client_wireless_get_enabled (BMClient *client);
void      bm_client_wireless_set_enabled (BMClient *client, gboolean enabled);
gboolean  bm_client_wireless_hardware_get_enabled (BMClient *client);

gboolean  bm_client_wwan_get_enabled (BMClient *client);
void      bm_client_wwan_set_enabled (BMClient *client, gboolean enabled);
gboolean  bm_client_wwan_hardware_get_enabled (BMClient *client);

BMState   bm_client_get_state            (BMClient *client);
gboolean  bm_client_get_manager_running  (BMClient *client);
const GPtrArray *bm_client_get_active_connections (BMClient *client);
void      bm_client_sleep                (BMClient *client, gboolean sleep);

BMClientPermissionResult bm_client_get_permission_result (BMClient *client,
                                                          BMClientPermission permission);

G_END_DECLS

#endif /* BM_CLIENT_H */
