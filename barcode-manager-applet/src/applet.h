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

#ifndef APPLET_H
#define APPLET_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gconf/gconf-client.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <net/ethernet.h>

#include <libnotify/notify.h>

#if ENABLE_INDICATOR
#include <libappindicator/app-indicator.h>
#endif

#include <bm-connection.h>
#include <bm-client.h>
#include <bm-device.h>
#include <BarcodeManager.h>
#include <bm-active-connection.h>
#include <bm-remote-settings-system.h>

#include "applet-dbus-manager.h"
#include "bma-gconf-settings.h"

#define BM_TYPE_APPLET			(bma_get_type())
#define BM_APPLET(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), BM_TYPE_APPLET, BMApplet))
#define BM_APPLET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), BM_TYPE_APPLET, BMAppletClass))
#define BM_IS_APPLET(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), BM_TYPE_APPLET))
#define BM_IS_APPLET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), BM_TYPE_APPLET))
#define BM_APPLET_GET_CLASS(object)(G_TYPE_INSTANCE_GET_CLASS((object), BM_TYPE_APPLET, BMAppletClass))

typedef struct
{
	GObjectClass	parent_class;
} BMAppletClass; 

#define APPLET_PREFS_PATH "/apps/bm-applet"
#define PREF_DISABLE_CONNECTED_NOTIFICATIONS      APPLET_PREFS_PATH "/disable-connected-notifications"
#define PREF_DISABLE_DISCONNECTED_NOTIFICATIONS   APPLET_PREFS_PATH "/disable-disconnected-notifications"
#define PREF_DISABLE_VPN_NOTIFICATIONS            APPLET_PREFS_PATH "/disable-vpn-notifications"
#define PREF_DISABLE_WIFI_CREATE                  APPLET_PREFS_PATH "/disable-wifi-create"
#define PREF_SUPPRESS_WIRELESS_NETWORKS_AVAILABLE APPLET_PREFS_PATH "/suppress-wireless-networks-available"

#define ICON_LAYER_LINK 0
#define ICON_LAYER_VPN 1
#define ICON_LAYER_MAX ICON_LAYER_VPN

typedef struct BMADeviceClass BMADeviceClass;

/*
 * Applet instance data
 *
 */
typedef struct
{
	GObject parent_instance;

	GMainLoop *loop;
	BMClient *bm_client;

	BMRemoteSettingsSystem *system_settings;
	BMAGConfSettings *gconf_settings;

	GConfClient *	gconf_client;
	char	*		ui_file;

	guint update_timestamps_id;

	/* Permissions */
	BMClientPermissionResult permissions[BM_CLIENT_PERMISSION_LAST + 1];

	/* Device classes */
	BMADeviceClass *wired_class;
	BMADeviceClass *wifi_class;
	BMADeviceClass *gsm_class;
	BMADeviceClass *cdma_class;
	BMADeviceClass *bt_class;

	/* Data model elements */
	guint			update_icon_id;

	GtkIconTheme *	icon_theme;
	GdkPixbuf *		no_connection_icon;
	GdkPixbuf *		wired_icon;
	GdkPixbuf *		adhoc_icon;
	GdkPixbuf *		wwan_icon;
	GdkPixbuf *		wireless_00_icon;
	GdkPixbuf *		wireless_25_icon;
	GdkPixbuf *		wireless_50_icon;
	GdkPixbuf *		wireless_75_icon;
	GdkPixbuf *		wireless_100_icon;
	GdkPixbuf *		secure_lock_icon;
#define NUM_CONNECTING_STAGES 3
#define NUM_CONNECTING_FRAMES 11
	GdkPixbuf *		network_connecting_icons[NUM_CONNECTING_STAGES][NUM_CONNECTING_FRAMES];
#define NUM_VPN_CONNECTING_FRAMES 14
	GdkPixbuf *		vpn_connecting_icons[NUM_VPN_CONNECTING_FRAMES];
	GdkPixbuf *		vpn_lock_icon;
	GdkPixbuf *		fallback_icon;

	/* Mobiel Broadband icons */
	GdkPixbuf *		wwan_tower_icon;
	GdkPixbuf *		mb_tech_1x_icon;
	GdkPixbuf *		mb_tech_evdo_icon;
	GdkPixbuf *		mb_tech_gprs_icon;
	GdkPixbuf *		mb_tech_edge_icon;
	GdkPixbuf *		mb_tech_umts_icon;
	GdkPixbuf *		mb_tech_hspa_icon;
	GdkPixbuf *		mb_roaming_icon;
	GdkPixbuf *		mb_tech_3g_icon;

	/* Active status icon pixbufs */
	GdkPixbuf *		icon_layers[ICON_LAYER_MAX + 1];

	/* Animation stuff */
	int				animation_step;
	guint			animation_id;

	/* Direct UI elements */
#if ENABLE_INDICATOR
	AppIndicator *status_icon;
	guint		menu_update_id;
#else
	GtkStatusIcon * status_icon;
#endif
	int             icon_size;

	GtkWidget *		menu;
	char *          tip;

	GtkWidget *		context_menu;
	GtkWidget *		networking_enabled_item;
	guint           networking_enabled_toggled_id;
	GtkWidget *		wifi_enabled_item;
	guint           wifi_enabled_toggled_id;
	GtkWidget *		wwan_enabled_item;
	guint           wwan_enabled_toggled_id;

	GtkWidget *     notifications_enabled_item;
	guint           notifications_enabled_toggled_id;

	GtkWidget *		info_menu_item;
	GtkWidget *		connections_menu_item;

	GtkBuilder *	info_dialog_ui;
	NotifyNotification*	notification;
	gboolean        notify_actions;

	gpointer notification_queue_data;
	guint deferred_id;
} BMApplet;

typedef void (*AppletNewAutoConnectionCallback) (BMConnection *connection,
                                                 gboolean created,
                                                 gboolean canceled,
                                                 gpointer user_data);

struct BMADeviceClass {
	gboolean       (*new_auto_connection)  (BMDevice *device,
	                                        gpointer user_data,
	                                        AppletNewAutoConnectionCallback callback,
	                                        gpointer callback_data);

	void           (*add_menu_item)        (BMDevice *device,
	                                        guint32 num_devices,
	                                        BMConnection *active,
	                                        GtkWidget *menu,
	                                        BMApplet *applet);

	void           (*device_added)         (BMDevice *device, BMApplet *applet);

	void           (*device_state_changed) (BMDevice *device,
	                                        BMDeviceState new_state,
	                                        BMDeviceState old_state,
	                                        BMDeviceStateReason reason,
	                                        BMApplet *applet);

	void           (*get_icon)             (BMDevice *device,
	                                        BMDeviceState state,
	                                        BMConnection *connection,
						GdkPixbuf **out_pixbuf,
						char **out_indicator_icon,
	                                        char **tip,
	                                        BMApplet *applet);

	void           (*get_more_info)        (BMDevice *device,
	                                        BMConnection *connection,
	                                        BMApplet *applet,
	                                        gpointer user_data);

	gboolean       (*get_secrets)          (BMDevice *device,
	                                        BMSettingsConnectionInterface *connection,
	                                        BMActiveConnection *active_connection,
	                                        const char *setting_name,
	                                        const char **hints,
	                                        BMANewSecretsRequestedFunc callback,
	                                        gpointer callback_data,
	                                        BMApplet *applet,
	                                        GError **error);
};

GType bma_get_type (void);

BMApplet *bm_applet_new (GMainLoop *loop);

void applet_schedule_update_icon (BMApplet *applet);

#ifdef ENABLE_INDICATOR
void applet_schedule_update_menu (BMApplet *applet);
#endif

BMSettingsInterface *applet_get_settings (BMApplet *applet);

GSList *applet_get_all_connections (BMApplet *applet);

gboolean bma_menu_device_check_unusable (BMDevice *device);

GtkWidget * bma_menu_device_get_menu_item (BMDevice *device,
                                           BMApplet *applet,
                                           const char *unavailable_msg);

void applet_menu_item_activate_helper (BMDevice *device,
                                       BMConnection *connection,
                                       const char *specific_object,
                                       BMApplet *applet,
                                       gpointer dclass_data);

void applet_menu_item_disconnect_helper (BMDevice *device,
                                         BMApplet *applet);

void applet_menu_item_add_complex_separator_helper (GtkWidget *menu,
                                                    BMApplet *applet,
                                                    const gchar* label,
                                                    int pos);

GtkWidget*
applet_menu_item_create_device_item_helper (BMDevice *device,
                                            BMApplet *applet,
                                            const gchar *text);

BMSettingsConnectionInterface *applet_get_exported_connection_for_device (BMDevice *device, BMApplet *applet);

void applet_do_notify (BMApplet *applet,
                       NotifyUrgency urgency,
                       const char *summary,
                       const char *message,
                       const char *icon,
                       const char *action1,
                       const char *action1_label,
                       NotifyActionCallback action1_cb,
                       gpointer action1_user_data);

void applet_do_notify_with_pref (BMApplet *applet,
                                 const char *summary,
                                 const char *message,
                                 const char *icon,
                                 const char *pref);

BMConnection * applet_find_active_connection_for_device (BMDevice *device,
                                                         BMApplet *applet,
                                                         BMActiveConnection **out_active);

GtkWidget * applet_new_menu_item_helper (BMConnection *connection,
                                         BMConnection *active,
                                         gboolean add_active);

GdkPixbuf * bma_icon_check_and_load (const char *name,
                                     GdkPixbuf **icon,
                                     BMApplet *applet);

#endif
