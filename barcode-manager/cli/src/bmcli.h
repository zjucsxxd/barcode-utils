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

#ifndef BMC_BMCLI_H
#define BMC_BMCLI_H

#include <glib.h>

#include <bm-client.h>
#include <bm-remote-settings.h>
#include <bm-remote-settings-system.h>

/* bmcli exit codes */
typedef enum {
	/* Indicates successful execution */
	BMC_RESULT_SUCCESS = 0,

	/* Unknown / unspecified error */
	BMC_RESULT_ERROR_UNKNOWN = 1,

	/* Wrong invocation of bmcli */
	BMC_RESULT_ERROR_USER_INPUT = 2,

	/* A timeout expired */
	BMC_RESULT_ERROR_TIMEOUT_EXPIRED = 3,

	/* Error in connection activation */
	BMC_RESULT_ERROR_CON_ACTIVATION = 4,

	/* Error in connection deactivation */
	BMC_RESULT_ERROR_CON_DEACTIVATION = 5,

	/* Error in device disconnect */
	BMC_RESULT_ERROR_DEV_DISCONNECT = 6,

	/* BarcodeManager is not running */
	BMC_RESULT_ERROR_BM_NOT_RUNNING = 7
} BMCResultCode;

typedef enum {
	BMC_PRINT_TERSE = 0,
	BMC_PRINT_NORMAL = 1,
	BMC_PRINT_PRETTY = 2
} BMCPrintOutput;

/* === Output fields === */
typedef struct {
	const char *name;       /* Field's name */
	const char *name_l10n;  /* Field's name for translation */
	int width;              /* Width in screen columns */
	const char *value;      /* Value of current field */
	guint32 flags;          /* Flags */
} BmcOutputField;

/* Flags for BmcPrintFields */
#define	BMC_PF_FLAG_MULTILINE          0x00000001   /* Multiline output instead of tabular */
#define	BMC_PF_FLAG_TERSE              0x00000002   /* Terse output mode */
#define	BMC_PF_FLAG_PRETTY             0x00000004   /* Pretty output mode */
#define	BMC_PF_FLAG_MAIN_HEADER_ADD    0x00000008   /* Print main header in addition to values/field names */
#define	BMC_PF_FLAG_MAIN_HEADER_ONLY   0x00000010   /* Print main header only */
#define	BMC_PF_FLAG_FIELD_NAMES        0x00000020   /* Print field names instead of values */
#define	BMC_PF_FLAG_ESCAPE             0x00000040   /* Escape column separator and '\' */
#define	BMC_PF_FLAG_SECTION_PREFIX     0x00000080   /* Use the first value as section prefix for the other field names - just in multiline */

typedef struct {
	GArray *indices;      /* Array of field indices to the array of allowed fields */
	char *header_name;    /* Name of the output */
	int indent;           /* Indent by this number of spaces */
	guint32 flags;        /* Various flags for controlling output: see BMC_PF_FLAG_* values */
} BmcPrintFields;

/* BmCli - main structure */
typedef struct _BmCli {
	BMClient *client;                                 /* Pointer to BMClient of libbm-glib */
	BMClient *(*get_client) (struct _BmCli *bmc);     /* Pointer to function for creating BMClient */

	BMCResultCode return_value;                       /* Return code of bmcli */
	GString *return_text;                             /* Reason text */

	int timeout;                                      /* Operation timeout */

	BMRemoteSettingsSystem *system_settings;          /* System settings */
	BMRemoteSettings *user_settings;                  /* User settings */

	gboolean system_settings_running;                 /* Is system settings service running? */
	gboolean user_settings_running;                   /* Is user settings service running? */

	GSList *system_connections;                       /* List of system connections */
	GSList *user_connections;                         /* List of user connections */

	gboolean should_wait;                             /* Indication that bmcli should not end yet */
	gboolean nowait_flag;                             /* '--nowait' option; used for passing to callbacks */
	BMCPrintOutput print_output;                      /* Output mode */
	gboolean multiline_output;                        /* Multiline output instead of default tabular */
	gboolean mode_specified;                          /* Whether tabular/multiline mode was specified via '--mode' option */
	gboolean escape_values;                           /* Whether to escape ':' and '\' in terse tabular mode */
	char *required_fields;                            /* Required fields in output: '--fields' option */
	BmcOutputField *allowed_fields;                   /* Array of allowed fields for particular commands */
	BmcPrintFields print_fields;                      /* Structure with field indices to print */
} BmCli;

#endif /* BMC_BMCLI_H */
