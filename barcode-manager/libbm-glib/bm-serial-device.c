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
 * Copyright (C) 2008 Novell, Inc.
 */

#include "bm-serial-device.h"
#include "bm-device-private.h"
#include "bm-object-private.h"
#include "bm-marshal.h"

G_DEFINE_ABSTRACT_TYPE (NMSerialDevice, bm_serial_device, BM_TYPE_DEVICE)

#define BM_SERIAL_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_SERIAL_DEVICE, NMSerialDevicePrivate))

typedef struct {
	DBusGProxy *proxy;

	guint32 in_bytes;
	guint32 out_bytes;

	gboolean disposed;
} NMSerialDevicePrivate;

enum {
	PPP_STATS,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/**
 * bm_serial_device_get_bytes_received:
 * @self: a #NMSerialDevice
 *
 * Gets the amount of bytes received by the serial device.
 * This counter is reset when the device is activated.
 *
 * Returns: bytes received
 **/
guint32
bm_serial_device_get_bytes_received (NMSerialDevice *self)
{
	g_return_val_if_fail (BM_IS_SERIAL_DEVICE (self), 0);

	return BM_SERIAL_DEVICE_GET_PRIVATE (self)->in_bytes;
}

/**
 * bm_serial_device_get_bytes_sent:
 * @self: a #NMSerialDevice
 *
 * Gets the amount of bytes sent by the serial device.
 * This counter is reset when the device is activated.
 *
 * Returns: bytes sent
 **/
guint32
bm_serial_device_get_bytes_sent (NMSerialDevice *self)
{
	g_return_val_if_fail (BM_IS_SERIAL_DEVICE (self), 0);

	return BM_SERIAL_DEVICE_GET_PRIVATE (self)->out_bytes;
}

static void
ppp_stats (DBusGProxy *proxy,
		   guint32 in_bytes,
		   guint32 out_bytes,
		   gpointer user_data)
{
	NMSerialDevice *self = BM_SERIAL_DEVICE (user_data);
	NMSerialDevicePrivate *priv = BM_SERIAL_DEVICE_GET_PRIVATE (self);

	priv->in_bytes = in_bytes;
	priv->out_bytes = out_bytes;

	g_signal_emit (self, signals[PPP_STATS], 0, in_bytes, out_bytes);
}

static void
device_state_changed (BMDevice *device, GParamSpec *pspec, gpointer user_data)
{
	NMSerialDevicePrivate *priv;

	switch (bm_device_get_state (device)) {
	case BM_DEVICE_STATE_UNMANAGED:
	case BM_DEVICE_STATE_UNAVAILABLE:
	case BM_DEVICE_STATE_DISCONNECTED:
		priv = BM_SERIAL_DEVICE_GET_PRIVATE (device);
		priv->in_bytes = priv->out_bytes = 0;
		break;
	default:
		break;
	}
}

static void
bm_serial_device_init (NMSerialDevice *device)
{
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	GObject *object;
	NMSerialDevicePrivate *priv;

	object = G_OBJECT_CLASS (bm_serial_device_parent_class)->constructor (type,
																		  n_construct_params,
																		  construct_params);
	if (!object)
		return NULL;

	priv = BM_SERIAL_DEVICE_GET_PRIVATE (object);

	priv->proxy = dbus_g_proxy_new_for_name (bm_object_get_connection (BM_OBJECT (object)),
											 BM_DBUS_SERVICE,
											 bm_object_get_path (BM_OBJECT (object)),
											 BM_DBUS_INTERFACE_SERIAL_DEVICE);

	dbus_g_object_register_marshaller (_bm_marshal_VOID__UINT_UINT,
	                                   G_TYPE_NONE,
	                                   G_TYPE_UINT, G_TYPE_UINT,
	                                   G_TYPE_INVALID);

	dbus_g_proxy_add_signal (priv->proxy, "PppStats", G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "PppStats",
								 G_CALLBACK (ppp_stats),
								 object,
								 NULL);

	/* Catch BMDevice::state changes to reset the counters */
	g_signal_connect (object, "notify::state",
					  G_CALLBACK (device_state_changed),
					  object);

	return object;
}

static void
dispose (GObject *object)
{
	NMSerialDevicePrivate *priv = BM_SERIAL_DEVICE_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_serial_device_parent_class)->dispose (object);
		return;
	}

	priv->disposed = TRUE;

	g_object_unref (priv->proxy);

	G_OBJECT_CLASS (bm_serial_device_parent_class)->dispose (object);
}

static void
bm_serial_device_class_init (NMSerialDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	g_type_class_add_private (device_class, sizeof (NMSerialDevicePrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->dispose = dispose;

	/* Signals */

	/**
	 * NMSerialDevice::ppp-stats:
	 * @device: the serial device that received the signal
	 * @in_bytes: the amount of bytes received
	 * @out_bytes: the amount of bytes sent
	 *
	 * Notifies that a #NMAccessPoint is added to the wifi device.
	 **/
	signals[PPP_STATS] =
		g_signal_new ("ppp-stats",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMSerialDeviceClass, ppp_stats),
					  NULL, NULL,
					  _bm_marshal_VOID__UINT_UINT,
					  G_TYPE_NONE, 2,
					  G_TYPE_UINT, G_TYPE_UINT);
}
