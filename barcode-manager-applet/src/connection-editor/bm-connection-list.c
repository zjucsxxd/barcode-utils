/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Scanner editor -- Scanner editor for BarcodeManager
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

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>

#include <bm-setting-connection.h>
#include <bm-connection.h>
#include <bm-setting.h>
#include <bm-setting-serial.h>
#include <bm-utils.h>
#include <bm-settings-system-interface.h>

#include "bm-connection-editor.h"
#include "bm-connection-list.h"
#include "gconf-helpers.h"
#include "utils.h"
#include "ce-polkit-button.h"

//G_DEFINE_TYPE (BMConnectionList, bm_connection_list, G_TYPE_OBJECT)

enum {
	LIST_DONE,
	LIST_LAST_SIGNAL
};

#define COL_ID 			0
#define COL_LAST_USED	1
#define COL_TIMESTAMP	2
#define COL_CONNECTION	3

typedef struct {
	BMConnectionList *list;
	GtkTreeView *treeview;
	GtkWindow *list_window;
	GtkWidget *button;
	//	PageNewConnectionFunc new_func;
} ActionInfo;

