/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- Barcode scanner manager
 *
 * Jakob Flierl <jakob.flierl@gmail.com>
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
 * (C) Copyright 2011 Jakob Flierl
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <glib.h>
#include <gnome-keyring.h>
#include <dbus/dbus-glib.h>

#include <bm-setting-bluetooth.h>
#include <bm-setting-connection.h>
#include <bm-utils.h>
#include <bm-settings-interface.h>

#include "gconf-helpers.h"
#include "gconf-upgrade.h"
#include "utils.h"
#include "applet.h"

#define S390_OPT_KEY_PREFIX "s390-opt-"

#define DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH    (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))
#define DBUS_TYPE_G_ARRAY_OF_STRING         (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))
#define DBUS_TYPE_G_ARRAY_OF_UINT           (dbus_g_type_get_collection ("GArray", G_TYPE_UINT))
#define DBUS_TYPE_G_ARRAY_OF_ARRAY_OF_UCHAR (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_UCHAR_ARRAY))
#define DBUS_TYPE_G_ARRAY_OF_ARRAY_OF_UINT  (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_ARRAY_OF_UINT))
#define DBUS_TYPE_G_MAP_OF_VARIANT          (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#define DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT   (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, DBUS_TYPE_G_MAP_OF_VARIANT))
#define DBUS_TYPE_G_MAP_OF_STRING           (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING))
#define DBUS_TYPE_G_LIST_OF_STRING          (dbus_g_type_get_collection ("GSList", G_TYPE_STRING))
#define DBUS_TYPE_G_IP6_ADDRESS             (dbus_g_type_get_struct ("GValueArray", DBUS_TYPE_G_UCHAR_ARRAY, G_TYPE_UINT, DBUS_TYPE_G_UCHAR_ARRAY, G_TYPE_INVALID))
#define DBUS_TYPE_G_ARRAY_OF_IP6_ADDRESS    (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_IP6_ADDRESS))
#define DBUS_TYPE_G_IP6_ROUTE               (dbus_g_type_get_struct ("GValueArray", DBUS_TYPE_G_UCHAR_ARRAY, G_TYPE_UINT, DBUS_TYPE_G_UCHAR_ARRAY, G_TYPE_UINT, G_TYPE_INVALID))
#define DBUS_TYPE_G_ARRAY_OF_IP6_ROUTE      (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_IP6_ROUTE))

const char *applet_8021x_cert_keys[] = {
	"ca-cert",
	"client-cert",
	"private-key",
	"phase2-ca-cert",
	"phase2-client-cert",
	"phase2-private-key",
	NULL
};

static PreKeyringCallback pre_keyring_cb = NULL;
static gpointer pre_keyring_user_data = NULL;

/* Sets a function to be called before each keyring access */
void
bm_gconf_set_pre_keyring_callback (PreKeyringCallback func, gpointer user_data)
{
	pre_keyring_cb = func;
	pre_keyring_user_data = user_data;
}

void
pre_keyring_callback (void)
{
	GnomeKeyringInfo *info = NULL;

	if (!pre_keyring_cb)
		return;

	/* Call the pre keyring callback if the keyring is locked or if there
	 * was an error talking to the keyring.
	 */
	if (gnome_keyring_get_info_sync (NULL, &info) == GNOME_KEYRING_RESULT_OK) {
		if (gnome_keyring_info_get_is_locked (info))
			(*pre_keyring_cb) (pre_keyring_user_data);
		gnome_keyring_info_free (info);
	} else
		(*pre_keyring_cb) (pre_keyring_user_data);
}


gboolean
bm_gconf_get_int_helper (GConfClient *client,
                         const char *path,
                         const char *key,
                         const char *setting,
                         int *value)
{
	char *		gc_key;
	GConfValue *	gc_value;
	gboolean		success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if ((gc_value = gconf_client_get (client, gc_key, NULL)))
	{
		if (gc_value->type == GCONF_VALUE_INT)
		{
			*value = gconf_value_get_int (gc_value);
			success = TRUE;
		}
		gconf_value_free (gc_value);
	}
	g_free (gc_key);

	return success;
}


gboolean
bm_gconf_get_float_helper (GConfClient *client,
                           const char *path,
                           const char *key,
                           const char *setting,
                           gfloat *value)
{
	char *		gc_key;
	GConfValue *	gc_value;
	gboolean		success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if ((gc_value = gconf_client_get (client, gc_key, NULL)))
	{
		if (gc_value->type == GCONF_VALUE_FLOAT)
		{
			*value = gconf_value_get_float (gc_value);
			success = TRUE;
		}
		gconf_value_free (gc_value);
	}
	g_free (gc_key);

	return success;
}


gboolean
bm_gconf_get_string_helper (GConfClient *client,
                            const char *path,
                            const char *key,
                            const char *setting,
                            char **value)
{
	char *		gc_key;
	GConfValue *	gc_value;
	gboolean		success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);
	g_return_val_if_fail (*value == NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if ((gc_value = gconf_client_get (client, gc_key, NULL)))
	{
		if (gc_value->type == GCONF_VALUE_STRING)
		{
			*value = g_strdup (gconf_value_get_string (gc_value));
			success = TRUE;
		}
		gconf_value_free (gc_value);
	}
	g_free (gc_key);

	return success;
}


gboolean
bm_gconf_get_bool_helper (GConfClient *client,
                          const char *path,
                          const char *key,
                          const char *setting,
                          gboolean *value)
{
	char *		gc_key;
	GConfValue *	gc_value;
	gboolean		success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if ((gc_value = gconf_client_get (client, gc_key, NULL)))
	{
		if (gc_value->type == GCONF_VALUE_BOOL)
		{
			*value = gconf_value_get_bool (gc_value);
			success = TRUE;
		}
		else if (gc_value->type == GCONF_VALUE_STRING && !*gconf_value_get_string (gc_value))
		{
			/* This is a kludge to deal with VPN connections migrated from NM 0.6 */
			*value = TRUE;
			success = TRUE;
		}

		gconf_value_free (gc_value);
	}
	g_free (gc_key);

	return success;
}

gboolean
bm_gconf_get_stringlist_helper (GConfClient *client,
                                const char *path,
                                const char *key,
                                const char *setting,
                                GSList **value)
{
	char *gc_key;
	GConfValue *gc_value;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!(gc_value = gconf_client_get (client, gc_key, NULL)))
		goto out;

	if (gc_value->type == GCONF_VALUE_LIST
	    && gconf_value_get_list_type (gc_value) == GCONF_VALUE_STRING)
	{
		GSList *elt;

		for (elt = gconf_value_get_list (gc_value); elt != NULL; elt = g_slist_next (elt))
		{
			const char *string = gconf_value_get_string ((GConfValue *) elt->data);

			*value = g_slist_append (*value, g_strdup (string));
		}

		success = TRUE;
	}

out:
	if (gc_value)
		gconf_value_free (gc_value);
	g_free (gc_key);
	return success;
}

gboolean
bm_gconf_get_stringarray_helper (GConfClient *client,
                                 const char *path,
                                 const char *key,
                                 const char *setting,
                                 GPtrArray **value)
{
	char *gc_key;
	GConfValue *gc_value;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);
	g_return_val_if_fail (*value == NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!(gc_value = gconf_client_get (client, gc_key, NULL)))
		goto out;

	if (gc_value->type == GCONF_VALUE_LIST
	    && gconf_value_get_list_type (gc_value) == GCONF_VALUE_STRING)
	{
		GSList *iter, *list;
		const char *string;

		*value = g_ptr_array_sized_new (3);

		list = gconf_value_get_list (gc_value);
		for (iter = list; iter != NULL; iter = g_slist_next (iter)) {
			string = gconf_value_get_string ((GConfValue *) iter->data);
			g_ptr_array_add (*value, g_strdup (string));
		}

		success = TRUE;
	}

out:
	if (gc_value)
		gconf_value_free (gc_value);
	g_free (gc_key);
	return success;
}

gboolean
bm_gconf_get_bytearray_helper (GConfClient *client,
                               const char *path,
                               const char *key,
                               const char *setting,
                               GByteArray **value)
{
	char *gc_key;
	GConfValue *gc_value;
	GByteArray *array;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!(gc_value = gconf_client_get (client, gc_key, NULL)))
		goto out;

	if (gc_value->type == GCONF_VALUE_LIST
	    && gconf_value_get_list_type (gc_value) == GCONF_VALUE_INT)
	{
		GSList *elt;

		array = g_byte_array_new ();
		for (elt = gconf_value_get_list (gc_value); elt != NULL; elt = g_slist_next (elt))
		{
			int i = gconf_value_get_int ((GConfValue *) elt->data);
			unsigned char val = (unsigned char) (i & 0xFF);

			if (i < 0 || i > 255) {
				g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
				       "value %d out-of-range for a byte value", i);
				g_byte_array_free (array, TRUE);
				goto out;
			}

			g_byte_array_append (array, (const unsigned char *) &val, sizeof (val));
		}

		*value = array;
		success = TRUE;
	}

out:
	if (gc_value)
		gconf_value_free (gc_value);
	g_free (gc_key);
	return success;
}

gboolean
bm_gconf_get_uint_array_helper (GConfClient *client,
                                const char *path,
                                const char *key,
                                const char *setting,
                                GArray **value)
{
	char *gc_key;
	GConfValue *gc_value;
	GArray *array;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!(gc_value = gconf_client_get (client, gc_key, NULL)))
		goto out;

	if (gc_value->type == GCONF_VALUE_LIST
	    && gconf_value_get_list_type (gc_value) == GCONF_VALUE_INT)
	{
		GSList *elt;

		array = g_array_new (FALSE, FALSE, sizeof (gint));
		for (elt = gconf_value_get_list (gc_value); elt != NULL; elt = g_slist_next (elt))
		{
			int i = gconf_value_get_int ((GConfValue *) elt->data);
			g_array_append_val (array, i);
		}

		*value = array;
		success = TRUE;
	}
	
out:
	if (gc_value)
		gconf_value_free (gc_value);
	g_free (gc_key);
	return success;
}

#if UNUSED
static void
property_value_destroy (gpointer data)
{
	GValue *value = (GValue *) data;

	g_value_unset (value);
	g_slice_free (GValue, data);
}


static void
add_property (GHashTable *properties, const char *key, GConfValue *gconf_value)
{
	GValue *value = NULL;

	if (!gconf_value)
		return;

	switch (gconf_value->type) {
	case GCONF_VALUE_STRING:
		value = g_slice_new0 (GValue);
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, gconf_value_get_string (gconf_value));
		break;
	case GCONF_VALUE_INT:
		value = g_slice_new0 (GValue);
		g_value_init (value, G_TYPE_INT);
		g_value_set_int (value, gconf_value_get_int (gconf_value));
		break;
	case GCONF_VALUE_BOOL:
		value = g_slice_new0 (GValue);
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, gconf_value_get_bool (gconf_value));
		break;
	default:
		break;
	}

	if (value)
		g_hash_table_insert (properties, gconf_unescape_key (key, -1), value);
}

gboolean
bm_gconf_get_valuehash_helper (GConfClient *client,
                               const char *path,
                               const char *setting,
                               GHashTable **value)
{
	char *gc_key;
	GSList *gconf_entries;
	GSList *iter;
	int prefix_len;

	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s", path, setting);
	prefix_len = strlen (gc_key);
	gconf_entries = gconf_client_all_entries (client, gc_key, NULL);
	g_free (gc_key);

	if (!gconf_entries)
		return FALSE;

	*value = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                (GDestroyNotify) g_free,
	                                property_value_destroy);

	for (iter = gconf_entries; iter; iter = iter->next) {
		GConfEntry *entry = (GConfEntry *) iter->data;

		gc_key = (char *) gconf_entry_get_key (entry);
		gc_key += prefix_len + 1; /* get rid of the full path */

		add_property (*value, gc_key, gconf_entry_get_value (entry));
		gconf_entry_unref (entry);
	}

	g_slist_free (gconf_entries);
	return TRUE;
}
#endif

gboolean
bm_gconf_set_int_helper (GConfClient *client,
                         const char *path,
                         const char *key,
                         const char *setting,
                         int value)
{
	char * gc_key;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}
	gconf_client_set_int (client, gc_key, value, NULL);
	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_float_helper (GConfClient *client,
                           const char *path,
                           const char *key,
                           const char *setting,
                           gfloat value)
{
	char * gc_key;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}
	gconf_client_set_float (client, gc_key, value, NULL);
	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_string_helper (GConfClient *client,
                            const char *path,
                            const char *key,
                            const char *setting,
                            const char *value)
{
	char * gc_key;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	if (value)
		gconf_client_set_string (client, gc_key, value, NULL);
	else
		gconf_client_unset (client, gc_key, NULL);

	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_bool_helper (GConfClient *client,
                          const char *path,
                          const char *key,
                          const char *setting,
                          gboolean value)
{
	char * gc_key;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}
	gconf_client_set_bool (client, gc_key, value, NULL);
	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_stringlist_helper (GConfClient *client,
                                const char *path,
                                const char *key,
                                const char *setting,
                                GSList *value)
{
	char *gc_key;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	gconf_client_set_list (client, gc_key, GCONF_VALUE_STRING, value, NULL);
	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_stringarray_helper (GConfClient *client,
                                 const char *path,
                                 const char *key,
                                 const char *setting,
                                 GPtrArray *value)
{
	char *gc_key;
	GSList *list = NULL;
	int i;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++)
		list = g_slist_append (list, g_ptr_array_index (value, i));

	gconf_client_set_list (client, gc_key, GCONF_VALUE_STRING, list, NULL);
	g_slist_free (list);
	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_bytearray_helper (GConfClient *client,
                               const char *path,
                               const char *key,
                               const char *setting,
                               GByteArray *value)
{
	char *gc_key;
	int i;
	GSList *list = NULL;

	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	if (!value)
		return TRUE;

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++)
		list = g_slist_append(list, GINT_TO_POINTER ((int) value->data[i]));

	gconf_client_set_list (client, gc_key, GCONF_VALUE_INT, list, NULL);

	g_slist_free (list);
	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_uint_array_helper (GConfClient *client,
                                const char *path,
                                const char *key,
                                const char *setting,
                                GArray *value)
{
	char *gc_key;
	int i;
	GSList *list = NULL;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	if (!value)
		return TRUE;

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++)
		list = g_slist_append (list, GUINT_TO_POINTER (g_array_index (value, guint, i)));

	gconf_client_set_list (client, gc_key, GCONF_VALUE_INT, list, NULL);

	g_slist_free (list);
	g_free (gc_key);
	return TRUE;
}

typedef struct {
	GConfClient *client;
	char *path;
} WritePropertiesInfo;

#if UNUSED
static void
write_properties_valuehash (gpointer key, gpointer val, gpointer user_data)
{
	GValue *value = (GValue *) val;
	WritePropertiesInfo *info = (WritePropertiesInfo *) user_data;
	char *esc_key;
	char *full_key;

	esc_key = gconf_escape_key ((char *) key, -1);
	full_key = g_strconcat (info->path, "/", esc_key, NULL);
	g_free (esc_key);

	if (G_VALUE_HOLDS_STRING (value))
		gconf_client_set_string (info->client, full_key, g_value_get_string (value), NULL);
	else if (G_VALUE_HOLDS_INT (value))
		gconf_client_set_int (info->client, full_key, g_value_get_int (value), NULL);
	else if (G_VALUE_HOLDS_BOOLEAN (value))
		gconf_client_set_bool (info->client, full_key, g_value_get_boolean (value), NULL);
	else
		g_warning ("Don't know how to write '%s' to gconf", G_VALUE_TYPE_NAME (value));

	g_free (full_key);
}

gboolean
bm_gconf_set_valuehash_helper (GConfClient *client,
                               const char *path,
                               const char *setting,
                               GHashTable *value)
{
	char *gc_key;
	WritePropertiesInfo info;

	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s", path, setting);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	info.client = client;
	info.path = gc_key;

	g_hash_table_foreach (value, write_properties_valuehash, &info);

	g_free (gc_key);
	return TRUE;
}
#endif

gboolean
bm_gconf_set_stringhash_helper (GConfClient *client,
                                const char *path,
                                const char *key,
                                const char *setting,
                                GHashTable *value)
{
	char *gc_key;
	GSList *existing, *iter;
	const char *key_prefix = NULL;
	GHashTableIter hash_iter;
	gpointer name, data;

	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s", path, setting);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	/* Delete GConf entries that are not in the hash table to be written */
	existing = gconf_client_all_entries (client, gc_key, NULL);
	for (iter = existing; iter; iter = g_slist_next (iter)) {
		GConfEntry *entry = (GConfEntry *) iter->data;
		const char *basename = strrchr (entry->key, '/');

		if (!basename) {
			g_warning ("GConf key '%s' had no basename", entry->key);
			continue;
		}
		basename++; /* Advance past the '/' */

		/* And if we have a key prefix, don't delete anything that does not
		 * have the prefix.
		 */
		if (key_prefix && (g_str_has_prefix (basename, key_prefix) == FALSE))
			continue;

		gconf_client_unset (client, entry->key, NULL);
		gconf_entry_unref (entry);
	}
	g_slist_free (existing);

	/* Now update entries and write new ones */
	g_hash_table_iter_init (&hash_iter, value);
	while (g_hash_table_iter_next (&hash_iter, &name, &data)) {
		char *esc_key, *full_key;

		esc_key = gconf_escape_key ((char *) name, -1);
		full_key = g_strdup_printf ("%s/%s%s",
		                            gc_key,
		                            key_prefix ? key_prefix : "",
		                            esc_key);
		gconf_client_set_string (client, full_key, (char *) data, NULL);
		g_free (esc_key);
		g_free (full_key);
	}

	g_free (gc_key);
	return TRUE;
}

gboolean
bm_gconf_set_ip4_helper (GConfClient *client,
                         const char *path,
                         const char *key,
                         const char *setting,
                         guint32 tuple_len,
                         GPtrArray *value)
{
	char *gc_key;
	int i;
	GSList *list = NULL;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (tuple_len > 0, FALSE);

	if (!value)
		return TRUE;

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++) {
		GArray *tuple = g_ptr_array_index (value, i);
		int j;

		if (tuple->len != tuple_len) {
			g_warning ("%s: invalid IPv4 address/route structure!", __func__);
			goto out;
		}

		for (j = 0; j < tuple_len; j++)
			list = g_slist_append (list, GUINT_TO_POINTER (g_array_index (tuple, guint32, j)));
	}

	gconf_client_set_list (client, gc_key, GCONF_VALUE_INT, list, NULL);
	success = TRUE;

out:
	g_slist_free (list);
	g_free (gc_key);
	return success;
}

gboolean
bm_gconf_set_ip6dns_array_helper (GConfClient *client,
								  const char *path,
								  const char *key,
								  const char *setting,
								  GPtrArray *value)
{
	char *gc_key;
	int i;
	GSList *list = NULL, *l;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	if (!value)
		return TRUE;

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++) {
		GByteArray *ba = g_ptr_array_index (value, i);
		char addr[INET6_ADDRSTRLEN];

		if (!inet_ntop (AF_INET6, ba->data, addr, sizeof (addr))) {
			g_warning ("%s: invalid IPv6 DNS server address!", __func__);
			goto out;
		}

		list = g_slist_append (list, g_strdup (addr));
	}

	gconf_client_set_list (client, gc_key, GCONF_VALUE_STRING, list, NULL);
	success = TRUE;

out:
	for (l = list; l; l = l->next)
		g_free (l->data);
	g_slist_free (list);
	g_free (gc_key);
	return success;
}

gboolean
bm_gconf_set_ip6addr_array_helper (GConfClient *client,
								   const char *path,
								   const char *key,
								   const char *setting,
								   GPtrArray *value)
{
	char *gc_key;
	int i;
	GSList *list = NULL, *l;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	if (!value)
		return TRUE;

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++) {
		GValueArray *elements = (GValueArray *) g_ptr_array_index (value, i);
		GValue *tmp;
		GByteArray *ba;
		guint prefix;
		char addr[INET6_ADDRSTRLEN];
		char gw[INET6_ADDRSTRLEN];
		gboolean have_gw = FALSE;
		char *gconf_str;

		if (elements->n_values < 1 || elements->n_values > 3) {
			g_warning ("%s: invalid IPv6 address!", __func__);
			goto out;
		}

		if (   (G_VALUE_TYPE (g_value_array_get_nth (elements, 0)) != DBUS_TYPE_G_UCHAR_ARRAY)
		    || (G_VALUE_TYPE (g_value_array_get_nth (elements, 1)) != G_TYPE_UINT)) {
			g_warning ("%s: invalid IPv6 address!", __func__);
			goto out;
		}

		if (   (elements->n_values == 3)
		    && (G_VALUE_TYPE (g_value_array_get_nth (elements, 2)) != DBUS_TYPE_G_UCHAR_ARRAY)) {
			g_warning ("%s: invalid IPv6 gateway!", __func__);
			goto out;
		}

		tmp = g_value_array_get_nth (elements, 0);
		ba = g_value_get_boxed (tmp);
		tmp = g_value_array_get_nth (elements, 1);
		prefix = g_value_get_uint (tmp);
		if (prefix > 128) {
			g_warning ("%s: invalid IPv6 address prefix %u", __func__, prefix);
			goto out;
		}

		if (!inet_ntop (AF_INET6, ba->data, addr, sizeof (addr))) {
			g_warning ("%s: invalid IPv6 address!", __func__);
			goto out;
		}

		if (elements->n_values == 3) {
			tmp = g_value_array_get_nth (elements, 2);
			ba = g_value_get_boxed (tmp);
			if (ba && !IN6_IS_ADDR_UNSPECIFIED (ba->data)) {
				if (!inet_ntop (AF_INET6, ba->data, gw, sizeof (gw))) {
					g_warning ("%s: invalid IPv6 gateway!", __func__);
					goto out;
				}
				have_gw = TRUE;
			}
		}

		gconf_str = g_strdup_printf ("%s/%u%s%s",
		                             addr,
		                             prefix,
		                             have_gw ? "," : "",
		                             have_gw ? gw : "");
		list = g_slist_append (list, gconf_str);
	}

	gconf_client_set_list (client, gc_key, GCONF_VALUE_STRING, list, NULL);
	success = TRUE;

out:
	for (l = list; l; l = l->next)
		g_free (l->data);
	g_slist_free (list);
	g_free (gc_key);
	return success;
}

gboolean
bm_gconf_set_ip6route_array_helper (GConfClient *client,
									const char *path,
									const char *key,
									const char *setting,
									GPtrArray *value)
{
	char *gc_key;
	int i;
	GSList *list = NULL, *l;
	gboolean success = FALSE;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	if (!value)
		return TRUE;

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	if (!gc_key) {
		g_warning ("Not enough memory to create gconf path");
		return FALSE;
	}

	for (i = 0; i < value->len; i++) {
		GValueArray *elements = (GValueArray *) g_ptr_array_index (value, i);
		GValue *tmp;
		GByteArray *ba;
		guint prefix, metric;
		char dest[INET6_ADDRSTRLEN], next_hop[INET6_ADDRSTRLEN];

		if (   (elements->n_values != 4)
		    || (G_VALUE_TYPE (g_value_array_get_nth (elements, 0)) != DBUS_TYPE_G_UCHAR_ARRAY)
		    || (G_VALUE_TYPE (g_value_array_get_nth (elements, 1)) != G_TYPE_UINT)
		    || (G_VALUE_TYPE (g_value_array_get_nth (elements, 2)) != DBUS_TYPE_G_UCHAR_ARRAY)
		    || (G_VALUE_TYPE (g_value_array_get_nth (elements, 3)) != G_TYPE_UINT))
 {
			g_warning ("%s: invalid IPv6 route!", __func__);
			goto out;
		}

		tmp = g_value_array_get_nth (elements, 0);
		ba = g_value_get_boxed (tmp);
		if (!inet_ntop (AF_INET6, ba->data, dest, sizeof (dest))) {
			g_warning ("%s: invalid IPv6 dest address!", __func__);
			goto out;
		}
		tmp = g_value_array_get_nth (elements, 1);
		prefix = g_value_get_uint (tmp);
		if (prefix > 128) {
			g_warning ("%s: invalid IPv6 dest prefix %u", __func__, prefix);
			goto out;
		}
		tmp = g_value_array_get_nth (elements, 2);
		ba = g_value_get_boxed (tmp);
		if (!inet_ntop (AF_INET6, ba->data, next_hop, sizeof (next_hop))) {
			g_warning ("%s: invalid IPv6 next_hop address!", __func__);
			goto out;
		}
		tmp = g_value_array_get_nth (elements, 3);
		metric = g_value_get_uint (tmp);

		list = g_slist_append (list,
							   g_strdup_printf ("%s/%u,%s,%u", dest, prefix,
												next_hop, metric));
	}

	gconf_client_set_list (client, gc_key, GCONF_VALUE_STRING, list, NULL);
	success = TRUE;

out:
	for (l = list; l; l = l->next)
		g_free (l->data);
	g_slist_free (list);
	g_free (gc_key);
	return success;
}


gboolean
bm_gconf_key_is_set (GConfClient *client,
                     const char *path,
                     const char *key,
                     const char *setting)
{
	char *gc_key;
	GConfValue *gc_value;
	gboolean exists = FALSE;

	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);

	gc_key = g_strdup_printf ("%s/%s/%s", path, setting, key);
	gc_value = gconf_client_get (client, gc_key, NULL);
	if (gc_value) {
		exists = TRUE;
		gconf_value_free (gc_value);
	}
	g_free (gc_key);
	return exists;
}

GSList *
bm_gconf_get_all_connections (GConfClient *client)
{
	GSList *connections;
	guint32 stamp = 0;
	GError *error = NULL;

	stamp = (guint32) gconf_client_get_int (client, APPLET_PREFS_STAMP, &error);
	if (error) {
		g_error_free (error);
		stamp = 0;
	}

	connections = gconf_client_all_dirs (client, GCONF_PATH_CONNECTIONS, NULL);

	/* Update the applet GConf stamp */
	if (stamp != APPLET_CURRENT_STAMP)
		gconf_client_set_int (client, APPLET_PREFS_STAMP, APPLET_CURRENT_STAMP, NULL);

	return connections;
}

typedef struct ReadFromGConfInfo {
	BMConnection *connection;
	GConfClient *client;
	const char *dir;
	guint32 dir_len;
} ReadFromGConfInfo;

#define FILE_TAG "file://"

static void
read_one_setting_value_from_gconf (BMSetting *setting,
                                   const char *key,
                                   const GValue *value,
                                   GParamFlags flags,
                                   gpointer user_data)
{
	ReadFromGConfInfo *info = (ReadFromGConfInfo *) user_data;
	const char *setting_name;
	GType type = G_VALUE_TYPE (value);

	/* The 'name' key is ignored when reading, because it's pulled from the
	 * gconf directory name instead.
	 */
	if (!strcmp (key, BM_SETTING_NAME))
		return;

	/* Don't read the BMSettingConnection object's 'read-only' property */
	if (   BM_IS_SETTING_CONNECTION (setting)
	    && !strcmp (key, BM_SETTING_CONNECTION_READ_ONLY))
		return;

	setting_name = bm_setting_get_name (setting);

	if (type == G_TYPE_STRING) {
		char *str_val = NULL;

		if (bm_gconf_get_string_helper (info->client, info->dir, key, setting_name, &str_val)) {
			g_object_set (setting, key, str_val, NULL);
			g_free (str_val);
		}
	} else if (type == G_TYPE_UINT) {
		int int_val = 0;

		if (bm_gconf_get_int_helper (info->client, info->dir, key, setting_name, &int_val)) {
			if (int_val < 0)
				g_warning ("Casting negative value (%i) to uint", int_val);

			g_object_set (setting, key, int_val, NULL);
		}
	} else if (type == G_TYPE_INT) {
		int int_val;

		if (bm_gconf_get_int_helper (info->client, info->dir, key, setting_name, &int_val))
			g_object_set (setting, key, int_val, NULL);
	} else if (type == G_TYPE_UINT64) {
		char *tmp_str = NULL;

		/* GConf doesn't do 64-bit values, so use strings instead */
		if (bm_gconf_get_string_helper (info->client, info->dir, key, setting_name, &tmp_str) && tmp_str) {
			guint64 uint_val = g_ascii_strtoull (tmp_str, NULL, 10);
			
			if (!(uint_val == G_MAXUINT64 && errno == ERANGE))
				g_object_set (setting, key, uint_val, NULL);
			g_free (tmp_str);
		}
	} else if (type == G_TYPE_BOOLEAN) {
		gboolean bool_val;

		if (bm_gconf_get_bool_helper (info->client, info->dir, key, setting_name, &bool_val))
			g_object_set (setting, key, bool_val, NULL);
	} else if (type == G_TYPE_CHAR) {
		int int_val = 0;

		if (bm_gconf_get_int_helper (info->client, info->dir, key, setting_name, &int_val)) {
			if (int_val < G_MININT8 || int_val > G_MAXINT8)
				g_warning ("Casting value (%i) to char", int_val);

			g_object_set (setting, key, int_val, NULL);
		}
	} else if (type == DBUS_TYPE_G_UCHAR_ARRAY) {
		GByteArray *ba_val = NULL;
		gboolean success = FALSE;

		success = bm_gconf_get_bytearray_helper (info->client, info->dir, key, setting_name, &ba_val);

		if (success) {
			g_object_set (setting, key, ba_val, NULL);
			g_byte_array_free (ba_val, TRUE);
		}
	} else if (type == DBUS_TYPE_G_LIST_OF_STRING) {
		GSList *sa_val = NULL;

		if (bm_gconf_get_stringlist_helper (info->client, info->dir, key, setting_name, &sa_val)) {
			g_object_set (setting, key, sa_val, NULL);
			g_slist_foreach (sa_val, (GFunc) g_free, NULL);
			g_slist_free (sa_val);
		}
#if UNUSED
	} else if (type == DBUS_TYPE_G_MAP_OF_VARIANT) {
		GHashTable *vh_val = NULL;

		if (bm_gconf_get_valuehash_helper (info->client, info->dir, setting_name, &vh_val)) {
			g_object_set (setting, key, vh_val, NULL);
			g_hash_table_destroy (vh_val);
		}
#endif
	} else if (type == DBUS_TYPE_G_MAP_OF_STRING) {
		// GHashTable *sh_val = NULL;

		/* FIXME if (bm_gconf_get_stringhash_helper (info->client, info->dir, key, setting_name, &sh_val)) {
			g_object_set (setting, key, sh_val, NULL);
			g_hash_table_destroy (sh_val);
			}*/
	} else if (type == DBUS_TYPE_G_ARRAY_OF_STRING) {
		GPtrArray *pa_val = NULL;

		if (bm_gconf_get_stringarray_helper (info->client, info->dir, key, setting_name, &pa_val)) {
			g_object_set (setting, key, pa_val, NULL);
			g_ptr_array_foreach (pa_val, (GFunc) g_free, NULL);
			g_ptr_array_free (pa_val, TRUE);
		}
	} else if (type == DBUS_TYPE_G_UINT_ARRAY) {
		GArray *a_val = NULL;

		if (bm_gconf_get_uint_array_helper (info->client, info->dir, key, setting_name, &a_val)) {
			g_object_set (setting, key, a_val, NULL);
			g_array_free (a_val, TRUE);
		}
	} else {
		g_warning ("Unhandled setting property type (read): '%s/%s' : '%s'",
				 setting_name, key, G_VALUE_TYPE_NAME (value));
	}
}

static void
read_one_setting (gpointer data, gpointer user_data)
{
	char *name;
	ReadFromGConfInfo *info = (ReadFromGConfInfo *) user_data;
	BMSetting *setting;

	/* Setting name is the gconf directory name. Since "data" here contains
	   full gconf path plus separator ('/'), omit that. */
	name =  (char *) data + info->dir_len + 1;
	setting = bm_connection_create_setting (name);
	if (setting) {
		bm_setting_enumerate_values (setting,
							    read_one_setting_value_from_gconf,
							    info);
		bm_connection_add_setting (info->connection, setting);
	}

	g_free (data);
}

BMConnection *
bm_gconf_read_connection (GConfClient *client,
                          const char *dir)
{
	ReadFromGConfInfo info;
	GSList *list;
	GError *err = NULL;

	list = gconf_client_all_dirs (client, dir, &err);
	if (err) {
		g_warning ("Error while reading connection: %s", err->message);
		g_error_free (err);
		return NULL;
	}

	if (!list) {
		g_warning ("Invalid connection (empty)");
		return NULL;
	}

	info.connection = bm_connection_new ();
	info.client = client;
	info.dir = dir;
	info.dir_len = strlen (dir);

	g_slist_foreach (list, read_one_setting, &info);
	g_slist_free (list);

	return info.connection;
}


void
bm_gconf_add_keyring_item (const char *connection_uuid,
                           const char *connection_name,
                           const char *setting_name,
                           const char *setting_key,
                           const char *secret)
{
	GnomeKeyringResult ret;
	char *display_name = NULL;
	GnomeKeyringAttributeList *attrs = NULL;
	guint32 id = 0;

	g_return_if_fail (connection_uuid != NULL);
	g_return_if_fail (setting_name != NULL);
	g_return_if_fail (setting_key != NULL);
	g_return_if_fail (secret != NULL);

	display_name = g_strdup_printf ("Network secret for %s/%s/%s",
	                                connection_name,
	                                setting_name,
	                                setting_key);

	attrs = gnome_keyring_attribute_list_new ();
	gnome_keyring_attribute_list_append_string (attrs,
	                                            KEYRING_UUID_TAG,
	                                            connection_uuid);
	gnome_keyring_attribute_list_append_string (attrs,
	                                            KEYRING_SN_TAG,
	                                            setting_name);
	gnome_keyring_attribute_list_append_string (attrs,
	                                            KEYRING_SK_TAG,
	                                            setting_key);

	pre_keyring_callback ();

	ret = gnome_keyring_item_create_sync (NULL,
	                                      GNOME_KEYRING_ITEM_GENERIC_SECRET,
	                                      display_name,
	                                      attrs,
	                                      secret,
	                                      TRUE,
	                                      &id);

	gnome_keyring_attribute_list_free (attrs);
	g_free (display_name);
}

static void
delete_done (GnomeKeyringResult result, gpointer user_data)
{
}

static void
keyring_delete_item (const char *connection_uuid,
                     const char *setting_name,
                     const char *setting_key)
{
	GList *found_list = NULL;
	GnomeKeyringResult ret;
	GList *iter;

	pre_keyring_callback ();

	ret = gnome_keyring_find_itemsv_sync (GNOME_KEYRING_ITEM_GENERIC_SECRET,
	                                      &found_list,
	                                      KEYRING_UUID_TAG,
	                                      GNOME_KEYRING_ATTRIBUTE_TYPE_STRING,
	                                      connection_uuid,
	                                      KEYRING_SN_TAG,
	                                      GNOME_KEYRING_ATTRIBUTE_TYPE_STRING,
	                                      setting_name,
	                                      KEYRING_SK_TAG,
	                                      GNOME_KEYRING_ATTRIBUTE_TYPE_STRING,
	                                      setting_key,
	                                      NULL);
	if (ret == GNOME_KEYRING_RESULT_OK) {
		for (iter = found_list; iter != NULL; iter = g_list_next (iter)) {
			GnomeKeyringFound *found = (GnomeKeyringFound *) iter->data;

			gnome_keyring_item_delete (found->keyring,
			                           found->item_id,
			                           delete_done,
			                           NULL,
			                           NULL);
		}
		gnome_keyring_found_list_free (found_list);
	}
}

typedef struct CopyOneSettingValueInfo {
	BMConnection *connection;
	GConfClient *client;
	const char *dir;
	const char *connection_uuid;
	const char *connection_name;
} CopyOneSettingValueInfo;

static void
write_one_secret_to_keyring (BMSetting *setting,
                             const char *key,
                             const GValue *value,
                             GParamFlags flags,
                             gpointer user_data)
{
	CopyOneSettingValueInfo *info = (CopyOneSettingValueInfo *) user_data;
	GType type = G_VALUE_TYPE (value);
	const char *secret;
	const char *setting_name;

	setting_name = bm_setting_get_name (setting);

	if (type != G_TYPE_STRING) {
		g_warning ("Unhandled setting secret type (write) '%s/%s' : '%s'", 
				 setting_name, key, g_type_name (type));
		return;
	}

	secret = g_value_get_string (value);
	if (secret && strlen (secret)) {
		bm_gconf_add_keyring_item (info->connection_uuid,
		                           info->connection_name,
		                           setting_name,
		                           key,
		                           secret);
	} else {
		/* We have to be careful about this, since if the connection we're
		 * given doesn't include secrets we'll blow anything in the keyring
		 * away here.  We rely on the caller knowing whether or not to do this.
		 */
		keyring_delete_item (info->connection_uuid,
		                     setting_name,
		                     key);
	}
}

static void
copy_one_setting_value_to_gconf (BMSetting *setting,
                                 const char *key,
                                 const GValue *value,
                                 GParamFlags flags,
                                 gpointer user_data)
{
	CopyOneSettingValueInfo *info = (CopyOneSettingValueInfo *) user_data;
	const char *setting_name;
	GType type = G_VALUE_TYPE (value);
	GParamSpec *pspec;

	/* Don't write the BMSettingConnection object's 'read-only' property */
	if (   BM_IS_SETTING_CONNECTION (setting)
	    && !strcmp (key, BM_SETTING_CONNECTION_READ_ONLY))
		return;

	setting_name = bm_setting_get_name (setting);

	/* If the value is the default value, remove the item from GConf */
	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (setting), key);
	if (pspec) {
		if (g_param_value_defaults (pspec, (GValue *) value)) {
			char *path;

			path = g_strdup_printf ("%s/%s/%s", info->dir, setting_name, key);
			if (path)
				gconf_client_unset (info->client, path, NULL);
			g_free (path);		
			return;
		}
	}

	if (type == G_TYPE_STRING) {
		bm_gconf_set_string_helper (info->client, info->dir, key, setting_name, g_value_get_string (value));
	} else if (type == G_TYPE_UINT) {
		bm_gconf_set_int_helper (info->client, info->dir,
							key, setting_name,
							g_value_get_uint (value));
	} else if (type == G_TYPE_INT) {
		bm_gconf_set_int_helper (info->client, info->dir,
							key, setting_name,
							g_value_get_int (value));
	} else if (type == G_TYPE_UINT64) {
		char *numstr;

		/* GConf doesn't do 64-bit values, so use strings instead */
		numstr = g_strdup_printf ("%" G_GUINT64_FORMAT, g_value_get_uint64 (value));
		bm_gconf_set_string_helper (info->client, info->dir,
							   key, setting_name, numstr);
		g_free (numstr);
	} else if (type == G_TYPE_BOOLEAN) {
		bm_gconf_set_bool_helper (info->client, info->dir,
							 key, setting_name,
							 g_value_get_boolean (value));
	} else if (type == G_TYPE_CHAR) {
		bm_gconf_set_int_helper (info->client, info->dir,
							key, setting_name,
							g_value_get_char (value));
	} else if (type == DBUS_TYPE_G_UCHAR_ARRAY) {
		GByteArray *ba_val = (GByteArray *) g_value_get_boxed (value);

		bm_gconf_set_bytearray_helper (info->client, info->dir, key, setting_name, ba_val);
	} else if (type == DBUS_TYPE_G_LIST_OF_STRING) {
		bm_gconf_set_stringlist_helper (info->client, info->dir,
								  key, setting_name,
								  (GSList *) g_value_get_boxed (value));
#if UNUSED
	} else if (type == DBUS_TYPE_G_MAP_OF_VARIANT) {
		bm_gconf_set_valuehash_helper (info->client, info->dir,
								 setting_name,
								 (GHashTable *) g_value_get_boxed (value));
#endif
	} else if (type == DBUS_TYPE_G_MAP_OF_STRING) {
		bm_gconf_set_stringhash_helper (info->client, info->dir, key,
		                                setting_name,
		                                (GHashTable *) g_value_get_boxed (value));
	} else if (type == DBUS_TYPE_G_ARRAY_OF_STRING) {
		bm_gconf_set_stringarray_helper (info->client, info->dir, key,
		                                setting_name,
		                                (GPtrArray *) g_value_get_boxed (value));
	} else if (type == DBUS_TYPE_G_UINT_ARRAY) {
		bm_gconf_set_uint_array_helper (info->client, info->dir,
								  key, setting_name,
								  (GArray *) g_value_get_boxed (value));
	} else
		g_warning ("Unhandled setting property type (write) '%s/%s' : '%s'", 
				 setting_name, key, g_type_name (type));
}

static void
remove_leftovers (CopyOneSettingValueInfo *info)
{
	GSList *dirs;
	GSList *iter;
	size_t prefix_len;

	prefix_len = strlen (info->dir) + 1;

	dirs = gconf_client_all_dirs (info->client, info->dir, NULL);
	for (iter = dirs; iter; iter = iter->next) {
		char *key = (char *) iter->data;
		BMSetting *setting;

		setting = bm_connection_get_setting_by_name (info->connection, key + prefix_len);
		if (!setting)
			gconf_client_recursive_unset (info->client, key, 0, NULL);

		g_free (key);
	}

	g_slist_free (dirs);
}

void
bm_gconf_write_connection (BMConnection *connection,
                           GConfClient *client,
                           const char *dir,
                           gboolean ignore_secrets)
{
	BMSettingConnection *s_con;
	CopyOneSettingValueInfo info;
	gboolean ignore;

	g_return_if_fail (BM_IS_CONNECTION (connection));
	g_return_if_fail (client != NULL);
	g_return_if_fail (dir != NULL);

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
	if (!s_con)
		return;

	info.connection = connection;
	info.client = client;
	info.dir = dir;
	info.connection_uuid = bm_setting_connection_get_uuid (s_con);
	info.connection_name = bm_setting_connection_get_id (s_con);

	bm_connection_for_each_setting_value (connection,
	                                      copy_one_setting_value_to_gconf,
	                                      &info);
	remove_leftovers (&info);

	/* Write/clear secrets; the caller must know whether or not to do this
	 * based on how the connection was updated; if only something like the
	 * BSSID or timestamp is getting updated, then you want to ignore
	 * secrets, since the secrets could not possibly have changed.  On the
	 * other hand, if the user cleared out a secret in the connection editor,
	 * you want to ensure that secret gets deleted from the keyring.
	 */
	if (ignore_secrets == FALSE) {
		bm_connection_for_each_setting_value (connection,
		                                      write_one_secret_to_keyring,
		                                      &info);
	}

	/* Update ignore CA cert status */
	ignore = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (connection), IGNORE_CA_CERT_TAG));
	bm_gconf_set_ignore_ca_cert (info.connection_uuid, FALSE, ignore);

	ignore = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (connection), IGNORE_PHASE2_CA_CERT_TAG));
	bm_gconf_set_ignore_ca_cert (info.connection_uuid, TRUE, ignore);
}

static char *
get_ignore_path (const char *uuid, gboolean phase2)
{
	return g_strdup_printf (APPLET_PREFS_PATH "/%s/%s",
	                        phase2 ? "ignore-phase2-ca-cert" : "ignore-ca-cert",
	                        uuid);
}

gboolean
bm_gconf_get_ignore_ca_cert (const char *uuid, gboolean phase2)
{
	GConfClient *client;
	char *key = NULL;
	gboolean ignore = FALSE;

	g_return_val_if_fail (uuid != NULL, FALSE);

	client = gconf_client_get_default ();

	key = get_ignore_path (uuid, phase2);
	ignore = gconf_client_get_bool (client, key, NULL);
	g_free (key);

	g_object_unref (client);
	return ignore;
}

void
bm_gconf_set_ignore_ca_cert (const char *uuid, gboolean phase2, gboolean ignore)
{
	GConfClient *client;
	char *key = NULL;

	g_return_if_fail (uuid != NULL);

	client = gconf_client_get_default ();

	key = get_ignore_path (uuid, phase2);
	if (ignore)
		gconf_client_set_bool (client, key, ignore, NULL);
	else
		gconf_client_unset (client, key, NULL);
	g_free (key);

	g_object_unref (client);
}

static char *
get_always_ask_path (const char *uuid)
{
	return g_strdup_printf (APPLET_PREFS_PATH "/8021x-password-always-ask/%s", uuid);
}

gboolean
bm_gconf_get_8021x_password_always_ask (const char *uuid)
{
	GConfClient *client;
	char *key = NULL;
	gboolean ask = FALSE;

	g_return_val_if_fail (uuid != NULL, FALSE);

	client = gconf_client_get_default ();

	key = get_always_ask_path (uuid);
	ask = gconf_client_get_bool (client, key, NULL);
	g_free (key);

	g_object_unref (client);
	return ask;
}

void
bm_gconf_set_8021x_password_always_ask (const char *uuid, gboolean ask)
{
	GConfClient *client;
	char *key = NULL;

	g_return_if_fail (uuid != NULL);

	client = gconf_client_get_default ();

	key = get_always_ask_path (uuid);
	if (ask)
		gconf_client_set_bool (client, key, TRUE, NULL);
	else
		gconf_client_unset (client, key, NULL);
	g_free (key);

	g_object_unref (client);
}

