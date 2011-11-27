/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Wireless Applet -- Display wireless access points and allow user control
 *
 * Dan Williams <dcbw@redhat.com>
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
 * (C) Copyright 2008 Red Hat, Inc.
 * (C) Copyright 2008 Novell, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <bm-device.h>
#include <bm-setting-connection.h>
#include <bm-setting-bluetooth.h>
#include <bm-device-bt.h>
#include <bm-utils.h>

#include "applet.h"
#include "applet-device-bt.h"
#include "utils.h"
#include "gconf-helpers.h"
#include "applet-dialogs.h"

typedef struct {
	BMApplet *applet;
	BMDevice *device;
	BMConnection *connection;
} BtMenuItemInfo;

static void
bt_menu_item_info_destroy (gpointer data)
{
	BtMenuItemInfo *info = data;

	g_object_unref (G_OBJECT (info->device));
	if (info->connection)
		g_object_unref (G_OBJECT (info->connection));

	g_slice_free (BtMenuItemInfo, data);
}

static gboolean
bt_new_auto_connection (BMDevice *device,
                        gpointer dclass_data,
                        AppletNewAutoConnectionCallback callback,
                        gpointer callback_data)
{

	// FIXME: call gnome-bluetooth setup wizard
	return FALSE;
}

static void
bt_menu_item_activate (GtkMenuItem *item, gpointer user_data)
{
	BtMenuItemInfo *info = user_data;

	applet_menu_item_activate_helper (info->device,
	                                  info->connection,
	                                  "/",
	                                  info->applet,
	                                  user_data);
}


typedef enum {
	ADD_ACTIVE = 1,
	ADD_INACTIVE = 2,
} AddActiveInactiveEnum;

static void
add_connection_items (BMDevice *device,
                      GSList *connections,
                      BMConnection *active,
                      AddActiveInactiveEnum flag,
                      GtkWidget *menu,
                      BMApplet *applet)
{
	GSList *iter;
	BtMenuItemInfo *info;

	for (iter = connections; iter; iter = g_slist_next (iter)) {
		BMConnection *connection = BM_CONNECTION (iter->data);
		GtkWidget *item;

		if (active == connection) {
			if ((flag & ADD_ACTIVE) == 0)
				continue;
		} else {
			if ((flag & ADD_INACTIVE) == 0)
				continue;
		}

		item = applet_new_menu_item_helper (connection, active, (flag & ADD_ACTIVE));

		info = g_slice_new0 (BtMenuItemInfo);
		info->applet = applet;
		info->device = g_object_ref (G_OBJECT (device));
		info->connection = g_object_ref (connection);

		g_signal_connect_data (item, "activate",
		                       G_CALLBACK (bt_menu_item_activate),
		                       info,
		                       (GClosureNotify) bt_menu_item_info_destroy, 0);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}
}

static void
bt_add_menu_item (BMDevice *device,
                  guint32 n_devices,
                  BMConnection *active,
                  GtkWidget *menu,
                  BMApplet *applet)
{
	const char *text;
	GtkWidget *item;
	GSList *connections, *all;

	all = applet_get_all_connections (applet);
	// FIXME connections = utils_filter_connections_for_device (device, all);
	connections = NULL;

	g_slist_free (all);

	text = bm_device_bt_get_name (BM_DEVICE_BT (device));
	if (!text) {
		text = utils_get_device_description (device);
		if (!text)
			text = bm_device_get_iface (device);
		g_assert (text);
	}

	item = applet_menu_item_create_device_item_helper (device, applet, text);

#ifndef ENABLE_INDICATOR
	gtk_widget_set_sensitive (item, FALSE);
#endif
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	if (g_slist_length (connections))
		add_connection_items (device, connections, active, ADD_ACTIVE, menu, applet);

	/* Notify user of ubmanaged or unavailable device */
	item = bma_menu_device_get_menu_item (device, applet, NULL);
	if (item) {
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		gtk_widget_show (item);
	}

	if (!bma_menu_device_check_unusable (device)) {
		/* Add menu items for existing bluetooth connections for this device */
		if (g_slist_length (connections)) {
			applet_menu_item_add_complex_separator_helper (menu, applet, _("Available"), -1);
			add_connection_items (device, connections, active, ADD_INACTIVE, menu, applet);
		}
	}

	g_slist_free (connections);
}

static void
bt_device_state_changed (BMDevice *device,
                         BMDeviceState new_state,
                         BMDeviceState old_state,
                         BMDeviceStateReason reason,
                         BMApplet *applet)
{
	if (new_state == BM_DEVICE_STATE_ACTIVATED) {
		BMConnection *connection;
		BMSettingConnection *s_con = NULL;
		char *str = NULL;

		connection = applet_find_active_connection_for_device (device, applet, NULL);
		if (connection) {
			const char *id;
			s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
			id = s_con ? bm_setting_connection_get_id (s_con) : NULL;
			if (id)
				str = g_strdup_printf (_("You are now connected to '%s'."), id);
		}

		applet_do_notify_with_pref (applet,
		                            _("Connection Established"),
		                            str ? str : _("You are now connected to the mobile broadband network."),
		                            "bm-device-wwan",
		                            PREF_DISABLE_CONNECTED_NOTIFICATIONS);
		g_free (str);
	}
}

static void
bt_get_icon (BMDevice *device,
             BMDeviceState state,
             BMConnection *connection,
             GdkPixbuf **out_pixbuf,
             char **out_indicator_icon,
             char **tip,
             BMApplet *applet)
{
	BMSettingConnection *s_con;
	const char *id;

	id = bm_device_get_iface (BM_DEVICE (device));
	if (connection) {
		s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
		id = bm_setting_connection_get_id (s_con);
	}

	switch (state) {
	case BM_DEVICE_STATE_PREPARE:
		*tip = g_strdup_printf (_("Preparing mobile broadband connection '%s'..."), id);
		break;
	case BM_DEVICE_STATE_CONFIG:
		*tip = g_strdup_printf (_("Configuring mobile broadband connection '%s'..."), id);
		break;
	case BM_DEVICE_STATE_NEED_AUTH:
		*tip = g_strdup_printf (_("User authentication required for mobile broadband connection '%s'..."), id);
		break;
	case BM_DEVICE_STATE_IP_CONFIG:
		*tip = g_strdup_printf (_("Requesting a network address for '%s'..."), id);
		break;
	case BM_DEVICE_STATE_ACTIVATED:
		*out_indicator_icon = g_strdup_printf ("bm-device-wwan");
		*out_pixbuf = bma_icon_check_and_load ("bm-device-wwan", &applet->wwan_icon, applet);
		*tip = g_strdup_printf (_("Mobile broadband connection '%s' active"), id);
		break;
	default:
		break;
	}
}

typedef struct {
	BMANewSecretsRequestedFunc callback;
	gpointer callback_data;
	BMApplet *applet;
	BMSettingsConnectionInterface *connection;
	BMActiveConnection *active_connection;
	GtkWidget *dialog;
	GtkEntry *secret_entry;
	char *secret_name;
	char *setting_name;
} BMBtSecretsInfo;

BMADeviceClass *
applet_device_bt_get_class (BMApplet *applet)
{
	BMADeviceClass *dclass;

	dclass = g_slice_new0 (BMADeviceClass);
	if (!dclass)
		return NULL;

	dclass->new_auto_connection = bt_new_auto_connection;
	dclass->add_menu_item = bt_add_menu_item;
	dclass->device_state_changed = bt_device_state_changed;
	dclass->get_icon = bt_get_icon;
	//	dclass->get_secrets = bt_get_secrets;

	return dclass;
}
