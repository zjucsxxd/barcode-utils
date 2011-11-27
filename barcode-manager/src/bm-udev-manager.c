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

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define G_UDEV_API_IS_SUBJECT_TO_CHANGE
#include <gudev/gudev.h>

#include "bm-udev-manager.h"
#include "bm-marshal.h"
#include "bm-logging.h"
#include "BarcodeManagerUtils.h"

typedef struct {
	GUdevClient *client;

	gboolean disposed;
} BMUdevManagerPrivate;

#define BM_UDEV_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_UDEV_MANAGER, BMUdevManagerPrivate))

G_DEFINE_TYPE (BMUdevManager, bm_udev_manager, G_TYPE_OBJECT)

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

BMUdevManager *
bm_udev_manager_new (void)
{
	return BM_UDEV_MANAGER (g_object_new (BM_TYPE_UDEV_MANAGER, NULL));
}

static GObject *
device_creator (BMUdevManager *manager,
                GUdevDevice *udev_device,
                gboolean sleeping)
{
	GObject *device = NULL;
	const char *ifname, *driver, *path, *subsys;
	GUdevDevice *parent = NULL, *grandparent = NULL;
	gint ifindex;

	ifname = g_udev_device_get_name (udev_device);
	g_assert (ifname);

	path = g_udev_device_get_sysfs_path (udev_device);
	if (!path) {
		bm_log_warn (LOGD_HW, "couldn't determine device path; ignoring...");
		return NULL;
	}

	driver = g_udev_device_get_driver (udev_device);
	if (!driver) {
		/* Try the parent */
		parent = g_udev_device_get_parent (udev_device);
		if (parent) {
			driver = g_udev_device_get_driver (parent);
			if (!driver) {
				/* try the grandparent only if it's an ibmebus device */
				subsys = g_udev_device_get_subsystem (parent);
				if (subsys && !strcmp (subsys, "ibmebus")) {
					grandparent = g_udev_device_get_parent (parent);
					if (grandparent)
						driver = g_udev_device_get_driver (grandparent);
				}
			}
		}
	}

	if (!driver) {
		bm_log_warn (LOGD_HW, "%s: couldn't determine device driver; ignoring...", path);
		goto out;
	}

	ifindex = g_udev_device_get_sysfs_attr_as_int (udev_device, "ifindex");
	if (ifindex <= 0) {
		bm_log_warn (LOGD_HW, "%s: device had invalid ifindex %d; ignoring...", path, (guint32) ifindex);
		goto out;
	}

out:
	if (grandparent)
		g_object_unref (grandparent);
	if (parent)
		g_object_unref (parent);
	return device;
}

void
bm_udev_manager_query_devices (BMUdevManager *self)
{
	BMUdevManagerPrivate *priv = BM_UDEV_MANAGER_GET_PRIVATE (self);
	GList *devices, *iter;

	g_return_if_fail (self != NULL);
	g_return_if_fail (BM_IS_UDEV_MANAGER (self));

	devices = g_udev_client_query_by_subsystem (priv->client, "hidraw");
	for (iter = devices; iter; iter = g_list_next (iter)) {
		// net_add (self, G_UDEV_DEVICE (iter->data));
		g_object_unref (G_UDEV_DEVICE (iter->data));
	}
	g_list_free (devices);
}

static void
handle_uevent (GUdevClient *client,
               const char *action,
               GUdevDevice *device,
               gpointer user_data)
{
	BMUdevManager *self = BM_UDEV_MANAGER (user_data);
	const char *subsys;

	g_return_if_fail (action != NULL);

	/* A bit paranoid */
	subsys = g_udev_device_get_subsystem (device);
	g_return_if_fail (subsys != NULL);

	bm_log_dbg (LOGD_HW, "UDEV event: action '%s' subsys '%s' device '%s'",
	            action, subsys, g_udev_device_get_name (device));
}

static void
bm_udev_manager_init (BMUdevManager *self)
{
	BMUdevManagerPrivate *priv = BM_UDEV_MANAGER_GET_PRIVATE (self);
	const char *subsys[3] = { "hidraw", NULL };
	GList *iter;
	guint32 i;

	priv->client = g_udev_client_new (subsys);
	g_signal_connect (priv->client, "uevent", G_CALLBACK (handle_uevent), self);
}

static void
dispose (GObject *object)
{
	BMUdevManager *self = BM_UDEV_MANAGER (object);
	BMUdevManagerPrivate *priv = BM_UDEV_MANAGER_GET_PRIVATE (self);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_udev_manager_parent_class)->dispose (object);
		return;
	}
	priv->disposed = TRUE;

	g_object_unref (priv->client);

	G_OBJECT_CLASS (bm_udev_manager_parent_class)->dispose (object);	
}

static void
bm_udev_manager_class_init (BMUdevManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BMUdevManagerPrivate));

	/* virtual methods */
	object_class->dispose = dispose;

	/* Signals */
	signals[DEVICE_ADDED] =
		g_signal_new ("device-added",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (BMUdevManagerClass, device_added),
					  NULL, NULL,
					  _bm_marshal_VOID__POINTER_POINTER,
					  G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

	signals[DEVICE_REMOVED] =
		g_signal_new ("device-removed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (BMUdevManagerClass, device_removed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__POINTER,
					  G_TYPE_NONE, 1, G_TYPE_POINTER);
}

