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

#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netinet/ether.h>

#include <bm-client.h>
#include <bm-setting-connection.h>
#include <bm-setting-bluetooth.h>
#include <bm-device-bt.h>
#include <bm-remote-settings.h>
#include <bm-remote-settings-system.h>
#include <bm-settings-interface.h>
#include <bm-settings-connection-interface.h>

#include "utils.h"
#include "settings.h"
#include "connections.h"

/* Available fields for 'con status' */
static BmcOutputField bmc_fields_con_status[] = {
	{"NAME",          N_("NAME"),         25, NULL, 0},  /* 0 */
	{"UUID",          N_("UUID"),         38, NULL, 0},  /* 1 */
	{"DEVICES",       N_("DEVICES"),      10, NULL, 0},  /* 2 */
	{"SCOPE",         N_("SCOPE"),         8, NULL, 0},  /* 3 */
	{"DEFAULT",       N_("DEFAULT"),       8, NULL, 0},  /* 4 */
	{"DBUS-SERVICE",  N_("DBUS-SERVICE"), 45, NULL, 0},  /* 5 */
	{"SPEC-OBJECT",   N_("SPEC-OBJECT"),  10, NULL, 0},  /* 6 */
	{"VPN",           N_("VPN"),           5, NULL, 0},  /* 7 */
	{"DBUS-PATH",     N_("DBUS-PATH"),    51, NULL, 0},  /* 8 */
	{NULL,            NULL,                0, NULL, 0}
};
#define BMC_FIELDS_CON_STATUS_ALL     "NAME,UUID,DEVICES,SCOPE,DEFAULT,VPN,DBUS-SERVICE,DBUS-PATH,SPEC-OBJECT"
#define BMC_FIELDS_CON_STATUS_COMMON  "NAME,UUID,DEVICES,SCOPE,DEFAULT,VPN"

/* Available fields for 'con list' */
static BmcOutputField bmc_fields_con_list[] = {
	{"NAME",            N_("NAME"),           25, NULL, 0},  /* 0 */
	{"UUID",            N_("UUID"),           38, NULL, 0},  /* 1 */
	{"TYPE",            N_("TYPE"),           17, NULL, 0},  /* 2 */
	{"SCOPE",           N_("SCOPE"),           8, NULL, 0},  /* 3 */
	{"TIMESTAMP",       N_("TIMESTAMP"),      12, NULL, 0},  /* 4 */
	{"TIMESTAMP-REAL",  N_("TIMESTAMP-REAL"), 34, NULL, 0},  /* 5 */
	{"AUTOCONNECT",     N_("AUTOCONNECT"),    13, NULL, 0},  /* 6 */
	{"READONLY",        N_("READONLY"),       10, NULL, 0},  /* 7 */
	{"DBUS-PATH",       N_("DBUS-PATH"),      42, NULL, 0},  /* 8 */
	{NULL,              NULL,                  0, NULL, 0}
};
#define BMC_FIELDS_CON_LIST_ALL     "NAME,UUID,TYPE,SCOPE,TIMESTAMP,TIMESTAMP-REAL,AUTOCONNECT,READONLY,DBUS-PATH"
#define BMC_FIELDS_CON_LIST_COMMON  "NAME,UUID,TYPE,SCOPE,TIMESTAMP-REAL"


/* Helper macro to define fields */
#define SETTING_FIELD(setting, width) { setting, N_(setting), width, NULL, 0 }

/* Available settings for 'con list id/uuid <con>' */
static BmcOutputField bmc_fields_settings_names[] = {
	SETTING_FIELD (BM_SETTING_CONNECTION_SETTING_NAME, 0),            /* 0 */
	SETTING_FIELD (BM_SETTING_SERIAL_SETTING_NAME, 0),                /* 7 */
	SETTING_FIELD (BM_SETTING_BLUETOOTH_SETTING_NAME, 0),             /* 12 */
	{NULL, NULL, 0, NULL, 0}
};
#define BMC_FIELDS_SETTINGS_NAMES_ALL    BM_SETTING_CONNECTION_SETTING_NAME","\
                                         BM_SETTING_SERIAL_SETTING_NAME","\
                                         BM_SETTING_BLUETOOTH_SETTING_NAME

typedef struct {
	BmCli *bmc;
	int argc;
	char **argv;
} ArgsInfo;

extern GMainLoop *loop;   /* glib main loop variable */

static ArgsInfo args_info;

/* static function prototypes */
static void usage (void);
static void quit (void);
static void show_connection (BMConnection *data, gpointer user_data);
static BMConnection *find_connection (GSList *list, const char *filter_type, const char *filter_val);
static gboolean find_device_for_connection (BmCli *bmc, BMConnection *connection, const char *iface, const char *ap,
                                            BMDevice **device, const char **spec_object, GError **error);
static const char *active_connection_state_to_string (BMActiveConnectionState state);
static void active_connection_state_cb (BMActiveConnection *active, GParamSpec *pspec, gpointer user_data);
static void activate_connection_cb (gpointer user_data, const char *path, GError *error);
static void get_connections_cb (BMSettingsInterface *settings, gpointer user_data);
static BMCResultCode do_connections_list (BmCli *bmc, int argc, char **argv);
static BMCResultCode do_connections_status (BmCli *bmc, int argc, char **argv);
static BMCResultCode do_connection_up (BmCli *bmc, int argc, char **argv);
static BMCResultCode do_connection_down (BmCli *bmc, int argc, char **argv);

static void
usage (void)
{
	fprintf (stderr,
	 	 _("Usage: bmcli con { COMMAND | help }\n"
		 "  COMMAND := { list | status | up | down }\n\n"
		 "  list [id <id> | uuid <id> | system | user]\n"
		 "  status\n"
		 "  up id <id> | uuid <id> [iface <iface>] [ap <hwaddr>] [--nowait] [--timeout <timeout>]\n"
		 "  down id <id> | uuid <id>\n"));
}

/* quit main loop */
static void
quit (void)
{
	g_main_loop_quit (loop);  /* quit main loop */
}

static gboolean
bmc_connection_detail (BMConnection *connection, BmCli *bmc)
{
	BMSetting *setting;
	GError *error = NULL;
	GArray *print_settings_array;
	int i;
	char *fields_str;
	char *fields_all =    BMC_FIELDS_SETTINGS_NAMES_ALL;
	char *fields_common = BMC_FIELDS_SETTINGS_NAMES_ALL;
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

	print_settings_array = parse_output_fields (fields_str, bmc_fields_settings_names, &error);
	if (error) {
		if (error->code == 0)
			g_string_printf (bmc->return_text, _("Error: 'con list': %s"), error->message);
		else
			g_string_printf (bmc->return_text, _("Error: 'con list': %s; allowed fields: %s"), error->message, BMC_FIELDS_SETTINGS_NAMES_ALL);
		g_error_free (error);
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		return FALSE;
	}

	bmc->allowed_fields = bmc_fields_settings_names;
	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ONLY;
	bmc->print_fields.header_name = _("Connection details");
	bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_SETTINGS_NAMES_ALL, bmc->allowed_fields, NULL);
	print_fields (bmc->print_fields, bmc->allowed_fields);

	/* Loop through the required settings and print them. */
	for (i = 0; i < print_settings_array->len; i++) {
		int section_idx = g_array_index (print_settings_array, int, i);

		if (bmc->print_output != BMC_PRINT_TERSE && !bmc->multiline_output && was_output)
			printf ("\n"); /* Empty line */

		was_output = FALSE;

		if (!strcasecmp (bmc_fields_settings_names[section_idx].name, bmc_fields_settings_names[0].name)) {
			setting = bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
			if (setting) {
				setting_connection_details (setting, bmc);
				was_output = TRUE;
				continue;
			}
		}

		if (!strcasecmp (bmc_fields_settings_names[section_idx].name, bmc_fields_settings_names[1].name)) {
			setting = bm_connection_get_setting (connection, BM_TYPE_SETTING_SERIAL);
			if (setting) {
				setting_serial_details (setting, bmc);
				was_output = TRUE;
				continue;
			}
		}
		if (!strcasecmp (bmc_fields_settings_names[section_idx].name, bmc_fields_settings_names[2].name)) {
			setting = bm_connection_get_setting (connection, BM_TYPE_SETTING_BLUETOOTH);
			if (setting) {
				setting_bluetooth_details (setting, bmc);
				was_output = TRUE;
				continue;
			}
		}
	}

	if (print_settings_array)
		g_array_free (print_settings_array, FALSE);

	return BMC_RESULT_SUCCESS;
}

static void
show_connection (BMConnection *data, gpointer user_data)
{
	BMConnection *connection = (BMConnection *) data;
	BmCli *bmc = (BmCli *) user_data;
	BMSettingConnection *s_con;
	guint64 timestamp;
	char *timestamp_str;
	char timestamp_real_str[64];

	s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
	if (s_con) {
		/* Obtain field values */
		timestamp = bm_setting_connection_get_timestamp (s_con);
		timestamp_str = g_strdup_printf ("%" G_GUINT64_FORMAT, timestamp);
		strftime (timestamp_real_str, sizeof (timestamp_real_str), "%c", localtime ((time_t *) &timestamp));

		bmc->allowed_fields[0].value = bm_setting_connection_get_id (s_con);
		bmc->allowed_fields[1].value = bm_setting_connection_get_uuid (s_con);
		bmc->allowed_fields[2].value = bm_setting_connection_get_connection_type (s_con);
		bmc->allowed_fields[3].value = bm_connection_get_scope (connection) == BM_CONNECTION_SCOPE_SYSTEM ? _("system") : _("user");
		bmc->allowed_fields[4].value = timestamp_str;
		bmc->allowed_fields[5].value = timestamp ? timestamp_real_str : _("never");
		bmc->allowed_fields[6].value = bm_setting_connection_get_autoconnect (s_con) ? _("yes") : _("no");
		bmc->allowed_fields[7].value = bm_setting_connection_get_read_only (s_con) ? _("yes") : _("no");
		bmc->allowed_fields[8].value = bm_connection_get_path (connection);

		bmc->print_fields.flags &= ~BMC_PF_FLAG_MAIN_HEADER_ADD & ~BMC_PF_FLAG_MAIN_HEADER_ONLY & ~BMC_PF_FLAG_FIELD_NAMES; /* Clear header flags */
		print_fields (bmc->print_fields, bmc->allowed_fields);

		g_free (timestamp_str);
	}
}

static BMConnection *
find_connection (GSList *list, const char *filter_type, const char *filter_val)
{
	BMSettingConnection *s_con;
	BMConnection *connection;
	GSList *iterator;
	const char *id;
	const char *uuid;

	iterator = list;
	while (iterator) {
		connection = BM_CONNECTION (iterator->data);
		s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
		if (s_con) {
			id = bm_setting_connection_get_id (s_con);
			uuid = bm_setting_connection_get_uuid (s_con);
			if (filter_type) {
				if ((strcmp (filter_type, "id") == 0 && strcmp (filter_val, id) == 0) ||
				    (strcmp (filter_type, "uuid") == 0 && strcmp (filter_val, uuid) == 0)) {
					return connection;
				}
			}
		}
		iterator = g_slist_next (iterator);
	}

	return NULL;
}

static BMCResultCode
do_connections_list (BmCli *bmc, int argc, char **argv)
{
	GError *error1 = NULL;
	GError *error2 = NULL;
	char *fields_str;
	char *fields_all =    BMC_FIELDS_CON_LIST_ALL;
	char *fields_common = BMC_FIELDS_CON_LIST_COMMON;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;
	gboolean valid_param_specified = FALSE;

	bmc->should_wait = FALSE;

	if (!bmc->required_fields || strcasecmp (bmc->required_fields, "common") == 0)
		fields_str = fields_common;
	else if (!bmc->required_fields || strcasecmp (bmc->required_fields, "all") == 0)
		fields_str = fields_all;
	else
		fields_str = bmc->required_fields;

	bmc->allowed_fields = bmc_fields_con_list;
	bmc->print_fields.indices = parse_output_fields (fields_str, bmc->allowed_fields, &error1);
	/* error1 is checked later - it's not valid for connection details */

	if (argc == 0) {
		if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error2))
			goto error;
		if (error1)
			goto error;
		valid_param_specified = TRUE;

		bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
		bmc->print_fields.header_name = _("System connections");
		print_fields (bmc->print_fields, bmc->allowed_fields);
		g_slist_foreach (bmc->system_connections, (GFunc) show_connection, bmc);

		bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
		bmc->print_fields.header_name = _("User connections");
		print_fields (bmc->print_fields, bmc->allowed_fields);
		g_slist_foreach (bmc->user_connections, (GFunc) show_connection, bmc);
	}
	else {
		while (argc > 0) {
			if (strcmp (*argv, "id") == 0 || strcmp (*argv, "uuid") == 0) {
				const char *selector = *argv;
				BMConnection *con1;
				BMConnection *con2;

				if (next_arg (&argc, &argv) != 0) {
					g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
					bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
					return bmc->return_value;
				}
				valid_param_specified = TRUE;
				if (!bmc->mode_specified)
					bmc->multiline_output = TRUE;  /* multiline mode is default for 'con list id|uuid' */

				con1 = find_connection (bmc->system_connections, selector, *argv);
				con2 = find_connection (bmc->user_connections, selector, *argv);
				if (con1) bmc_connection_detail (con1, bmc);
				if (con2) bmc_connection_detail (con2, bmc);
				if (!con1 && !con2) {
					g_string_printf (bmc->return_text, _("Error: %s - no such connection."), *argv);
					bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
				}
				break;
			}
			else if (strcmp (*argv, "system") == 0) {
				if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error2))
					goto error;
				if (error1)
					goto error;
				valid_param_specified = TRUE;

				bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
				bmc->print_fields.header_name = _("System connections");
				print_fields (bmc->print_fields, bmc->allowed_fields);
				g_slist_foreach (bmc->system_connections, (GFunc) show_connection, bmc);
				break;
			}
			else if (strcmp (*argv, "user") == 0) {
				if (!bmc_terse_option_check (bmc->print_output, bmc->required_fields, &error2))
					goto error;
				if (error1)
					goto error;
				valid_param_specified = TRUE;

				bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_MAIN_HEADER_ADD | BMC_PF_FLAG_FIELD_NAMES;
				bmc->print_fields.header_name = _("User connections");
				print_fields (bmc->print_fields, bmc->allowed_fields);
				g_slist_foreach (bmc->user_connections, (GFunc) show_connection, bmc);
				break;
			}
			else {
				fprintf (stderr, _("Unknown parameter: %s\n"), *argv);
			}

			argc--;
			argv++;
		}
	}

	if (!valid_param_specified) {
		g_string_printf (bmc->return_text, _("Error: no valid parameter specified."));
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
	}
	return bmc->return_value;

error:
	if (error1) {
		if (error1->code == 0)
			g_string_printf (bmc->return_text, _("Error: 'con list': %s"), error1->message);
		else
			g_string_printf (bmc->return_text, _("Error: 'con list': %s; allowed fields: %s"), error1->message, BMC_FIELDS_CON_LIST_ALL);
		g_error_free (error1);
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
	}
	if (error2) {
		g_string_printf (bmc->return_text, _("Error: %s."), error2->message);
		bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
		g_error_free (error2);
	}

	return bmc->return_value;
}

typedef struct {
	BmCli *bmc;
	BMConnectionScope scope;
} StatusInfo;

static void
show_active_connection (gpointer data, gpointer user_data)
{
	BMActiveConnection *active = BM_ACTIVE_CONNECTION (data);
	StatusInfo *info = (StatusInfo *) user_data;
	GSList *con_list, *iter;
	const char *active_path;
	BMConnectionScope active_service_scope;
	BMSettingConnection *s_con;
	const GPtrArray *devices;
	GString *dev_str;
	int i;

	active_path = bm_active_connection_get_connection (active);
	active_service_scope = bm_active_connection_get_scope (active);

	if (active_service_scope != info->scope)
		return;

	/* Get devices of the active connection */
	dev_str = g_string_new (NULL);
	devices = bm_active_connection_get_devices (active);
	for (i = 0; devices && (i < devices->len); i++) {
		BMDevice *device = g_ptr_array_index (devices, i);

		g_string_append (dev_str, bm_device_get_iface (device));
		g_string_append_c (dev_str, ',');
	}
	if (dev_str->len > 0)
		g_string_truncate (dev_str, dev_str->len - 1);  /* Cut off last ',' */

	con_list = (info->scope == BM_CONNECTION_SCOPE_SYSTEM) ? info->bmc->system_connections : info->bmc->user_connections;
	for (iter = con_list; iter; iter = g_slist_next (iter)) {
		BMConnection *connection = (BMConnection *) iter->data;
		const char *con_path = bm_connection_get_path (connection);

		if (!strcmp (active_path, con_path)) {
			/* This connection is active */
			s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
			g_assert (s_con != NULL);

			/* Obtain field values */
			info->bmc->allowed_fields[0].value = bm_setting_connection_get_id (s_con);
			info->bmc->allowed_fields[1].value = bm_setting_connection_get_uuid (s_con);
			info->bmc->allowed_fields[2].value = dev_str->str;
			info->bmc->allowed_fields[3].value = active_service_scope == BM_CONNECTION_SCOPE_SYSTEM ? _("system") : _("user");
			info->bmc->allowed_fields[4].value = bm_active_connection_get_default (active) ? _("yes") : _("no");
			info->bmc->allowed_fields[5].value = bm_active_connection_get_service_name (active);
			info->bmc->allowed_fields[6].value = bm_active_connection_get_specific_object (active);
			info->bmc->allowed_fields[7].value = BM_IS_VPN_CONNECTION (active) ? _("yes") : _("no");
			info->bmc->allowed_fields[8].value = bm_object_get_path (BM_OBJECT (active));

			info->bmc->print_fields.flags &= ~BMC_PF_FLAG_MAIN_HEADER_ADD & ~BMC_PF_FLAG_MAIN_HEADER_ONLY & ~BMC_PF_FLAG_FIELD_NAMES; /* Clear header flags */
			print_fields (info->bmc->print_fields, info->bmc->allowed_fields);
			break;
		}
	}

	g_string_free (dev_str, TRUE);
}

static BMCResultCode
do_connections_status (BmCli *bmc, int argc, char **argv)
{
	const GPtrArray *active_cons;
	GError *error = NULL;
	StatusInfo *info;
	char *fields_str;
	char *fields_all =    BMC_FIELDS_CON_STATUS_ALL;
	char *fields_common = BMC_FIELDS_CON_STATUS_COMMON;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	bmc->should_wait = FALSE;

	if (!bmc->required_fields || strcasecmp (bmc->required_fields, "common") == 0)
		fields_str = fields_common;
	else if (!bmc->required_fields || strcasecmp (bmc->required_fields, "all") == 0)
		fields_str = fields_all;
	else
		fields_str = bmc->required_fields;

	bmc->allowed_fields = bmc_fields_con_status;
	bmc->print_fields.indices = parse_output_fields (fields_str, bmc->allowed_fields, &error);

	if (error) {
		if (error->code == 0)
			g_string_printf (bmc->return_text, _("Error: 'con status': %s"), error->message);
		else
			g_string_printf (bmc->return_text, _("Error: 'con status': %s; allowed fields: %s"), error->message, BMC_FIELDS_CON_STATUS_ALL);
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
	bmc->print_fields.header_name = _("Active connections");
	print_fields (bmc->print_fields, bmc->allowed_fields);

	bmc->get_client (bmc);
	active_cons = bm_client_get_active_connections (bmc->client);
	if (active_cons && active_cons->len) {
		info = g_malloc0 (sizeof (StatusInfo));
		info->bmc = bmc;
		info->scope = BM_CONNECTION_SCOPE_SYSTEM;
		g_ptr_array_foreach ((GPtrArray *) active_cons, show_active_connection, (gpointer) info);
		info->scope = BM_CONNECTION_SCOPE_USER;
		g_ptr_array_foreach ((GPtrArray *) active_cons, show_active_connection, (gpointer) info);
		g_free (info);
	}

error:
	return bmc->return_value;
}

static gboolean
check_bt_compatible (BMDeviceBt *device, BMConnection *connection, GError **error)
{
	BMSettingConnection *s_con;
	BMSettingBluetooth *s_bt;
	const GByteArray *array;
	char *str;
	const char *device_hw_str;
	int addr_match = FALSE;
	const char *bt_type_str;
	guint32 bt_type, bt_capab;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	if (strcmp (bm_setting_connection_get_connection_type (s_con), BM_SETTING_BLUETOOTH_SETTING_NAME)) {
		g_set_error (error, 0, 0,
		             "The connection was not a Bluetooth connection.");
		return FALSE;
	}

	s_bt = BM_SETTING_BLUETOOTH (bm_connection_get_setting (connection, BM_TYPE_SETTING_BLUETOOTH));
	if (!s_bt) {
		g_set_error (error, 0, 0,
		             "The connection was not a valid Bluetooth connection.");
		return FALSE;
	}

	array = bm_setting_bluetooth_get_bdaddr (s_bt);
	if (!array || (array->len != ETH_ALEN)) {
		g_set_error (error, 0, 0,
		             "The connection did not contain a valid Bluetooth address.");
		return FALSE;
	}

	bt_type_str = bm_setting_bluetooth_get_connection_type (s_bt);
	g_assert (bt_type_str);

	bt_type = BM_BT_CAPABILITY_NONE;
	if (!strcmp (bt_type_str, BM_SETTING_BLUETOOTH_TYPE_DUN))
		bt_type = BM_BT_CAPABILITY_DUN;
	else if (!strcmp (bt_type_str, BM_SETTING_BLUETOOTH_TYPE_PANU))
		bt_type = BM_BT_CAPABILITY_NAP;

	bt_capab = bm_device_bt_get_capabilities (device);
	if (!(bt_type & bt_capab)) {
		g_set_error (error, 0, 0,
		             "The connection was not compatible with the device's capabilities.");
		return FALSE;
	}

	device_hw_str = bm_device_bt_get_hw_address (device);

	str = g_strdup_printf ("%02X:%02X:%02X:%02X:%02X:%02X",
	                       array->data[0], array->data[1], array->data[2],
	                       array->data[3], array->data[4], array->data[5]);
	addr_match = !strcmp (device_hw_str, str);
	g_free (str);

	return addr_match;
}

static gboolean
bm_device_is_connection_compatible (BMDevice *device, BMConnection *connection, GError **error)
{
	g_return_val_if_fail (BM_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (BM_IS_CONNECTION (connection), FALSE);

	if (BM_IS_DEVICE_BT (device))
		return check_bt_compatible (BM_DEVICE_BT (device), connection, error);

	g_set_error (error, 0, 0, "unhandled device type '%s'", G_OBJECT_TYPE_NAME (device));
	return FALSE;
}


/**
 * bm_client_get_active_connection_by_path:
 * @client: a #BMClient
 * @object_path: the object path to search for
 *
 * Gets a #BMActiveConnection from a #BMClient.
 *
 * Returns: the #BMActiveConnection for the given @object_path or %NULL if none is found.
 **/
static BMActiveConnection *
bm_client_get_active_connection_by_path (BMClient *client, const char *object_path)
{
	const GPtrArray *actives;
	int i;
	BMActiveConnection *active = NULL;

	g_return_val_if_fail (BM_IS_CLIENT (client), NULL);
	g_return_val_if_fail (object_path, NULL);

	actives = bm_client_get_active_connections (client);
	if (!actives)
		return NULL;

	for (i = 0; i < actives->len; i++) {
		BMActiveConnection *candidate = g_ptr_array_index (actives, i);
		if (!strcmp (bm_object_get_path (BM_OBJECT (candidate)), object_path)) {
			active = candidate;
			break;
		}
	}

	return active;
}
/* -------------------- */

static BMActiveConnection *
get_default_active_connection (BmCli *bmc, BMDevice **device)
{
	BMActiveConnection *default_ac = NULL;
	BMDevice *non_default_device = NULL;
	BMActiveConnection *non_default_ac = NULL;
	const GPtrArray *connections;
	int i;

	g_return_val_if_fail (bmc != NULL, NULL);
	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (*device == NULL, NULL);

	connections = bm_client_get_active_connections (bmc->client);
	for (i = 0; connections && (i < connections->len); i++) {
		BMActiveConnection *candidate = g_ptr_array_index (connections, i);
		const GPtrArray *devices;

		devices = bm_active_connection_get_devices (candidate);
		if (!devices || !devices->len)
			continue;

		if (bm_active_connection_get_default (candidate)) {
			if (!default_ac) {
				*device = g_ptr_array_index (devices, 0);
				default_ac = candidate;
			}
		} else {
			if (!non_default_ac) {
				non_default_device = g_ptr_array_index (devices, 0);
				non_default_ac = candidate;
			}
		}
	}

	/* Prefer the default connection if one exists, otherwise return the first
	 * non-default connection.
	 */
	if (!default_ac && non_default_ac) {
		default_ac = non_default_ac;
		*device = non_default_device;
	}
	return default_ac;
}

/* Find a device to activate the connection on.
 * IN:  connection:  connection to activate
 *      iface:       device interface name to use (optional)
 *      ap:          access point to use (optional; valid just for 802-11-wireless)
 * OUT: device:      found device
 *      spec_object: specific_object path of BMAccessPoint
 * RETURNS: TRUE when a device is found, FALSE otherwise.
 */
static gboolean
find_device_for_connection (BmCli *bmc, BMConnection *connection, const char *iface, const char *ap,
                            BMDevice **device, const char **spec_object, GError **error)
{
	BMSettingConnection *s_con;
	const char *con_type;
	int i, j;

	g_return_val_if_fail (bmc != NULL, FALSE);
	g_return_val_if_fail (device != NULL && *device == NULL, FALSE);
	g_return_val_if_fail (spec_object != NULL && *spec_object == NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
	g_assert (s_con);
	con_type = bm_setting_connection_get_connection_type (s_con);

	if (strcmp (con_type, "vpn") == 0) {
		/* VPN connections */
		BMActiveConnection *active = NULL;
		if (iface) {
			const GPtrArray *connections = bm_client_get_active_connections (bmc->client);
			for (i = 0; connections && (i < connections->len) && !active; i++) {
				BMActiveConnection *candidate = g_ptr_array_index (connections, i);
				const GPtrArray *devices = bm_active_connection_get_devices (candidate);
				if (!devices || !devices->len)
					continue;

				for (j = 0; devices && (j < devices->len); j++) {
					BMDevice *dev = g_ptr_array_index (devices, j);
					if (!strcmp (iface, bm_device_get_iface (dev))) {
						active = candidate;
						*device = dev;
						break;
					}
				}
			}
			if (!active) {
				g_set_error (error, 0, 0, _("no active connection on device '%s'"), iface);
				return FALSE;
			}
			*spec_object = bm_object_get_path (BM_OBJECT (active));
			return TRUE;
		} else {
			active = get_default_active_connection (bmc, device);
			if (!active) {
				g_set_error (error, 0, 0, _("no active connection or device"));
				return FALSE;
			}
			*spec_object = bm_object_get_path (BM_OBJECT (active));
			return TRUE;
		}
	} else {
		/* Other connections */
		BMDevice *found_device = NULL;
		const GPtrArray *devices = bm_client_get_devices (bmc->client);

		for (i = 0; devices && (i < devices->len) && !found_device; i++) {
			BMDevice *dev = g_ptr_array_index (devices, i);

			if (iface) {
				const char *dev_iface = bm_device_get_iface (dev);
				if (   !strcmp (dev_iface, iface)
				    && bm_device_is_connection_compatible (dev, connection, NULL)) {
					found_device = dev;
				}
			} else {
				if (bm_device_is_connection_compatible (dev, connection, NULL)) {
					found_device = dev;
				}
			}

			if (found_device && ap && !strcmp (con_type, "802-11-wireless") && BM_IS_DEVICE_WIFI (dev)) {
				char *hwaddr_up = g_ascii_strup (ap, -1);
				const GPtrArray *aps = bm_device_wifi_get_access_points (BM_DEVICE_WIFI (dev));
				found_device = NULL;  /* Mark as not found; set to the device again later, only if AP matches */

				for (j = 0; aps && (j < aps->len); j++) {
					BMAccessPoint *candidate_ap = g_ptr_array_index (aps, j);
					const char *candidate_hwaddr = bm_access_point_get_hw_address (candidate_ap);

					if (!strcmp (hwaddr_up, candidate_hwaddr)) {
						found_device = dev;
						*spec_object = bm_object_get_path (BM_OBJECT (candidate_ap));
						break;
					}
				}
				g_free (hwaddr_up);
			}
		}

		if (found_device) {
			*device = found_device;
			return TRUE;
		} else {
			if (iface)
				g_set_error (error, 0, 0, _("device '%s' not compatible with connection '%s'"), iface, bm_setting_connection_get_id (s_con));
			else
				g_set_error (error, 0, 0, _("no device found for connection '%s'"), bm_setting_connection_get_id (s_con));
			return FALSE;
		}
	}
}

static const char *
active_connection_state_to_string (BMActiveConnectionState state)
{
	switch (state) {
	case BM_ACTIVE_CONNECTION_STATE_ACTIVATING:
		return _("activating");
	case BM_ACTIVE_CONNECTION_STATE_ACTIVATED:
		return _("activated");
	case BM_ACTIVE_CONNECTION_STATE_UNKNOWN:
	default:
		return _("unknown");
	}
}
 
static void
active_connection_state_cb (BMActiveConnection *active, GParamSpec *pspec, gpointer user_data)
{
	BmCli *bmc = (BmCli *) user_data;
	BMActiveConnectionState state;

	state = bm_active_connection_get_state (active);

	printf (_("state: %s\n"), active_connection_state_to_string (state));

	if (state == BM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
		printf (_("Connection activated\n"));
		quit ();
	} else if (state == BM_ACTIVE_CONNECTION_STATE_UNKNOWN) {
		g_string_printf (bmc->return_text, _("Error: Connection activation failed."));
		bmc->return_value = BMC_RESULT_ERROR_CON_ACTIVATION;
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
foo_active_connections_changed_cb (BMClient *client,
                                   GParamSpec *pspec,
                                   gpointer user_data)
{
	/* Call again activate_connection_cb with dummy arguments;
	 * the correct ones are taken from its first call.
	 */
	activate_connection_cb (NULL, NULL, NULL);
}

static void
activate_connection_cb (gpointer user_data, const char *path, GError *error)
{
	BmCli *bmc = (BmCli *) user_data;
	BMActiveConnection *active;
	BMActiveConnectionState state;
	static gulong handler_id = 0;
	static BmCli *orig_bmc;
	static const char *orig_path;
	static GError *orig_error;

	if (bmc)
	{
		/* Called first time; store actual arguments */
		orig_bmc = bmc;
		orig_path = path;
		orig_error = error;
	}

	/* Disconnect the handler not to be run any more */
	if (handler_id != 0) {
		g_signal_handler_disconnect (orig_bmc->client, handler_id);
		handler_id = 0;
	}

	if (orig_error) {
		g_string_printf (orig_bmc->return_text, _("Error: Connection activation failed: %s"), orig_error->message);
		orig_bmc->return_value = BMC_RESULT_ERROR_CON_ACTIVATION;
		quit ();
	} else {
		active = bm_client_get_active_connection_by_path (orig_bmc->client, orig_path);
		if (!active) {
			/* The active connection path is not in active connections list yet; wait for active-connections signal. */
			/* This is basically the case for VPN connections. */
			if (bmc) {
				/* Called first time, i.e. by bm_client_activate_connection() */
				handler_id = g_signal_connect (orig_bmc->client, "notify::active-connections",
				                               G_CALLBACK (foo_active_connections_changed_cb), NULL);
				return;
			} else {
				g_string_printf (orig_bmc->return_text, _("Error: Obtaining active connection for '%s' failed."), orig_path);
				orig_bmc->return_value = BMC_RESULT_ERROR_CON_ACTIVATION;
				quit ();
				return;
			}
		}

		state = bm_active_connection_get_state (active);

		printf (_("Active connection state: %s\n"), active_connection_state_to_string (state));
		printf (_("Active connection path: %s\n"), orig_path);

		if (orig_bmc->nowait_flag || state == BM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
			/* don't want to wait or already activated */
			quit ();
		} else {
		        g_signal_connect (active, "notify::state", G_CALLBACK (active_connection_state_cb), orig_bmc);

			/* Start timer not to loop forever when signals are not emitted */
			g_timeout_add_seconds (orig_bmc->timeout, timeout_cb, orig_bmc);
		}
	}
}

static BMCResultCode
do_connection_up (BmCli *bmc, int argc, char **argv)
{
	BMDevice *device = NULL;
	const char *spec_object = NULL;
	gboolean device_found;
	BMConnection *connection = NULL;
	BMSettingConnection *s_con;
	gboolean is_system;
	const char *con_path;
	const char *con_type;
	const char *iface = NULL;
	const char *ap = NULL;
	gboolean id_specified = FALSE;
	gboolean wait = TRUE;
	GError *error = NULL;

	/* Set default timeout for connection activation. It can take quite a long time.
	 * Using 90 seconds.
	 */
	bmc->timeout = 90;

	while (argc > 0) {
		if (strcmp (*argv, "id") == 0 || strcmp (*argv, "uuid") == 0) {
			const char *selector = *argv;
			id_specified = TRUE;

			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			if ((connection = find_connection (bmc->system_connections, selector, *argv)) == NULL)
				connection = find_connection (bmc->user_connections, selector, *argv);

			if (!connection) {
				g_string_printf (bmc->return_text, _("Error: Unknown connection: %s."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
				goto error;
			}
		}
		else if (strcmp (*argv, "iface") == 0) {
			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			iface = *argv;
		}
		else if (strcmp (*argv, "ap") == 0) {
			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			ap = *argv;
		}
		else if (strcmp (*argv, "--nowait") == 0) {
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

	if (!id_specified) {
		g_string_printf (bmc->return_text, _("Error: id or uuid has to be specified."));
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

	/* create BMClient */
	bmc->get_client (bmc);

	is_system = (bm_connection_get_scope (connection) == BM_CONNECTION_SCOPE_SYSTEM) ? TRUE : FALSE;
	con_path = bm_connection_get_path (connection);

	s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
	g_assert (s_con);
	con_type = bm_setting_connection_get_connection_type (s_con);

	device_found = find_device_for_connection (bmc, connection, iface, ap, &device, &spec_object, &error);

	if (!device_found) {
		if (error)
			g_string_printf (bmc->return_text, _("Error: No suitable device found: %s."), error->message);
		else
			g_string_printf (bmc->return_text, _("Error: No suitable device found."));
		bmc->return_value = BMC_RESULT_ERROR_CON_ACTIVATION;
		g_clear_error (&error);
		goto error;
	}

	/* Use nowait_flag instead of should_wait because exitting has to be postponed till active_connection_state_cb()
	 * is called, giving BM time to check our permissions */
	bmc->nowait_flag = !wait;
	bmc->should_wait = TRUE;
	bm_client_activate_connection (bmc->client,
	                               is_system ? BM_DBUS_SERVICE_SYSTEM_SETTINGS : BM_DBUS_SERVICE_USER_SETTINGS,
	                               con_path,
	                               device,
	                               spec_object,
	                               activate_connection_cb,
	                               bmc);

	return bmc->return_value;
error:
	bmc->should_wait = FALSE;
	return bmc->return_value;
}

static BMCResultCode
do_connection_down (BmCli *bmc, int argc, char **argv)
{
	BMConnection *connection = NULL;
	BMActiveConnection *active = NULL;
	GError *error = NULL;
	const GPtrArray *active_cons;
	const char *con_path;
	const char *active_path;
	BMConnectionScope active_service_scope, con_scope;
	gboolean id_specified = FALSE;
	gboolean wait = TRUE;
	int i;

	while (argc > 0) {
		if (strcmp (*argv, "id") == 0 || strcmp (*argv, "uuid") == 0) {
			const char *selector = *argv;
			id_specified = TRUE;

			if (next_arg (&argc, &argv) != 0) {
				g_string_printf (bmc->return_text, _("Error: %s argument is missing."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				goto error;
			}

			if ((connection = find_connection (bmc->system_connections, selector, *argv)) == NULL)
				connection = find_connection (bmc->user_connections, selector, *argv);

			if (!connection) {
				g_string_printf (bmc->return_text, _("Error: Unknown connection: %s."), *argv);
				bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
				goto error;
			}
		}
		else if (strcmp (*argv, "--nowait") == 0) {
			wait = FALSE;
		}
		else {
			fprintf (stderr, _("Unknown parameter: %s\n"), *argv);
		}

		argc--;
		argv++;
	}

	if (!id_specified) {
		g_string_printf (bmc->return_text, _("Error: id or uuid has to be specified."));
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

	/* create BMClient */
	bmc->get_client (bmc);

	con_path = bm_connection_get_path (connection);
	con_scope = bm_connection_get_scope (connection);

	active_cons = bm_client_get_active_connections (bmc->client);
	for (i = 0; active_cons && (i < active_cons->len); i++) {
		BMActiveConnection *candidate = g_ptr_array_index (active_cons, i);

		active_path = bm_active_connection_get_connection (candidate);
		active_service_scope = bm_active_connection_get_scope (candidate);
		if (!strcmp (active_path, con_path) && active_service_scope == con_scope) {
			active = candidate;
			break;
		}
	}

	if (active)
		vm_client_deactivate_connection (bmc->client, active);
	else {
		fprintf (stderr, _("Warning: Connection not active\n"));
	}
	sleep (1);  /* Don't quit immediatelly and give BM time to check our permissions */

error:
	bmc->should_wait = FALSE;
	return bmc->return_value;
}

/* callback called when connections are obtained from the settings service */
static void
get_connections_cb (BMSettingsInterface *settings, gpointer user_data)
{
	ArgsInfo *args = (ArgsInfo *) user_data;
	static gboolean system_cb_called = FALSE;
	static gboolean user_cb_called = FALSE;
	GError *error = NULL;

	if (BM_IS_REMOTE_SETTINGS_SYSTEM (settings)) {
		system_cb_called = TRUE;
		args->bmc->system_connections = bm_settings_interface_list_connections (settings);
	}
	else {
		user_cb_called = TRUE;
		args->bmc->user_connections = bm_settings_interface_list_connections (settings);
	}

	/* return and wait for the callback of the second settings is called */
	if (   (args->bmc->system_settings_running && !system_cb_called)
	    || (args->bmc->user_settings_running && !user_cb_called))
		return;

	if (args->argc == 0) {
		if (!bmc_terse_option_check (args->bmc->print_output, args->bmc->required_fields, &error))
			goto opt_error;
		args->bmc->return_value = do_connections_list (args->bmc, args->argc, args->argv);
	} else {

	 	if (matches (*args->argv, "list") == 0) {
			args->bmc->return_value = do_connections_list (args->bmc, args->argc-1, args->argv+1);
		}
		else if (matches(*args->argv, "status") == 0) {
			if (!bmc_terse_option_check (args->bmc->print_output, args->bmc->required_fields, &error))
				goto opt_error;
			args->bmc->return_value = do_connections_status (args->bmc, args->argc-1, args->argv+1);
		}
		else if (matches(*args->argv, "up") == 0) {
			args->bmc->return_value = do_connection_up (args->bmc, args->argc-1, args->argv+1);
		}
		else if (matches(*args->argv, "down") == 0) {
			args->bmc->return_value = do_connection_down (args->bmc, args->argc-1, args->argv+1);
		}
		else if (matches (*args->argv, "help") == 0) {
			usage ();
			args->bmc->should_wait = FALSE;
		} else {
			usage ();
			g_string_printf (args->bmc->return_text, _("Error: 'con' command '%s' is not valid."), *args->argv);
			args->bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
			args->bmc->should_wait = FALSE;
		}
	}

	if (!args->bmc->should_wait)
		quit ();
	return;

opt_error:
	g_string_printf (args->bmc->return_text, _("Error: %s."), error->message);
	args->bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
	args->bmc->should_wait = FALSE;
	g_error_free (error);
	quit ();
	return;
}


/* Entry point function for connections-related commands: 'bmcli con' */
BMCResultCode
do_connections (BmCli *bmc, int argc, char **argv)
{
	DBusGConnection *bus;
	GError *error = NULL;

	bmc->should_wait = TRUE;

	args_info.bmc = bmc;
	args_info.argc = argc;
	args_info.argv = argv;

	/* connect to DBus' system bus */
	bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (error || !bus) {
		g_string_printf (bmc->return_text, _("Error: could not connect to D-Bus."));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		return bmc->return_value;
	}

	/* get system settings */
	if (!(bmc->system_settings = bm_remote_settings_system_new (bus))) {
		g_string_printf (bmc->return_text, _("Error: Could not get system settings."));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		return bmc->return_value;

	}

	/* get user settings */
	if (!(bmc->user_settings = bm_remote_settings_new (bus, BM_CONNECTION_SCOPE_USER))) {
		g_string_printf (bmc->return_text, _("Error: Could not get user settings."));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		return bmc->return_value;
	}

	/* find out whether setting services are running */
	g_object_get (bmc->system_settings, BM_REMOTE_SETTINGS_SERVICE_RUNNING, &bmc->system_settings_running, NULL);
	g_object_get (bmc->user_settings, BM_REMOTE_SETTINGS_SERVICE_RUNNING, &bmc->user_settings_running, NULL);

	if (!bmc->system_settings_running && !bmc->user_settings_running) {
		g_string_printf (bmc->return_text, _("Error: Can't obtain connections: settings services are not running."));
		bmc->return_value = BMC_RESULT_ERROR_UNKNOWN;
		return bmc->return_value;
	}

	/* connect to signal "connections-read" - emitted when connections are fetched and ready */
	if (bmc->system_settings_running)
		g_signal_connect (bmc->system_settings, BM_SETTINGS_INTERFACE_CONNECTIONS_READ,
		                  G_CALLBACK (get_connections_cb), &args_info);

	if (bmc->user_settings_running)
		g_signal_connect (bmc->user_settings, BM_SETTINGS_INTERFACE_CONNECTIONS_READ,
		                  G_CALLBACK (get_connections_cb), &args_info);

	dbus_g_connection_unref (bus);

	/* The rest will be done in get_connection_cb() callback.
	 * We need to wait for signals that connections are read.
	 */
	return BMC_RESULT_SUCCESS;
}
