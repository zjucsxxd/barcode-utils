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

#ifndef BM_DEVICE_BT_H
#define BM_DEVICE_BT_H

#include <bm-device.h>
#include "bm-modem.h"

G_BEGIN_DECLS

#define BM_TYPE_DEVICE_BT		(bm_device_bt_get_type ())
#define BM_DEVICE_BT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE_BT, BMDeviceBt))
#define BM_DEVICE_BT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  BM_TYPE_DEVICE_BT, BMDeviceBtClass))
#define BM_IS_DEVICE_BT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE_BT))
#define BM_IS_DEVICE_BT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  BM_TYPE_DEVICE_BT))
#define BM_DEVICE_BT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  BM_TYPE_DEVICE_BT, BMDeviceBtClass))

#define BM_DEVICE_BT_HW_ADDRESS   "hw-address"
#define BM_DEVICE_BT_NAME         "name"
#define BM_DEVICE_BT_CAPABILITIES "bt-capabilities"

typedef struct {
	BMDevice parent;
} BMDeviceBt;

typedef struct {
	BMDeviceClass parent;

	/* Signals */
	void (*ppp_stats) (BMDeviceBt *device, guint32 in_bytes, guint32 out_bytes);
	void (*properties_changed) (BMDeviceBt *device, GHashTable *properties);
} BMDeviceBtClass;

GType bm_device_bt_get_type (void);

BMDevice *bm_device_bt_new (const char *udi,
                            const char *bdaddr,
                            const char *name,
                            guint32 capabilities,
                            gboolean managed);

guint32 bm_device_bt_get_capabilities (BMDeviceBt *device);

const char *bm_device_bt_get_hw_address (BMDeviceBt *device);

gboolean bm_device_bt_modem_added (BMDeviceBt *device,
                                   NMModem *modem,
                                   const char *driver);

gboolean bm_device_bt_modem_removed (BMDeviceBt *device, NMModem *modem);

G_END_DECLS

#endif /* BM_GSM_DEVICE_H */
