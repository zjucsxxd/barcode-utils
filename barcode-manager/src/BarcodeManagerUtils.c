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
 * Copyright (C) 2004 - 2010 Red Hat, Inc.
 * Copyright (C) 2005 - 2008 Novell, Inc.
 */

#include <glib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "BarcodeManagerUtils.h"
#include "bm-utils.h"
#include "bm-logging.h"
#include "bm-device.h"
#include "bm-dbus-manager.h"
#include "bm-dispatcher-action.h"
#include "bm-dbus-glib-types.h"

#include <netlink/addr.h>
#include <netinet/in.h>

int
bm_spawn_process (const char *args)
{
	gint num_args;
	char **argv = NULL;
	int status = -1;
	GError *error = NULL;

	g_return_val_if_fail (args != NULL, -1);

	if (!g_shell_parse_argv (args, &num_args, &argv, &error)) {
		bm_log_warn (LOGD_CORE, "could not parse arguments for '%s': %s", args, error->message);
		g_error_free (error);
		return -1;
	}

	if (!g_spawn_sync ("/", argv, NULL, 0, NULL, NULL, NULL, NULL, &status, &error)) {
		bm_log_warn (LOGD_CORE, "could not spawn process '%s': %s", args, error->message);
		g_error_free (error);
	}

	g_strfreev (argv);
	return status;
}

static void
dispatcher_done_cb (DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	dbus_g_proxy_end_call (proxy, call, NULL, G_TYPE_INVALID);
	g_object_unref (proxy);
}

static void
bm_gvalue_destroy (gpointer data)
{
	GValue *value = (GValue *) data;

	g_value_unset (value);
	g_slice_free (GValue, value);
}

GHashTable *
value_hash_create (void)
{
	return g_hash_table_new_full (g_str_hash, g_str_equal, g_free, bm_gvalue_destroy);
}

void
value_hash_add (GHashTable *hash,
				const char *key,
				GValue *value)
{
	g_hash_table_insert (hash, g_strdup (key), value);
}

void
value_hash_add_str (GHashTable *hash,
					const char *key,
					const char *str)
{
	GValue *value;

	value = g_slice_new0 (GValue);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_string (value, str);

	value_hash_add (hash, key, value);
}

void
value_hash_add_object_path (GHashTable *hash,
							const char *key,
							const char *op)
{
	GValue *value;

	value = g_slice_new0 (GValue);
	g_value_init (value, DBUS_TYPE_G_OBJECT_PATH);
	g_value_set_boxed (value, op);

	value_hash_add (hash, key, value);
}

void
value_hash_add_uint (GHashTable *hash,
					 const char *key,
					 guint32 val)
{
	GValue *value;

	value = g_slice_new0 (GValue);
	g_value_init (value, G_TYPE_UINT);
	g_value_set_uint (value, val);

	value_hash_add (hash, key, value);
}

void
value_hash_add_bool (GHashTable *hash,
					 const char *key,
					 gboolean val)
{
	GValue *value;

	value = g_slice_new0 (GValue);
	g_value_init (value, G_TYPE_BOOLEAN);
	g_value_set_boolean (value, val);

	value_hash_add (hash, key, value);
}

gboolean
bm_utils_do_sysctl (const char *path, const char *value)
{
	int fd, len, nwrote, total;

	fd = open (path, O_WRONLY | O_TRUNC);
	if (fd == -1)
		return FALSE;

	len = strlen (value);
	total = 0;
	do {
		nwrote = write (fd, value + total, len - total);
		if (nwrote == -1) {
			if (errno == EINTR)
				continue;
			close (fd);
			return FALSE;
		}
		total += nwrote;
	} while (total < len);

	close (fd);
	return TRUE;
}

gboolean
bm_utils_get_proc_sys_net_value (const char *path,
                                 const char *iface,
                                 guint32 *out_value)
{
	GError *error = NULL;
	char *contents = NULL;
	gboolean success = FALSE;
	long int tmp;

	if (!g_file_get_contents (path, &contents, NULL, &error)) {
		bm_log_dbg (LOGD_DEVICE, "(%s): error reading %s: (%d) %s",
		            iface, path,
		            error ? error->code : -1,
		            error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
	} else {
		errno = 0;
		tmp = strtol (contents, NULL, 10);
		if ((errno == 0) && (tmp == 0 || tmp == 1)) {
			*out_value = (guint32) tmp;
			success = TRUE;
		}
		g_free (contents);
	}

	return success;
}

