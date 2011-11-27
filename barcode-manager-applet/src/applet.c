/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Applet -- Display barcode scanners and allow user control
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
 * Copyright (C) 2011 Jakob Flierl
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <string.h>
#include <strings.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <sys/socket.h>

#include <bm-device.h>
#include <bm-device-bt.h>
#include <bm-utils.h>
#include <bm-connection.h>
#include <bm-setting-connection.h>
#include <bm-active-connection.h>

#include <gconf/gconf-client.h>
#include <gnome-keyring.h>
#include <libnotify/notify.h>

#include "applet.h"
#include "applet-device-bt.h"
#include "applet-dialogs.h"
#include "applet-dbus-manager.h"
#include "utils.h"
#include "gconf-helpers.h"

#define NOTIFY_CAPS_ACTIONS_KEY "actions"

G_DEFINE_TYPE(BMApplet, bma, G_TYPE_OBJECT)

struct _OfflineNotificationContextInfo {
	BMState state;
	BMDeviceState device_state;
	BMDeviceStateReason device_state_reason;
	BMDeviceType device_type;
	gchar* title;
	const gchar* text;
	const gchar* icon;
	NotifyUrgency urgency;
};

typedef struct _OfflineNotificationContextInfo OfflineNotificationContextInfo;

static BMActiveConnection *
applet_get_best_activating_connection (BMApplet *applet, BMDevice **device)
{
	BMActiveConnection *best = NULL;
	BMDevice *best_dev = NULL;
	const GPtrArray *connections;
	int i;

	g_return_val_if_fail (BM_IS_APPLET (applet), NULL);
	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (*device == NULL, NULL);

	connections = bm_client_get_active_connections (applet->bm_client);
	for (i = 0; connections && (i < connections->len); i++) {
		BMActiveConnection *candidate = g_ptr_array_index (connections, i);
		const GPtrArray *devices;
		BMDevice *candidate_dev;

		if (bm_active_connection_get_state (candidate) != BM_ACTIVE_CONNECTION_STATE_ACTIVATING)
			continue;

		devices = bm_active_connection_get_devices (candidate);
		if (!devices || !devices->len)
			continue;

		candidate_dev = g_ptr_array_index (devices, 0);
		if (!best_dev) {
			best_dev = candidate_dev;
			best = candidate;
			continue;
		}

	}

	*device = best_dev;
	return best;
}

static BMActiveConnection *
applet_get_default_active_connection (BMApplet *applet, BMDevice **device)
{
	BMActiveConnection *default_ac = NULL;
	BMDevice *non_default_device = NULL;
	BMActiveConnection *non_default_ac = NULL;
	const GPtrArray *connections;
	int i;

	g_return_val_if_fail (BM_IS_APPLET (applet), NULL);
	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (*device == NULL, NULL);

	connections = bm_client_get_active_connections (applet->bm_client);
	for (i = 0; connections && (i < connections->len); i++) {
		BMActiveConnection *candidate = g_ptr_array_index (connections, i);
		const GPtrArray *devices;

		devices = bm_active_connection_get_devices (candidate);
		if (!devices || !devices->len)
			continue;

		if (bm_active_connection_get_default (candidate)) {
			if (!default_ac) {
				*device = g_ptr_array_index (devices, 0);
				default_ac = candidate;
			}
		} else {
			if (!non_default_ac) {
				non_default_device = g_ptr_array_index (devices, 0);
				non_default_ac = candidate;
			}
		}
	}

	/* Prefer the default connection if one exists, otherwise return the first
	 * non-default connection.
	 */
	if (!default_ac && non_default_ac) {
		default_ac = non_default_ac;
		*device = non_default_device;
	}
	return default_ac;
}

BMSettingsInterface *
applet_get_settings (BMApplet *applet)
{
	return BM_SETTINGS_INTERFACE (applet->gconf_settings);
}

GSList *
applet_get_all_connections (BMApplet *applet)
{
	return g_slist_concat (bm_settings_interface_list_connections (BM_SETTINGS_INTERFACE (applet->system_settings)),
	                       bm_settings_interface_list_connections (BM_SETTINGS_INTERFACE (applet->gconf_settings)));
}


static inline BMADeviceClass *
get_device_class (BMDevice *device, BMApplet *applet)
{
	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (applet != NULL, NULL);

	if (BM_IS_DEVICE_BT (device))
		return applet->bt_class;
	else
		g_message ("%s: Unknown device type '%s'", __func__, G_OBJECT_TYPE_NAME (device));
	return NULL;
}

static gboolean
is_system_connection (BMConnection *connection)
{
	return (bm_connection_get_scope (connection) == BM_CONNECTION_SCOPE_SYSTEM) ? TRUE : FALSE;
}

static void
activate_connection_cb (gpointer user_data, const char *path, GError *error)
{
	if (error)
		bm_warning ("Connection activation failed: %s", error->message);

	applet_schedule_update_icon (BM_APPLET (user_data));
}

typedef struct {
	BMApplet *applet;
	BMDevice *device;
	char *specific_object;
	gpointer dclass_data;
} AppletItemActivateInfo;

static void
applet_item_activate_info_destroy (AppletItemActivateInfo *info)
{
	g_return_if_fail (info != NULL);

	if (info->device)
		g_object_unref (info->device);
	g_free (info->specific_object);
	memset (info, 0, sizeof (AppletItemActivateInfo));
	g_free (info);
}

static void
applet_menu_item_activate_helper_part2 (BMConnection *connection,
                                        gboolean auto_created,
                                        gboolean canceled,
                                        gpointer user_data)
{
	AppletItemActivateInfo *info = user_data;
	const char *con_path;
	gboolean is_system = FALSE;

	if (canceled) {
		applet_item_activate_info_destroy (info);
		return;
	}

	g_return_if_fail (connection != NULL);

	if (!auto_created)
		is_system = is_system_connection (connection);
	else {
		BMAGConfConnection *exported;

		exported = bma_gconf_settings_add_connection (info->applet->gconf_settings, connection);
		if (!exported) {
			BMADeviceClass *dclass = get_device_class (info->device, info->applet);

			/* If the setting isn't valid, because it needs more authentication
			 * or something, ask the user for it.
			 */

			g_assert (dclass);
			bm_warning ("Invalid connection; asking for more information.");
			if (dclass->get_more_info)
				dclass->get_more_info (info->device, connection, info->applet, info->dclass_data);
			g_object_unref (connection);

			applet_item_activate_info_destroy (info);
			return;
		}
		g_object_unref (connection);
		connection = BM_CONNECTION (exported);
	}

	g_assert (connection);
	con_path = bm_connection_get_path (connection);
	g_assert (con_path);

	/* Finally, tell NM to activate the connection */
	bm_client_activate_connection (info->applet->bm_client,
	                               is_system ? BM_DBUS_SERVICE_SYSTEM_SETTINGS : BM_DBUS_SERVICE_USER_SETTINGS,
	                               con_path,
	                               info->device,
	                               info->specific_object,
	                               activate_connection_cb,
	                               info->applet);
	applet_item_activate_info_destroy (info);
}

void
applet_menu_item_disconnect_helper (BMDevice *device,
                                    BMApplet *applet)
{
	g_return_if_fail (BM_IS_DEVICE (device));

	// FIXME bm_device_disconnect (device, disconnect_cb, NULL);
}


void
applet_menu_item_activate_helper (BMDevice *device,
                                  BMConnection *connection,
                                  const char *specific_object,
                                  BMApplet *applet,
                                  gpointer dclass_data)
{
	AppletItemActivateInfo *info;
	BMADeviceClass *dclass;

	g_return_if_fail (BM_IS_DEVICE (device));

	info = g_malloc0 (sizeof (AppletItemActivateInfo));
	info->applet = applet;
	info->specific_object = g_strdup (specific_object);
	info->device = g_object_ref (device);
	info->dclass_data = dclass_data;

	if (connection) {
		applet_menu_item_activate_helper_part2 (connection, FALSE, FALSE, info);
		return;
	}

	dclass = get_device_class (device, applet);

	/* If no connection was passed in, ask the device class to create a new
	 * default connection for this device type.  This could be a wizard,
	 * and thus take a while.
	 */
	g_assert (dclass);
	if (!dclass->new_auto_connection (device, dclass_data,
	                                  applet_menu_item_activate_helper_part2,
	                                  info)) {
		bm_warning ("Couldn't create default connection.");
		applet_item_activate_info_destroy (info);
	}
}

void
applet_menu_item_add_complex_separator_helper (GtkWidget *menu,
                                               BMApplet *applet,
                                               const gchar* label,
                                               int pos)
{
	GtkWidget *menu_item = gtk_image_menu_item_new ();
#ifndef ENABLE_INDICATOR
	GtkWidget *box = gtk_hbox_new (FALSE, 0);
	GtkWidget *xlabel = NULL;

	if (label) {
		xlabel = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (xlabel), label);

		gtk_box_pack_start (GTK_BOX (box), gtk_hseparator_new (), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), xlabel, FALSE, FALSE, 2);
	}

	gtk_box_pack_start (GTK_BOX (box), gtk_hseparator_new (), TRUE, TRUE, 0);

	g_object_set (G_OBJECT (menu_item),
	              "child", box,
	              "sensitive", FALSE,
	              NULL);
#else
	menu_item = gtk_separator_menu_item_new ();
#endif
	if (pos < 0)
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	else
		gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menu_item, pos);
	return;
}

GtkWidget *
applet_new_menu_item_helper (BMConnection *connection,
                             BMConnection *active,
                             gboolean add_active)
{
	GtkWidget *item;
	BMSettingConnection *s_con;
#ifndef ENABLE_INDICATOR
	char *markup;
	GtkWidget *label;
#endif

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));

#ifndef ENABLE_INDICATOR
	item = gtk_image_menu_item_new_with_label ("");
	if (add_active && (active == connection)) {
		/* Pure evil */
		label = gtk_bin_get_child (GTK_BIN (item));
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		markup = g_markup_printf_escaped ("<b>%s</b>", bm_setting_connection_get_id (s_con));
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	} else
		gtk_menu_item_set_label (GTK_MENU_ITEM (item), bm_setting_connection_get_id (s_con));

	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (item), TRUE);
#else
	item = gtk_menu_item_new_with_label (bm_setting_connection_get_id (s_con));
#endif
	return item;
}

#ifndef ENABLE_INDICATOR
#define TITLE_TEXT_R ((double) 0x5e / 255.0 )
#define TITLE_TEXT_G ((double) 0x5e / 255.0 )
#define TITLE_TEXT_B ((double) 0x5e / 255.0 )

static gboolean
menu_title_item_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GtkAllocation allocation;
	GtkStyle *style;
	GtkWidget *label;
	PangoFontDescription *desc;
	cairo_t *cr;
	PangoLayout *layout;
	int width = 0, height = 0, owidth, oheight;
	gdouble extraheight = 0, extrawidth = 0;
	const char *text;
	gdouble xpadding = 10.0;
	gdouble ypadding = 5.0;
	gdouble postpadding = 0.0;

	label = gtk_bin_get_child (GTK_BIN (widget));

	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	/* The drawing area we get is the whole menu; clip the drawing to the
	 * event area, which should just be our menu item.
	 */
	cairo_rectangle (cr,
	                 event->area.x, event->area.y,
	                 event->area.width, event->area.height);
	cairo_clip (cr);

	/* We also need to reposition the cairo context so that (0, 0) is the
	 * top-left of where we're supposed to start drawing.
	 */
#if GTK_CHECK_VERSION(2,18,0)
	gtk_widget_get_allocation (widget, &allocation);
#else
	allocation = widget->allocation;
#endif
	cairo_translate (cr, widget->allocation.x, widget->allocation.y);

	text = gtk_label_get_text (GTK_LABEL (label));

	layout = pango_cairo_create_layout (cr);
	style = gtk_widget_get_style (widget);
	desc = pango_font_description_copy (style->font_desc);
	pango_font_description_set_variant (desc, PANGO_VARIANT_SMALL_CAPS);
	pango_font_description_set_weight (desc, PANGO_WEIGHT_SEMIBOLD);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, text, -1);
	pango_cairo_update_layout (cr, layout);
	pango_layout_get_size (layout, &owidth, &oheight);
	width = owidth / PANGO_SCALE;
	height += oheight / PANGO_SCALE;

	cairo_save (cr);

	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
	cairo_rectangle (cr, 0, 0,
	                 (double) (width + 2 * xpadding),
	                 (double) (height + ypadding + postpadding));
	cairo_fill (cr);

	/* now the in-padding content */
	cairo_translate (cr, xpadding , ypadding);
	cairo_set_source_rgb (cr, TITLE_TEXT_R, TITLE_TEXT_G, TITLE_TEXT_B);
	cairo_move_to (cr, extrawidth, extraheight);
	pango_cairo_show_layout (cr, layout);

	cairo_restore(cr);

	pango_font_description_free (desc);
	g_object_unref (layout);
	cairo_destroy (cr);

	gtk_widget_set_size_request (widget, width + 2 * xpadding, height + ypadding + postpadding);
	return TRUE;
}
#endif

GtkWidget *
applet_menu_item_create_device_item_helper (BMDevice *device,
                                            BMApplet *applet,
                                            const gchar *text)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_mnemonic (text);
	gtk_widget_set_sensitive (item, FALSE);
#ifndef ENABLE_INDICATOR
	g_signal_connect (item, "expose-event", G_CALLBACK (menu_title_item_expose), NULL);
#endif
	return item;
}

static gboolean
applet_notify_server_has_actions (void)
{
	gboolean has_actions = FALSE;
	GList *server_caps, *iter;

	server_caps = notify_get_server_caps();
	for (iter = server_caps; iter; iter = g_list_next (iter)) {
		if (!strcmp ((const char *) iter->data, NOTIFY_CAPS_ACTIONS_KEY)) {
			has_actions = TRUE;
			break;
		}
	}
	g_list_foreach (server_caps, (GFunc) g_free, NULL);
	g_list_free (server_caps);

	return has_actions;
}

void
applet_do_notify (BMApplet *applet,
                  NotifyUrgency urgency,
                  const char *summary,
                  const char *message,
                  const char *icon,
                  const char *action1,
                  const char *action1_label,
                  NotifyActionCallback action1_cb,
                  gpointer action1_user_data)
{
	NotifyNotification *notify;
	GError *error = NULL;
	char *escaped;

	g_return_if_fail (applet != NULL);
	g_return_if_fail (summary != NULL);
	g_return_if_fail (message != NULL);

#ifndef ENABLE_INDICATOR
	if (!gtk_status_icon_is_embedded (applet->status_icon))
#else
	if (app_indicator_get_status (applet->status_icon) == APP_INDICATOR_STATUS_PASSIVE)
#endif
		return;

	escaped = utils_escape_notify_message (message);

	if (applet->notification == NULL) {
		notify = notify_notification_new (summary,
		                                  escaped,
		                                  icon ? icon : GTK_STOCK_NETWORK
#if HAVE_LIBNOTIFY_07
		                                  );
#else
		                                 , NULL);
#endif

		applet->notification = notify;
	} else {
		notify = applet->notification;
		notify_notification_update (notify,
		                            summary,
		                            escaped,
		                            icon ? icon : GTK_STOCK_NETWORK);
	}

	g_free (escaped);

#ifndef ENABLE_INDICATOR
#if !HAVE_LIBNOTIFY_07
	notify_notification_attach_to_status_icon (notify, applet->status_icon);
#endif
#endif
	notify_notification_set_urgency (notify, urgency);
	notify_notification_set_timeout (notify, NOTIFY_EXPIRES_DEFAULT);

	if (applet->notify_actions && action1) {
		notify_notification_add_action (notify, action1, action1_label,
		                                action1_cb, action1_user_data, NULL);
	}

	if (!notify_notification_show (notify, &error)) {
		g_warning ("Failed to show notification: %s",
		           error && error->message ? error->message : "(unknown)");
		g_clear_error (&error);
	}
}

static void
notify_dont_show_cb (NotifyNotification *notify,
                     gchar *id,
                     gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);

	if (!id)
		return;

	if (   strcmp (id, PREF_DISABLE_CONNECTED_NOTIFICATIONS)
	    && strcmp (id, PREF_DISABLE_DISCONNECTED_NOTIFICATIONS))
		return;

	gconf_client_set_bool (applet->gconf_client, id, TRUE, NULL);
}

void applet_do_notify_with_pref (BMApplet *applet,
                                 const char *summary,
                                 const char *message,
                                 const char *icon,
                                 const char *pref)
{
	if (gconf_client_get_bool (applet->gconf_client, pref, NULL))
		return;
	
	applet_do_notify (applet, NOTIFY_URGENCY_LOW, summary, message, icon, pref,
	                  _("Don't show this message again"),
	                  notify_dont_show_cb,
	                  applet);
}

static gboolean
animation_timeout (gpointer data)
{
	applet_schedule_update_icon (BM_APPLET (data));
	return TRUE;
}

static void
start_animation_timeout (BMApplet *applet)
{
	if (applet->animation_id == 0) {
		applet->animation_step = 0;
		applet->animation_id = g_timeout_add (100, animation_timeout, applet);
	}
}

static void
clear_animation_timeout (BMApplet *applet)
{
	if (applet->animation_id) {
		g_source_remove (applet->animation_id);
		applet->animation_id = 0;
		applet->animation_step = 0;
	}
}

static gboolean
applet_is_any_device_activating (BMApplet *applet)
{
	const GPtrArray *devices;
	int i;

	/* Check for activating devices */
	devices = bm_client_get_devices (applet->bm_client);
	for (i = 0; devices && (i < devices->len); i++) {
		BMDevice *candidate = BM_DEVICE (g_ptr_array_index (devices, i));
		BMDeviceState state;

		state = bm_device_get_state (candidate);
		if (state > BM_DEVICE_STATE_DISCONNECTED && state < BM_DEVICE_STATE_ACTIVATED)
			return TRUE;
	}
	return FALSE;
}

static void
update_connection_timestamp (BMActiveConnection *active,
                             BMConnection *connection,
                             BMApplet *applet)
{
	BMSettingsConnectionInterface *gconf_connection;
	BMSettingConnection *s_con;

	if (bm_active_connection_get_scope (active) != BM_CONNECTION_SCOPE_USER)
		return;

	gconf_connection = bm_settings_interface_get_connection_by_path (BM_SETTINGS_INTERFACE (applet->gconf_settings),
	                                                                 bm_connection_get_path (connection));
	if (!gconf_connection || !BMA_IS_GCONF_CONNECTION (gconf_connection))
		return;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	g_object_set (s_con, BM_SETTING_CONNECTION_TIMESTAMP, (guint64) time (NULL), NULL);
	/* Ignore secrets since we're just updating the timestamp */
	bma_gconf_connection_update (BMA_GCONF_CONNECTION (gconf_connection), TRUE);
}

/*
 * bma_menu_add_separator_item
 *
 */
static void
bma_menu_add_separator_item (GtkWidget *menu)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
}

/*
 * bma_menu_add_text_item
 *
 * Add a non-clickable text item to a menu
 *
 */
static void bma_menu_add_text_item (GtkWidget *menu, char *text)
{
	GtkWidget		*menu_item;

	g_return_if_fail (text != NULL);
	g_return_if_fail (menu != NULL);

	menu_item = gtk_menu_item_new_with_label (text);
	gtk_widget_set_sensitive (menu_item, FALSE);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
}

static gint
sort_devices (gconstpointer a, gconstpointer b)
{
	BMDevice *aa = BM_DEVICE (a);
	BMDevice *bb = BM_DEVICE (b);
	GType aa_type;
	GType bb_type;

	aa_type = G_OBJECT_TYPE (G_OBJECT (aa));
	bb_type = G_OBJECT_TYPE (G_OBJECT (bb));

	if (aa_type == bb_type) {
		char *aa_desc = NULL;
		char *bb_desc = NULL;

		aa_desc = (char *) utils_get_device_description (aa);
		if (!aa_desc)
			aa_desc = (char *) bm_device_get_iface (aa);

		bb_desc = (char *) utils_get_device_description (bb);
		if (!bb_desc)
			bb_desc = (char *) bm_device_get_iface (bb);

		if (!aa_desc && bb_desc)
			return -1;
		else if (aa_desc && !bb_desc)
			return 1;
		else if (!aa_desc && !bb_desc)
			return 0;

		g_assert (aa_desc);
		g_assert (bb_desc);
		return strcmp (aa_desc, bb_desc);
	}

	return 1;
}

static gboolean
bm_g_ptr_array_contains (const GPtrArray *haystack, gpointer needle)
{
	int i;

	for (i = 0; haystack && (i < haystack->len); i++) {
		if (g_ptr_array_index (haystack, i) == needle)
			return TRUE;
	}
	return FALSE;
}

BMConnection *
applet_find_active_connection_for_device (BMDevice *device,
                                          BMApplet *applet,
                                          BMActiveConnection **out_active)
{
	const GPtrArray *active_connections;
	BMConnection *connection = NULL;
	int i;

	g_return_val_if_fail (BM_IS_DEVICE (device), NULL);
	g_return_val_if_fail (BM_IS_APPLET (applet), NULL);
	if (out_active)
		g_return_val_if_fail (*out_active == NULL, NULL);

	active_connections = bm_client_get_active_connections (applet->bm_client);
	for (i = 0; active_connections && (i < active_connections->len); i++) {
		BMSettingsConnectionInterface *tmp;
		BMSettingsInterface *settings = NULL;
		BMActiveConnection *active;
		const char *service_name;
		const char *connection_path;
		const GPtrArray *devices;

		active = BM_ACTIVE_CONNECTION (g_ptr_array_index (active_connections, i));
		devices = bm_active_connection_get_devices (active);
		service_name = bm_active_connection_get_service_name (active);
		connection_path = bm_active_connection_get_connection (active);

		if (!devices || !service_name || !connection_path)
			continue;

		if (!bm_g_ptr_array_contains (devices, device))
			continue;

		if (!strcmp (service_name, BM_DBUS_SERVICE_SYSTEM_SETTINGS))
			settings = BM_SETTINGS_INTERFACE (applet->system_settings);
		else if (!strcmp (service_name, BM_DBUS_SERVICE_USER_SETTINGS))
			settings = BM_SETTINGS_INTERFACE (applet->gconf_settings);
		else
			g_assert_not_reached ();

		tmp = bm_settings_interface_get_connection_by_path (settings, connection_path);
		if (tmp) {
			connection = BM_CONNECTION (tmp);
			if (out_active)
				*out_active = active;
			break;
		}
	}

	return connection;
}

gboolean
bma_menu_device_check_unusable (BMDevice *device)
{
	switch (bm_device_get_state (device)) {
	case BM_DEVICE_STATE_UNKNOWN:
	case BM_DEVICE_STATE_UNAVAILABLE:
		return TRUE;
	default:
		break;
	}
	return FALSE;
}


struct AppletDeviceMenuInfo {
	BMDevice *device;
	BMApplet *applet;
};

static void
applet_device_info_destroy (struct AppletDeviceMenuInfo *info)
{
	g_return_if_fail (info != NULL);

	if (info->device)
		g_object_unref (info->device);
	memset (info, 0, sizeof (struct AppletDeviceMenuInfo));
	g_free (info);
}

static void
applet_device_disconnect_db (GtkMenuItem *item, gpointer user_data)
{
	struct AppletDeviceMenuInfo *info = user_data;

	applet_menu_item_disconnect_helper (info->device,
	                                    info->applet);
}

GtkWidget *
bma_menu_device_get_menu_item (BMDevice *device,
                               BMApplet *applet,
                               const char *unavailable_msg)
{
	GtkWidget *item = NULL;
	gboolean managed = TRUE;

	if (!unavailable_msg) {
		if (bm_device_get_firmware_missing (device))
			unavailable_msg = _("device not ready (firmware missing)");
		else
			unavailable_msg = _("device not ready");
	}

	switch (bm_device_get_state (device)) {
	case BM_DEVICE_STATE_UNKNOWN:
	case BM_DEVICE_STATE_UNAVAILABLE:
		item = gtk_menu_item_new_with_label (unavailable_msg);
		gtk_widget_set_sensitive (item, FALSE);
		break;
	case BM_DEVICE_STATE_DISCONNECTED:
		unavailable_msg = _("disconnected");
		item = gtk_menu_item_new_with_label (unavailable_msg);
		gtk_widget_set_sensitive (item, FALSE);
		break;
	case BM_DEVICE_STATE_PREPARE:
	case BM_DEVICE_STATE_CONFIG:
	case BM_DEVICE_STATE_NEED_AUTH:
	case BM_DEVICE_STATE_IP_CONFIG:
	case BM_DEVICE_STATE_ACTIVATED:
	{
		struct AppletDeviceMenuInfo *info = g_new0 (struct AppletDeviceMenuInfo, 1);
		info->device = g_object_ref (device);
		info->applet = applet;
		item = gtk_menu_item_new_with_label (_("Disconnect"));
		g_signal_connect_data (item, "activate",
		                       G_CALLBACK (applet_device_disconnect_db),
		                       info,
		                       (GClosureNotify) applet_device_info_destroy, 0);
		gtk_widget_set_sensitive (item, TRUE);
		break;
	}
	default:
		managed = bm_device_get_managed (device);
		break;
	}

	if (!managed) {
		item = gtk_menu_item_new_with_label (_("device not managed"));
		gtk_widget_set_sensitive (item, FALSE);
	}

	return item;
}

static guint32
bma_menu_add_devices (GtkWidget *menu, BMApplet *applet)
{
	const GPtrArray *temp = NULL;
	GSList *devices = NULL, *iter = NULL;
	gint n_wifi_devices = 0;
	gint n_usable_wifi_devices = 0;
	gint n_wired_devices = 0;
	gint n_mb_devices = 0;
	gint n_bt_devices = 0;
	int i;

	temp = bm_client_get_devices (applet->bm_client);
	for (i = 0; temp && (i < temp->len); i++)
		devices = g_slist_append (devices, g_ptr_array_index (temp, i));
	if (devices)
		devices = g_slist_sort (devices, sort_devices);

	for (iter = devices; iter; iter = iter->next) {
		BMDevice *device = BM_DEVICE (iter->data);

		/* Ignore unsupported devices */
		if (!(bm_device_get_capabilities (device) & BM_DEVICE_CAP_BM_SUPPORTED))
			continue;

		if (BM_IS_DEVICE_BT (device))
			n_bt_devices++;
	}

	if (!n_wired_devices && !n_wifi_devices && !n_mb_devices && !n_bt_devices) {
		bma_menu_add_text_item (menu, _("No network devices available"));
		goto out;
	}

	/* Add all devices in our device list to the menu */
	for (iter = devices; iter; iter = iter->next) {
		BMDevice *device = BM_DEVICE (iter->data);
		gint n_devices = 0;
		BMADeviceClass *dclass;
		BMConnection *active;

		/* Ignore unsupported devices */
		if (!(bm_device_get_capabilities (device) & BM_DEVICE_CAP_BM_SUPPORTED))
			continue;

		active = applet_find_active_connection_for_device (device, applet, NULL);

		dclass = get_device_class (device, applet);
		if (dclass)
			dclass->add_menu_item (device, n_devices, active, menu, applet);

		bma_menu_add_separator_item (menu);
	}

 out:
	g_slist_free (devices);

	/* Return # of usable wifi devices here for correct enable/disable state
	 * of things like Enable Wireless, "Connect to other..." and such.
	 */
	return n_usable_wifi_devices;
}

#ifndef ENABLE_INDICATOR
static void
bma_set_notifications_enabled_cb (GtkWidget *widget, BMApplet *applet)
{
	gboolean state;

	g_return_if_fail (applet != NULL);

	state = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));

	gconf_client_set_bool (applet->gconf_client,
	                       PREF_DISABLE_CONNECTED_NOTIFICATIONS,
	                       !state,
	                       NULL);
	gconf_client_set_bool (applet->gconf_client,
	                       PREF_DISABLE_DISCONNECTED_NOTIFICATIONS,
	                       !state,
	                       NULL);
	gconf_client_set_bool (applet->gconf_client,
	                       PREF_SUPPRESS_WIRELESS_NETWORKS_AVAILABLE,
	                       !state,
	                       NULL);
}
#endif

/*
 * bma_menu_show_cb
 *
 * Pop up the wireless networks menu
 *
 */
static void bma_menu_show_cb (GtkWidget *menu, BMApplet *applet)
{
	guint32 n_wireless;

	g_return_if_fail (menu != NULL);
	g_return_if_fail (applet != NULL);

#ifndef ENABLE_INDICATOR /* indicators don't get tooltips */

#if GTK_CHECK_VERSION(2, 15, 0)
	gtk_status_icon_set_tooltip_text (applet->status_icon, NULL);
#else
	gtk_status_icon_set_tooltip (applet->status_icon, NULL);
#endif

#endif /* ENABLE_INDICATOR */

	if (!bm_client_get_manager_running (applet->bm_client)) {
		bma_menu_add_text_item (menu, _("BarcodeManager is not running..."));
		return;
	}

	if (bm_client_get_state (applet->bm_client) == BM_STATE_ASLEEP) {
		bma_menu_add_text_item (menu, _("Networking disabled"));
		return;
	}

	n_wireless = bma_menu_add_devices (menu, applet);

	if (n_wireless > 0 && bm_client_wireless_get_enabled (applet->bm_client)) {
		/* Add the "Hidden wireless network..." entry */
		//		bma_menu_add_hidden_network_item (menu, applet);
		// bma_menu_add_create_network_item (menu, applet);
		bma_menu_add_separator_item (menu);
	}

	// bma_menu_add_vpn_submenu (menu, applet);

	gtk_widget_show_all (menu);

//	nmi_dbus_signal_user_interface_activated (applet->connection);
}

static gboolean bma_menu_clear (BMApplet *applet);

#ifndef ENABLE_INDICATOR
static void
bma_menu_deactivate_cb (GtkWidget *widget, BMApplet *applet)
{
	/* Must punt the destroy to a low-priority idle to ensure that
	 * the menu items don't get destroyed before any 'activate' signal
	 * fires for an item.
	 */
	g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc) bma_menu_clear, applet, NULL);


	/* Re-set the tooltip */
#if GTK_CHECK_VERSION(2, 15, 0)
	gtk_status_icon_set_tooltip_text (applet->status_icon, applet->tip);
#else
	gtk_status_icon_set_tooltip (applet->status_icon, applet->tip);
#endif

}
#endif /* ENABLE_INDICATOR */

/*
 * bma_menu_create
 *
 * Create the applet's dropdown menu
 *
 */
static GtkWidget *
bma_menu_create (BMApplet *applet)
{
	GtkWidget	*menu;

	g_return_val_if_fail (applet != NULL, NULL);

	menu = gtk_menu_new ();
	gtk_container_set_border_width (GTK_CONTAINER (menu), 0);
#ifndef ENABLE_INDICATOR
	g_signal_connect (menu, "show", G_CALLBACK (bma_menu_show_cb), applet);
	g_signal_connect (menu, "deactivate", G_CALLBACK (bma_menu_deactivate_cb), applet);
#endif
	return menu;
}

/*
 * bma_menu_clear
 *
 * Destroy the menu and each of its items data tags
 *
 */
static gboolean bma_menu_clear (BMApplet *applet)
{
	g_return_val_if_fail (applet != NULL, FALSE);

	if (applet->menu)
		gtk_widget_destroy (applet->menu);
	applet->menu = bma_menu_create (applet);
	return FALSE;
}

static gboolean
is_permission_yes (BMApplet *applet, BMClientPermission perm)
{
	if (   applet->permissions[perm] == BM_CLIENT_PERMISSION_RESULT_YES
	    || applet->permissions[perm] == BM_CLIENT_PERMISSION_RESULT_AUTH)
		return TRUE;
	return FALSE;
}

/*
 * bma_context_menu_update
 *
 */
static void
bma_context_menu_update (BMApplet *applet)
{
	BMState state;
	gboolean net_enabled = TRUE;
#ifndef ENABLE_INDICATOR
	gboolean notifications_enabled = TRUE;
#endif

	state = bm_client_get_state (applet->bm_client);

	gtk_widget_set_sensitive (applet->info_menu_item, state == BM_STATE_CONNECTED);

	/* Update checkboxes, and block 'toggled' signal when updating so that the
	 * callback doesn't get triggered.
	 */

	/* Enabled Networking */
	g_signal_handler_block (G_OBJECT (applet->networking_enabled_item),
	                        applet->networking_enabled_toggled_id);
	net_enabled = bm_client_networking_get_enabled (applet->bm_client);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (applet->networking_enabled_item),
	                                net_enabled && (state != BM_STATE_ASLEEP));
	g_signal_handler_unblock (G_OBJECT (applet->networking_enabled_item),
	                          applet->networking_enabled_toggled_id);
	gtk_widget_set_sensitive (applet->networking_enabled_item,
	                          is_permission_yes (applet, BM_CLIENT_PERMISSION_ENABLE_DISABLE_NETWORK));

#ifndef ENABLE_INDICATOR
	/* Enabled notifications */
	g_signal_handler_block (G_OBJECT (applet->notifications_enabled_item),
	                        applet->notifications_enabled_toggled_id);
	if (   gconf_client_get_bool (applet->gconf_client, PREF_DISABLE_CONNECTED_NOTIFICATIONS, NULL)
	    && gconf_client_get_bool (applet->gconf_client, PREF_DISABLE_DISCONNECTED_NOTIFICATIONS, NULL)
	    && gconf_client_get_bool (applet->gconf_client, PREF_SUPPRESS_WIRELESS_NETWORKS_AVAILABLE, NULL))
		notifications_enabled = FALSE;
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (applet->notifications_enabled_item), notifications_enabled);
	g_signal_handler_unblock (G_OBJECT (applet->notifications_enabled_item),
	                          applet->notifications_enabled_toggled_id);
#endif

	/* Don't show wifi-specific stuff if wireless is off */
	if (state != BM_STATE_ASLEEP) {
		const GPtrArray *devices;
		int i;

		devices = bm_client_get_devices (applet->bm_client);
		for (i = 0; devices && (i < devices->len); i++) {
			//BMDevice *candidate = g_ptr_array_index (devices, i);
		}
	}
}

static void
ce_child_setup (gpointer user_data G_GNUC_UNUSED)
{
	/* We are in the child process at this point */
	pid_t pid = getpid ();
	setpgid (pid, pid);
}

static void
bma_edit_connections_cb (GtkMenuItem *mi, BMApplet *applet)
{
	char *argv[2];
	GError *error = NULL;
	gboolean success;

	argv[0] = BINDIR "/bm-connection-editor";
	argv[1] = NULL;

	success = g_spawn_async ("/", argv, NULL, 0, &ce_child_setup, NULL, NULL, &error);
	if (!success) {
		g_warning ("Error launching connection editor: %s", error->message);
		g_error_free (error);
	}
}

static void
applet_connection_info_cb (BMApplet *applet)
{
	applet_info_dialog_show (applet);
}

/*
 * bma_context_menu_create
 *
 * Generate the contextual popup menu.
 *
 */
static GtkWidget *bma_context_menu_create (BMApplet *applet, GtkMenuShell *menu)
{
#ifndef ENABLE_INDICATOR
	GtkWidget *menu_item;
#endif
	GtkWidget *image;
	guint id;

	g_return_val_if_fail (applet != NULL, NULL);

	// FIXME items here

	bma_menu_add_separator_item (GTK_WIDGET (menu));

#ifndef ENABLE_INDICATOR
	/* Toggle notifications item */
	applet->notifications_enabled_item = gtk_check_menu_item_new_with_mnemonic (_("Enable N_otifications"));
	id = g_signal_connect (applet->notifications_enabled_item,
	                       "toggled",
	                       G_CALLBACK (bma_set_notifications_enabled_cb),
	                       applet);
	applet->notifications_enabled_toggled_id = id;
	gtk_menu_shell_append (menu, applet->notifications_enabled_item);

	bma_menu_add_separator_item (GTK_WIDGET (menu));
#endif /* ifndef ENABLE_INDICATOR */

	/* 'Connection Information' item */
	applet->info_menu_item = gtk_image_menu_item_new_with_mnemonic (_("Connection _Information"));
	g_signal_connect_swapped (applet->info_menu_item,
	                          "activate",
	                          G_CALLBACK (applet_connection_info_cb),
	                          applet);
	image = gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (applet->info_menu_item), image);
	gtk_menu_shell_append (menu, applet->info_menu_item);

	/* 'Edit Connections...' item */
	applet->connections_menu_item = gtk_image_menu_item_new_with_mnemonic (_("Edit Connections..."));
	g_signal_connect (applet->connections_menu_item,
				   "activate",
				   G_CALLBACK (bma_edit_connections_cb),
				   applet);
	image = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (applet->connections_menu_item), image);
	gtk_menu_shell_append (menu, applet->connections_menu_item);

#ifndef ENABLE_INDICATOR
	/* Separator */
	bma_menu_add_separator_item (GTK_WIDGET (menu));

	/* About item */
	menu_item = gtk_image_menu_item_new_with_mnemonic (_("_About"));
	g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (applet_about_dialog_show), applet);
	image = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (menu, menu_item);
#endif

	gtk_widget_show_all (GTK_WIDGET (menu));

	return GTK_WIDGET (menu);
}


/*****************************************************************************/

static void
foo_set_icon (BMApplet *applet, guint32 layer, GdkPixbuf *pixbuf, char *icon_name, char *new_tip)
{
	GString *tip = NULL;
#ifndef ENABLE_INDICATOR
	int i;
#endif

	if (layer > ICON_LAYER_MAX) {
		g_warning ("Tried to icon to invalid layer %d", layer);
		return;
	}

	switch (layer) {
		case ICON_LAYER_LINK:
			if (new_tip == NULL)
                		new_tip = g_strdup (_("No network connection"));
			tip = g_string_new (new_tip);
			break;
		default:
			tip = g_string_new ("");
			break;
	}

	if (tip->len) {
		g_free (applet->tip);
		applet->tip = tip->str;
	}

	g_free (new_tip);
	g_string_free (tip, FALSE);

#ifndef ENABLE_INDICATOR

	/* Ignore setting of the same icon as is already displayed */
	if (applet->icon_layers[layer] == pixbuf)
		return;

	if (applet->icon_layers[layer]) {
		g_object_unref (applet->icon_layers[layer]);
		applet->icon_layers[layer] = NULL;
	}

	if (pixbuf)
		applet->icon_layers[layer] = g_object_ref (pixbuf);

	if (!applet->icon_layers[0]) {
		bma_icon_check_and_load ("bm-no-connection", &applet->no_connection_icon, applet);
		pixbuf = g_object_ref (applet->no_connection_icon);
	} else {
		pixbuf = gdk_pixbuf_copy (applet->icon_layers[0]);

		for (i = ICON_LAYER_LINK + 1; i <= ICON_LAYER_MAX; i++) {
			GdkPixbuf *top = applet->icon_layers[i];

			if (!top)
				continue;

			gdk_pixbuf_composite (top, pixbuf, 0, 0, gdk_pixbuf_get_width (top),
							  gdk_pixbuf_get_height (top),
							  0, 0, 1.0, 1.0,
							  GDK_INTERP_NEAREST, 255);
		}
	}

	gtk_status_icon_set_from_pixbuf (applet->status_icon, pixbuf);
	g_object_unref (pixbuf);

#if GTK_CHECK_VERSION(2, 15, 0)
	gtk_status_icon_set_tooltip_text (applet->status_icon, applet->tip);
#else
	gtk_status_icon_set_tooltip (applet->status_icon, applet->tip);
#endif

#else /* ENABLE_INDICATOR */

	if (icon_name == NULL && layer == ICON_LAYER_LINK) {
                icon_name = g_strdup ("bm-no-connection");
	}

	if (icon_name != NULL && g_strcmp0 (app_indicator_get_icon (applet->status_icon), icon_name) != 0) {
		app_indicator_set_icon_full (applet->status_icon, icon_name, applet->tip);
		g_free (icon_name);
	}

#endif /* ENABLE_INDICATOR */
}

BMSettingsConnectionInterface *
applet_get_exported_connection_for_device (BMDevice *device, BMApplet *applet)
{
	const GPtrArray *active_connections;
	int i;

	active_connections = bm_client_get_active_connections (applet->bm_client);
	for (i = 0; active_connections && (i < active_connections->len); i++) {
		BMActiveConnection *active;
		BMSettingsConnectionInterface *connection;
		const char *service_name;
		const char *connection_path;
		const GPtrArray *devices;

		active = g_ptr_array_index (active_connections, i);
		if (!active)
			continue;

		devices = bm_active_connection_get_devices (active);
		service_name = bm_active_connection_get_service_name (active);
		connection_path = bm_active_connection_get_connection (active);
		if (!devices || !service_name || !connection_path)
			continue;

		if (strcmp (service_name, BM_DBUS_SERVICE_USER_SETTINGS) != 0)
			continue;

		if (!bm_g_ptr_array_contains (devices, device))
			continue;

		connection = bm_settings_interface_get_connection_by_path (BM_SETTINGS_INTERFACE (applet->gconf_settings), connection_path);
		if (connection)
			return connection;
	}
	return NULL;
}

static gboolean
select_merged_notification_text (OfflineNotificationContextInfo *info)
{
	info->urgency = NOTIFY_URGENCY_LOW;
	/* only do something if this is about full offline state */
	if(info->state != BM_STATE_UNKNOWN || info->device_state != BM_DEVICE_STATE_UNKNOWN) {
		info->urgency = NOTIFY_URGENCY_NORMAL;
		if (!info->title)
			info->title = g_strdup (_("Network"));
	        if (info->state == BM_STATE_DISCONNECTED || info->state == BM_STATE_ASLEEP) {
			info->urgency = NOTIFY_URGENCY_CRITICAL;
			info->text = _("Disconnected - you are now offline");
		} else
			info->text = _("Disconnected");

		switch (info->device_type) {
			default:
				info->icon = "notification-network-disconnected";
				break;
		}
		g_debug("going for offline with icon: %s", info->icon);
		return TRUE;
	}
	return FALSE;
}

static gboolean
foo_online_offline_deferred_notify (gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	OfflineNotificationContextInfo *info = applet->notification_queue_data;
	if(select_merged_notification_text (info))
		applet_do_notify (applet, info->urgency, info->title, info->text, info->icon, NULL, NULL, NULL, applet);
	else
		g_debug("no notification because merged found that we have nothing to say (e.g. not offline)");
	if (info->title)
		g_free (info->title);
	info->title = NULL;
	g_free (applet->notification_queue_data);
	applet->notification_queue_data = NULL;
	applet->deferred_id = 0;
	return FALSE;
}

static void
applet_common_device_state_changed (BMDevice *device,
                                    BMDeviceState new_state,
                                    BMDeviceState old_state,
                                    BMDeviceStateReason reason,
                                    BMApplet *applet)
{
	gboolean device_activating = FALSE;
	BMConnection *connection;
	BMActiveConnection *active = NULL;

	device_activating = applet_is_any_device_activating (applet);

	switch (new_state) {
	case BM_DEVICE_STATE_FAILED:
	case BM_DEVICE_STATE_DISCONNECTED:
	case BM_DEVICE_STATE_UNAVAILABLE:
	{
		if (old_state != BM_DEVICE_STATE_FAILED &&
		    old_state != BM_DEVICE_STATE_UNKNOWN &&
		    old_state != BM_DEVICE_STATE_DISCONNECTED &&
		    old_state != BM_DEVICE_STATE_UNAVAILABLE) {
	                OfflineNotificationContextInfo *info = applet->notification_queue_data;
			if (!info) {
				info = g_new0(OfflineNotificationContextInfo, 1);
				applet->notification_queue_data = info;
			}

	                info->device_state = new_state;
	                info->device_state_reason = reason;
			if (info->title) {
				g_free(info->title);
				info->title = NULL;
			}

			info->device_type = BM_DEVICE_TYPE_UNKNOWN;
			info->title = g_strdup (_("Network"));

	                if (applet->deferred_id)
	                        g_source_remove (applet->deferred_id);
	                applet->deferred_id = g_timeout_add (1000, foo_online_offline_deferred_notify, applet);

			clear_animation_timeout (applet);
		} else {
			g_debug ("old state indicates that this was not a disconnect %d", old_state);
		}
		break;
	}
	case BM_DEVICE_STATE_PREPARE:
	case BM_DEVICE_STATE_CONFIG:
	case BM_DEVICE_STATE_NEED_AUTH:
	case BM_DEVICE_STATE_IP_CONFIG:
		/* Be sure to turn animation timeout on here since the dbus signals
		 * for new active connections or devices might not have come through yet.
		 */
		device_activating = TRUE;
		break;
	case BM_DEVICE_STATE_ACTIVATED:
		/* If the device activation was successful, update the corresponding
		 * connection object with a current timestamp.
		 */
		connection = applet_find_active_connection_for_device (device, applet, &active);
		if (connection && (bm_connection_get_scope (connection) == BM_CONNECTION_SCOPE_USER))
			update_connection_timestamp (active, connection, applet);
		break;
	default:
		break;
	}

	if (device_activating)
		start_animation_timeout (applet);
	else
		clear_animation_timeout (applet);
}

static void
foo_device_state_changed_cb (BMDevice *device,
                             BMDeviceState new_state,
                             BMDeviceState old_state,
                             BMDeviceStateReason reason,
                             gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	BMADeviceClass *dclass;

	dclass = get_device_class (device, applet);
	g_assert (dclass);

	dclass->device_state_changed (device, new_state, old_state, reason, applet);
	applet_common_device_state_changed (device, new_state, old_state, reason, applet);

	applet_schedule_update_icon (applet);
	// FIXME applet_schedule_update_menu (applet);
}

#ifdef ENABLE_INDICATOR
static void
foo_device_removed_cb (BMClient *client, BMDevice *device, gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);

	applet_schedule_update_icon (applet);
	applet_schedule_update_menu (applet);
}
#endif

static void
foo_device_added_cb (BMClient *client, BMDevice *device, gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	BMADeviceClass *dclass;

	dclass = get_device_class (device, applet);
	g_return_if_fail (dclass != NULL);

	if (dclass->device_added)
		dclass->device_added (device, applet);

	g_signal_connect (device, "state-changed",
				   G_CALLBACK (foo_device_state_changed_cb),
				   user_data);

	foo_device_state_changed_cb	(device,
	                             bm_device_get_state (device),
	                             BM_DEVICE_STATE_UNKNOWN,
	                             BM_DEVICE_STATE_REASON_NONE,
	                             applet);
}

static void
foo_client_state_changed_cb (BMClient *client, GParamSpec *pspec, gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);

	g_debug("foo_client_state_changed_cb");
	switch (bm_client_get_state (client)) {
	case BM_STATE_DISCONNECTED:
	case BM_STATE_ASLEEP:
	{
		OfflineNotificationContextInfo *info = applet->notification_queue_data;
		if (!info) {
			info = g_new0(OfflineNotificationContextInfo, 1);
			applet->notification_queue_data = info;
		}

		info->state = bm_client_get_state (client);
		select_merged_notification_text (info);

		if (applet->deferred_id)
			g_source_remove (applet->deferred_id);
		applet->deferred_id = g_timeout_add (1000, foo_online_offline_deferred_notify, applet);

		/* Fall through */
	}
	default:
		break;
	}

	applet_schedule_update_icon (applet);
	// FIMXE applet_schedule_update_menu (applet);
}

static void
foo_manager_running_cb (BMClient *client,
                        GParamSpec *pspec,
                        gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);

	if (bm_client_get_manager_running (client)) {
		g_message ("NM appeared");
	} else {
		g_message ("NM disappeared");
		clear_animation_timeout (applet);
	}

	applet_schedule_update_icon (applet);
}

static void
foo_active_connections_changed_cb (BMClient *client,
                                   GParamSpec *pspec,
                                   gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	// const GPtrArray *active_list;

	applet_schedule_update_icon (applet);
}

static void
foo_manager_permission_changed (BMClient *client,
                                BMClientPermission permission,
                                BMClientPermissionResult result,
                                gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);

	if (permission <= BM_CLIENT_PERMISSION_LAST)
		applet->permissions[permission] = result;
}

static gboolean
foo_set_initial_state (gpointer data)
{
	BMApplet *applet = BM_APPLET (data);
	const GPtrArray *devices;
	int i;

	devices = bm_client_get_devices (applet->bm_client);
	for (i = 0; devices && (i < devices->len); i++)
		foo_device_added_cb (applet->bm_client, BM_DEVICE (g_ptr_array_index (devices, i)), applet);

	foo_active_connections_changed_cb (applet->bm_client, NULL, applet);

	applet_schedule_update_icon (applet);

	return FALSE;
}

static void
foo_client_setup (BMApplet *applet)
{
	applet->bm_client = bm_client_new ();
	if (!applet->bm_client)
		return;

	g_signal_connect (applet->bm_client, "notify::state",
	                  G_CALLBACK (foo_client_state_changed_cb),
	                  applet);
	g_signal_connect (applet->bm_client, "notify::active-connections",
	                  G_CALLBACK (foo_active_connections_changed_cb),
	                  applet);
	g_signal_connect (applet->bm_client, "device-added",
	                  G_CALLBACK (foo_device_added_cb),
	                  applet);
#ifdef ENABLE_INDICATOR
	g_signal_connect (applet->bm_client, "device-removed",
			  G_CALLBACK (foo_device_removed_cb),
			  applet);
#endif
	g_signal_connect (applet->bm_client, "notify::manager-running",
	                  G_CALLBACK (foo_manager_running_cb),
	                  applet);

	g_signal_connect (applet->bm_client, "permission-changed",
	                  G_CALLBACK (foo_manager_permission_changed),
	                  applet);
	/* Initialize permissions - the initial 'permission-changed' signal is emitted from BMClient constructor, and thus not caught */
	applet->permissions[BM_CLIENT_PERMISSION_ENABLE_DISABLE_NETWORK] = bm_client_get_permission_result (applet->bm_client, BM_CLIENT_PERMISSION_ENABLE_DISABLE_NETWORK);
	applet->permissions[BM_CLIENT_PERMISSION_ENABLE_DISABLE_WIFI] = bm_client_get_permission_result (applet->bm_client, BM_CLIENT_PERMISSION_ENABLE_DISABLE_WIFI);
	applet->permissions[BM_CLIENT_PERMISSION_ENABLE_DISABLE_WWAN] = bm_client_get_permission_result (applet->bm_client, BM_CLIENT_PERMISSION_ENABLE_DISABLE_WWAN);
	applet->permissions[BM_CLIENT_PERMISSION_USE_USER_CONNECTIONS] = bm_client_get_permission_result (applet->bm_client, BM_CLIENT_PERMISSION_USE_USER_CONNECTIONS);

	if (bm_client_get_manager_running (applet->bm_client))
		g_idle_add (foo_set_initial_state, applet);

	applet_schedule_update_icon (applet);
}

static void
applet_common_get_device_icon (BMDeviceState state,
				GdkPixbuf **out_pixbuf,
				char **out_indicator_icon,
				BMApplet *applet)
{
	int stage = -1;

	switch (state) {
	case BM_DEVICE_STATE_PREPARE:
		stage = 0;
		break;
	case BM_DEVICE_STATE_CONFIG:
	case BM_DEVICE_STATE_NEED_AUTH:
		stage = 1;
		break;
	case BM_DEVICE_STATE_IP_CONFIG:
		stage = 2;
		break;
	default:
		break;
	}

	if (stage >= 0) {
		/* Don't overwrite pixbufs or names given by specific device classes */
		if (out_pixbuf && !*out_pixbuf) {
			int i, j;
			for (i = 0; i < NUM_CONNECTING_STAGES; i++) {
				for (j = 0; j < NUM_CONNECTING_FRAMES; j++) {
					char *name;

					name = g_strdup_printf ("bm-stage%02d-connecting%02d", i+1, j+1);
					bma_icon_check_and_load (name, &applet->network_connecting_icons[i][j], applet);
					g_free (name);
				}
			}
			*out_pixbuf = applet->network_connecting_icons[stage][applet->animation_step];
		}
		if (out_indicator_icon && !*out_indicator_icon) {
			*out_indicator_icon = g_strdup_printf ("bm-stage%02d-connecting%02d",
								stage + 1,
								applet->animation_step + 1);
		}

		applet->animation_step++;
		if (applet->animation_step >= NUM_CONNECTING_FRAMES)
			applet->animation_step = 0;
	}
}

static char *
get_tip_for_device_state (BMDevice *device,
                          BMDeviceState state,
                          BMConnection *connection)
{
	BMSettingConnection *s_con;
	char *tip = NULL;
	const char *id = NULL;

	id = bm_device_get_iface (device);
	if (connection) {
		s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
		id = bm_setting_connection_get_id (s_con);
	}

	switch (state) {
	case BM_DEVICE_STATE_PREPARE:
	case BM_DEVICE_STATE_CONFIG:
		tip = g_strdup_printf (_("Preparing network connection '%s'..."), id);
		break;
	case BM_DEVICE_STATE_NEED_AUTH:
		tip = g_strdup_printf (_("User authentication required for network connection '%s'..."), id);
		break;
	case BM_DEVICE_STATE_IP_CONFIG:
		tip = g_strdup_printf (_("Requesting a network address for '%s'..."), id);
		break;
	case BM_DEVICE_STATE_ACTIVATED:
		tip = g_strdup_printf (_("Network connection '%s' active"), id);
		break;
	default:
		break;
	}

	return tip;
}

static void
applet_get_device_icon_for_state (BMApplet *applet,
                                  GdkPixbuf **out_pixbuf,
                                  char **out_indicator_icon,
                                  char **out_tip)
{
	BMActiveConnection *active;
	BMDevice *device = NULL;
	BMDeviceState state = BM_DEVICE_STATE_UNKNOWN;
	BMADeviceClass *dclass;

	// FIXME: handle multiple device states here

	/* First show the best activating device's state */
	active = applet_get_best_activating_connection (applet, &device);
	if (!active || !device) {
		/* If there aren't any activating devices, then show the state of
		 * the default active connection instead.
		 */
		active = applet_get_default_active_connection (applet, &device);
		if (!active || !device)
			goto out;
	}

	state = bm_device_get_state (device);

	dclass = get_device_class (device, applet);
	if (dclass) {
		BMConnection *connection;

		connection = applet_find_active_connection_for_device (device, applet, NULL);
		dclass->get_icon (device, state, connection, out_pixbuf, out_indicator_icon, out_tip, applet);
		if (out_tip && !*out_tip)
			*out_tip = get_tip_for_device_state (device, state, connection);
	}

out:
	applet_common_get_device_icon (state, out_pixbuf, out_indicator_icon, applet);
}

static gboolean
applet_update_icon (gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	GdkPixbuf *pixbuf = NULL;
	BMState state;
	char *dev_tip = NULL, *icon_name = NULL;
	gboolean bm_running;

	applet->update_icon_id = 0;

	bm_running = bm_client_get_manager_running (applet->bm_client);

	/* Handle device state first */

	state = bm_client_get_state (applet->bm_client);
	if (!bm_running)
		state = BM_STATE_UNKNOWN;

#ifdef ENABLE_INDICATOR
	if (bm_running)
		app_indicator_set_status (applet->status_icon, APP_INDICATOR_STATUS_ACTIVE);
	else
		app_indicator_set_status (applet->status_icon, APP_INDICATOR_STATUS_PASSIVE);
#endif

	switch (state) {
	case BM_STATE_UNKNOWN:
	case BM_STATE_ASLEEP:
		icon_name = g_strdup ("bm-no-connection");
		pixbuf = bma_icon_check_and_load (icon_name, &applet->no_connection_icon, applet);
		dev_tip = g_strdup (_("Networking disabled"));
		break;
	case BM_STATE_DISCONNECTED:
		icon_name = g_strdup ("bm-no-connection");
		pixbuf = bma_icon_check_and_load (icon_name, &applet->no_connection_icon, applet);
		dev_tip = g_strdup (_("No network connection"));
		break;
	default:
		applet_get_device_icon_for_state (applet, &pixbuf, &icon_name, &dev_tip);
		break;
	}

	foo_set_icon (applet, ICON_LAYER_LINK, pixbuf, icon_name, dev_tip);

	return FALSE;
}

#ifdef ENABLE_INDICATOR
static gboolean
indicator_update_menu (BMApplet *applet)
{
	bma_menu_clear (applet);
	bma_menu_show_cb (applet->menu, applet);
	bma_menu_add_separator_item (applet->menu);
	applet->menu = bma_context_menu_create (applet, GTK_MENU_SHELL(applet->menu));
	bma_context_menu_update (applet);
	app_indicator_set_menu (applet->status_icon, GTK_MENU(applet->menu));
	applet->menu_update_id = 0;

	return FALSE;
}
#endif /* ENABLE_INDICATOR */

void
applet_schedule_update_icon (BMApplet *applet)
{
	if (!applet->update_icon_id)
		applet->update_icon_id = g_idle_add (applet_update_icon, applet);
}


static BMDevice *
find_active_device (BMAGConfConnection *connection,
                    BMApplet *applet,
                    BMActiveConnection **out_active_connection)
{
	const GPtrArray *active_connections;
	int i;

	g_return_val_if_fail (connection != NULL, NULL);
	g_return_val_if_fail (applet != NULL, NULL);
	g_return_val_if_fail (out_active_connection != NULL, NULL);
	g_return_val_if_fail (*out_active_connection == NULL, NULL);

	/* Look through the active connection list trying to find the D-Bus
	 * object path of applet_connection.
	 */
	active_connections = bm_client_get_active_connections (applet->bm_client);
	for (i = 0; active_connections && (i < active_connections->len); i++) {
		BMActiveConnection *active;
		const char *service_name;
		const char *connection_path;
		const GPtrArray *devices;

		active = BM_ACTIVE_CONNECTION (g_ptr_array_index (active_connections, i));
		service_name = bm_active_connection_get_service_name (active);
		if (!service_name) {
			/* Shouldn't happen; but we shouldn't crash either */
			g_warning ("%s: couldn't get service name for active connection!", __func__);
			continue;
		}

		if (strcmp (service_name, BM_DBUS_SERVICE_USER_SETTINGS))
			continue;

		connection_path = bm_active_connection_get_connection (active);
		if (!connection_path) {
			/* Shouldn't happen; but we shouldn't crash either */
			g_warning ("%s: couldn't get connection path for active connection!", __func__);
			continue;
		}

		if (!strcmp (connection_path, bm_connection_get_path (BM_CONNECTION (connection)))) {
			devices = bm_active_connection_get_devices (active);
			if (devices)
				*out_active_connection = active;
			return devices ? BM_DEVICE (g_ptr_array_index (devices, 0)) : NULL;
		}
	}

	return NULL;
}

static void
applet_settings_new_secrets_requested_cb (BMAGConfSettings *settings,
                                          BMAGConfConnection *connection,
                                          const char *setting_name,
                                          const char **hints,
                                          gboolean ask_user,
                                          BMANewSecretsRequestedFunc callback,
                                          gpointer callback_data,
                                          gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	BMActiveConnection *active_connection = NULL;
	BMSettingConnection *s_con;
	BMDevice *device;
	BMADeviceClass *dclass;
	GError *error = NULL;

	s_con = (BMSettingConnection *) bm_connection_get_setting (BM_CONNECTION (connection), BM_TYPE_SETTING_CONNECTION);
	g_return_if_fail (s_con != NULL);

	/* Find the active device for this connection */
	device = find_active_device (connection, applet, &active_connection);
	if (!device || !active_connection) {
		g_set_error (&error,
		             BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		             "%s.%d (%s): couldn't find details for connection",
		             __FILE__, __LINE__, __func__);
		goto error;
	}

	dclass = get_device_class (device, applet);
	if (!dclass) {
		g_set_error (&error,
		             BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
		             "%s.%d (%s): device type unknown",
		             __FILE__, __LINE__, __func__);
		goto error;
	}

	if (!dclass->get_secrets) {
		g_set_error (&error,
		             BM_SETTINGS_INTERFACE_ERROR,
		             BM_SETTINGS_INTERFACE_ERROR_SECRETS_UNAVAILABLE,
		             "%s.%d (%s): no secrets found",
		             __FILE__, __LINE__, __func__);
		goto error;
	}

	// FIXME: get secrets locally and populate connection with previous secrets
	// before asking user for other secrets

	/* Let the device class handle secrets */
	if (dclass->get_secrets (device, BM_SETTINGS_CONNECTION_INTERFACE (connection),
	                         active_connection, setting_name, hints, callback,
	                         callback_data, applet, &error))
		return;  /* success */

error:
	g_warning ("%s", error->message);
	callback (BM_SETTINGS_CONNECTION_INTERFACE (connection), NULL, error, callback_data);
	g_error_free (error);
}

static gboolean
periodic_update_active_connection_timestamps (gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	const GPtrArray *connections;
	int i;

	if (!applet->bm_client || !bm_client_get_manager_running (applet->bm_client))
		return TRUE;

	connections = bm_client_get_active_connections (applet->bm_client);
	for (i = 0; connections && (i < connections->len); i++) {
		BMActiveConnection *active = BM_ACTIVE_CONNECTION (g_ptr_array_index (connections, i));
		const char *path;
		BMSettingsConnectionInterface *connection;
		const GPtrArray *devices;
		int k;

		if (bm_active_connection_get_scope (active) == BM_CONNECTION_SCOPE_SYSTEM)
			continue;

		path = bm_active_connection_get_connection (active);
		connection = bm_settings_interface_get_connection_by_path (BM_SETTINGS_INTERFACE (applet->gconf_settings), path);
		if (!connection || !BMA_IS_GCONF_CONNECTION (connection))
			continue;

		devices = bm_active_connection_get_devices (active);
		if (!devices || !devices->len)
			continue;

		/* Check if a device owned by the active connection is completely
		 * activated before updating timestamp.
		 */
		for (k = 0; devices && (k < devices->len); k++) {
			BMDevice *device = BM_DEVICE (g_ptr_array_index (devices, k));

			if (bm_device_get_state (device) == BM_DEVICE_STATE_ACTIVATED) {
				BMSettingConnection *s_con;

				s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (BM_CONNECTION (connection), BM_TYPE_SETTING_CONNECTION));
				g_assert (s_con);

				g_object_set (s_con, BM_SETTING_CONNECTION_TIMESTAMP, (guint64) time (NULL), NULL);
				/* Ignore secrets since we're just updating the timestamp */
				bma_gconf_connection_update (BMA_GCONF_CONNECTION (connection), TRUE);
				break;
			}
		}
	}

	return TRUE;
}

/*****************************************************************************/

static void
bma_clear_icon (GdkPixbuf **icon, BMApplet *applet)
{
	g_return_if_fail (icon != NULL);
	g_return_if_fail (applet != NULL);

	if (*icon && (*icon != applet->fallback_icon)) {
		g_object_unref (*icon);
		*icon = NULL;
	}
}

static void bma_icons_free (BMApplet *applet)
{
	int i, j;

	for (i = 0; i <= ICON_LAYER_MAX; i++)
		bma_clear_icon (&applet->icon_layers[i], applet);

	bma_clear_icon (&applet->no_connection_icon, applet);
	bma_clear_icon (&applet->wired_icon, applet);
	bma_clear_icon (&applet->adhoc_icon, applet);
	bma_clear_icon (&applet->wwan_icon, applet);
	bma_clear_icon (&applet->wwan_tower_icon, applet);
	bma_clear_icon (&applet->wireless_00_icon, applet);
	bma_clear_icon (&applet->wireless_25_icon, applet);
	bma_clear_icon (&applet->wireless_50_icon, applet);
	bma_clear_icon (&applet->wireless_75_icon, applet);
	bma_clear_icon (&applet->wireless_100_icon, applet);
	bma_clear_icon (&applet->secure_lock_icon, applet);

	bma_clear_icon (&applet->mb_tech_1x_icon, applet);
	bma_clear_icon (&applet->mb_tech_evdo_icon, applet);
	bma_clear_icon (&applet->mb_tech_gprs_icon, applet);
	bma_clear_icon (&applet->mb_tech_edge_icon, applet);
	bma_clear_icon (&applet->mb_tech_umts_icon, applet);
	bma_clear_icon (&applet->mb_tech_hspa_icon, applet);
	bma_clear_icon (&applet->mb_roaming_icon, applet);
	bma_clear_icon (&applet->mb_tech_3g_icon, applet);

	for (i = 0; i < NUM_CONNECTING_STAGES; i++) {
		for (j = 0; j < NUM_CONNECTING_FRAMES; j++)
			bma_clear_icon (&applet->network_connecting_icons[i][j], applet);
	}

	for (i = 0; i <= ICON_LAYER_MAX; i++)
		bma_clear_icon (&applet->icon_layers[i], applet);
}

GdkPixbuf *
bma_icon_check_and_load (const char *name, GdkPixbuf **icon, BMApplet *applet)
{
	GError *error = NULL;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (icon != NULL, NULL);
	g_return_val_if_fail (applet != NULL, NULL);

	/* icon already loaded successfully */
	if (*icon && (*icon != applet->fallback_icon))
		return *icon;

	/* Try to load the icon; if the load fails, log the problem, and set
	 * the icon to the fallback icon if requested.
	 */
	*icon = gtk_icon_theme_load_icon (applet->icon_theme, name, applet->icon_size, 0, &error);
	if (!*icon) {
		g_warning ("Icon %s missing: (%d) %s",
		           name,
		           error ? error->code : -1,
			       (error && error->message) ? error->message : "(unknown)");
		g_clear_error (&error);

		*icon = applet->fallback_icon;
	}
	return *icon;
}

#include "fallback-icon.h"

static gboolean
bma_icons_reload (BMApplet *applet)
{
	GError *error = NULL;
	GdkPixbufLoader *loader;

	g_return_val_if_fail (applet->icon_size > 0, FALSE);

	bma_icons_free (applet);

	loader = gdk_pixbuf_loader_new_with_type ("png", &error);
	if (!loader)
		goto error;

	if (!gdk_pixbuf_loader_write (loader,
	                              fallback_icon_data,
	                              sizeof (fallback_icon_data),
	                              &error))
		goto error;

	if (!gdk_pixbuf_loader_close (loader, &error))
		goto error;

	applet->fallback_icon = gdk_pixbuf_loader_get_pixbuf (loader);
	g_object_ref (applet->fallback_icon);
	g_assert (applet->fallback_icon);
	g_object_unref (loader);

	return TRUE;

error:
	g_warning ("Could not load fallback icon: (%d) %s",
	           error ? error->code : -1,
		       (error && error->message) ? error->message : "(unknown)");
	g_clear_error (&error);
	/* Die if we can't get a fallback icon */
	g_assert (FALSE);
	return FALSE;
}

static void bma_icon_theme_changed (GtkIconTheme *icon_theme, BMApplet *applet)
{
	bma_icons_reload (applet);
}

static void bma_icons_init (BMApplet *applet)
{
	GdkScreen *screen;
	gboolean path_appended;

	if (applet->icon_theme) {
		g_signal_handlers_disconnect_by_func (applet->icon_theme,
						      G_CALLBACK (bma_icon_theme_changed),
						      applet);
		g_object_unref (G_OBJECT (applet->icon_theme));
	}

#ifdef ENABLE_INDICATOR
	screen = gdk_screen_get_default();
#else
	screen = gtk_status_icon_get_screen (applet->status_icon);
#endif
	g_assert (screen);
	applet->icon_theme = gtk_icon_theme_get_for_screen (screen);

	/* If not done yet, append our search path */
	path_appended = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (applet->icon_theme),
					 		    "BMAIconPathAppended"));
	if (path_appended == FALSE) {
		gtk_icon_theme_append_search_path (applet->icon_theme, ICONDIR);
		g_object_set_data (G_OBJECT (applet->icon_theme),
				   "BMAIconPathAppended",
				   GINT_TO_POINTER (TRUE));
	}

	g_signal_connect (applet->icon_theme, "changed", G_CALLBACK (bma_icon_theme_changed), applet);
}

#ifndef ENABLE_INDICATOR
static void
status_icon_screen_changed_cb (GtkStatusIcon *icon,
                               GParamSpec *pspec,
                               BMApplet *applet)
{
	bma_icons_init (applet);
	bma_icon_theme_changed (NULL, applet);
}

static gboolean
status_icon_size_changed_cb (GtkStatusIcon *icon,
                             gint size,
                             BMApplet *applet)
{
	/* icon_size may be 0 if for example the panel hasn't given us any space
	 * yet.  We'll get resized later, but for now just load the 16x16 icons.
	 */
	applet->icon_size = MAX (16, size);

	bma_icons_reload (applet);

	applet_schedule_update_icon (applet);

	return TRUE;
}
#endif

#ifndef ENABLE_INDICATOR
static void
status_icon_activate_cb (GtkStatusIcon *icon, BMApplet *applet)
{
	/* Have clicking on the applet act also as acknowledgement
	 * of the notification. 
	 */
	bma_menu_clear (applet);
	gtk_menu_popup (GTK_MENU (applet->menu), NULL, NULL,
			gtk_status_icon_position_menu, icon,
			1, gtk_get_current_event_time ());
}
#endif /* ENABLE_INDICATOR */

#ifndef ENABLE_INDICATOR
static void
status_icon_popup_menu_cb (GtkStatusIcon *icon,
                           guint button,
                           guint32 activate_time,
                           BMApplet *applet)
{
	/* Have clicking on the applet act also as acknowledgement
	 * of the notification. 
	 */

	bma_context_menu_update (applet);
	gtk_menu_popup (GTK_MENU (applet->context_menu), NULL, NULL,
			gtk_status_icon_position_menu, icon,
			button, activate_time);
}
#endif

#ifdef ENABLE_INDICATOR

static gboolean
setup_indicator_menu (BMApplet *applet)
{
	g_return_val_if_fail (BM_IS_APPLET (applet), FALSE);

	bma_menu_clear (applet);  /* calls create too */
	if (!applet->menu)
		return FALSE;

	applet->menu = bma_context_menu_create (applet, GTK_MENU_SHELL(applet->menu));
	bma_context_menu_update(applet);

	app_indicator_set_menu(applet->status_icon, GTK_MENU(applet->menu));

	return TRUE;
}

#else

static gboolean
setup_statusicon_menu (BMApplet *applet)
{
	g_return_val_if_fail (BM_IS_APPLET (applet), FALSE);
	g_return_val_if_fail (applet->status_icon, FALSE);

	applet->menu = bma_menu_create (applet);
	if (!applet->menu)
		return FALSE;

	// FIXME	applet->context_menu = GTK_MENU_SHELL (gtk_menu_new ());
	applet->context_menu = bma_context_menu_create (applet, GTK_MENU_SHELL (applet->context_menu));
	if (!applet->context_menu)
		return FALSE;

	return TRUE;
}

#endif /* ENABLE_INDICATOR */

static gboolean
setup_widgets (BMApplet *applet)
{
	gboolean success = FALSE;

	g_return_val_if_fail (BM_IS_APPLET (applet), FALSE);
#if 0
	if (!applet->status_icon)
		return FALSE;
#endif

#ifdef ENABLE_INDICATOR
	success = setup_indicator_menu (applet);
#else
	success = setup_statusicon_menu (applet);
#endif

	return success;
}

static void
applet_pre_keyring_callback (gpointer user_data)
{
	BMApplet *applet = BM_APPLET (user_data);
	GdkScreen *screen;
	GdkDisplay *display;
	GdkWindow *window = NULL;

	if (applet->menu)
		window = gtk_widget_get_window (applet->menu);
	if (window) {
#if GTK_CHECK_VERSION(2,23,0)
		screen = gdk_window_get_screen (window);
#else
		screen = gdk_drawable_get_screen (window);
#endif
		display = gdk_screen_get_display (screen);
		g_object_ref (display);

		gtk_widget_hide (applet->menu);
		gtk_widget_destroy (applet->menu);
		applet->menu = NULL;

		/* Ensure that the widget really gets destroyed before letting the
		 * keyring calls happen; if the X events haven't all gone through when
		 * the keyring dialog comes up, then the menu will actually still have
		 * the screen grab even after we've called gtk_widget_destroy().
		 */
		gdk_display_sync (display);
		g_object_unref (display);
	}

	window = NULL;
	if (applet->context_menu)
		window = gtk_widget_get_window (applet->context_menu);
	if (window) {
#if GTK_CHECK_VERSION(2,23,0)
		screen = gdk_window_get_screen (window);
#else
		screen = gdk_drawable_get_screen (window);
#endif
		display = gdk_screen_get_display (screen);
		g_object_ref (display);

		gtk_widget_hide (applet->context_menu);

		/* Ensure that the widget really gets hidden before letting the
		 * keyring calls happen; if the X events haven't all gone through when
		 * the keyring dialog comes up, then the menu will actually still have
		 * the screen grab even after we've called gtk_widget_hide().
		 */
		gdk_display_sync (display);
		g_object_unref (display);
	}
}

static void
exit_cb (GObject *ignored, gpointer user_data)
{
	BMApplet *applet = user_data;

	g_main_loop_quit (applet->loop);
}

#ifndef ENABLE_INDICATOR
static void
applet_embedded_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	gboolean embedded = gtk_status_icon_is_embedded (GTK_STATUS_ICON (object));

	g_message ("applet now %s the notification area",
	           embedded ? "embedded in" : "removed from");
}
#endif

static GObject *
constructor (GType type,
             guint n_props,
             GObjectConstructParam *construct_props)
{
	BMApplet *applet;
	AppletDBusManager *dbus_mgr;
	GError* error = NULL;

	applet = BM_APPLET (G_OBJECT_CLASS (bma_parent_class)->constructor (type, n_props, construct_props));

	g_set_application_name (_("BarcodeManager Applet"));
	gtk_window_set_default_icon_name (GTK_STOCK_NETWORK);

	applet->ui_file = g_build_filename (UIDIR, "/applet.ui", NULL);
	if (!applet->ui_file || !g_file_test (applet->ui_file, G_FILE_TEST_IS_REGULAR)) {
		GtkWidget *dialog;
		dialog = applet_warning_dialog_show (_("The BarcodeManager Applet could not find some required resources (the .ui file was not found)."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		goto error;
	}

	applet->info_dialog_ui = gtk_builder_new ();

	if (!gtk_builder_add_from_file (applet->info_dialog_ui, applet->ui_file, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
		goto error;
	}

	applet->gconf_client = gconf_client_get_default ();
	if (!applet->gconf_client)
		goto error;

	/* Note that we don't care about change notifications for prefs values... */
	gconf_client_add_dir (applet->gconf_client,
	                      APPLET_PREFS_PATH,
	                      GCONF_CLIENT_PRELOAD_ONELEVEL,
	                      NULL);

	if (!notify_is_initted ())
		notify_init ("BarcodeManager");

	dbus_mgr = applet_dbus_manager_get ();
	if (dbus_mgr == NULL) {
		bm_warning ("Couldn't initialize the D-Bus manager.");
		g_object_unref (applet);
		return NULL;
	}
	g_signal_connect (G_OBJECT (dbus_mgr), "exit-now", G_CALLBACK (exit_cb), applet);

	applet->system_settings = bm_remote_settings_system_new (applet_dbus_manager_get_connection (dbus_mgr));

	applet->gconf_settings = bma_gconf_settings_new (applet_dbus_manager_get_connection (dbus_mgr));
	g_signal_connect (applet->gconf_settings, "new-secrets-requested",
	                  G_CALLBACK (applet_settings_new_secrets_requested_cb),
	                  applet);

	bm_settings_service_export (BM_SETTINGS_SERVICE (applet->gconf_settings));

	/* Start our DBus service */
	if (!applet_dbus_manager_start_service (dbus_mgr)) {
		g_object_unref (applet);
		return NULL;
	}

	/* Initialize device classes */
	applet->bt_class = applet_device_bt_get_class (applet);
	g_assert (applet->bt_class);

	foo_client_setup (applet);

#ifdef ENABLE_INDICATOR
	applet->status_icon = app_indicator_new
				("bm-applet", "bm-no-connection",
				 APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);
#else
	applet->status_icon = gtk_status_icon_new ();

	g_signal_connect (applet->status_icon, "notify::screen",
			  G_CALLBACK (status_icon_screen_changed_cb), applet);
	g_signal_connect (applet->status_icon, "size-changed",
			  G_CALLBACK (status_icon_size_changed_cb), applet);
	g_signal_connect (applet->status_icon, "activate",
			  G_CALLBACK (status_icon_activate_cb), applet);
	g_signal_connect (applet->status_icon, "popup-menu",
			  G_CALLBACK (status_icon_popup_menu_cb), applet);

	/* Track embedding to help debug issues where user has removed the
	 * notification area applet from the panel, and thus bm-applet too.
	 */
	g_signal_connect (applet->status_icon, "notify::embedded",
	                  G_CALLBACK (applet_embedded_cb), NULL);
#endif

	/* Load pixmaps and create applet widgets */
	if (!setup_widgets (applet))
		goto error;
	bma_icons_init (applet);

	/* timeout to update connection timestamps every 5 minutes */
	applet->update_timestamps_id = g_timeout_add_seconds (300,
			(GSourceFunc) periodic_update_active_connection_timestamps, applet);

	bm_gconf_set_pre_keyring_callback (applet_pre_keyring_callback, applet);

#ifndef ENABLE_INDICATOR
	applet_embedded_cb (G_OBJECT (applet->status_icon), NULL, NULL);
#endif

	applet->notify_actions = applet_notify_server_has_actions ();

	return G_OBJECT (applet);

error:
	g_object_unref (applet);
	return NULL;
}

static void finalize (GObject *object)
{
	BMApplet *applet = BM_APPLET (object);

	bm_gconf_set_pre_keyring_callback (NULL, NULL);

	if (applet->update_timestamps_id)
		g_source_remove (applet->update_timestamps_id);

	g_slice_free (BMADeviceClass, applet->wired_class);
	g_slice_free (BMADeviceClass, applet->wifi_class);
	g_slice_free (BMADeviceClass, applet->gsm_class);

	if (applet->update_icon_id)
		g_source_remove (applet->update_icon_id);

	bma_menu_clear (applet);
	bma_icons_free (applet);

	g_free (applet->tip);

	if (applet->notification) {
		notify_notification_close (applet->notification, NULL);
		g_object_unref (applet->notification);
	}

	g_free (applet->ui_file);
	if (applet->info_dialog_ui)
		g_object_unref (applet->info_dialog_ui);

	if (applet->gconf_client) {
		gconf_client_remove_dir (applet->gconf_client,
		                         APPLET_PREFS_PATH,
		                         NULL);
		g_object_unref (applet->gconf_client);
	}

	if (applet->status_icon)
		g_object_unref (applet->status_icon);

	if (applet->bm_client)
		g_object_unref (applet->bm_client);

	if (applet->fallback_icon)
		g_object_unref (applet->fallback_icon);

	if (applet->gconf_settings) {
		g_object_unref (applet->gconf_settings);
		applet->gconf_settings = NULL;
	}
	if (applet->system_settings) {
		g_object_unref (applet->system_settings);
		applet->system_settings = NULL;
	}

	G_OBJECT_CLASS (bma_parent_class)->finalize (object);
}

static void bma_init (BMApplet *applet)
{
	applet->animation_id = 0;
	applet->animation_step = 0;
	applet->icon_theme = NULL;
	applet->notification = NULL;
	applet->icon_size = 16;
}

enum {
	PROP_0,
	PROP_LOOP,
	LAST_PROP
};

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	BMApplet *applet = BM_APPLET (object);

	switch (prop_id) {
	case PROP_LOOP:
		applet->loop = g_value_get_pointer (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void bma_class_init (BMAppletClass *klass)
{
	GObjectClass *oclass = G_OBJECT_CLASS (klass);
	GParamSpec *pspec;

	oclass->set_property = set_property;
	oclass->constructor = constructor;
	oclass->finalize = finalize;

	pspec = g_param_spec_pointer ("loop", "Loop", "Applet mainloop", G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
	g_object_class_install_property (oclass, PROP_LOOP, pspec);
}

BMApplet *
bm_applet_new (GMainLoop *loop)
{
	return g_object_new (BM_TYPE_APPLET, "loop", loop, NULL);
}

