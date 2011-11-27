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
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <bm-client.h>
#include <bm-setting-connection.h>

#include "utils.h"
#include "barcode-manager.h"

/* Available fields for 'bm status' */
static BmcOutputField bmc_fields_bm_status[] = {
	{"RUNNING",        N_("RUNNING"),         15, NULL, 0},  /* 0 */
	{"STATE",          N_("STATE"),           15, NULL, 0},  /* 1 */
	{"NET-ENABLED",    N_("NET-ENABLED"),     13, NULL, 0},  /* 2 */
	{NULL,             NULL,                   0, NULL, 0}
};
#define BMC_FIELDS_BM_STATUS_ALL     "RUNNING,STATE,NET-ENABLED"
#define BMC_FIELDS_BM_STATUS_COMMON  "RUNNING,STATE"
#define BMC_FIELDS_BM_NET_ENABLED    "NET-ENABLED"
#define BMC_FIELDS_BM_WIFI           "WIFI"
#define BMC_FIELDS_BM_WWAN           "WWAN"


extern GMainLoop *loop;

/* static function prototypes */
static void usage (void);
static void quit (void);
static const char *bm_state_to_string (BMState state);
static BMCResultCode show_bm_status (BmCli *bmc);


static void
usage (void)
{
	fprintf (stderr,
	 	 _("Usage: bmcli bm { COMMAND | help }\n\n"
		 "  COMMAND := { status | enable | sleep  }\n\n"
		 "  status\n"
		 "  enable [true|false]\n"
		 "  sleep [true|false]\n\n"));
}

/* quit main loop */
static void
quit (void)
{
	g_main_loop_quit (loop);  /* quit main loop */
}

static const char *
bm_state_to_string (BMState state)
{
	switch (state) {
	case BM_STATE_ASLEEP:
		return _("asleep");
	case BM_STATE_CONNECTING:
		return _("connecting");
	case BM_STATE_CONNECTED:
		return _("connected");
	case BM_STATE_DISCONNECTED:
		return _("disconnected");
	case BM_STATE_UNKNOWN:
	default:
		return _("unknown");
	}
}

static BMCResultCode
show_bm_status (BmCli *bmc)
{
	gboolean bm_running;
	BMState state = BM_STATE_UNKNOWN;
	const char *net_enabled_str;
	GError *error = NULL;
	const char *fields_str;
	const char *fields_all =    BMC_FIELDS_BM_STATUS_ALL;
	const char *fields_common = BMC_FIELDS_BM_STATUS_COMMON;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	if (!bmc->required_fields || strcasecmp (bmc->required_fields, "common") == 0)
		fields_str = fields_common;
	else if (!bmc->required_fields || strcasecmp (bmc->required_fields, "all") == 0)
		fields_str = fields_all;
	else 
		fields_str = bmc->required_fields;

	bmc->allowed_fields = bmc_fields_bm_status;
	bmc->print_fields.indices = parse_output_fields (fields_str, bmc->allowed_fields, &error);

	if (error) {
		if (error->code == 0)
			g_string_printf (bmc->return_text, _("Error: 'bm status': %s"), error->message);
		else
			g_string_printf (bmc->return_text, _("Error: 'bm status': %s; allowed fields: %s"), error->message, BMC_FIELDS_BM_STATUS_ALL);
		g_error_free (error);
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		return bmc->return_value;
	}

	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
	bmc->print_fields.header_name = _("BarcodeManager status");
	print_fields (bmc->print_fields, bmc->allowed_fields); /* Print header */

	bm_running = bmc_is_bm_running (bmc, NULL);
	if (bm_running) {
		bmc->get_client (bmc); /* create BMClient */
		state = bm_client_get_state (bmc->client);
		net_enabled_str = bm_client_networking_get_enabled (bmc->client) ? _("enabled") : _("disabled");
	} else {
		net_enabled_str = _("unknown");
	}

	bmc->allowed_fields[0].value = bm_running ? _("running") : _("not running");
	bmc->allowed_fields[1].value = bm_state_to_string (state);
	bmc->allowed_fields[2].value = net_enabled_str;

	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag;
	print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */

	return BMC_RESULT_SUCCESS;
}

/* libbm-glib doesn't provide API fro Sleep method - implement D-Bus call ourselves */
static void networking_set_sleep (BmCli *bmc, gboolean in_sleep)
{
	DBusGConnection *connection = NULL;
	DBusGProxy *proxy = NULL;
	GError *err = NULL;

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!connection) {
		g_string_printf (bmc->return_text, _("Error: Couldn't connect to system bus: %s"), err->message);
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
        	g_error_free (err);
	        goto gone;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
	                                   "org.freedesktop.BarcodeManager",
	                                   "/org/freedesktop/BarcodeManager",
	                                   "org.freedesktop.BarcodeManager");
	if (!proxy) {
		g_string_printf (bmc->return_text, _("Error: Couldn't create D-Bus object proxy."));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		goto gone;
        }
 
	if (!dbus_g_proxy_call (proxy, "Sleep", &err, G_TYPE_BOOLEAN, in_sleep, G_TYPE_INVALID, G_TYPE_INVALID)) {
		g_string_printf (bmc->return_text, _("Error in sleep: %s"), err->message);
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		g_error_free (err);
	}

gone:
	if (connection) dbus_g_connection_unref (connection);
	if (proxy) g_object_unref (proxy);
}

/* entry point function for global network manager related commands 'bmcli nm' */
BMCResultCode
do_barcode_manager (BmCli *bmc, int argc, char **argv)
{
	GError *error = NULL;
	gboolean sleep_flag;
	gboolean enable_net;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	if (argc == 0) {
		if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error))
			goto opt_error;
		bmc->return_value = show_bm_status (bmc);
	}

	if (argc > 0) {
		if (matches (*argv, "status") == 0) {
			if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error))
				goto opt_error;
			bmc->return_value = show_bm_status (bmc);
		}
		else if (matches (*argv, "enable") == 0) {
			if (next_arg (&argc, &argv) != 0) {
				/* no argument, show current state of networking */
				if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error))
					goto opt_error;
				if (bmc->required_fields && strcasecmp (bmc->required_fields, "NET-ENABLED")) {
					g_string_printf (bmc->return_text, _("Error: '--fields' value '%s' is not valid here; allowed fields: %s"),
					                 bmc->required_fields, BMC_FIELDS_BM_NET_ENABLED);
					bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
					goto end;
				}
				bmc->allowed_fields = bmc_fields_bm_status;
				bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_BM_NET_ENABLED, bmc->allowed_fields, NULL);
				bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
				bmc->print_fields.header_name = _("Networking enabled");
				print_fields (bmc->print_fields, bmc->allowed_fields); /* Print header */

				if (bmc_is_bm_running (bmc, NULL)) {
					bmc->get_client (bmc); /* create BMClient */
					bmc->allowed_fields[2].value = bm_client_networking_get_enabled (bmc->client) ? _("enabled") : _("disabled");
				} else
					bmc->allowed_fields[2].value = _("unknown");
				bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag;
				print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */
			} else {
				if (!strcmp (*argv, "true"))
					enable_net = TRUE;
				else if (!strcmp (*argv, "false"))
					enable_net = FALSE;
				else {
					g_string_printf (bmc->return_text, _("Error: invalid 'enable' parameter: '%s'; use 'true' or 'false'."), *argv);
					bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
					goto end;
				}
				bmc->get_client (bmc); /* create BMClient */
				bm_client_networking_set_enabled (bmc->client, enable_net);
			}
		}
		else if (matches (*argv, "sleep") == 0) {
			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: Sleeping status is not exported by BarcodeManager."));
				bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
			} else {
				if (!strcmp (*argv, "true"))
					sleep_flag = TRUE;
				else if (!strcmp (*argv, "false"))
					sleep_flag = FALSE;
				else {
					g_string_printf (bmc->return_text, _("Error: invalid 'sleep' parameter: '%s'; use 'true' or 'false'."), *argv);
					bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
					goto end;
				}
				networking_set_sleep (bmc, sleep_flag);
			}
		}
		else if (strcmp (*argv, "help") == 0) {
			usage ();
		}
		else {
			g_string_printf (bmc->return_text, _("Error: 'nm' command '%s' is not valid."), *argv);
			bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		}
	}

end:
	quit ();
	return bmc->return_value;

opt_error:
	quit ();
	g_string_printf (bmc->return_text, _("Error: %s."), error->message);
	bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
	g_error_free (error);
	return bmc->return_value;
}
