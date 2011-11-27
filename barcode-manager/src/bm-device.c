/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager - barcode scanner manager
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

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "bm-glib-compat.h"
#include "bm-device-interface.h"
#include "bm-device.h"
#include "bm-device-private.h"
#include "BarcodeManagerUtils.h"
#include "bm-system.h"
#include "bm-dbus-manager.h"
#include "bm-utils.h"
#include "bm-logging.h"
#include "bm-setting-connection.h"
#include "bm-marshal.h"

enum {
	AUTOCONNECT_ALLOWED,
	LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

#define BM_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_DEVICE, BMDevicePrivate))

typedef struct {
	gboolean disposed;
	gboolean initialized;

	BMDeviceState state;
	guint         failed_to_disconnected_id;
	guint         unavailable_to_disconnected_id;

	char *        udi;
	char *        path;
	char *        iface;   /* may change, could be renamed by user */
	int           ifindex;
	BMDeviceType  type;
	char *        type_desc;
	guint32       capabilities;
	char *        driver;
	gboolean      managed; /* whether managed by NM or not */
	gboolean      firmware_missing;

    guint           act_source_id;

	/* inhibit autoconnect feature */
	gboolean	autoconnect_inhibit;
} BMDevicePrivate;

static gboolean spec_match_list (BMDeviceInterface *device, const GSList *specs);
static BMConnection *connection_match_config (BMDeviceInterface *device, const GSList *connections);
static gboolean can_assume_connections (BMDeviceInterface *device);

static gboolean bm_device_bring_up (BMDevice *self, gboolean block, gboolean *no_firmware);
static gboolean bm_device_is_up (BMDevice *self);


static void
bm_device_init (BMDevice *self)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (self);

	priv->type = BM_DEVICE_TYPE_UNKNOWN;
	priv->capabilities = BM_DEVICE_CAP_NONE;
	priv->state = BM_DEVICE_STATE_UNMANAGED;
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	GObject *object;
	BMDevice *dev;
	BMDevicePrivate *priv;

	// FIXME object = G_OBJECT_CLASS (bm_device_parent_class)->constructor (type, n_construct_params, construct_params);
	if (!object)
		return NULL;

	dev = BM_DEVICE (object);
	priv = BM_DEVICE_GET_PRIVATE (dev);

	priv->capabilities |= BM_DEVICE_GET_CLASS (dev)->get_generic_capabilities (dev);
	if (!(priv->capabilities & BM_DEVICE_CAP_BM_SUPPORTED)) {
		bm_log_warn (LOGD_DEVICE, "(%s): Device unsupported, ignoring.", priv->iface);
		goto error;
	}

	update_accept_ra_save (dev);

	priv->initialized = TRUE;
	return object;

error:
	g_object_unref (dev);
	return NULL;
}

static gboolean
bm_device_hw_is_up (BMDevice *self)
{
	g_return_val_if_fail (BM_IS_DEVICE (self), FALSE);

	if (BM_DEVICE_GET_CLASS (self)->hw_is_up)
		return BM_DEVICE_GET_CLASS (self)->hw_is_up (self);

	return TRUE;
}

static guint32
real_get_generic_capabilities (BMDevice *dev)
{
	return 0;
}

void
bm_device_set_path (BMDevice *self, const char *path)
{
	BMDevicePrivate *priv;

	g_return_if_fail (self != NULL);

	priv = BM_DEVICE_GET_PRIVATE (self);
	g_return_if_fail (priv->path == NULL);

	priv->path = g_strdup (path);
}

const char *
bm_device_get_path (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return BM_DEVICE_GET_PRIVATE (self)->path;
}

/*
 * Get/set functions for iface
 */
const char *
bm_device_get_iface (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return BM_DEVICE_GET_PRIVATE (self)->iface;
}

int
bm_device_get_ifindex (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, 0);

	return BM_DEVICE_GET_PRIVATE (self)->ifindex;
}

/*
 * Get/set functions for driver
 */
const char *
bm_device_get_driver (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return BM_DEVICE_GET_PRIVATE (self)->driver;
}


/*
 * Get/set functions for type
 */
BMDeviceType
bm_device_get_device_type (BMDevice *self)
{
	g_return_val_if_fail (BM_IS_DEVICE (self), BM_DEVICE_TYPE_UNKNOWN);

	return BM_DEVICE_GET_PRIVATE (self)->type;
}


int
bm_device_get_priority (BMDevice *dev)
{
	g_return_val_if_fail (BM_IS_DEVICE (dev), -1);

	return (int) bm_device_get_device_type (dev);
}


/*
 * Accessor for capabilities
 */
guint32
bm_device_get_capabilities (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, BM_DEVICE_CAP_NONE);

	return BM_DEVICE_GET_PRIVATE (self)->capabilities;
}

/*
 * Accessor for type-specific capabilities
 */
guint32
bm_device_get_type_capabilities (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, BM_DEVICE_CAP_NONE);

	return BM_DEVICE_GET_CLASS (self)->get_type_capabilities (self);
}

static guint32
real_get_type_capabilities (BMDevice *self)
{
	return BM_DEVICE_CAP_NONE;
}


const char *
bm_device_get_type_desc (BMDevice *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return BM_DEVICE_GET_PRIVATE (self)->type_desc;
}

/*                                                                                                                                                            
 * nm_device_is_activating                                                                                                                                    
 *                                                                                                                                                            
 * Return whether or not the device is currently activating itself.                                                                                           
 *                                                                                                                                                            
 */
gboolean
bm_device_is_activating (BMDevice *device)
{
    BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (device);

    g_return_val_if_fail (BM_IS_DEVICE (device), FALSE);

    switch (bm_device_get_state (device)) {
    case BM_DEVICE_STATE_PREPARE:
    case BM_DEVICE_STATE_CONFIG:
    case BM_DEVICE_STATE_NEED_AUTH:
    case BM_DEVICE_STATE_IP_CONFIG:
        return TRUE;
        break;
    default:
        break;
    }

    /* There's a small race between the time when stage 1 is scheduled
	 * and when the device actually sets STATE_PREPARE when the activation
     * handler is actually run.  If there's an activation handler scheduled
     * we're activating anyway.  */
    if (priv->act_source_id)
        return TRUE;

    return FALSE;
}

gboolean
bm_device_is_available (BMDevice *self)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (self);

	if (priv->firmware_missing)
		return FALSE;

	if (BM_DEVICE_GET_CLASS (self)->is_available)
		return BM_DEVICE_GET_CLASS (self)->is_available (self);
	return TRUE;
}

static gboolean
autoconnect_allowed_accumulator (GSignalInvocationHint *ihint,
                                 GValue *return_accu,
                                 const GValue *handler_return, gpointer data)
{
	if (!g_value_get_boolean (handler_return))
		g_value_set_boolean (return_accu, FALSE);
	return TRUE;
}

static void
share_child_setup (gpointer user_data G_GNUC_UNUSED)
{
	/* We are in the child process at this point */
	pid_t pid = getpid ();
	setpgid (pid, pid);
}

static void
set_property (GObject *object, guint prop_id,
			  const GValue *value, GParamSpec *pspec)
{
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (object);
 
	switch (prop_id) {
	case BM_DEVICE_INTERFACE_PROP_IFACE:
		g_free (priv->iface);
		priv->ifindex = 0;
		priv->iface = g_value_dup_string (value);
		if (priv->iface) {
			priv->ifindex = bm_netlink_iface_to_index (priv->iface);
			if (priv->ifindex < 0) {
				bm_log_warn (LOGD_HW, "(%s): failed to look up interface index", priv->iface);
			}
		}
		break;
	case BM_DEVICE_INTERFACE_PROP_DRIVER:
		priv->driver = g_strdup (g_value_get_string (value));
		break;
	case BM_DEVICE_INTERFACE_PROP_CAPABILITIES:
		priv->capabilities = g_value_get_uint (value);
		break;
	case BM_DEVICE_INTERFACE_PROP_MANAGED:
		priv->managed = g_value_get_boolean (value);
		break;
	case BM_DEVICE_INTERFACE_PROP_TYPE_DESC:
		g_free (priv->type_desc);
		priv->type_desc = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object, guint prop_id,
			  GValue *value, GParamSpec *pspec)
{
	BMDevice *self = BM_DEVICE (object);
	BMDevicePrivate *priv = BM_DEVICE_GET_PRIVATE (self);
	BMDeviceState state;

	state = bm_device_get_state (self);

	switch (prop_id) {
	case BM_DEVICE_INTERFACE_PROP_IFACE:
		g_value_set_string (value, priv->iface);
		break;
	case BM_DEVICE_INTERFACE_PROP_IFINDEX:
		g_value_set_int (value, priv->ifindex);
		break;
	case BM_DEVICE_INTERFACE_PROP_DRIVER:
		g_value_set_string (value, priv->driver);
		break;
	case BM_DEVICE_INTERFACE_PROP_CAPABILITIES:
		g_value_set_uint (value, priv->capabilities);
		break;
	case BM_DEVICE_INTERFACE_PROP_STATE:
		g_value_set_uint (value, priv->state);
		break;
	case BM_DEVICE_INTERFACE_PROP_DEVICE_TYPE:
		g_value_set_uint (value, priv->type);
		break;
	case BM_DEVICE_INTERFACE_PROP_TYPE_DESC:
		g_value_set_string (value, priv->type_desc);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
bm_device_class_init (BMDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (BMDevicePrivate));

	/* Virtual methods */
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->constructor = constructor;

	klass->get_type_capabilities = real_get_type_capabilities;
	klass->get_generic_capabilities = real_get_generic_capabilities;

	/* Properties */

	g_object_class_override_property (object_class,
									  BM_DEVICE_INTERFACE_PROP_IFACE,
									  BM_DEVICE_INTERFACE_IFACE);

	g_object_class_override_property (object_class,
	                                  BM_DEVICE_INTERFACE_PROP_IFINDEX,
	                                  BM_DEVICE_INTERFACE_IFINDEX);

	g_object_class_override_property (object_class,
									  BM_DEVICE_INTERFACE_PROP_DRIVER,
									  BM_DEVICE_INTERFACE_DRIVER);

	g_object_class_override_property (object_class,
									  BM_DEVICE_INTERFACE_PROP_CAPABILITIES,
									  BM_DEVICE_INTERFACE_CAPABILITIES);

	g_object_class_override_property (object_class,
									  BM_DEVICE_INTERFACE_PROP_STATE,
									  BM_DEVICE_INTERFACE_STATE);

	g_object_class_override_property (object_class,
									  BM_DEVICE_INTERFACE_PROP_DEVICE_TYPE,
									  BM_DEVICE_INTERFACE_DEVICE_TYPE);

	g_object_class_override_property (object_class,
									  BM_DEVICE_INTERFACE_PROP_TYPE_DESC,
									  BM_DEVICE_INTERFACE_TYPE_DESC);

	signals[AUTOCONNECT_ALLOWED] =
		g_signal_new ("autoconnect-allowed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              autoconnect_allowed_accumulator, NULL,
		              _bm_marshal_BOOLEAN__VOID,
		              G_TYPE_BOOLEAN, 0);
}

