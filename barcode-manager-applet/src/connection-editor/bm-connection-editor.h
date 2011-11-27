/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Connection editor -- Connection editor for BarcodeManager
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
 * (C) Copyright 2007 Rodrigo Moya <rodrigo@gnome-db.org>
 * (C) Copyright 2007 - 2010 Red Hat, Inc.
 */

#ifndef BM_CONNECTION_EDITOR_H
#define BM_CONNECTION_EDITOR_H

#include "config.h"

#include <glib-object.h>

#include "bm-remote-settings-system.h"

#define BM_TYPE_CONNECTION_EDITOR    (bm_connection_editor_get_type ())
#define BM_IS_CONNECTION_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_CONNECTION_EDITOR))
#define BM_CONNECTION_EDITOR(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_CONNECTION_EDITOR, BMConnectionEditor))

typedef struct GetSecretsInfo GetSecretsInfo;

typedef struct {
	GObject parent;
	gboolean disposed;

	/* private data */
	BMConnection *connection;
	BMConnection *orig_connection;

	BMConnectionScope orig_scope;

	GetSecretsInfo *secrets_call;
	GSList *pending_secrets_calls;

	GtkWidget *system_checkbutton;
	gboolean system_settings_can_modify;

	GSList *initializing_pages;
	GSList *pages;
	GtkBuilder *builder;
	GtkWidget *window;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	gboolean busy;
	gboolean init_run;
} BMConnectionEditor;

typedef struct {
	GObjectClass parent_class;

	/* Signals */
	void (*done)  (BMConnectionEditor *editor, gint result, GError *error);
} BMConnectionEditorClass;

GType               bm_connection_editor_get_type (void);
BMConnectionEditor *bm_connection_editor_new (BMConnection *connection,
                                              BMRemoteSettingsSystem *settings,
                                              GError **error);

void                bm_connection_editor_present (BMConnectionEditor *editor);
void                bm_connection_editor_run (BMConnectionEditor *editor);
void                bm_connection_editor_save_vpn_secrets (BMConnectionEditor *editor);
BMConnection *      bm_connection_editor_get_connection (BMConnectionEditor *editor);
gboolean            bm_connection_editor_update_connection (BMConnectionEditor *editor, GError **error);
GtkWindow *         bm_connection_editor_get_window (BMConnectionEditor *editor);
gboolean            bm_connection_editor_get_busy (BMConnectionEditor *editor);
void                bm_connection_editor_set_busy (BMConnectionEditor *editor, gboolean busy);

#endif
