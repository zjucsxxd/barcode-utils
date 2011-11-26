/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager system settings service
 *
 * Søren Sandmann <sandmann@daimi.au.dk>
 * Dan Williams <dcbw@redhat.com>
 * Tambet Ingo <tambet@gmail.com>
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
 * (C) Copyright 2007 - 2009 Red Hat, Inc.
 * (C) Copyright 2008 Novell, Inc.
 */

#ifndef __BM_SYSCONFIG_SETTINGS_H__
#define __BM_SYSCONFIG_SETTINGS_H__

#include <bm-connection.h>
#include <bm-settings-service.h>

#include "bm-sysconfig-connection.h"
#include "bm-system-config-interface.h"
#include "bm-device.h"

#define BM_TYPE_SYSCONFIG_SETTINGS            (bm_sysconfig_settings_get_type ())
#define BM_SYSCONFIG_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SYSCONFIG_SETTINGS, BMSysconfigSettings))
#define BM_SYSCONFIG_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BM_TYPE_SYSCONFIG_SETTINGS, BMSysconfigSettingsClass))
#define BM_IS_SYSCONFIG_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SYSCONFIG_SETTINGS))
#define BM_IS_SYSCONFIG_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BM_TYPE_SYSCONFIG_SETTINGS))
#define BM_SYSCONFIG_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BM_TYPE_SYSCONFIG_SETTINGS, BMSysconfigSettingsClass))

#define BM_SYSCONFIG_SETTINGS_UNMANAGED_SPECS "unmanaged-specs"
#define BM_SYSCONFIG_SETTINGS_TIMESTAMPS_FILE LOCALSTATEDIR"/lib/BarcodeManager/timestamps"
#define BM_SYSCONFIG_SETTINGS_TIMESTAMP_TAG   "timestamp-tag"

typedef struct {
	BMSettingsService parent_instance;
} BMSysconfigSettings;

typedef struct {
	BMSettingsServiceClass parent_class;

	/* Signals */
	void (*properties_changed) (BMSysconfigSettings *self, GHashTable *properties);
} BMSysconfigSettingsClass;

GType bm_sysconfig_settings_get_type (void);

BMSysconfigSettings *bm_sysconfig_settings_new (const char *config_file,
                                                const char *plugins,
                                                DBusGConnection *bus,
                                                GError **error);

const GSList *bm_sysconfig_settings_get_unmanaged_specs (BMSysconfigSettings *self);

char *bm_sysconfig_settings_get_hostname (BMSysconfigSettings *self);

void bm_sysconfig_settings_device_added (BMSysconfigSettings *self, BMDevice *device);

void bm_sysconfig_settings_device_removed (BMSysconfigSettings *self, BMDevice *device);

#endif  /* __BM_SYSCONFIG_SETTINGS_H__ */
