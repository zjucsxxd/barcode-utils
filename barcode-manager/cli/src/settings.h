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

#ifndef BMC_SETTINGS_H
#define BMC_SETTINGS_H

#include <bm-setting-connection.h>
#include <bm-setting-serial.h>
// #include <bm-setting-usb.h>
#include <bm-setting-bluetooth.h>

#include "bmcli.h"
#include "utils.h"


gboolean setting_connection_details (BMSetting *setting, BmCli *bmc);
gboolean setting_serial_details (BMSetting *setting, BmCli *bmc);
gboolean setting_usb_details (BMSetting *setting, BmCli *bmc);
gboolean setting_bluetooth_details (BMSetting *setting, BmCli *bmc);

#endif /* BMC_SETTINGS_H */
