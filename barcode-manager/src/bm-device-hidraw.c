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

#include <glib.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <string.h>
//#include <net/ethernet.h>

//#include "bm-glib-compat.h"
//#include "bm-dbus-manager.h"
#include "bm-device-hidraw.h"
#include "bm-device-interface.h"
#include "bm-device-private.h"
#include "bm-logging.h"
#include "bm-marshal.h"
#include "bm-properties-changed-signal.h"
//#include "bm-setting-connection.h"
//#include "bm-setting-bluetooth.h"
#include "BarcodeManagerUtils.h"

#include "bm-device-hidraw-glue.h"

G_DEFINE_TYPE (BMDeviceHidraw, bm_device_hidraw, BM_TYPE_DEVICE)

#define BM_DEVICE_HIDRAW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_DEVICE_HIDRAW, BMDeviceHidrawPrivate))

typedef struct {
    char *bdaddr;
    char *name;
    guint32 capabilities;

    gboolean connected;
    gboolean have_iface;

    DBusGProxy *type_proxy;
    DBusGProxy *dev_proxy;

    guint32 timeout_id;

    guint32 bt_type;  /* BT type of the current connection */
} BMDeviceHidrawPrivate;

enum {
	PROPERTIES_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

BMDevice *
bm_device_hidraw_new (const char *udi,
					  const char *bdaddr,
					  const char *name)
{
	BMDevice *device;

	bm_log_dbg (LOGD_CORE, "udi = %s", udi);
	bm_log_dbg (LOGD_CORE, "bdaddr = %s", bdaddr);
	bm_log_dbg (LOGD_CORE, "name = %s", name);

	g_return_val_if_fail (udi != NULL, NULL);
	g_return_val_if_fail (bdaddr != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	device = (BMDevice *) g_object_new (BM_TYPE_DEVICE_HIDRAW,
										BM_DEVICE_INTERFACE_UDI, udi,
										BM_DEVICE_INTERFACE_IFACE, bdaddr,
										BM_DEVICE_INTERFACE_DRIVER, "hidraw",
										BM_DEVICE_INTERFACE_TYPE_DESC, "Hidraw",
										BM_DEVICE_INTERFACE_DEVICE_TYPE, BM_DEVICE_TYPE_HIDRAW,
										NULL);

	bm_log_dbg (LOGD_HW, "(%s)", (device == NULL) ? "NULL" : "!= NULL");

	return device;
}

static void
bm_device_hidraw_init (BMDeviceHidraw *self)
{
	bm_log_dbg (LOGD_HW, "init called");

	//	g_signal_connect (self, "state-changed", G_CALLBACK (device_state_changed), NULL);
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
    BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
finalize (GObject *object)
{
    BMDeviceHidrawPrivate *priv = BM_DEVICE_HIDRAW_GET_PRIVATE (object);

    if (priv->timeout_id) {
        g_source_remove (priv->timeout_id);
        priv->timeout_id = 0;
    }

    if (priv->type_proxy)
        g_object_unref (priv->type_proxy);

    if (priv->dev_proxy)
        g_object_unref (priv->dev_proxy);

    g_free (priv->bdaddr);
    g_free (priv->name);

    G_OBJECT_CLASS (bm_device_hidraw_parent_class)->finalize (object);
}

static void
bm_device_hidraw_class_init (BMDeviceHidrawClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BMDeviceClass *device_class = BM_DEVICE_CLASS (klass);

	bm_log_dbg (LOGD_HW, "class init called");

	g_type_class_add_private (object_class, sizeof (BMDeviceHidrawPrivate));

	// object_class->constructor = constructor;
	// object_class->dispose = dispose;

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	// device_class->get_generic_capabilities = real_get_generic_capabilities;

	/* Properties */

	/* Signals */
    signals[PROPERTIES_CHANGED] =
        bm_properties_changed_signal_new (object_class,
                                          G_STRUCT_OFFSET (BMDeviceHidrawClass, properties_changed));

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
	                                 &dbus_glib_bm_device_hidraw_object_info);

	// dbus_g_error_domain_register (BM_HIDRAW_ERROR, NULL, BM_TYPE_HIDRAW_ERROR);
}
