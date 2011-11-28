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

#ifndef BM_DEVICE_HIDRAW_H
#define BM_DEVICE_HIDRAW_H

#include <glib-object.h>
#include <bm-device.h>

G_BEGIN_DECLS

#define BM_TYPE_DEVICE_HIDRAW		(bm_device_hidraw_get_type ())
#define BM_DEVICE_HIDRAW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidraw))
#define BM_DEVICE_HIDRAW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawClass))
#define BM_IS_DEVICE_HIDRAW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE_HIDRAW))
#define BM_IS_DEVICE_HIDRAW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  BM_TYPE_DEVICE_HIDRAW))
#define BM_DEVICE_HIDRAW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawClass))

typedef struct {
	BMDevice parent;
} BMDeviceHidraw;

typedef struct {
	BMDeviceClass parent;

    void (*properties_changed) (BMDeviceHidraw *device, GHashTable *properties);
} BMDeviceHidrawClass;

GType bm_device_hidraw_get_type (void);

BMDevice *bm_device_hidraw_new (const char *udi,
                            const char *bdaddr,
                            const char *name);

G_END_DECLS

#endif /* BM_DEVICE_HIDRAW_H */
