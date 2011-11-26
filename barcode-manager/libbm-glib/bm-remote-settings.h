/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access network status & information from glib applications
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
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2009 Red Hat, Inc.
 */

#ifndef BM_REMOTE_SETTINGS_H
#define BM_REMOTE_SETTINGS_H

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <bm-connection.h>
#include <bm-remote-connection.h>

G_BEGIN_DECLS

#define BM_TYPE_REMOTE_SETTINGS            (bm_remote_settings_get_type ())
#define BM_REMOTE_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_REMOTE_SETTINGS, NMRemoteSettings))
#define BM_REMOTE_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_REMOTE_SETTINGS, NMRemoteSettingsClass))
#define BM_IS_REMOTE_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_REMOTE_SETTINGS))
#define BM_IS_REMOTE_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_REMOTE_SETTINGS))
#define BM_REMOTE_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_REMOTE_SETTINGS, NMRemoteSettingsClass))

#define BM_REMOTE_SETTINGS_BUS "bus"
#define BM_REMOTE_SETTINGS_SCOPE "scope"
#define BM_REMOTE_SETTINGS_SERVICE_RUNNING "service-running"

typedef struct {
	GObject parent;
} NMRemoteSettings;

typedef struct {
	GObjectClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} NMRemoteSettingsClass;

GType bm_remote_settings_get_type (void);

NMRemoteSettings *bm_remote_settings_new (DBusGConnection *bus, BMConnectionScope scope);

G_END_DECLS

#endif /* BM_REMOTE_SETTINGS_H */
