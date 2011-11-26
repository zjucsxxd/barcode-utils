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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2010 Red Hat, Inc.
 */

#ifndef BM_DEVICE_H
#define BM_DEVICE_H

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "bm-object.h"
#include "BarcodeManager.h"
#include "bm-connection.h"

G_BEGIN_DECLS

#define BM_TYPE_DEVICE            (bm_device_get_type ())
#define BM_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE, BMDevice))
#define BM_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_DEVICE, BMDeviceClass))
#define BM_IS_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE))
#define BM_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_DEVICE))
#define BM_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_DEVICE, BMDeviceClass))

#define BM_DEVICE_UDI "udi"
#define BM_DEVICE_INTERFACE "interface"
#define BM_DEVICE_DRIVER "driver"
#define BM_DEVICE_CAPABILITIES "capabilities"
#define BM_DEVICE_MANAGED "managed"
#define BM_DEVICE_FIRMWARE_MISSING "firmware-missing"
#define BM_DEVICE_STATE "state"
#define BM_DEVICE_VENDOR "vendor"
#define BM_DEVICE_PRODUCT "product"

typedef struct {
	BMObject parent;
} BMDevice;

typedef struct {
	BMObjectClass parent;

	/* Signals */
	void (*state_changed) (BMDevice *device,
	                       BMDeviceState new_state,
	                       BMDeviceState old_state,
	                       BMDeviceStateReason reason);

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} BMDeviceClass;

GType bm_device_get_type (void);

GObject * bm_device_new (DBusGConnection *connection, const char *path);

const char *  bm_device_get_iface            (BMDevice *device);
const char *  bm_device_get_udi              (BMDevice *device);
const char *  bm_device_get_driver           (BMDevice *device);
guint32       bm_device_get_capabilities     (BMDevice *device);
gboolean      bm_device_get_managed          (BMDevice *device);
gboolean      bm_device_get_firmware_missing (BMDevice *device);
BMDeviceState bm_device_get_state            (BMDevice *device);
const char *  bm_device_get_product          (BMDevice *device);
const char *  bm_device_get_vendor           (BMDevice *device);

typedef void (*BMDeviceDeactivateFn) (BMDevice *device, GError *error, gpointer user_data);

void          bm_device_disconnect         (BMDevice *device,
                                            BMDeviceDeactivateFn callback,
                                            gpointer user_data);

G_END_DECLS

#endif /* BM_DEVICE_H */
