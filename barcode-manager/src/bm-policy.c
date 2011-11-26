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

#include <config.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>

#include "bm-policy.h"
#include "BarcodeManagerUtils.h"
#include "bm-activation-request.h"
#include "bm-logging.h"
#include "bm-device-interface.h"
#include "bm-device.h"
#include "bm-dbus-manager.h"
#include "bm-setting-connection.h"
#include "bm-system.h"

struct BMPolicy {
	BMManager *manager;
	guint update_state_id;
	GSList *signal_ids;
	GSList *dev_signal_ids;
};

typedef struct {
	gulong id;
	BMDevice *device;
} DeviceSignalID;

#define INVALID_TAG "invalid"

static const char *
get_connection_id (BMConnection *connection)
{
	BMSettingConnection *s_con;

	g_return_val_if_fail (BM_IS_CONNECTION (connection), NULL);

	s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
	g_return_val_if_fail (s_con != NULL, NULL);

	return bm_setting_connection_get_id (s_con);
}

static void
global_state_changed (BMManager *manager, BMState state, gpointer user_data)
{
}

BMPolicy *
bm_policy_new (BMManager *manager)
{
	BMPolicy *policy;
	static gboolean initialized = FALSE;
	gulong id;

	g_return_val_if_fail (BM_IS_MANAGER (manager), NULL);
	g_return_val_if_fail (initialized == FALSE, NULL);

	policy = g_malloc0 (sizeof (BMPolicy));
	policy->manager = g_object_ref (manager);
	policy->update_state_id = 0;

	/*
	id = g_signal_connect (manager, "state-changed",
	                       G_CALLBACK (global_state_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "notify::" BM_MANAGER_HOSTNAME,
	                       G_CALLBACK (hostname_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "notify::" BM_MANAGER_SLEEPING,
	                       G_CALLBACK (sleeping_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "notify::" BM_MANAGER_NETWORKING_ENABLED,
	                       G_CALLBACK (sleeping_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "device-added",
	                       G_CALLBACK (device_added), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "device-removed",
	                       G_CALLBACK (device_removed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	*/

	/*
	id = g_signal_connect (manager, "user-permissions-changed",
	                       G_CALLBACK (manager_user_permissions_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);
	*/

	return policy;
}

void
bm_policy_destroy (BMPolicy *policy)
{
	GSList *iter;

	g_return_if_fail (policy != NULL);

	for (iter = policy->signal_ids; iter; iter = g_slist_next (iter))
		g_signal_handler_disconnect (policy->manager, (gulong) iter->data);
	g_slist_free (policy->signal_ids);

	for (iter = policy->dev_signal_ids; iter; iter = g_slist_next (iter)) {
		DeviceSignalID *data = (DeviceSignalID *) iter->data;

		g_signal_handler_disconnect (data->device, data->id);
		g_free (data);
	}
	g_slist_free (policy->dev_signal_ids);

	g_object_unref (policy->manager);
	g_free (policy);
}

