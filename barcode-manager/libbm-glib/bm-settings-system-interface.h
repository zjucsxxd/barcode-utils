/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- Network link manager
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

#ifndef BM_SETTINGS_SYSTEM_INTERFACE_H
#define BM_SETTINGS_SYSTEM_INTERFACE_H

#include <glib-object.h>

#include "BarcodeManager.h"

G_BEGIN_DECLS

typedef enum {
	BM_SETTINGS_SYSTEM_PERMISSION_NONE = 0x0,
	BM_SETTINGS_SYSTEM_PERMISSION_CONNECTION_MODIFY = 0x1,
	BM_SETTINGS_SYSTEM_PERMISSION_WIFI_SHARE_PROTECTED = 0x2,
	BM_SETTINGS_SYSTEM_PERMISSION_WIFI_SHARE_OPEN = 0x4,
	BM_SETTINGS_SYSTEM_PERMISSION_HOSTNAME_MODIFY = 0x8
} BMSettingsSystemPermissions;

#define BM_TYPE_SETTINGS_SYSTEM_INTERFACE               (bm_settings_system_interface_get_type ())
#define BM_SETTINGS_SYSTEM_INTERFACE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SETTINGS_SYSTEM_INTERFACE, BMSettingsSystemInterface))
#define BM_IS_SETTINGS_SYSTEM_INTERFACE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SETTINGS_SYSTEM_INTERFACE))
#define BM_SETTINGS_SYSTEM_INTERFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BM_TYPE_SETTINGS_SYSTEM_INTERFACE, BMSettingsSystemInterface))

#define BM_SETTINGS_SYSTEM_INTERFACE_HOSTNAME          "hostname"
#define BM_SETTINGS_SYSTEM_INTERFACE_CAN_MODIFY        "can-modify"

#define BM_SETTINGS_SYSTEM_INTERFACE_CHECK_PERMISSIONS "check-permissions"

typedef enum {
	BM_SETTINGS_SYSTEM_INTERFACE_PROP_FIRST = 0x1000,

	BM_SETTINGS_SYSTEM_INTERFACE_PROP_HOSTNAME = BM_SETTINGS_SYSTEM_INTERFACE_PROP_FIRST,
	BM_SETTINGS_SYSTEM_INTERFACE_PROP_CAN_MODIFY
} BMSettingsSystemInterfaceProp;


typedef struct _BMSettingsSystemInterface BMSettingsSystemInterface;


typedef void (*BMSettingsSystemSaveHostnameFunc) (BMSettingsSystemInterface *settings,
                                                  GError *error,
                                                  gpointer user_data);

typedef void (*BMSettingsSystemGetPermissionsFunc) (BMSettingsSystemInterface *settings,
                                                    BMSettingsSystemPermissions permissions,
                                                    GError *error,
                                                    gpointer user_data);

struct _BMSettingsSystemInterface {
	GTypeInterface g_iface;

	/* Methods */
	gboolean (*save_hostname) (BMSettingsSystemInterface *settings,
	                           const char *hostname,
	                           BMSettingsSystemSaveHostnameFunc callback,
	                           gpointer user_data);

	gboolean (*get_permissions) (BMSettingsSystemInterface *settings,
	                             BMSettingsSystemGetPermissionsFunc callback,
	                             gpointer user_data);

	/* Signals */
	void (*check_permissions) (BMSettingsSystemInterface *settings);

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
};

GType bm_settings_system_interface_get_type (void);

gboolean bm_settings_system_interface_save_hostname (BMSettingsSystemInterface *settings,
                                                     const char *hostname,
                                                     BMSettingsSystemSaveHostnameFunc callback,
                                                     gpointer user_data);

gboolean bm_settings_system_interface_get_permissions (BMSettingsSystemInterface *settings,
                                                       BMSettingsSystemGetPermissionsFunc callback,
                                                       gpointer user_data);

G_END_DECLS

#endif /* BM_SETTINGS_SYSTEM_INTERFACE_H */
