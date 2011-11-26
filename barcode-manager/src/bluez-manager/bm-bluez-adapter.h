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

#ifndef BM_BLUEZ_ADAPTER_H
#define BM_BLUEZ_ADAPTER_H

#include <glib.h>
#include <glib-object.h>

#include "bm-bluez-device.h"

#define BM_TYPE_BLUEZ_ADAPTER            (bm_bluez_adapter_get_type ())
#define BM_BLUEZ_ADAPTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_BLUEZ_ADAPTER, NMBluezAdapter))
#define BM_BLUEZ_ADAPTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_BLUEZ_ADAPTER, NMBluezAdapterClass))
#define BM_IS_BLUEZ_ADAPTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_BLUEZ_ADAPTER))
#define BM_IS_BLUEZ_ADAPTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_BLUEZ_ADAPTER))
#define BM_BLUEZ_ADAPTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_BLUEZ_ADAPTER, NMBluezAdapterClass))

#define BM_BLUEZ_ADAPTER_PATH    "path"
#define BM_BLUEZ_ADAPTER_ADDRESS "address"

typedef struct {
	GObject parent;
} NMBluezAdapter;

typedef struct {
	GObjectClass parent;

	/* virtual functions */
	void (*initialized)    (NMBluezAdapter *self, gboolean success);

	void (*device_added)   (NMBluezAdapter *self, NMBluezDevice *device);

	void (*device_removed) (NMBluezAdapter *self, NMBluezDevice *device);
} NMBluezAdapterClass;

GType bm_bluez_adapter_get_type (void);

NMBluezAdapter *bm_bluez_adapter_new (const char *path);

const char *bm_bluez_adapter_get_path (NMBluezAdapter *self);

const char *bm_bluez_adapter_get_address (NMBluezAdapter *self);

gboolean bm_bluez_adapter_get_initialized (NMBluezAdapter *self);

GSList *bm_bluez_adapter_get_devices (NMBluezAdapter *self);

#endif /* BM_BLUEZ_ADAPTER_H */

