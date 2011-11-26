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

#ifndef BM_MANAGER_H
#define BM_MANAGER_H 1

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "bm-device.h"
#include "bm-device-interface.h"

#define BM_TYPE_MANAGER            (bm_manager_get_type ())
#define BM_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_MANAGER, NMManager))
#define BM_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_MANAGER, NMManagerClass))
#define BM_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_MANAGER))
#define BM_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_MANAGER))
#define BM_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_MANAGER, NMManagerClass))

#define BM_MANAGER_VERSION "version"
#define BM_MANAGER_STATE "state"
#define BM_MANAGER_NETWORKING_ENABLED "networking-enabled"
#define BM_MANAGER_WIRELESS_ENABLED "wireless-enabled"
#define BM_MANAGER_WIRELESS_HARDWARE_ENABLED "wireless-hardware-enabled"
#define BM_MANAGER_WWAN_ENABLED "wwan-enabled"
#define BM_MANAGER_WWAN_HARDWARE_ENABLED "wwan-hardware-enabled"
#define BM_MANAGER_ACTIVE_CONNECTIONS "active-connections"

/* Not exported */
#define BM_MANAGER_HOSTNAME "hostname"
#define BM_MANAGER_SLEEPING "sleeping"

typedef struct {
	GObject parent;
} NMManager;

typedef struct {
	GObjectClass parent;

	/* Signals */
	void (*device_added) (NMManager *manager, BMDevice *device);
	void (*device_removed) (NMManager *manager, BMDevice *device);
	void (*state_changed) (NMManager *manager, guint state);
	void (*properties_changed) (NMManager *manager, GHashTable *properties);

	void (*connections_added) (NMManager *manager, BMConnectionScope scope);

	void (*connection_added) (NMManager *manager,
				  BMConnection *connection,
				  BMConnectionScope scope);

	void (*connection_updated) (NMManager *manager,
				  BMConnection *connection,
				  BMConnectionScope scope);

	void (*connection_removed) (NMManager *manager,
				    BMConnection *connection,
				    BMConnectionScope scope);
} NMManagerClass;

GType bm_manager_get_type (void);

NMManager *bm_manager_get (const char *config_file,
                           const char *plugins,
                           const char *state_file,
                           gboolean initial_net_enabled,
                           gboolean initial_wifi_enabled,
                           gboolean initial_wwan_enabled,
                           GError **error);

void bm_manager_start (NMManager *manager);

/* Device handling */

GSList *bm_manager_get_devices (NMManager *manager);

const char * bm_manager_activate_connection (NMManager *manager,
                                             BMConnection *connection,
                                             const char *specific_object,
                                             const char *device_path,
                                             gboolean user_requested,
                                             GError **error);

gboolean bm_manager_deactivate_connection (NMManager *manager,
                                           const char *connection_path,
                                           BMDeviceStateReason reason,
                                           GError **error);

/* State handling */

BMState bm_manager_get_state (NMManager *manager);

/* Connections */

GSList *bm_manager_get_connections    (NMManager *manager, BMConnectionScope scope);

gboolean bm_manager_auto_user_connections_allowed (NMManager *manager);

BMConnection * bm_manager_get_connection_by_object_path (NMManager *manager,
                                                         BMConnectionScope scope,
                                                         const char *path);

GPtrArray * bm_manager_get_active_connections_by_connection (NMManager *manager,
                                                             BMConnection *connection);

#endif /* BM_MANAGER_H */
