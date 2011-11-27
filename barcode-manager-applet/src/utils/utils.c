/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Applet -- Display barcode scanners and allow user control
 *
 * Dan Williams <dcbw@redhat.com>
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

#include <config.h>
#include <string.h>
#include <netinet/ether.h>
#include <glib.h>

#include <bm-device-bt.h>

#include <bm-setting-connection.h>
#include <bm-setting-bluetooth.h>
#include <bm-utils.h>

#include "utils.h"
#include "gconf-helpers.h"

static char *ignored_words[] = {
	"Semiconductor",
	"Components",
	"Corporation",
	"Communications",
	"Company",
	"Corp.",
	"Corp",
	"Co.",
	"Inc.",
	"Inc",
	"Incorporated",
	"Ltd.",
	"Limited.",
	"Intel?",
	"chipset",
	"adapter",
	"[hex]",
	"NDIS",
	"Module",
	NULL
};

static char *ignored_phrases[] = {
	"Multiprotocol MAC/baseband processor",
	"Wireless LAN Controller",
	"Wireless LAN Adapter",
	"Wireless Adapter",
	"Network Connection",
	"Wireless Cardbus Adapter",
	"Wireless CardBus Adapter",
	"54 Mbps Wireless PC Card",
	"Wireless PC Card",
	"Wireless PC",
	"PC Card with XJACK(r) Antenna",
	"Wireless cardbus",
	"Wireless LAN PC Card",
	"Technology Group Ltd.",
	"Communication S.p.A.",
	"Business Mobile Networks BV",
	"Mobile Broadband Minicard Composite Device",
	"Mobile Communications AB",
	"(PC-Suite Mode)",
	NULL
};

static char *
fixup_desc_string (const char *desc)
{
	char *p, *temp;
	char **words, **item;
	GString *str;

	p = temp = g_strdup (desc);
	while (*p) {
		if (*p == '_' || *p == ',')
			*p = ' ';
		p++;
	}

	/* Attempt to shorten ID by ignoring certain phrases */
	for (item = ignored_phrases; *item; item++) {
		guint32 ignored_len = strlen (*item);

		p = strstr (temp, *item);
		if (p)
			memmove (p, p + ignored_len, strlen (p + ignored_len) + 1); /* +1 for the \0 */
	}

	/* Attmept to shorten ID by ignoring certain individual words */
	words = g_strsplit (temp, " ", 0);
	str = g_string_new_len (NULL, strlen (temp));
	g_free (temp);

	for (item = words; *item; item++) {
		int i = 0;
		gboolean ignore = FALSE;

		if (g_ascii_isspace (**item) || (**item == '\0'))
			continue;

		while (ignored_words[i] && !ignore) {
			if (!strcmp (*item, ignored_words[i]))
				ignore = TRUE;
			i++;
		}

		if (!ignore) {
			if (str->len)
				g_string_append_c (str, ' ');
			g_string_append (str, *item);
		}
	}
	g_strfreev (words);

	temp = str->str;
	g_string_free (str, FALSE);

	return temp;
}

#define DESC_TAG "description"

const char *
utils_get_device_description (BMDevice *device)
{
	char *description = NULL;
	const char *dev_product;
	const char *dev_vendor;
	char *product = NULL;
	char *vendor = NULL;
	GString *str;

	g_return_val_if_fail (device != NULL, NULL);

	description = g_object_get_data (G_OBJECT (device), DESC_TAG);
	if (description)
		return description;

	dev_product = bm_device_get_product (device);
	dev_vendor = bm_device_get_vendor (device);
	if (!dev_product || !dev_vendor)
		return NULL;

	product = fixup_desc_string (dev_product);
	vendor = fixup_desc_string (dev_vendor);

	str = g_string_new_len (NULL, strlen (vendor) + strlen (product) + 1);

	/* Another quick hack; if all of the fixed up vendor string
	 * is found in product, ignore the vendor.
	 */
	if (!strcasestr (product, vendor)) {
		g_string_append (str, vendor);
		g_string_append_c (str, ' ');
	}

	g_string_append (str, product);
	g_free (product);
	g_free (vendor);

	description = str->str;
	g_string_free (str, FALSE);

	g_object_set_data_full (G_OBJECT (device),
	                        "description", description,
	                        (GDestroyNotify) g_free);

	return description;
}

char *
utils_next_available_name (GSList *connections, const char *format)
{
	GSList *names = NULL, *iter;
	char *cname = NULL;
	int i = 0;

	for (iter = connections; iter; iter = g_slist_next (iter)) {
		BMConnection *candidate = BM_CONNECTION (iter->data);
		BMSettingConnection *s_con;
		const char *id;

		s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (candidate, BM_TYPE_SETTING_CONNECTION));
		id = bm_setting_connection_get_id (s_con);
		g_assert (id);
		names = g_slist_append (names, (gpointer) id);
	}	

	/* Find the next available unique connection name */
	while (!cname && (i++ < 10000)) {
		char *temp;
		gboolean found = FALSE;

		temp = g_strdup_printf (format, i);
		for (iter = names; iter; iter = g_slist_next (iter)) {
			if (!strcmp (iter->data, temp)) {
				found = TRUE;
				break;
			}
		}
		if (!found)
			cname = temp;
		else
			g_free (temp);
	}

	g_slist_free (names);
	return cname;
}

typedef struct {
	const char *tag;
	const char *replacement;
} Tag;

static Tag escaped_tags[] = {
	{ "<center>", NULL },
	{ "</center>", NULL },
	{ "<p>", "\n" },
	{ "</p>", NULL },
	{ "<B>", "<b>" },
	{ "</B>", "</b>" },
	{ "<I>", "<i>" },
	{ "</I>", "</i>" },
	{ "<u>", "<u>" },
	{ "</u>", "</u>" },
	{ "&", "&amp;" },
	{ NULL, NULL }
};

char *
utils_escape_notify_message (const char *src)
{
	const char *p = src;
	GString *escaped;

	/* Filter the source text and get rid of some HTML tags since the
	 * notification spec only allows a subset of HTML.  Substitute
	 * HTML code for characters like & that are invalid in HTML.
	 */

	escaped = g_string_sized_new (strlen (src) + 5);
	while (*p) {
		Tag *t = &escaped_tags[0];
		gboolean found = FALSE;

		while (t->tag) {
			if (strncasecmp (p, t->tag, strlen (t->tag)) == 0) {
				p += strlen (t->tag);
				if (t->replacement)
					g_string_append (escaped, t->replacement);
				found = TRUE;
				break;
			}
			t++;
		}
		if (!found)
			g_string_append_c (escaped, *p++);
	}

	return g_string_free (escaped, FALSE);
}

