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

#ifndef BM_DEVICE_BT_H
#define BM_DEVICE_BT_H

#include "BarcodeManager.h"
#include "bm-device.h"

G_BEGIN_DECLS

#define BM_TYPE_DEVICE_BT            (bm_device_bt_get_type ())
#define BM_DEVICE_BT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE_BT, BMDeviceBt))
#define BM_DEVICE_BT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_DEVICE_BT, BMDeviceBtClass))
#define BM_IS_DEVICE_BT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE_BT))
#define BM_IS_DEVICE_BT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_DEVICE_BT))
#define BM_DEVICE_BT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_DEVICE_BT, BMDeviceBtClass))

#define BM_DEVICE_BT_HW_ADDRESS   "hw-address"
#define BM_DEVICE_BT_NAME         "name"
#define BM_DEVICE_BT_CAPABILITIES "bt-capabilities"

typedef struct {
	BMDevice parent;
} BMDeviceBt;

typedef struct {
	BMDeviceClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} BMDeviceBtClass;

GType bm_device_bt_get_type (void);

GObject *bm_device_bt_new (DBusGConnection *connection, const char *path);

const char *bm_device_bt_get_hw_address   (BMDeviceBt *device);

const char *bm_device_bt_get_name         (BMDeviceBt *device);

BMBluetoothCapabilities bm_device_bt_get_capabilities (BMDeviceBt *device);

G_END_DECLS

#endif /* BM_DEVICE_BT_H */
