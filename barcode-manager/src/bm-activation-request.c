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
 * Copyright (C) 2005 - 2010 Red Hat, Inc.
 * Copyright (C) 2007 - 2008 Novell, Inc.
 */

#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dbus/dbus-glib.h>

#include "bm-activation-request.h"
#include "bm-marshal.h"
#include "bm-logging.h"
#include "bm-setting-wireless-security.h"
#include "bm-setting-8021x.h"
#include "bm-dbus-manager.h"
#include "bm-device.h"
#include "bm-properties-changed-signal.h"
#include "bm-active-connection.h"
#include "bm-dbus-glib-types.h"
#include "bm-active-connection-glue.h"


static void secrets_provider_interface_init (NMSecretsProviderInterface *sp_interface_class);

G_DEFINE_TYPE_EXTENDED (NMActRequest, bm_act_request, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BM_TYPE_SECRETS_PROVIDER_INTERFACE,
                                               secrets_provider_interface_init))

#define BM_ACT_REQUEST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BM_TYPE_ACT_REQUEST, NMActRequestPrivate))

enum {
	CONNECTION_SECRETS_UPDATED,
	CONNECTION_SECRETS_FAILED,
	PROPERTIES_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct {
	char *table;
	char *rule;
} ShareRule;

typedef struct {
	gboolean disposed;

	BMConnection *connection;
	guint32 secrets_call_id;

	char *specific_object;
	BMDevice *device;
	gboolean user_requested;

	BMActiveConnectionState state;
	gboolean is_default;
	gboolean is_default6;
	gboolean shared;
	GSList *share_rules;

	char *ac_path;

	gboolean assumed;
} NMActRequestPrivate;

enum {
	PROP_0,
	PROP_SERVICE_NAME,
	PROP_CONNECTION,
	PROP_SPECIFIC_OBJECT,
	PROP_DEVICES,
	PROP_STATE,
	PROP_DEFAULT,
	PROP_DEFAULT6,
	PROP_VPN,

	LAST_PROP
};


static void
device_state_changed (BMDevice *device,
                      BMDeviceState new_state,
                      BMDeviceState old_state,
                      BMDeviceStateReason reason,
                      gpointer user_data)
{
	NMActRequest *self = BM_ACT_REQUEST (user_data);
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (self);
	BMActiveConnectionState new_ac_state;
	gboolean new_default = FALSE, new_default6 = FALSE;

	/* Set BMActiveConnection state based on the device's state */
	switch (new_state) {
	case BM_DEVICE_STATE_PREPARE:
	case BM_DEVICE_STATE_CONFIG:
	case BM_DEVICE_STATE_NEED_AUTH:
	case BM_DEVICE_STATE_IP_CONFIG:
		new_ac_state = BM_ACTIVE_CONNECTION_STATE_ACTIVATING;
		break;
	case BM_DEVICE_STATE_ACTIVATED:
		new_ac_state = BM_ACTIVE_CONNECTION_STATE_ACTIVATED;
		new_default = priv->is_default;
		new_default6 = priv->is_default6;
		break;
	default:
		new_ac_state = BM_ACTIVE_CONNECTION_STATE_UNKNOWN;
		break;
	}

	if (new_ac_state != priv->state) {
		priv->state = new_ac_state;
		g_object_notify (G_OBJECT (self), BM_ACTIVE_CONNECTION_STATE);
	}

	if (new_default != priv->is_default) {
		priv->is_default = new_default;
		g_object_notify (G_OBJECT (self), BM_ACTIVE_CONNECTION_DEFAULT);
	}

	if (new_default6 != priv->is_default6) {
		priv->is_default6 = new_default6;
		g_object_notify (G_OBJECT (self), BM_ACTIVE_CONNECTION_DEFAULT6);
	}
}

NMActRequest *
bm_act_request_new (BMConnection *connection,
                    const char *specific_object,
                    gboolean user_requested,
                    gboolean assumed,
                    gpointer *device)
{
	GObject *object;
	NMActRequestPrivate *priv;

	g_return_val_if_fail (BM_IS_CONNECTION (connection), NULL);
	g_return_val_if_fail (BM_DEVICE (device), NULL);

	object = g_object_new (BM_TYPE_ACT_REQUEST, NULL);
	if (!object)
		return NULL;

	priv = BM_ACT_REQUEST_GET_PRIVATE (object);

	priv->connection = g_object_ref (connection);
	if (specific_object)
		priv->specific_object = g_strdup (specific_object);

	priv->device = BM_DEVICE (device);
	g_signal_connect (device, "state-changed",
	                  G_CALLBACK (device_state_changed),
	                  BM_ACT_REQUEST (object));

	priv->user_requested = user_requested;
	priv->assumed = assumed;

	return BM_ACT_REQUEST (object);
}

static void
bm_act_request_init (NMActRequest *req)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (req);
	NMDBusManager *dbus_mgr;

	priv->ac_path = bm_active_connection_get_next_object_path ();
	priv->state = BM_ACTIVE_CONNECTION_STATE_UNKNOWN;

	dbus_mgr = bm_dbus_manager_get ();
	dbus_g_connection_register_g_object (bm_dbus_manager_get_connection (dbus_mgr),
	                                     priv->ac_path,
	                                     G_OBJECT (req));
	g_object_unref (dbus_mgr);
}

static void
dispose (GObject *object)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (object);

	if (priv->disposed) {
		G_OBJECT_CLASS (bm_act_request_parent_class)->dispose (object);
		return;
	}
	priv->disposed = TRUE;

	g_assert (priv->connection);

	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->device),
	                                      G_CALLBACK (device_state_changed),
	                                      BM_ACT_REQUEST (object));

	/* Clear any share rules */
	bm_act_request_set_shared (BM_ACT_REQUEST (object), FALSE);

	g_object_unref (priv->connection);

	G_OBJECT_CLASS (bm_act_request_parent_class)->dispose (object);
}

static void
clear_share_rules (NMActRequest *req)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (req);
	GSList *iter;

	for (iter = priv->share_rules; iter; iter = g_slist_next (iter)) {
		ShareRule *rule = (ShareRule *) iter->data;

		g_free (rule->table);
		g_free (rule->rule);
		g_free (rule);
	}

	g_slist_free (priv->share_rules);
	priv->share_rules = NULL;
}

static void
finalize (GObject *object)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (object);

	g_free (priv->specific_object);
	g_free (priv->ac_path);

	clear_share_rules (BM_ACT_REQUEST (object));

	G_OBJECT_CLASS (bm_act_request_parent_class)->finalize (object);
}

static void
get_property (GObject *object, guint prop_id,
			  GValue *value, GParamSpec *pspec)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (object);
	GPtrArray *devices;

	switch (prop_id) {
	case PROP_SERVICE_NAME:
		bm_active_connection_scope_to_value (priv->connection, value);
		break;
	case PROP_CONNECTION:
		g_value_set_boxed (value, bm_connection_get_path (priv->connection));
		break;
	case PROP_SPECIFIC_OBJECT:
		if (priv->specific_object)
			g_value_set_boxed (value, priv->specific_object);
		else
			g_value_set_boxed (value, "/");
		break;
	case PROP_DEVICES:
		devices = g_ptr_array_sized_new (1);
		g_ptr_array_add (devices, g_strdup (bm_device_get_path (priv->device)));
		g_value_take_boxed (value, devices);
		break;
	case PROP_STATE:
		g_value_set_uint (value, priv->state);
		break;
	case PROP_DEFAULT:
		g_value_set_boolean (value, priv->is_default);
		break;
	case PROP_DEFAULT6:
		g_value_set_boolean (value, priv->is_default6);
		break;
	case PROP_VPN:
		g_value_set_boolean (value, FALSE);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_act_request_class_init (NMActRequestClass *req_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (req_class);

	g_type_class_add_private (req_class, sizeof (NMActRequestPrivate));

	/* virtual methods */
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	/* properties */
    bm_active_connection_install_properties (object_class,
                                             PROP_SERVICE_NAME,
                                             PROP_CONNECTION,
                                             PROP_SPECIFIC_OBJECT,
                                             PROP_DEVICES,
                                             PROP_STATE,
                                             PROP_DEFAULT,
                                             PROP_DEFAULT6,
                                             PROP_VPN);

	/* Signals */
	signals[CONNECTION_SECRETS_UPDATED] =
		g_signal_new ("connection-secrets-updated",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMActRequestClass, secrets_updated),
					  NULL, NULL,
					  _bm_marshal_VOID__OBJECT_POINTER_UINT,
					  G_TYPE_NONE, 3,
					  G_TYPE_OBJECT, G_TYPE_POINTER, G_TYPE_UINT);

	signals[CONNECTION_SECRETS_FAILED] =
		g_signal_new ("connection-secrets-failed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMActRequestClass, secrets_failed),
					  NULL, NULL,
					  _bm_marshal_VOID__OBJECT_STRING_UINT,
					  G_TYPE_NONE, 3,
					  G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_UINT);

	signals[PROPERTIES_CHANGED] = 
		bm_properties_changed_signal_new (object_class,
		                                  G_STRUCT_OFFSET (NMActRequestClass, properties_changed));

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (req_class),
	                                 &dbus_glib_bm_active_connection_object_info);
}

static gboolean
secrets_update_setting (NMSecretsProviderInterface *interface,
                        const char *setting_name,
                        GHashTable *new)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (interface);
	BMSetting *setting = NULL;
	GError *error = NULL;
	GType type;

	g_return_val_if_fail (priv->connection != NULL, FALSE);

	/* Check whether a complete & valid BMSetting object was returned.  If
	 * yes, replace the setting object in the connection.  If not, just try
	 * updating the secrets.
	 */
	type = bm_connection_lookup_setting_type (setting_name);
	if (type == 0)
		return FALSE;

	setting = bm_setting_new_from_hash (type, new);
	if (setting) {
		BMSetting *s_8021x = NULL;
		GSList *all_settings = NULL;

		/* The wireless-security setting might need the 802.1x setting in
		 * the all_settings argument of the verify function. Ugh.
		 */
		s_8021x = bm_connection_get_setting (priv->connection, BM_TYPE_SETTING_802_1X);
		if (s_8021x)
			all_settings = g_slist_append (all_settings, s_8021x);

		if (!bm_setting_verify (setting, all_settings, NULL)) {
			/* Just try updating secrets */
			g_object_unref (setting);
			setting = NULL;
		}

		g_slist_free (all_settings);
	}

	if (setting)
		bm_connection_add_setting (priv->connection, setting);
	else {
		if (!bm_connection_update_secrets (priv->connection, setting_name, new, &error)) {
			bm_log_warn (LOGD_DEVICE, "Failed to update connection secrets: %d %s",
			             error ? error->code : -1,
			             error && error->message ? error->message : "(none)");
			g_clear_error (&error);
		}
	}

	return TRUE;
}

static void
secrets_result (NMSecretsProviderInterface *interface,
	            const char *setting_name,
	            RequestSecretsCaller caller,
	            const GSList *updated,
	            GError *error)
{
	NMActRequest *self = BM_ACT_REQUEST (interface);
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (self);

	g_return_if_fail (priv->connection != NULL);

	if (error) {
		g_signal_emit (self, signals[CONNECTION_SECRETS_FAILED], 0,
		               priv->connection, setting_name, caller);
	} else {
		g_signal_emit (self, signals[CONNECTION_SECRETS_UPDATED], 0,
		               priv->connection, updated, caller);
	}
}

static void
secrets_provider_interface_init (NMSecretsProviderInterface *sp_interface_class)
{
	/* interface implementation */
	sp_interface_class->update_setting = secrets_update_setting;
	sp_interface_class->result = secrets_result;
}

gboolean
bm_act_request_get_secrets (NMActRequest *self,
                            const char *setting_name,
                            gboolean request_new,
                            RequestSecretsCaller caller,
                            const char *hint1,
                            const char *hint2)
{
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (BM_IS_ACT_REQUEST (self), FALSE);

	return bm_secrets_provider_interface_get_secrets (BM_SECRETS_PROVIDER_INTERFACE (self),
	                                                  bm_act_request_get_connection (self),
	                                                  setting_name,
	                                                  request_new,
	                                                  caller,
	                                                  hint1,
	                                                  hint2);
}

BMConnection *
bm_act_request_get_connection (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), NULL);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->connection;
}

const char *
bm_act_request_get_specific_object (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), NULL);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->specific_object;
}

void
bm_act_request_set_specific_object (NMActRequest *req,
                                    const char *specific_object)
{
	NMActRequestPrivate *priv;

	g_return_if_fail (BM_IS_ACT_REQUEST (req));
	g_return_if_fail (specific_object != NULL);

	priv = BM_ACT_REQUEST_GET_PRIVATE (req);

	if (priv->specific_object)
		g_free (priv->specific_object);
	priv->specific_object = g_strdup (specific_object);
}

gboolean
bm_act_request_get_user_requested (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), FALSE);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->user_requested;
}

const char *
bm_act_request_get_active_connection_path (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), NULL);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->ac_path;
}

void
bm_act_request_set_default (NMActRequest *req, gboolean is_default)
{
	NMActRequestPrivate *priv;

	g_return_if_fail (BM_IS_ACT_REQUEST (req));

	priv = BM_ACT_REQUEST_GET_PRIVATE (req);
	if (priv->is_default == is_default)
		return;

	priv->is_default = is_default;
	g_object_notify (G_OBJECT (req), BM_ACTIVE_CONNECTION_DEFAULT);
}

gboolean
bm_act_request_get_default (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), FALSE);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->is_default;
}

void
bm_act_request_set_default6 (NMActRequest *req, gboolean is_default6)
{
	NMActRequestPrivate *priv;

	g_return_if_fail (BM_IS_ACT_REQUEST (req));

	priv = BM_ACT_REQUEST_GET_PRIVATE (req);
	if (priv->is_default6 == is_default6)
		return;

	priv->is_default6 = is_default6;
	g_object_notify (G_OBJECT (req), BM_ACTIVE_CONNECTION_DEFAULT6);
}

gboolean
bm_act_request_get_default6 (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), FALSE);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->is_default6;
}

static void
share_child_setup (gpointer user_data G_GNUC_UNUSED)
{
	/* We are in the child process at this point */
	pid_t pid = getpid ();
	setpgid (pid, pid);
}

void
bm_act_request_set_shared (NMActRequest *req, gboolean shared)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (req);
	GSList *list, *iter;

	g_return_if_fail (BM_IS_ACT_REQUEST (req));

	BM_ACT_REQUEST_GET_PRIVATE (req)->shared = shared;

	/* Tear the rules down in reverse order when sharing is stopped */
	list = g_slist_copy (priv->share_rules);
	if (!shared)
		list = g_slist_reverse (list);

	/* Send the rules to iptables */
	for (iter = list; iter; iter = g_slist_next (iter)) {
		ShareRule *rule = (ShareRule *) iter->data;
		char *envp[1] = { NULL };
		char **argv;
		char *cmd;

		cmd = g_strdup_printf ("%s --table %s %s %s",
		                       IPTABLES_PATH,
		                       rule->table,
		                       shared ? "--insert" : "--delete",
		                       rule->rule);
		if (!cmd)
			continue;

		argv = g_strsplit (cmd, " ", 0);
		if (argv && argv[0]) {
			int status;
			GError *error = NULL;

			bm_log_info (LOGD_SHARING, "Executing: %s", cmd);
			if (!g_spawn_sync ("/", argv, envp, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
			                   share_child_setup, NULL, NULL, NULL, &status, &error)) {
				bm_log_warn (LOGD_SHARING, "Error executing command: (%d) %s",
				             error ? error->code : -1,
				             (error && error->message) ? error->message : "(unknown)");
				g_clear_error (&error);
			} else if (WEXITSTATUS (status)) {
				bm_log_warn (LOGD_SHARING, "** Command returned exit status %d.",
				             WEXITSTATUS (status));
			}
		}
		g_free (cmd);
		if (argv)
			g_strfreev (argv);
	}

	g_slist_free (list);

	/* Clear the share rule list when sharing is stopped */
	if (!shared)
		clear_share_rules (req);
}

gboolean
bm_act_request_get_shared (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), FALSE);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->shared;
}

void
bm_act_request_add_share_rule (NMActRequest *req,
                               const char *table,
                               const char *table_rule)
{
	NMActRequestPrivate *priv = BM_ACT_REQUEST_GET_PRIVATE (req);
	ShareRule *rule;

	g_return_if_fail (BM_IS_ACT_REQUEST (req));
	g_return_if_fail (table != NULL);
	g_return_if_fail (table_rule != NULL);
	
	rule = g_malloc0 (sizeof (ShareRule));
	rule->table = g_strdup (table);
	rule->rule = g_strdup (table_rule);
	priv->share_rules = g_slist_append (priv->share_rules, rule);
}

GObject *
bm_act_request_get_device (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), FALSE);

	return G_OBJECT (BM_ACT_REQUEST_GET_PRIVATE (req)->device);
}

gboolean
bm_act_request_get_assumed (NMActRequest *req)
{
	g_return_val_if_fail (BM_IS_ACT_REQUEST (req), FALSE);

	return BM_ACT_REQUEST_GET_PRIVATE (req)->assumed;
}

