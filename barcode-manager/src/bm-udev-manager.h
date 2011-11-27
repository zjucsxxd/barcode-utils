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

#ifndef BM_UDEV_MANAGER_H
#define BM_UDEV_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#define G_UDEV_API_IS_SUBJECT_TO_CHANGE
#include <gudev/gudev.h>

G_BEGIN_DECLS

#define BM_TYPE_UDEV_MANAGER            (bm_udev_manager_get_type ())
#define BM_UDEV_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_UDEV_MANAGER, BMUdevManager))
#define BM_UDEV_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_UDEV_MANAGER, BMUdevManagerClass))
#define BM_IS_UDEV_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_UDEV_MANAGER))
#define BM_IS_UDEV_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_UDEV_MANAGER))
#define BM_UDEV_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_UDEV_MANAGER, BMUdevManagerClass))

typedef struct {
	GObject parent;
} BMUdevManager;

typedef GObject *(*BMDeviceCreatorFn) (BMUdevManager *manager,
                                       GUdevDevice *device,
                                       gboolean sleeping);

typedef struct {
	GObjectClass parent;

	/* Virtual functions */
	void (*device_added) (BMUdevManager *manager,
	                      GUdevDevice *device,
	                      BMDeviceCreatorFn creator_fn);

	void (*device_removed) (BMUdevManager *manager, GUdevDevice *device);

} BMUdevManagerClass;

GType bm_udev_manager_get_type (void);

BMUdevManager *bm_udev_manager_new (void);

void bm_udev_manager_query_devices (BMUdevManager *manager);

#endif /* BM_UDEV_MANAGER_H */

