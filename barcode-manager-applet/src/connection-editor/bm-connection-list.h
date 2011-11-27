/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Connection editor -- Connection editor for BarcodeManager
 *
 * Rodrigo Moya <rodrigo@gnome-db.org>
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
 * (C) Copyright 2004-2009 Red Hat, Inc.
 */

#ifndef BM_CONNECTION_LIST_H
#define BM_CONNECTION_LIST_H

#include <glib-object.h>
#include <gconf/gconf-client.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <bm-remote-settings-system.h>
#include "bma-gconf-settings.h"

#define BM_TYPE_CONNECTION_LIST    (bm_connection_list_get_type ())
#define BM_IS_CONNECTION_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_CONNECTION_LIST))
#define BM_CONNECTION_LIST(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_CONNECTION_LIST, BMConnectionList))

typedef struct {
	GObject parent;

	/* private data */
	GHashTable *editors;
	GSList *treeviews;

	GConfClient *client;
	BMAGConfSettings *gconf_settings;
	BMRemoteSettingsSystem *system_settings;

	GtkBuilder *gui;
	GtkWidget *dialog;

	GdkPixbuf *wired_icon;
	GdkPixbuf *wireless_icon;
	GdkPixbuf *wwan_icon;
	GdkPixbuf *vpn_icon;
	GdkPixbuf *unknown_icon;
	GtkIconTheme *icon_theme;
} BMConnectionList;

typedef struct {
	GObjectClass parent_class;

	/* Signals */
	void (*done)  (BMConnectionList *list, gint result);
} BMConnectionListClass;

GType             bm_connection_list_get_type (void);
BMConnectionList *bm_connection_list_new (GType def_type);

void              bm_connection_list_run (BMConnectionList *list);

void              bm_connection_list_set_type (BMConnectionList *list, GType ctype);

void              bm_connection_list_present (BMConnectionList *list);

#endif
