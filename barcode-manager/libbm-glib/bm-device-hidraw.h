/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access network status & information from glib applications
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
 * Copyright (C) 2008 - 2009 Red Hat, Inc.
 * Copyright (C) 2008 Novell, Inc.
 */

#ifndef BM_DEVICE_HIDRAW_H
#define BM_DEVICE_HIDRAW_H

#include "BarcodeManager.h"
#include "bm-device.h"

G_BEGIN_DECLS

#define BM_TYPE_DEVICE_HIDRAW            (bm_device_hidraw_get_type ())
#define BM_DEVICE_HIDRAW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidraw))
#define BM_DEVICE_HIDRAW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawClass))
#define BM_IS_DEVICE_HIDRAW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE_HIDRAW))
#define BM_IS_DEVICE_HIDRAW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_DEVICE_HIDRAW))
#define BM_DEVICE_HIDRAW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawClass))

#define BM_DEVICE_HIDRAW_HW_ADDRESS   "hw-address"
#define BM_DEVICE_HIDRAW_NAME         "name"
#define BM_DEVICE_HIDRAW_CAPABILITIES "hidraw-capabilities"

typedef struct {
	BMDevice parent;
} BMDeviceHidraw;

typedef struct {
	BMDeviceClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} BMDeviceHidrawClass;

GType bm_device_hidraw_get_type (void);

GObject *bm_device_hidraw_new (DBusGConnection *connection, const char *path);

const char *bm_device_hidraw_get_hw_address   (BMDeviceHidraw *device);

const char *bm_device_hidraw_get_name         (BMDeviceHidraw *device);

BMBluetoothCapabilities bm_device_hidraw_get_capabilities (BMDeviceHidraw *device);

G_END_DECLS

#endif /* BM_DEVICE_HIDRAW_H */
