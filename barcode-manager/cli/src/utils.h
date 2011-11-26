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
 * (C) Copyright 2010 - 2011 Red Hat, Inc.
 */

#ifndef BMC_UTILS_H
#define BMC_UTILS_H

#include <glib.h>

#include "bmcli.h"

/* === Functions === */
int matches (const char *cmd, const char *pattern);
int next_arg (int *argc, char ***argv);
int bmc_string_screen_width (const char *start, const char *end);
GArray *parse_output_fields (const char *fields_str, const  BmcOutputField fields_array[], GError **error);
gboolean bmc_terse_option_check (BMCPrintOutput print_output, const char *fields, GError **error);
void print_fields (const BmcPrintFields fields, const BmcOutputField field_values[]);
gboolean bmc_is_bm_running (BmCli *nmc, GError **error);

#endif /* BMC_UTILS_H */
