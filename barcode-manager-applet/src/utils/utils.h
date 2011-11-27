/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Applet -- Display barcode scanners and allow user control
 *
 * Jakob Flierl <jakob.flierl@gmail.com>
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

#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <bm-connection.h>
#include <bm-device.h>

const char *utils_get_device_description (BMDevice *device);

GSList *utils_filter_connections_for_device (BMDevice *device, GSList *connections);

char *utils_next_available_name (GSList *connections, const char *format);

char *utils_escape_notify_message (const char *src);

#endif /* UTILS_H */

