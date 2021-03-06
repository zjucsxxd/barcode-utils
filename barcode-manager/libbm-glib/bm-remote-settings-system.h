/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access barcode scanner hardware & information from glib applications
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2011 Jakob Flierl
 */

#ifndef BM_REMOTE_SETTINGS_SYSTEM_H
#define BM_REMOTE_SETTINGS_SYSTEM_H

#include <glib.h>
#include <dbus/dbus-glib.h>

#include "bm-remote-settings.h"
#include "bm-settings-system-interface.h"

G_BEGIN_DECLS

#define BM_TYPE_REMOTE_SETTINGS_SYSTEM            (bm_remote_settings_system_get_type ())
#define BM_REMOTE_SETTINGS_SYSTEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_REMOTE_SETTINGS_SYSTEM, BMRemoteSettingsSystem))
#define BM_REMOTE_SETTINGS_SYSTEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_REMOTE_SETTINGS_SYSTEM, BMRemoteSettingsSystemClass))
#define BM_IS_REMOTE_SETTINGS_SYSTEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_REMOTE_SETTINGS_SYSTEM))
#define BM_IS_REMOTE_SETTINGS_SYSTEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_REMOTE_SETTINGS_SYSTEM))
#define BM_REMOTE_SETTINGS_SYSTEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_REMOTE_SETTINGS_SYSTEM, BMRemoteSettingsSystemClass))

typedef struct {
	BMRemoteSettings parent;
} BMRemoteSettingsSystem;

typedef struct {
	BMRemoteSettingsClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} BMRemoteSettingsSystemClass;

GType bm_remote_settings_system_get_type (void);

BMRemoteSettingsSystem *bm_remote_settings_system_new (DBusGConnection *bus);

G_END_DECLS

#endif /* BM_REMOTE_SETTINGS_SYSTEM_H */
