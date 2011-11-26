/* bmcli - command-line tool to control BarcodeManager
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

#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib-bindings.h>

#include "utils.h"

int
matches (const char *cmd, const char *pattern)
{
	int len = strlen (cmd);
	if (len > strlen (pattern))
		return -1;
	return memcmp (pattern, cmd, len);
}

int
next_arg (int *argc, char ***argv)
{
	if (*argc <= 1) {
		return -1;
	}
	else {
		(*argc)--;
		(*argv)++;
	}
	return 0;
}

/*
 * Find out how many columns an UTF-8 string occupies on the screen
 */
int
bmc_string_screen_width (const char *start, const char *end)
{
	int width = 0;

	if (end == NULL)
		end = start + strlen (start);

	while (start < end) {
		width += g_unichar_iswide (g_utf8_get_char (start)) ? 2 : g_unichar_iszerowidth (g_utf8_get_char (start)) ? 0 : 1;
		start = g_utf8_next_char (start);
	}
	return width;
}

/*
 * Parse comma separated fields in 'fields_str' according to 'fields_array'.
 * IN:  'field_str':    comma-separated fields names
 *      'fields_array': array of allowed fields
 * RETURN: GArray with indices representing fields in 'fields_array'.
 *         Caller is responsible to free it.
 */
GArray *
parse_output_fields (const char *fields_str, const BmcOutputField fields_array[], GError **error)
{
	char **fields, **iter;
	GArray *array;
	int i;

	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	array = g_array_new (FALSE, FALSE, sizeof (int));

	/* Split supplied fields string */
	fields = g_strsplit_set (fields_str, ",", -1);
	for (iter = fields; iter && *iter; iter++) {
		for (i = 0; fields_array[i].name; i++) {
			if (strcasecmp (*iter, fields_array[i].name) == 0) {
				g_array_append_val (array, i);
				break;
			}
		}
		if (fields_array[i].name == NULL) {
			if (!strcasecmp (*iter, "all") || !strcasecmp (*iter, "common"))
				g_set_error (error, 0, 0, _("field '%s' has to be alone"), *iter);

			else
				g_set_error (error, 0, 1, _("invalid field '%s'"), *iter);
			g_array_free (array, TRUE);
			array = NULL;
			goto done;
		}
	}
done:
	if (fields)
		g_strfreev (fields);
	return array;
}

gboolean
bmc_terse_option_check (BMCPrintOutput print_output, const char *fields, GError **error)
{
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (print_output == BMC_PRINT_TERSE) {
		if (!fields) {
			g_set_error (error, 0, 0, _("Option '--terse' requires specifying '--fields'"));
			return FALSE;
		} else if (   !strcasecmp (fields, "all")
		           || !strcasecmp (fields, "common")) {
			g_set_error (error, 0, 0, _("Option '--terse' requires specific '--fields' option values , not '%s'"), fields);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * Print both headers or values of 'field_values' array.
 * Entries to print and their order are specified via indices
 * in 'fields.indices' array.
 * 'fields.flags' specify various aspects influencing the output.
 */
void
print_fields (const BmcPrintFields fields, const BmcOutputField field_values[])
{
	GString *str;
	int width1, width2;
	int table_width = 0;
	char *line = NULL;
	char *indent_str;
	const char *value;
	const char *not_set_str = _("not set");
	int i, idx;
	gboolean multiline = fields.flags & BMC_PF_FLAG_MULTILINE;
	gboolean terse = fields.flags & BMC_PF_FLAG_TERSE;
	gboolean pretty = fields.flags & BMC_PF_FLAG_PRETTY;
	gboolean main_header_add = fields.flags & BMC_PF_FLAG_MAIN_HEADER_ADD;
	gboolean main_header_only = fields.flags & BMC_PF_FLAG_MAIN_HEADER_ONLY;
	gboolean field_names = fields.flags & BMC_PF_FLAG_FIELD_NAMES;
	gboolean escape = fields.flags & BMC_PF_FLAG_ESCAPE;
	gboolean section_prefix = fields.flags & BMC_PF_FLAG_SECTION_PREFIX;
	gboolean main_header = main_header_add || main_header_only;

	/* No headers are printed in terse mode:
	 * - neither main header nor field (column) names
	 */
	if ((main_header_only || field_names) && terse)
		return;

	if (multiline) {
	/* --- Multiline mode --- */
		enum { ML_HEADER_WIDTH = 79 };
		if (main_header && pretty) {
			/* Print the main header */
			int header_width = bmc_string_screen_width (fields.header_name, NULL) + 4;
			table_width = header_width < ML_HEADER_WIDTH ? ML_HEADER_WIDTH : header_width;

			line = g_strnfill (ML_HEADER_WIDTH, '=');
			width1 = strlen (fields.header_name);
			width2 = bmc_string_screen_width (fields.header_name, NULL);
			printf ("%s\n", line);
			printf ("%*s\n", (table_width + width2)/2 + width1 - width2, fields.header_name);
			printf ("%s\n", line);
			g_free (line);
		}

		/* Print values */
		if (!main_header_only && !field_names) {
			for (i = 0; i < fields.indices->len; i++) {
				char *tmp;
				idx = g_array_index (fields.indices, int, i);
				if (section_prefix && idx == 0)  /* The first field is section prefix */
					continue;
				tmp = g_strdup_printf ("%s%s%s:", section_prefix ? field_values[0].value : "",
				                                  section_prefix ? "." : "",
				                                  _(field_values[idx].name_l10n));
				printf ("%-*s%s\n", terse ? 0 : 32, tmp, field_values[idx].value ? field_values[idx].value : not_set_str);
				g_free (tmp);
			}
			if (pretty) {
				line = g_strnfill (ML_HEADER_WIDTH, '-');
				printf ("%s\n", line);
				g_free (line);
			}
		}
		return;
	}

	/* --- Tabular mode: each line = one object --- */
	str = g_string_new (NULL);

	for (i = 0; i < fields.indices->len; i++) {
		idx = g_array_index (fields.indices, int, i);
		if (field_names)
			value = _(field_values[idx].name_l10n);
		else
			value = field_values[idx].value ? field_values[idx].value : not_set_str;
		if (terse) {
			if (escape) {
				const char *p = value;
				while (*p) {
					if (*p == ':' || *p == '\\')
						g_string_append_c (str, '\\');  /* Escaping by '\' */
					g_string_append_c (str, *p);
					p++;
				}
			}
			else 
				g_string_append_printf (str, "%s", value);
			g_string_append_c (str, ':');  /* Column separator */
		} else {
			width1 = strlen (value);
			width2 = bmc_string_screen_width (value, NULL);  /* Width of the string (in screen colums) */
			if (strlen (value) == 0)
				value = "--";
			g_string_append_printf (str, "%-*s", field_values[idx].width + width1 - width2, value);
			g_string_append_c (str, ' ');  /* Column separator */
			table_width += field_values[idx].width + width1 - width2 + 1;
		}
	}

	/* Print the main table header */
	if (main_header && pretty) {
		int header_width = bmc_string_screen_width (fields.header_name, NULL) + 4;
		table_width = table_width < header_width ? header_width : table_width;

		line = g_strnfill (table_width, '=');
		width1 = strlen (fields.header_name);
		width2 = bmc_string_screen_width (fields.header_name, NULL);
		printf ("%s\n", line);
		printf ("%*s\n", (table_width + width2)/2 + width1 - width2, fields.header_name);
		printf ("%s\n", line);
		g_free (line);
	}

	/* Print actual values */
	if (!main_header_only && str->len > 0) {
		g_string_truncate (str, str->len-1);  /* Chop off last column separator */
		if (fields.indent > 0) {
			indent_str = g_strnfill (fields.indent, ' ');
			g_string_prepend (str, indent_str);
			g_free (indent_str);
		}
		printf ("%s\n", str->str);
	}

	/* Print horizontal separator */
	if (!main_header_only && field_names && pretty) {
		if (str->len > 0) {
			line = g_strnfill (table_width, '-');
			printf ("%s\n", line);
			g_free (line);
		}
	}

	g_string_free (str, TRUE);
}

/*
 * Find out whether BarcodeManager is running (via D-Bus NameHasOwner), assuring
 * BarcodeManager won't be autostart (by D-Bus) if not running.
 * We can't use BMClient (bm_client_get_manager_running()) because BMClient
 * constructor calls GetPermissions of BM_DBUS_SERVICE, which would autostart
 * BarcodeManger if it is configured as D-Bus launchable service.
 */
gboolean
bmc_is_bm_running (BmCli *bmc, GError **error)
{
	DBusGConnection *connection = NULL;
	DBusGProxy *proxy = NULL;
	GError *err = NULL;
	gboolean has_owner = FALSE;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!connection) {
		g_string_printf (bmc->return_text, _("Error: Couldn't connect to system bus: %s"), err->message);
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		g_propagate_error (error, err);
		goto done;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
	                                   "org.freedesktop.DBus",
	                                   "/org/freedesktop/DBus",
	                                   "org.freedesktop.DBus");
	if (!proxy) {
		g_string_printf (bmc->return_text, _("Error: Couldn't create D-Bus object proxy for org.freedesktop.DBus"));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		if (error)
			g_set_error (error, 0, 0, "%s", bmc->return_text->str);
		goto done;
	}
 
	if (!org_freedesktop_DBus_name_has_owner (proxy, BM_DBUS_SERVICE, &has_owner, &err)) {
		g_string_printf (bmc->return_text, _("Error: NameHasOwner request failed: %s"),
		                 (err && err->message) ? err->message : _("(unknown)"));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		g_propagate_error (error, err);
		goto done;
	}

done:
	if (connection)
		dbus_g_connection_unref (connection);
	if (proxy)
		g_object_unref (proxy);

	return has_owner;
}

