/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager system settings service
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
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 */

#ifndef BM_SYSTEM_CONFIG_ERROR_H
#define BM_SYSTEM_CONFIG_ERROR_H

#include <glib.h>
#include <glib-object.h>

enum {
	BM_SYSCONFIG_SETTINGS_ERROR_GENERAL = 0,
	BM_SYSCONFIG_SETTINGS_ERROR_NOT_PRIVILEGED,
	BM_SYSCONFIG_SETTINGS_ERROR_ADD_NOT_SUPPORTED,
	BM_SYSCONFIG_SETTINGS_ERROR_UPDATE_NOT_SUPPORTED,
	BM_SYSCONFIG_SETTINGS_ERROR_DELETE_NOT_SUPPORTED,
	BM_SYSCONFIG_SETTINGS_ERROR_ADD_FAILED,
	BM_SYSCONFIG_SETTINGS_ERROR_SAVE_HOSTNAME_NOT_SUPPORTED,
	BM_SYSCONFIG_SETTINGS_ERROR_SAVE_HOSTNAME_FAILED,
};

#define BM_SYSCONFIG_SETTINGS_ERROR (bm_sysconfig_settings_error_quark ())
#define BM_TYPE_SYSCONFIG_SETTINGS_ERROR (bm_sysconfig_settings_error_get_type ())

GQuark bm_sysconfig_settings_error_quark    (void);
GType  bm_sysconfig_settings_error_get_type (void);

#endif /* BM_SYSTEM_CONFIG_ERROR_H */
