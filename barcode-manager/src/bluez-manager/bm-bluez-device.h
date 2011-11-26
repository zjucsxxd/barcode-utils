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
 * Copyright (C) 2009 Red Hat, Inc.
 */

#ifndef BM_BLUEZ_DEVICE_H
#define BM_BLUEZ_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#define BM_TYPE_BLUEZ_DEVICE            (bm_bluez_device_get_type ())
#define BM_BLUEZ_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_BLUEZ_DEVICE, NMBluezDevice))
#define BM_BLUEZ_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_BLUEZ_DEVICE, NMBluezDeviceClass))
#define BM_IS_BLUEZ_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_BLUEZ_DEVICE))
#define BM_IS_BLUEZ_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_BLUEZ_DEVICE))
#define BM_BLUEZ_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_BLUEZ_DEVICE, NMBluezDeviceClass))

#define BM_BLUEZ_DEVICE_PATH         "path"
#define BM_BLUEZ_DEVICE_ADDRESS      "address"
#define BM_BLUEZ_DEVICE_NAME         "name"
#define BM_BLUEZ_DEVICE_CAPABILITIES "capabilities"
#define BM_BLUEZ_DEVICE_RSSI         "rssi"
#define BM_BLUEZ_DEVICE_USABLE       "usable"

typedef struct {
	GObject parent;
} NMBluezDevice;

typedef struct {
	GObjectClass parent;

	/* virtual functions */
	void (*initialized) (NMBluezDevice *self, gboolean success);

	void (*invalid)     (NMBluezDevice *self);
} NMBluezDeviceClass;

GType bm_bluez_device_get_type (void);

NMBluezDevice *bm_bluez_device_new (const char *path);

const char *bm_bluez_device_get_path (NMBluezDevice *self);

gboolean bm_bluez_device_get_initialized (NMBluezDevice *self);

gboolean bm_bluez_device_get_usable (NMBluezDevice *self);

const char *bm_bluez_device_get_address (NMBluezDevice *self);

const char *bm_bluez_device_get_name (NMBluezDevice *self);

guint32 bm_bluez_device_get_class (NMBluezDevice *self);

guint32 bm_bluez_device_get_capabilities (NMBluezDevice *self);

gint bm_bluez_device_get_rssi (NMBluezDevice *self);

#endif /* BM_BLUEZ_DEVICE_H */

