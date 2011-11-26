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

#ifndef BM_MANAGER_H
#define BM_MANAGER_H 1

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "bm-device.h"
#include "bm-device-interface.h"

G_BEGIN_DECLS

#define BM_TYPE_MANAGER            (bm_manager_get_type ())
#define BM_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_MANAGER, BMManager))
#define BM_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_MANAGER, BMManagerClass))
#define BM_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_MANAGER))
#define BM_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_MANAGER))
#define BM_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_MANAGER, BMManagerClass))

#define BM_MANAGER_VERSION "version"
#define BM_MANAGER_STATE "state"

typedef struct {
	GObject parent;
} BMManager;

typedef struct {
	GObjectClass parent;

	/* Signals */
	void (*device_added) (BMManager *manager, BMDevice *device);
	void (*device_removed) (BMManager *manager, BMDevice *device);
	void (*state_changed) (BMManager *manager, guint state);
	void (*properties_changed) (BMManager *manager, GHashTable *properties);

	void (*connections_added) (BMManager *manager, BMConnectionScope scope);

	void (*connection_added) (BMManager *manager,
				  BMConnection *connection,
				  BMConnectionScope scope);

	void (*connection_updated) (BMManager *manager,
				  BMConnection *connection,
				  BMConnectionScope scope);

	void (*connection_removed) (BMManager *manager,
				    BMConnection *connection,
				    BMConnectionScope scope);
} BMManagerClass;

GType bm_manager_get_type (void);

BMManager *bm_manager_get (const char *config_file,
                           const char *plugins,
                           const char *state_file,
                           GError **error);

void bm_manager_start (BMManager *manager);

/* State handling */

BMState bm_manager_get_state (BMManager *manager);

/* Connections */

GSList *bm_manager_get_connections    (BMManager *manager, BMConnectionScope scope);

gboolean bm_manager_auto_user_connections_allowed (BMManager *manager);

BMConnection * bm_manager_get_connection_by_object_path (BMManager *manager,
                                                         BMConnectionScope scope,
                                                         const char *path);

#endif /* BM_MANAGER_H */
