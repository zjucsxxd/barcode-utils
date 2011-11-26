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

#include <string.h>

#include "bm-marshal.h"
#include "bm-secrets-provider-interface.h"

#include "bm-logging.h"

static void
bm_secrets_provider_interface_init (gpointer g_iface)
{
	GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);
	static gboolean initialized = FALSE;

	if (initialized)
		return;
	initialized = TRUE;

	/* Signals */
	g_signal_new ("manager-get-secrets",
	              iface_type,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (NMSecretsProviderInterface, manager_get_secrets),
	              NULL, NULL,
	              _bm_marshal_BOOLEAN__POINTER_STRING_BOOLEAN_UINT_STRING_STRING,
	              G_TYPE_BOOLEAN, 6,
	              G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);

	g_signal_new ("manager-cancel-secrets",
	              iface_type,
	              G_SIGNAL_RUN_FIRST,
	              G_STRUCT_OFFSET (NMSecretsProviderInterface, manager_cancel_secrets),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE, 0);
}


GType
bm_secrets_provider_interface_get_type (void)
{
	static GType interface_type = 0;

	if (!interface_type) {
		const GTypeInfo interface_info = {
			sizeof (NMSecretsProviderInterface), /* class_size */
			bm_secrets_provider_interface_init,   /* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			0,
			0,              /* n_preallocs */
			NULL
		};

		interface_type = g_type_register_static (G_TYPE_INTERFACE,
		                                         "NMSecretsProviderInterface",
		                                         &interface_info, 0);

		g_type_interface_add_prerequisite (interface_type, G_TYPE_OBJECT);
	}

	return interface_type;
}

gboolean
bm_secrets_provider_interface_get_secrets (NMSecretsProviderInterface *self,
                                           BMConnection *connection,
                                           const char *setting_name,
                                           gboolean request_new,
                                           RequestSecretsCaller caller,
                                           const char *hint1,
                                           const char *hint2)
{
	guint success = FALSE;

	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (BM_IS_SECRETS_PROVIDER_INTERFACE (self), FALSE);
	g_return_val_if_fail (connection != NULL, FALSE);
	g_return_val_if_fail (BM_IS_CONNECTION (connection), FALSE);
	g_return_val_if_fail (setting_name != NULL, FALSE);

	bm_secrets_provider_interface_cancel_get_secrets (self);

	g_signal_emit_by_name (self, "manager-get-secrets",
	                       connection, setting_name, request_new, caller, hint1, hint2,
	                       &success);
	if (!success) {
		bm_log_warn (LOGD_CORE, "failed to get connection secrets.");
		return FALSE;
	}

	return TRUE;
}

void
bm_secrets_provider_interface_cancel_get_secrets (NMSecretsProviderInterface *self)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (BM_IS_SECRETS_PROVIDER_INTERFACE (self));

	g_signal_emit_by_name (self, "manager-cancel-secrets");
}


static void
add_one_key_to_list (gpointer key, gpointer data, gpointer user_data)
{
	GSList **list = (GSList **) user_data;

	*list = g_slist_append (*list, key);
}

