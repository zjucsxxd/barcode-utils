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
#include <libbm-util/bm-utils.h>

#include "utils.h"
#include "settings.h"


/* Helper macro to define fields */
#define SETTING_FIELD(setting, width) { setting, N_(setting), width, NULL, 0 }

/* Available fields for BM_SETTING_CONNECTION_SETTING_NAME */
static BmcOutputField bmc_fields_setting_connection[] = {
	SETTING_FIELD ("name",  15),                            /* 0 */
	SETTING_FIELD (BM_SETTING_CONNECTION_ID, 25),           /* 1 */
	SETTING_FIELD (BM_SETTING_CONNECTION_UUID, 38),         /* 2 */
	SETTING_FIELD (BM_SETTING_CONNECTION_TYPE, 17),         /* 3 */
	SETTING_FIELD (BM_SETTING_CONNECTION_AUTOCONNECT, 13),  /* 4 */
	SETTING_FIELD (BM_SETTING_CONNECTION_TIMESTAMP, 10),    /* 5 */
	SETTING_FIELD (BM_SETTING_CONNECTION_READ_ONLY, 10),    /* 6 */
	{NULL, NULL, 0, NULL, 0}
};
#define BMC_FIELDS_SETTING_CONNECTION_ALL     "name"","\
                                              BM_SETTING_CONNECTION_ID","\
                                              BM_SETTING_CONNECTION_UUID","\
                                              BM_SETTING_CONNECTION_TYPE","\
                                              BM_SETTING_CONNECTION_AUTOCONNECT","\
                                              BM_SETTING_CONNECTION_TIMESTAMP","\
                                              BM_SETTING_CONNECTION_READ_ONLY
#define BMC_FIELDS_SETTING_CONNECTION_COMMON  BMC_FIELDS_SETTING_CONNECTION_ALL

/* Available fields for BM_SETTING_SERIAL_SETTING_NAME */
static BmcOutputField bmc_fields_setting_serial[] = {
	SETTING_FIELD ("name", 10),                                        /* 0 */
	SETTING_FIELD (BM_SETTING_SERIAL_BAUD, 10),                        /* 1 */
	SETTING_FIELD (BM_SETTING_SERIAL_BITS, 10),                        /* 2 */
	SETTING_FIELD (BM_SETTING_SERIAL_PARITY, 10),                      /* 3 */
	SETTING_FIELD (BM_SETTING_SERIAL_STOPBITS, 10),                    /* 4 */
	SETTING_FIELD (BM_SETTING_SERIAL_SEND_DELAY, 12),                  /* 5 */
	{NULL, NULL, 0, NULL, 0}
};
#define BMC_FIELDS_SETTING_SERIAL_ALL     "name"","\
                                          BM_SETTING_SERIAL_BAUD","\
                                          BM_SETTING_SERIAL_BITS","\
                                          BM_SETTING_SERIAL_PARITY","\
                                          BM_SETTING_SERIAL_STOPBITS","\
                                          BM_SETTING_SERIAL_SEND_DELAY
#define BMC_FIELDS_SETTING_SERIAL_COMMON  BBMC_FIELDS_SETTING_SERIAL_ALL

/* Available fields for BM_SETTING_BLUETOOTH_SETTING_NAME */
static BmcOutputField bmc_fields_setting_bluetooth[] = {
	SETTING_FIELD ("name", 11),                                        /* 0 */
	SETTING_FIELD (BM_SETTING_BLUETOOTH_BDADDR, 19),                   /* 1 */
	SETTING_FIELD (BM_SETTING_BLUETOOTH_TYPE, 10),                     /* 2 */
	{NULL, NULL, 0, NULL, 0}
};
#define BMC_FIELDS_SETTING_BLUETOOTH_ALL     "name"","\
                                             BM_SETTING_BLUETOOTH_BDADDR","\
                                             BM_SETTING_BLUETOOTH_TYPE
#define BMC_FIELDS_SETTING_BLUETOOTH_COMMON  BMC_FIELDS_SETTING_BLUETOOTH_ALL

gboolean
setting_connection_details (BMSetting *setting, BmCli *bmc)
{
	BMSettingConnection *s_con;
	guint64 timestamp;
	char *timestamp_str;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	g_return_val_if_fail (BM_IS_SETTING_CONNECTION (setting), FALSE);
	s_con = (BMSettingConnection *) setting;

	bmc->allowed_fields = bmc_fields_setting_connection;
	bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_SETTING_CONNECTION_ALL, bmc->allowed_fields, NULL);
	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_FIELD_NAMES;
	print_fields (bmc->print_fields, bmc->allowed_fields);  /* Print field names */

	timestamp = bm_setting_connection_get_timestamp (s_con);
	timestamp_str = g_strdup_printf ("%" G_GUINT64_FORMAT, timestamp);

	bmc->allowed_fields[0].value = BM_SETTING_CONNECTION_SETTING_NAME;
	bmc->allowed_fields[1].value = bm_setting_connection_get_id (s_con);
	bmc->allowed_fields[2].value = bm_setting_connection_get_uuid (s_con);
	bmc->allowed_fields[3].value = bm_setting_connection_get_connection_type (s_con);
	bmc->allowed_fields[4].value = bm_setting_connection_get_autoconnect (s_con) ? _("yes") : _("no");
	bmc->allowed_fields[5].value = timestamp_str;
	bmc->allowed_fields[6].value = bm_setting_connection_get_read_only (s_con) ? ("yes") : _("no");

	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_SECTION_PREFIX;
	print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */

	g_free (timestamp_str);

	return TRUE;
}

gboolean
setting_serial_details (BMSetting *setting, BmCli *bmc)
{
	BMSettingSerial *s_serial;
	char *baud_str, *bits_str, *parity_str, *stopbits_str, *send_delay_str;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	g_return_val_if_fail (BM_IS_SETTING_SERIAL (setting), FALSE);
	s_serial = (BMSettingSerial *) setting;

	bmc->allowed_fields = bmc_fields_setting_serial;
	bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_SETTING_SERIAL_ALL, bmc->allowed_fields, NULL);
	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_FIELD_NAMES;
	print_fields (bmc->print_fields, bmc->allowed_fields);  /* Print field names */

	baud_str = g_strdup_printf ("%d", bm_setting_serial_get_baud (s_serial));
	bits_str = g_strdup_printf ("%d", bm_setting_serial_get_bits (s_serial));
	parity_str = g_strdup_printf ("%c", bm_setting_serial_get_parity (s_serial));
	stopbits_str = g_strdup_printf ("%d", bm_setting_serial_get_stopbits (s_serial));
	send_delay_str = g_strdup_printf ("%" G_GUINT64_FORMAT, bm_setting_serial_get_send_delay (s_serial));

	bmc->allowed_fields[0].value = BM_SETTING_SERIAL_SETTING_NAME;
	bmc->allowed_fields[1].value = baud_str;
	bmc->allowed_fields[2].value = bits_str;
	bmc->allowed_fields[3].value = parity_str;
	bmc->allowed_fields[4].value = stopbits_str;
	bmc->allowed_fields[5].value = send_delay_str;

	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_SECTION_PREFIX;
	print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */

	g_free (baud_str);
	g_free (bits_str);
	g_free (parity_str);
	g_free (stopbits_str);
	g_free (send_delay_str);

	return TRUE;
}

gboolean
setting_bluetooth_details (BMSetting *setting, BmCli *bmc)
{
	BMSettingBluetooth *s_bluetooth;
	const GByteArray *bdaddr;
	char *bdaddr_str = NULL;
	guint32 mode_flag = (bmc->print_output == BMC_PRINT_PRETTY) ? BMC_PF_FLAG_PRETTY : (bmc->print_output == BMC_PRINT_TERSE) ? BMC_PF_FLAG_TERSE : 0;
	guint32 multiline_flag = bmc->multiline_output ? BMC_PF_FLAG_MULTILINE : 0;
	guint32 escape_flag = bmc->escape_values ? BMC_PF_FLAG_ESCAPE : 0;

	g_return_val_if_fail (BM_IS_SETTING_BLUETOOTH (setting), FALSE);
	s_bluetooth = (BMSettingBluetooth *) setting;

	bmc->allowed_fields = bmc_fields_setting_bluetooth;
	bmc->print_fields.indices = parse_output_fields (BMC_FIELDS_SETTING_BLUETOOTH_ALL, bmc->allowed_fields, NULL);
	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_FIELD_NAMES;
	print_fields (bmc->print_fields, bmc->allowed_fields);  /* Print field names */

	bdaddr = bm_setting_bluetooth_get_bdaddr (s_bluetooth);
	if (bdaddr)
		bdaddr_str = g_strdup_printf ("%02X:%02X:%02X:%02X:%02X:%02X", bdaddr->data[0], bdaddr->data[1], bdaddr->data[2],
		                                                               bdaddr->data[3], bdaddr->data[4], bdaddr->data[5]);
	bmc->allowed_fields[0].value = BM_SETTING_BLUETOOTH_SETTING_NAME;
	bmc->allowed_fields[1].value = bdaddr_str;
	bmc->allowed_fields[2].value = bm_setting_bluetooth_get_connection_type (s_bluetooth);

	bmc->print_fields.flags = multiline_flag | mode_flag | escape_flag | BMC_PF_FLAG_SECTION_PREFIX;
	print_fields (bmc->print_fields, bmc->allowed_fields); /* Print values */

	g_free (bdaddr_str);

	return TRUE;
}
