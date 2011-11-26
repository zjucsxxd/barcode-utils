/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/* BarcodeManager -- Network link manager
 *
 * Jakob Flierl <jakob.flierl@gmail.com>
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
 * (C) Copyright 2011 Jakob Flierl
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <uuid/uuid.h>

#include "bm-utils.h"
#include "bm-utils-private.h"
#include "BarcodeManager.h"
#include "bm-dbus-glib-types.h"

/**
 * SECTION:bm-utils
 * @short_description: Utility functions
 * @include: bm-utils.h
 *
 * A collection of utility functions for working SSIDs, IP addresses, WiFi
 * access points and devices, among other things.
 */

struct EncodingTriplet
{
	const char *encoding1;
	const char *encoding2;
	const char *encoding3;
};

struct IsoLangToEncodings
{
	const char *	lang;
	struct EncodingTriplet encodings;
};

/* 5-letter language codes */
static const struct IsoLangToEncodings isoLangEntries5[] =
{
	/* Simplified Chinese */
	{ "zh_cn",	{"euc-cn",	"gb2312",			"gb18030"} },	/* PRC */
	{ "zh_sg",	{"euc-cn",	"gb2312",			"gb18030"} },	/* Singapore */

	/* Traditional Chinese */
	{ "zh_tw",	{"big5",		"euc-tw",			NULL} },		/* Taiwan */
	{ "zh_hk",	{"big5",		"euc-tw",			"big5-hkcs"} },/* Hong Kong */
	{ "zh_mo",	{"big5",		"euc-tw",			NULL} },		/* Macau */

	/* Table end */
	{ NULL, {NULL, NULL, NULL} }
};

/* 2-letter language codes; we don't care about the other 3 in this table */
static const struct IsoLangToEncodings isoLangEntries2[] =
{
	/* Japanese */
	{ "ja",		{"euc-jp",	"shift_jis",		"iso-2022-jp"} },

	/* Korean */
	{ "ko",		{"euc-kr",	"iso-2022-kr",		"johab"} },

	/* Thai */
	{ "th",		{"iso-8859-11","windows-874",		NULL} },

	/* Central European */
	{ "hu",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Hungarian */
	{ "cs",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Czech */
	{ "hr",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Croatian */
	{ "pl",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Polish */
	{ "ro",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Romanian */
	{ "sk",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Slovakian */
	{ "sl",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Slovenian */
	{ "sh",		{"iso-8859-2",	"windows-1250",	NULL} },	/* Serbo-Croatian */

	/* Cyrillic */
	{ "ru",		{"koi8-r",	"windows-1251",	"iso-8859-5"} },	/* Russian */
	{ "be",		{"koi8-r",	"windows-1251",	"iso-8859-5"} },	/* Belorussian */
	{ "bg",		{"windows-1251","koi8-r",		"iso-8859-5"} },	/* Bulgarian */
	{ "mk",		{"koi8-r",	"windows-1251",	"iso-8859-5"} },	/* Macedonian */
	{ "sr",		{"koi8-r",	"windows-1251",	"iso-8859-5"} },	/* Serbian */
	{ "uk",		{"koi8-u",	"koi8-r",			"windows-1251"} },	/* Ukranian */

	/* Arabic */
	{ "ar",		{"iso-8859-6",	"windows-1256",	NULL} },

	/* Baltic */
	{ "et",		{"iso-8859-4",	"windows-1257",	NULL} },	/* Estonian */
	{ "lt",		{"iso-8859-4",	"windows-1257",	NULL} },	/* Lithuanian */
	{ "lv",		{"iso-8859-4",	"windows-1257",	NULL} },	/* Latvian */

	/* Greek */
	{ "el",		{"iso-8859-7",	"windows-1253",	NULL} },

	/* Hebrew */
	{ "he",		{"iso-8859-8",	"windows-1255",	NULL} },
	{ "iw",		{"iso-8859-8",	"windows-1255",	NULL} },

	/* Turkish */
	{ "tr",		{"iso-8859-9",	"windows-1254",	NULL} },

	/* Table end */
	{ NULL, {NULL, NULL, NULL} }
};


static GHashTable * langToEncodings5 = NULL;
static GHashTable * langToEncodings2 = NULL;

static void
init_lang_to_encodings_hash (void)
{
	struct IsoLangToEncodings *enc;

	if (G_UNLIKELY (langToEncodings5 == NULL)) {
		/* Five-letter codes */
		enc = (struct IsoLangToEncodings *) &isoLangEntries5[0];
		langToEncodings5 = g_hash_table_new (g_str_hash, g_str_equal);
		while (enc->lang) {
			g_hash_table_insert (langToEncodings5, (gpointer) enc->lang,
					(gpointer) &enc->encodings);
			enc++;
		}
	}

	if (G_UNLIKELY (langToEncodings2 == NULL)) {
		/* Two-letter codes */
		enc = (struct IsoLangToEncodings *) &isoLangEntries2[0];
		langToEncodings2 = g_hash_table_new (g_str_hash, g_str_equal);
		while (enc->lang) {
			g_hash_table_insert (langToEncodings2, (gpointer) enc->lang,
					(gpointer) &enc->encodings);
			enc++;
		}
	}
}


static gboolean
get_encodings_for_lang (const char *lang,
                        char **encoding1,
                        char **encoding2,
                        char **encoding3)
{
	struct EncodingTriplet *	encodings;
	gboolean				success = FALSE;
	char *				tmp_lang;

	g_return_val_if_fail (lang != NULL, FALSE);
	g_return_val_if_fail (encoding1 != NULL, FALSE);
	g_return_val_if_fail (encoding2 != NULL, FALSE);
	g_return_val_if_fail (encoding3 != NULL, FALSE);

	*encoding1 = "iso-8859-1";
	*encoding2 = "windows-1251";
	*encoding3 = NULL;

	init_lang_to_encodings_hash ();

	tmp_lang = g_strdup (lang);
	if ((encodings = g_hash_table_lookup (langToEncodings5, tmp_lang)))
	{
		*encoding1 = (char *) encodings->encoding1;
		*encoding2 = (char *) encodings->encoding2;
		*encoding3 = (char *) encodings->encoding3;
		success = TRUE;
	}

	/* Truncate tmp_lang to length of 2 */
	if (strlen (tmp_lang) > 2)
		tmp_lang[2] = '\0';
	if (!success && (encodings = g_hash_table_lookup (langToEncodings2, tmp_lang)))
	{
		*encoding1 = (char *) encodings->encoding1;
		*encoding2 = (char *) encodings->encoding2;
		*encoding3 = (char *) encodings->encoding3;
		success = TRUE;
	}

	g_free (tmp_lang);
	return success;
}

static char *
string_to_utf8 (const char *str, gsize len)
{
	char *converted = NULL;
	char *lang, *e1 = NULL, *e2 = NULL, *e3 = NULL;

	g_return_val_if_fail (str != NULL, NULL);

	if (g_utf8_validate (str, len, NULL))
		return g_strdup (str);

	/* LANG may be a good encoding hint */
	g_get_charset ((const char **)(&e1));
	if ((lang = getenv ("LANG"))) {
		char * dot;

		lang = g_ascii_strdown (lang, -1);
		if ((dot = strchr (lang, '.')))
			*dot = '\0';

		get_encodings_for_lang (lang, &e1, &e2, &e3);
		g_free (lang);
	}

	converted = g_convert (str, len, "UTF-8", e1, NULL, NULL, NULL);
	if (!converted && e2)
		converted = g_convert (str, len, "UTF-8", e2, NULL, NULL, NULL);

	if (!converted && e3)
		converted = g_convert (str, len, "UTF-8", e3, NULL, NULL, NULL);

	if (!converted) {
		converted = g_convert_with_fallback (str, len, "UTF-8", e1,
	                "?", NULL, NULL, NULL);
	}

	return converted;
}

/* init, deinit for libbm_util */

static gboolean initialized = FALSE;

/**
 * bm_utils_init:
 * @error: location to store error, or %NULL
 *
 * Initializes libbm-util; should be called when starting and program that
 * uses libbm-util.  Sets up an atexit() handler to ensure de-initialization
 * is performed, but calling bm_utils_deinit() to explicitly deinitialize
 * libbm-util can also be done.  This function can be called more than once.
 * 
 * Returns: TRUE if the initialization was successful, FALSE on failure.
 **/
gboolean
bm_utils_init (GError **error)
{
	if (!initialized) {
		_bm_utils_register_value_transformations ();

		atexit (bm_utils_deinit);
		initialized = TRUE;
	}
	return TRUE;
}

/**
 * bm_utils_deinit:
 *
 * Frees all resources used internally by libbm-util.  This function is called
 * from an atexit() handler, set up by bm_utils_init(), but is safe to be called
 * more than once.  Subsequent calls have no effect until bm_utils_init() is
 * called again.
 **/
void
bm_utils_deinit (void)
{
	if (initialized) {
		initialized = FALSE;
	}
}

static void
value_destroy (gpointer data)
{
	GValue *value = (GValue *) data;

	g_value_unset (value);
	g_slice_free (GValue, value);
}

static void
value_dup (gpointer key, gpointer val, gpointer user_data)
{
	GHashTable *table = (GHashTable *) user_data;
	GValue *value = (GValue *) val;
	GValue *dup_value;

	dup_value = g_slice_new0 (GValue);
	g_value_init (dup_value, G_VALUE_TYPE (val));
	g_value_copy (value, dup_value);

	g_hash_table_insert (table, g_strdup ((char *) key), dup_value);
}

/**
 * bm_utils_gvalue_hash_dup:
 * @hash: a #GHashTable mapping string:GValue
 *
 * Utility function to duplicate a hash table of GValues.
 *
 * Returns: a newly allocated duplicated #GHashTable, caller must free the
 * returned hash with g_hash_table_unref() or g_hash_table_destroy()
 **/
GHashTable *
bm_utils_gvalue_hash_dup (GHashTable *hash)
{
	GHashTable *table;

	g_return_val_if_fail (hash != NULL, NULL);

	table = g_hash_table_new_full (g_str_hash, g_str_equal,
						    (GDestroyNotify) g_free,
						    value_destroy);

	g_hash_table_foreach (hash, value_dup, table);

	return table;
}

/**
 * bm_utils_slist_free:
 * @list: a #GSList
 * @elem_destroy_fn: user function called for each element in @list
 *
 * Utility function to free a #GSList.
 **/
void
bm_utils_slist_free (GSList *list, GDestroyNotify elem_destroy_fn)
{
	if (!list)
		return;

	if (elem_destroy_fn)
		g_slist_foreach (list, (GFunc) elem_destroy_fn, NULL);

	g_slist_free (list);
}

gboolean
_bm_utils_string_in_list (const char *str, const char **valid_strings)
{
	int i;

	for (i = 0; valid_strings[i]; i++)
		if (strcmp (str, valid_strings[i]) == 0)
			break;

	return valid_strings[i] != NULL;
}

gboolean
_bm_utils_string_slist_validate (GSList *list, const char **valid_values)
{
	GSList *iter;

	for (iter = list; iter; iter = iter->next) {
		if (!_bm_utils_string_in_list ((char *) iter->data, valid_values))
			return FALSE;
	}

	return TRUE;
}

static void
bm_utils_convert_strv_to_slist (const GValue *src_value, GValue *dest_value)
{
	char **str;
	GSList *list = NULL;
	guint i = 0;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), G_TYPE_STRV));

	str = (char **) g_value_get_boxed (src_value);

	while (str && str[i])
		list = g_slist_prepend (list, g_strdup (str[i++]));

	g_value_take_boxed (dest_value, g_slist_reverse (list));
}

static void
bm_utils_convert_strv_to_ptrarray (const GValue *src_value, GValue *dest_value)
{
	char **str;
	GPtrArray *array = NULL;
	guint i = 0;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), G_TYPE_STRV));

	str = (char **) g_value_get_boxed (src_value);

	array = g_ptr_array_sized_new (3);
	while (str && str[i])
		g_ptr_array_add (array, g_strdup (str[i++]));

	g_value_take_boxed (dest_value, array);
}

static void
bm_utils_convert_strv_to_string (const GValue *src_value, GValue *dest_value)
{
	GSList *strings;
	GString *printable;
	GSList *iter;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_LIST_OF_STRING));

	strings = (GSList *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	for (iter = strings; iter; iter = g_slist_next (iter)) {
		if (iter != strings)
			g_string_append (printable, ", '");
		else
			g_string_append_c (printable, '\'');
		g_string_append (printable, iter->data);
		g_string_append_c (printable, '\'');
	}
	g_string_append_c (printable, ']');

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

static void
bm_utils_convert_string_array_to_string (const GValue *src_value, GValue *dest_value)
{
	GPtrArray *strings;
	GString *printable;
	int i;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_ARRAY_OF_STRING));

	strings = (GPtrArray *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	for (i = 0; strings && i < strings->len; i++) {
		if (i > 0)
			g_string_append (printable, ", '");
		else
			g_string_append_c (printable, '\'');
		g_string_append (printable, g_ptr_array_index (strings, i));
		g_string_append_c (printable, '\'');
	}
	g_string_append_c (printable, ']');

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

static void
bm_utils_convert_uint_array_to_string (const GValue *src_value, GValue *dest_value)
{
	GArray *array;
	GString *printable;
	guint i = 0;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_UINT_ARRAY));

	array = (GArray *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	while (array && (i < array->len)) {
		char buf[INET_ADDRSTRLEN + 1];
		struct in_addr addr;

		if (i > 0)
			g_string_append (printable, ", ");

		memset (buf, 0, sizeof (buf));
		addr.s_addr = g_array_index (array, guint32, i++);
		if (!inet_ntop (AF_INET, &addr, buf, INET_ADDRSTRLEN))
			bm_warning ("%s: error converting IP4 address 0x%X",
			            __func__, ntohl (addr.s_addr));
		g_string_append_printf (printable, "%u (%s)", addr.s_addr, buf);
	}
	g_string_append_c (printable, ']');

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

static void
bm_utils_convert_ip4_addr_route_struct_array_to_string (const GValue *src_value, GValue *dest_value)
{
	GPtrArray *ptr_array;
	GString *printable;
	guint i = 0;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_ARRAY_OF_ARRAY_OF_UINT));

	ptr_array = (GPtrArray *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	while (ptr_array && (i < ptr_array->len)) {
		GArray *array;
		char buf[INET_ADDRSTRLEN + 1];
		struct in_addr addr;
		gboolean is_addr; /* array contains address x route */

		if (i > 0)
			g_string_append (printable, ", ");

		g_string_append (printable, "{ ");
		array = (GArray *) g_ptr_array_index (ptr_array, i++);
		if (array->len < 2) {
			g_string_append (printable, "invalid");
			continue;
		}
		is_addr = (array->len < 4);

		memset (buf, 0, sizeof (buf));
		addr.s_addr = g_array_index (array, guint32, 0);
		if (!inet_ntop (AF_INET, &addr, buf, INET_ADDRSTRLEN))
			bm_warning ("%s: error converting IP4 address 0x%X",
			            __func__, ntohl (addr.s_addr));
		if (is_addr)
			g_string_append_printf (printable, "ip = %s", buf);
		else
			g_string_append_printf (printable, "dst = %s", buf);
		g_string_append (printable, ", ");

		memset (buf, 0, sizeof (buf));
		g_string_append_printf (printable, "px = %u",
		                        g_array_index (array, guint32, 1));

		if (array->len > 2) {
			g_string_append (printable, ", ");

			memset (buf, 0, sizeof (buf));
			addr.s_addr = g_array_index (array, guint32, 2);
			if (!inet_ntop (AF_INET, &addr, buf, INET_ADDRSTRLEN))
				bm_warning ("%s: error converting IP4 address 0x%X",
				            __func__, ntohl (addr.s_addr));
			if (is_addr)
				g_string_append_printf (printable, "gw = %s", buf);
			else
				g_string_append_printf (printable, "nh = %s", buf);
		}

		if (array->len > 3) {
			g_string_append (printable, ", ");

			memset (buf, 0, sizeof (buf));
			g_string_append_printf (printable, "mt = %u",
			                        g_array_index (array, guint32, 3));
		}

		g_string_append (printable, " }");
	}
	g_string_append_c (printable, ']');

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

static void
convert_one_gvalue_hash_entry (gpointer key, gpointer value, gpointer user_data)
{
	GString *printable = (GString *) user_data;
	char *value_as_string;

	value_as_string = g_strdup_value_contents ((GValue *) value);
	g_string_append_printf (printable, " { '%s': %s },", (const char *) key, value_as_string);
	g_free (value_as_string);
}

static void
bm_utils_convert_gvalue_hash_to_string (const GValue *src_value, GValue *dest_value)
{
	GHashTable *hash;
	GString *printable;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_MAP_OF_VARIANT));

	hash = (GHashTable *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	g_hash_table_foreach (hash, convert_one_gvalue_hash_entry, printable);
	g_string_append (printable, " ]");

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

static void
convert_one_string_hash_entry (gpointer key, gpointer value, gpointer user_data)
{
	GString *printable = (GString *) user_data;

	g_string_append_printf (printable, " { '%s': %s },", (const char *) key, (const char *) value);
}

static void
bm_utils_convert_string_hash_to_string (const GValue *src_value, GValue *dest_value)
{
	GHashTable *hash;
	GString *printable;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_MAP_OF_STRING));

	hash = (GHashTable *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	if (hash)
		g_hash_table_foreach (hash, convert_one_string_hash_entry, printable);
	g_string_append (printable, " ]");

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

static void
bm_utils_convert_byte_array_to_string (const GValue *src_value, GValue *dest_value)
{
	GArray *array;
	GString *printable;
	guint i = 0;

	g_return_if_fail (g_type_is_a (G_VALUE_TYPE (src_value), DBUS_TYPE_G_UCHAR_ARRAY));

	array = (GArray *) g_value_get_boxed (src_value);

	printable = g_string_new ("[");
	if (array) {
		while (i < MIN (array->len, 35)) {
			if (i > 0)
				g_string_append_c (printable, ' ');
			g_string_append_printf (printable, "0x%02X",
			                        g_array_index (array, unsigned char, i++));
		}
		if (i < array->len)
			g_string_append (printable, " ... ");
	}
	g_string_append_c (printable, ']');

	g_value_take_string (dest_value, printable->str);
	g_string_free (printable, FALSE);
}

void
_bm_utils_register_value_transformations (void)
{
	static gboolean registered = FALSE;

	if (G_UNLIKELY (!registered)) {
		g_value_register_transform_func (G_TYPE_STRV, 
		                                 DBUS_TYPE_G_LIST_OF_STRING,
		                                 bm_utils_convert_strv_to_slist);
		g_value_register_transform_func (G_TYPE_STRV,
		                                 DBUS_TYPE_G_ARRAY_OF_STRING,
		                                 bm_utils_convert_strv_to_ptrarray);
		g_value_register_transform_func (DBUS_TYPE_G_LIST_OF_STRING,
		                                 G_TYPE_STRING, 
		                                 bm_utils_convert_strv_to_string);
		g_value_register_transform_func (DBUS_TYPE_G_ARRAY_OF_STRING,
		                                 G_TYPE_STRING,
		                                 bm_utils_convert_string_array_to_string);
		g_value_register_transform_func (DBUS_TYPE_G_UINT_ARRAY,
		                                 G_TYPE_STRING, 
		                                 bm_utils_convert_uint_array_to_string);
		g_value_register_transform_func (DBUS_TYPE_G_ARRAY_OF_ARRAY_OF_UINT,
		                                 G_TYPE_STRING, 
		                                 bm_utils_convert_ip4_addr_route_struct_array_to_string);
		g_value_register_transform_func (DBUS_TYPE_G_MAP_OF_VARIANT,
		                                 G_TYPE_STRING, 
		                                 bm_utils_convert_gvalue_hash_to_string);
		g_value_register_transform_func (DBUS_TYPE_G_MAP_OF_STRING,
		                                 G_TYPE_STRING, 
		                                 bm_utils_convert_string_hash_to_string);
		g_value_register_transform_func (DBUS_TYPE_G_UCHAR_ARRAY,
		                                 G_TYPE_STRING,
		                                 bm_utils_convert_byte_array_to_string);
		registered = TRUE;
	}
}

/*
 * utils_bin2hexstr
 *
 * Convert a byte-array into a hexadecimal string.
 *
 * Code originally by Alex Larsson <alexl@redhat.com> and
 *  copyright Red Hat, Inc. under terms of the LGPL.
 *
 */
static char *
utils_bin2hexstr (const char *bytes, int len, int final_len)
{
	static char hex_digits[] = "0123456789abcdef";
	char *result;
	int i;
	gsize buflen = (len * 2) + 1;

	g_return_val_if_fail (bytes != NULL, NULL);
	g_return_val_if_fail (len > 0, NULL);
	g_return_val_if_fail (len < 4096, NULL);   /* Arbitrary limit */
	if (final_len > -1)
		g_return_val_if_fail (final_len < buflen, NULL);

	result = g_malloc0 (buflen);
	for (i = 0; i < len; i++)
	{
		result[2*i] = hex_digits[(bytes[i] >> 4) & 0xf];
		result[2*i+1] = hex_digits[bytes[i] & 0xf];
	}
	/* Cut converted key off at the correct length for this cipher type */
	if (final_len > -1)
		result[final_len] = '\0';
	else
		result[buflen - 1] = '\0';

	return result;
}

