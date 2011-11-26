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
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <bm-client.h>
#include <bm-device.h>
#include <bm-device-ethernet.h>
#include <bm-device-wifi.h>
#include <bm-gsm-device.h>
#include <bm-cdma-device.h>
#include <bm-device-bt.h>
//#include <bm-device-olpc-mesh.h>
#include <bm-utils.h>
#include <bm-setting-ip4-config.h>
#include <bm-setting-ip6-config.h>
#include <bm-vpn-connection.h>
#include <bm-setting-connection.h>
#include <bm-setting-wired.h>
#include <bm-setting-pppoe.h>
#include <bm-setting-wireless.h>
#include <bm-setting-gsm.h>
#include <bm-setting-cdma.h>
#include <bm-setting-bluetooth.h>
#include <bm-setting-olpc-mesh.h>

#include "utils.h"
#include "devices.h"


/* Available fields for 'dev status' */
static BmcOutputField bmc_fields_dev_status[] = {
	{"DEVICE",    N_("DEVICE"),      10, NULL, 0},  /* 0 */
	{"TYPE",      N_("TYPE"),        17, NULL, 0},  /* 1 */
	{"STATE",     N_("STATE"),       13, NULL, 0},  /* 2 */
	{"DBUS-PATH", N_("DBUS-PATH"),   43, NULL, 0},  /* 3 */
	{NULL,        NULL,               0, NULL, 0}
};
#define BMC_FIELDS_DEV_STATUS_ALL     "DEVICE,TYPE,STATE,DBUS-PATH"
#define BMC_FIELDS_DEV_STATUS_COMMON  "DEVICE,TYPE,STATE"


/* Available sections for 'dev list' */
static BmcOutputField bmc_fields_dev_list_sections[] = {
	{"GENERAL",           N_("GENERAL"),           0, NULL, 0},  /* 0 */
	{"CAPABILITIES",      N_("CAPABILITIES"),      0, NULL, 0},  /* 1 */
	{"WIFI-PROPERTIES",   N_("WIFI-PROPERTIES"),   0, NULL, 0},  /* 2 */
	{"AP",                N_("AP"),                0, NULL, 0},  /* 3 */
	{"WIRED-PROPERTIES",  N_("WIRED-PROPERTIES"),  0, NULL, 0},  /* 4 */
	{"IP4-SETTINGS",      N_("IP4-SETTINGS"),      0, NULL, 0},  /* 5 */
	{"IP4-DNS",           N_("IP4-DNS"),           0, NULL, 0},  /* 6 */
	{"IP6-SETTINGS",      N_("IP6-SETTINGS"),      0, NULL, 0},  /* 7 */
	{"IP6-DNS",           N_("IP6-DNS"),           0, NULL, 0},  /* 8 */
	{NULL,                NULL,                    0, NULL, 0}
};
#define BMC_FIELDS_DEV_LIST_SECTIONS_ALL     "GENERAL,CAPABILITIES,WIFI-PROPERTIES,AP,WIRED-PROPERTIES,IP4-SETTINGS,IP4-DNS,IP6-SETTINGS,IP6-DNS"
#define BMC_FIELDS_DEV_LIST_SECTIONS_COMMON  "GENERAL,CAPABILITIES,WIFI-PROPERTIES,AP,WIRED-PROPERTIES,IP4-SETTINGS,IP4-DNS,IP6-SETTINGS,IP6-DNS"

/* Available fields for 'dev list' - GENERAL part */
static BmcOutputField bmc_fields_dev_list_general[] = {
	{"NAME",       N_("NAME"),        10, NULL, 0},  /* 0 */
	{"DEVICE",     N_("DEVICE"),      10, NULL, 0},  /* 1 */
	{"TYPE",       N_("TYPE"),        17, NULL, 0},  /* 2 */
	{"DRIVER",     N_("DRIVER"),      10, NULL, 0},  /* 3 */
	{"HWADDR",     N_("HWADDR"),      19, NULL, 0},  /* 4 */
	{"STATE",      N_("STATE"),       14, NULL, 0},  /* 5 */
	{NULL,         NULL,               0, NULL, 0}
};
#define BMC_FIELDS_DEV_LIST_GENERAL_ALL     "NAME,DEVICE,TYPE,DRIVER,HWADDR,STATE"
#define BMC_FIELDS_DEV_LIST_GENERAL_COMMON  "NAME,DEVICE,TYPE,DRIVER,HWADDR,STATE"

/* Available fields for 'dev list' - CAPABILITIES part */
static BmcOutputField bmc_fields_dev_list_cap[] = {
	{"NAME",            N_("NAME"),            13, NULL, 0},  /* 0 */
	{"CARRIER-DETECT",  N_("CARRIER-DETECT"),  16, NULL, 0},  /* 1 */
	{"SPEED",           N_("SPEED"),           10, NULL, 0},  /* 2 */
	{NULL,              NULL,                   0, NULL, 0}
};
#define BMC_FIELDS_DEV_LIST_CAP_ALL     "NAME,CARRIER-DETECT,SPEED"
#define BMC_FIELDS_DEV_LIST_CAP_COMMON  "NAME,CARRIER-DETECT,SPEED"

/* static function prototypes */
static void usage (void);
static const char *device_state_to_string (BMDeviceState state);
static BMCResultCode do_devices_status (BmCli *bmc, int argc, char **argv);
static BMCResultCode do_devices_list (BmCli *bmc, int argc, char **argv);
static BMCResultCode do_device_disconnect (BmCli *bmc, int argc, char **argv);
static BMCResultCode do_device_wifi (BmCli *bmc, int argc, char **argv);


extern GMainLoop *loop;   /* glib main loop variable */

static void
usage (void)
{
	fprintf (stderr,
	 	 _("Usage: bmcli dev { COMMAND | help }\n\n"
		 "  COMMAND := { status | list | disconnect | wifi }\n\n"
		 "  status\n"
		 "  list [iface <iface>]\n"
		 "  disconnect iface <iface> [--nowait] [--timeout <timeout>]\n"
		 "  wifi [list [iface <iface>] [hwaddr <hwaddr>]]\n\n"));
}

/* quit main loop */
static void
quit (void)
{
	g_main_loop_quit (loop);  /* quit main loop */
}

static const char *
device_state_to_string (BMDeviceState state)
{
	switch (state) {
	case BM_DEVICE_STATE_UNMANAGED:
		return _("unmanaged");
	case BM_DEVICE_STATE_UNAVAILABLE:
		return _("unavailable");
	case BM_DEVICE_STATE_DISCONNECTED:
		return _("disconnected");
	case BM_DEVICE_STATE_PREPARE:
		return _("connecting (prepare)");
	case BM_DEVICE_STATE_CONFIG:
		return _("connecting (configuring)");
	case BM_DEVICE_STATE_NEED_AUTH:
		return _("connecting (need authentication)");
	case BM_DEVICE_STATE_IP_CONFIG:
		return _("connecting (getting IP configuration)");
	case BM_DEVICE_STATE_ACTIVATED:
		return _("connected");
	case BM_DEVICE_STATE_FAILED:
		return _("connection failed");
	default:
		return _("unknown");
	}
}

/* Return device type - use setting names to match with connection types */
static const char *
get_device_type ( BMDevice * device)
{
        if (BM_IS_USB_DEVICE (device))
		return BM_SETTING_USB_SETTING_NAME;
	else if (BM_IS_DEVICE_BT (device))
		return BM_SETTING_BLUETOOTH_SETTING_NAME;
	else
		return _("Unknown");
}

struct cb_info {
	BMClient *client;
	const GPtrArray *active;
};

static void
show_device_info (gpointer data, gpointer user_data)
{
	BMDevice *device = BM_DEVICE (data);
	BmCli *bmc = (BmCli *) user_data;
	GError *error = NULL;
	char *tmp;
	const char *hwaddr = NULL;
	BMDeviceState state = BM_DEVICE_STATE_UNKNOWN;
	guint32 caps;
	guint32 speed;
	char *speed_str = NULL;
	const GArray *array;
	GArray *sections_array;
	int k;
	char *fields_str;
	char *fields_all =    BMC_FIELDS_DEV_LIST_SECTIONS_ALL;
	char *fields_common = BMC_FIELDS_DEV_LIST_SECTIONS_COMMON;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;
	gboolean was_output = FALSE;

	if (!bmc->required_fields || strcasecmp (bmc->required_fields, "common") == 0)
		fields_str = fields_common;
	else if (!bmc->required_fields || strcasecmp (bmc->required_fields, "all") == 0)
		fields_str = fields_all;
	else
		fields_str = bmc->required_fields;

	sections_array = parse_output_fields (fields_str, bmc_fields_dev_list_sections, &error);
	if (error) {
		if (error->code == 0)
			g_string_printf (bmc->return_text, _("Error: 'dev list': %s"), error->message);
		else
			g_string_printf (bmc->return_text, _("Error: 'dev list': %s; allowed fields: %s"), error->message, BMC_FIELDS_DEV_LIST_SECTIONS_ALL);
		g_error_free (error);
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		return;
	}

	/* Main header */
	bmc->allowed_fields = bmc_fields_dev_list_general;
	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ONLY;
	bmc->print_fields.header_name = _("Device details");
	bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_DEV_LIST_GENERAL_ALL, bmc->allowed_fields, NULL);
	print_fields (bmc->print_fields, bmc->allowed_fields); /* Print header */

	/* Loop through the required sections and print them. */
	for (k = 0; k < sections_array->len; k++) {
		int section_idx = g_array_index (sections_array, int, k);

		if (bmc->print_output != BMC_PRINT_TERSE && !bmc->multiline_output && was_output)
			printf ("\n"); /* Empty line */

		was_output = FALSE;

		state = bm_device_get_state (device);

		/* section GENERAL */
		if (!strcasecmp (bmc_fields_dev_list_sections[section_idx].name, bmc_fields_dev_list_sections[0].name)) {
			bmc->allowed_fields = bmc_fields_dev_list_general;
			bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_FIELD_NAMES;
			bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_DEV_LIST_GENERAL_ALL, bmc->allowed_fields, NULL);
			print_fields (bmc->print_fields, bmc->allowed_fields); /* Print header */

			if (BM_IS_DEVICE_ETHERNET (device))
				hwaddr = bm_device_ethernet_get_hw_address (BM_DEVICE_ETHERNET (device));
			else if (BM_IS_DEVICE_WIFI (device))
				hwaddr = bm_device_wifi_get_hw_address (BM_DEVICE_WIFI (device));

			bmc->allowed_fields[0].value = bmc_fields_dev_list_sections[0].name;  /* "GENERAL"*/
			bmc->allowed_fields[1].value = bm_device_get_iface (device);
			bmc->allowed_fields[2].value = get_device_type (device);
			bmc->allowed_fields[3].value = bm_device_get_driver (device) ? bm_device_get_driver (device) : _("(unknown)");
			bmc->allowed_fields[4].value = hwaddr ? hwaddr : _("unknown)");
			bmc->allowed_fields[5].value = device_state_to_string (state);

			bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_SECTION_PREFIX;
			print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */
			was_output = TRUE;
		}

		/* section CAPABILITIES */
		if (!strcasecmp (bmc_fields_dev_list_sections[section_idx].name, bmc_fields_dev_list_sections[1].name)) {
			bmc->allowed_fields = bmc_fields_dev_list_cap;
			bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_FIELD_NAMES;
			bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_DEV_LIST_CAP_ALL, bmc->allowed_fields, NULL);
			print_fields (bmc->print_fields, bmc->allowed_fields); /* Print header */

			caps = bm_device_get_capabilities (device);
			speed = 0;

			bmc->allowed_fields[0].value = bmc_fields_dev_list_sections[1].name;  /* "CAPABILITIES" */
			bmc->allowed_fields[1].value = (caps & BM_DEVICE_CAP_CARRIER_DETECT) ? _("yes") : _("no");
			bmc->allowed_fields[2].value = speed_str ? speed_str : _("unknown");

			bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_SECTION_PREFIX;
			print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */
			g_free (speed_str);
			was_output = TRUE;
		}
	}

	if (sections_array)
		g_array_free (sections_array, TRUE);
}

static void
show_device_status (BMDevice *device, BmCli *bmc)
{
	bmc->allowed_fields[0].value = bm_device_get_iface (device);
	bmc->allowed_fields[1].value = get_device_type (device);
	bmc->allowed_fields[2].value = device_state_to_string (bm_device_get_state (device));
	bmc->allowed_fields[3].value = bm_object_get_path (BM_OBJECT (device));

	bmc->print_fields.flags &= ~BMC_PF_FLAG_MAIN_HEADER_ADD & ~BMC_PF_FLAG_MAIN_HEADER_ONLY & ~BMC_PF_FLAG_FIELD_NAMES; /* Clear header flags */
	print_fields (bmc->print_fields, bmc->allowed_fields);
}

static BMCResultCode
do_devices_status (BmCli *bmc, int argc, char **argv)
{
	GError *error = NULL;
	const GPtrArray *devices;
	int i;
	char *fields_str;
	char *fields_all =    BMC_FIELDS_DEV_STATUS_ALL;
	char *fields_common = BMC_FIELDS_DEV_STATUS_COMMON;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	while (argc > 0) {
		fprintf (stderr, _("Unknown parameter: %s\n"), *argv);
		argc--;
		argv++;
	}

	if (!bmc->required_fields || strcasecmp (bmc->required_fields, "common") == 0)
		fields_str = fields_common;
	else if (!bmc->required_fields || strcasecmp (bmc->required_fields, "all") == 0)
		fields_str = fields_all;
	else
		fields_str = bmc->required_fields;

	bmc->allowed_fields = bmc_fields_dev_status;
	bmc->print_fields.indices = parse_output_fields (fields_str, bmc->allowed_fields, &error);

	if (error) {
		if (error->code == 0)
			g_string_printf (bmc->return_text, _("Error: 'dev status': %s"), error->message);
		else
			g_string_printf (bmc->return_text, _("Error: 'dev status': %s; allowed fields: %s"), error->message, BMC_FIELDS_DEV_STATUS_ALL);
		g_error_free (error);
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		goto error;
	}

	if (!bmc_is_bm_running (bmc, &error)) {
		if (error) {
			g_string_printf (bmc->return_text, _("Error: Can't find out if BarcodeManager is running: %s."), error->message);
			bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
			g_error_free (error);
		} else {
			g_string_printf (bmc->return_text, _("Error: BarcodeManager is not running."));
			bmc->return_value = BMC_RESULT_ERROR_BM_NOT_RUNNING;
		}
		goto error;
	}

	/* Print headers */
	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
	bmc->print_fields.header_name = _("Status of devices");
	print_fields (bmc->print_fields, bmc->allowed_fields);

	bmc->get_client (bmc);
	devices = bm_client_get_devices (bmc->client);
	for (i = 0; devices && (i < devices->len); i++) {
		BMDevice *device = g_ptr_array_index (devices, i);
		show_device_status (device, bmc);
	}

	return BMC_RESULT_SUCCESS;

error:
	return bmc->return_value;
}

static BMCResultCode
do_devices_list (BmCli *bmc, int argc, char **argv)
{
	const GPtrArray *devices;
	GError *error = NULL;
	BMDevice *device = NULL;
	const char *iface = NULL;
	gboolean iface_specified = FALSE;
	int i;

	while (argc > 0) {
		if (strcmp (*argv, "iface") == 0) {
			iface_specified = TRUE;

			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: '%s' argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			iface = *argv;
		} else {
			fprintf (stderr, _("Unknown parameter: %s\n"), *argv);
		}

		argc--;
		argv++;
	}

	if (!bmc_is_bm_running (bmc, &error)) {
		if (error) {
			g_string_printf (bmc->return_text, _("Error: Can't find out if BarcodeManager is running: %s."), error->message);
			bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
			g_error_free (error);
		} else {
			g_string_printf (bmc->return_text, _("Error: BarcodeManager is not running."));
			bmc->return_value = BMC_RESULT_ERROR_BM_NOT_RUNNING;
		}
		goto error;
	}

	bmc->get_client (bmc);
	devices = bm_client_get_devices (bmc->client);

	if (iface_specified) {
		for (i = 0; devices && (i < devices->len); i++) {
			BMDevice *candidate = g_ptr_array_index (devices, i);
			const char *dev_iface = bm_device_get_iface (candidate);

			if (!strcmp (dev_iface, iface))
				device = candidate;
		}
		if (!device) {
		 	g_string_printf (bmc->return_text, _("Error: Device '%s' not found."), iface);
			bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
			goto error;
		}
		show_device_info (device, bmc);
	} else {
		if (devices)
			g_ptr_array_foreach ((GPtrArray *) devices, show_device_info, bmc);
	}

error:
	return bmc->return_value;
}

static void
device_state_cb (BMDevice *device, GParamSpec *pspec, gpointer user_data)
{
	BmCli *bmc = (BmCli *) user_data;
	BMDeviceState state;

	state = bm_device_get_state (device);

	if (state == BM_DEVICE_STATE_DISCONNECTED) {
		g_string_printf (bmc->return_text, _("Success: Device '%s' successfully disconnected."), bm_device_get_iface (device));
		quit ();
	}
}

static gboolean
timeout_cb (gpointer user_data)
{
	/* Time expired -> exit bmcli */

	BmCli *bmc = (BmCli *) user_data;

	g_string_printf (bmc->return_text, _("Error: Timeout %d sec expired."), bmc->timeout);
	bmc->return_value = BMC_RESULT_ERROR_TIMEOUT_EXPIRED;
	quit ();
	return FALSE;
}

static void
disconnect_device_cb (BMDevice *device, GError *error, gpointer user_data)
{
	BmCli *bmc = (BmCli *) user_data;
	BMDeviceState state;

	if (error) {
		g_string_printf (bmc->return_text, _("Error: Device '%s' (%s) disconnecting failed: %s"),
		                 bm_device_get_iface (device),
		                 bm_object_get_path (BM_OBJECT (device)),
		                 error->message ? error->message : _("(unknown)"));
		bmc->return_value = BMC_RESULT_ERROR_DEV_DISCONNECT;
		quit ();
	} else {
		state = bm_device_get_state (device);
		printf (_("Device state: %d (%s)\n"), state, device_state_to_string (state));

		if (bmc->nowait_flag || state == BM_DEVICE_STATE_DISCONNECTED) {
			/* Don't want to wait or device already disconnected */
			quit ();
		} else {
			g_signal_connect (device, "notify::state", G_CALLBACK (device_state_cb), bmc);
			/* Start timer not to loop forever if "notify::state" signal is not issued */
			g_timeout_add_seconds (bmc->timeout, timeout_cb, bmc);
		}

	}
}

static BMCResultCode
do_device_disconnect (BmCli *bmc, int argc, char **argv)
{
	const GPtrArray *devices;
	GError *error = NULL;
	BMDevice *device = NULL;
	const char *iface = NULL;
	gboolean iface_specified = FALSE;
	gboolean wait = TRUE;
	int i;

	/* Set default timeout for disconnect operation */
	bmc->timeout = 10;

	while (argc > 0) {
		if (strcmp (*argv, "iface") == 0) {
			iface_specified = TRUE;

			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			iface = *argv;
		} else if (strcmp (*argv, "--nowait") == 0) {
			wait = FALSE;
		} else if (strcmp (*argv, "--timeout") == 0) {
			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			errno = 0;
			bmc->timeout = strtol (*argv, NULL, 10);
			if (errno || bmc->timeout < 0) {
				g_string_printf (bmc->return_text, _("Error: timeout value '%s' is not valid."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

		} else {
			fprintf (stderr, _("Unknown parameter: %s\n"), *argv);
		}

		argc--;
		argv++;
	}

	if (!iface_specified) {
		g_string_printf (bmc->return_text, _("Error: iface has to be specified."));
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		goto error;
	}

	if (!bmc_is_bm_running (bmc, &error)) {
		if (error) {
			g_string_printf (bmc->return_text, _("Error: Can't find out if BarcodeManager is running: %s."), error->message);
			bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
			g_error_free (error);
		} else {
			g_string_printf (bmc->return_text, _("Error: BarcodeManager is not running."));
			bmc->return_value = BMC_RESULT_ERROR_BM_NOT_RUNNING;
		}
		goto error;
	}

	bmc->get_client (bmc);
	devices = bm_client_get_devices (bmc->client);
	for (i = 0; devices && (i < devices->len); i++) {
		BMDevice *candidate = g_ptr_array_index (devices, i);
		const char *dev_iface = bm_device_get_iface (candidate);

		if (!strcmp (dev_iface, iface))
			device = candidate;
	}

	if (!device) {
		g_string_printf (bmc->return_text, _("Error: Device '%s' not found."), iface);
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		goto error;
	}

	/* Use nowait_flag instead of should_wait because exitting has to be postponed till disconnect_device_cb()
	 * is called, giving BM time to check our permissions */
	bmc->nowait_flag = !wait;
	bmc->should_wait = TRUE;
	bm_device_disconnect (device, disconnect_device_cb, bmc);

error:
	return bmc->return_value;
}

BMCResultCode
do_devices (BmCli *bmc, int argc, char **argv)
{
	GError *error = NULL;

	if (argc == 0) {
		if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error))
			goto opt_error;
		bmc->return_value = do_devices_status (bmc, 0, NULL);
	}

	if (argc > 0) {
		if (matches (*argv, "status") == 0) {
			if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error))
				goto opt_error;
			bmc->return_value = do_devices_status (bmc, argc-1, argv+1);
		}
		else if (matches (*argv, "list") == 0) {
			if (!bmc->mode_specified)
				bmc->multiline_output = TRUE;  /* multiline mode is default for 'dev list' */
			bmc->return_value = do_devices_list (bmc, argc-1, argv+1);
		}
		else if (matches (*argv, "disconnect") == 0) {
			bmc->return_value = do_device_disconnect (bmc, argc-1, argv+1);
		}
		else if (matches (*argv, "wifi") == 0) {
			if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error))
				goto opt_error;
			bmc->return_value = do_device_wifi (bmc, argc-1, argv+1);
		}
		else if (strcmp (*argv, "help") == 0) {
			usage ();
		}
		else {
			g_string_printf (bmc->return_text, _("Error: 'dev' command '%s' is not valid."), *argv);
			bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		}
	}

	return bmc->return_value;

opt_error:
	g_string_printf (bmc->return_text, _("Error: %s."), error->message);
	bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
	g_error_free (error);
	return bmc->return_value;
}
