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

#ifndef BM_DEVICE_H
#define BM_DEVICE_H

#include <glib-object.h>
#include <dbus/dbus.h>

#include "BarcodeManager.h"
#include "bm-connection.h"

G_BEGIN_DECLS

#define BM_TYPE_DEVICE			(bm_device_get_type ())
#define BM_DEVICE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE, BMDevice))
#define BM_DEVICE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  BM_TYPE_DEVICE, BMDeviceClass))
#define BM_IS_DEVICE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE))
#define BM_IS_DEVICE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  BM_TYPE_DEVICE))
#define BM_DEVICE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  BM_TYPE_DEVICE, BMDeviceClass))

typedef struct {
	GObject parent;
} BMDevice;

typedef struct {
	GObjectClass parent;

	/* Hardware state, ie IFF_UP */
	gboolean        (*hw_is_up)      (BMDevice *self);
	gboolean        (*hw_bring_up)   (BMDevice *self, gboolean *no_firmware);
	void            (*hw_take_down)  (BMDevice *self);

	/* Additional stuff required to operate the device, like a 
	 * connection to the supplicant, Bluez, etc
	 */
	gboolean        (*is_up)         (BMDevice *self);
	gboolean        (*bring_up)      (BMDevice *self);
	void            (*take_down)     (BMDevice *self);

	guint32		(* get_type_capabilities)	(BMDevice *self);
	guint32		(* get_generic_capabilities)	(BMDevice *self);

	gboolean	(* is_available) (BMDevice *self);

} BMDeviceClass;


GType bm_device_get_type (void);

const char *    bm_device_get_path (BMDevice *dev);
void            bm_device_set_path (BMDevice *dev, const char *path);

const char *	bm_device_get_iface		(BMDevice *dev);
int             bm_device_get_ifindex	(BMDevice *dev);
const char *	bm_device_get_driver	(BMDevice *dev);
const char *	bm_device_get_type_desc (BMDevice *dev);

BMDeviceType	bm_device_get_device_type	(BMDevice *dev);
guint32		bm_device_get_capabilities	(BMDevice *dev);
guint32		bm_device_get_type_capabilities	(BMDevice *dev);

int			bm_device_get_priority (BMDevice *dev);

void *		bm_device_get_system_config_data	(BMDevice *dev);

gboolean		bm_device_is_available (BMDevice *dev);

BMConnection * bm_device_get_best_auto_connection (BMDevice *dev,
                                                   GSList *connections,
                                                   char **specific_object);

BMDeviceState bm_device_get_state (BMDevice *device);

G_END_DECLS

#endif	/* BM_DEVICE_H */
