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
 * Copyright (C) 2007 - 2010 Red Hat, Inc.
 */

#ifndef BM_SETTINGS_INTERFACE_H
#define BM_SETTINGS_INTERFACE_H

#include <glib-object.h>

#include "BarcodeManager.h"
#include "bm-settings-connection-interface.h"

G_BEGIN_DECLS

typedef enum {
	BM_SETTINGS_INTERFACE_ERROR_INVALID_CONNECTION = 0,
	BM_SETTINGS_INTERFACE_ERROR_READ_ONLY_CONNECTION,
	BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
	BM_SETTINGS_INTERFACE_ERROR_SECRETS_UNAVAILABLE,
	BM_SETTINGS_INTERFACE_ERROR_SECRETS_REQUEST_CANCELED,
	BM_SETTINGS_INTERFACE_ERROR_PERMISSION_DENIED,
	BM_SETTINGS_INTERFACE_ERROR_INVALID_SETTING,
} BMSettingsInterfaceError;

#define BM_SETTINGS_INTERFACE_ERROR (bm_settings_interface_error_quark ())
GQuark bm_settings_interface_error_quark (void);

#define BM_TYPE_SETTINGS_INTERFACE_ERROR (bm_settings_interface_error_get_type ()) 
GType bm_settings_interface_error_get_type (void);


#define BM_TYPE_SETTINGS_INTERFACE               (bm_settings_interface_get_type ())
#define BM_SETTINGS_INTERFACE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SETTINGS_INTERFACE, BMSettingsInterface))
#define BM_IS_SETTINGS_INTERFACE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SETTINGS_INTERFACE))
#define BM_SETTINGS_INTERFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BM_TYPE_SETTINGS_INTERFACE, BMSettingsInterface))

#define BM_SETTINGS_INTERFACE_NEW_CONNECTION   "new-connection"
#define BM_SETTINGS_INTERFACE_CONNECTIONS_READ "connections-read"

typedef struct _BMSettingsInterface BMSettingsInterface;

typedef void (*BMSettingsAddConnectionFunc) (BMSettingsInterface *settings,
                                             GError *error,
                                             gpointer user_data);

struct _BMSettingsInterface {
	GTypeInterface g_iface;

	/* Methods */
	/* Returns a list of objects implementing BMSettingsConnectionInterface */
	GSList * (*list_connections) (BMSettingsInterface *settings);

	BMSettingsConnectionInterface * (*get_connection_by_path) (BMSettingsInterface *settings,
	                                                           const char *path);

	gboolean (*add_connection) (BMSettingsInterface *settings,
	                            BMConnection *connection,
	                            BMSettingsAddConnectionFunc callback,
	                            gpointer user_data);

	/* Signals */
	void (*new_connection) (BMSettingsInterface *settings,
	                        BMSettingsConnectionInterface *connection);

	void (*connections_read) (BMSettingsInterface *settings);

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
};

GType bm_settings_interface_get_type (void);

/* Returns a list of objects implementing BMSettingsConnectionInterface */
GSList *bm_settings_interface_list_connections (BMSettingsInterface *settings);

BMSettingsConnectionInterface *bm_settings_interface_get_connection_by_path (BMSettingsInterface *settings,
                                                                             const char *path); 

gboolean bm_settings_interface_add_connection (BMSettingsInterface *settings,
                                               BMConnection *connection,
                                               BMSettingsAddConnectionFunc callback,
                                               gpointer user_data);

G_END_DECLS

#endif /* BM_SETTINGS_INTERFACE_H */
