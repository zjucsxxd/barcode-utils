/*
 * indicator-barcode - user interface for barman
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * Kalle Valo <kalle.valo@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <glib-object.h>

#include "barman-manager.h"
#include "ui-proxy.h"

G_BEGIN_DECLS

#define TYPE_MANAGER manager_get_type()

#define MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_MANAGER, Manager))

#define MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_MANAGER, ManagerClass))

#define IS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_MANAGER))

#define IS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_MANAGER))

#define MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_MANAGER, ManagerClass))

typedef struct {
  GObject parent;
} Manager;

typedef struct {
  GObjectClass parent_class;
} ManagerClass;

#define ICON_CONNECTED_WIRED		"nm-device-wired"
#define ICON_CONNECTED_WIFI_0		"nm-signal-0"
#define ICON_CONNECTED_WIFI_25		"nm-signal-25"
#define ICON_CONNECTED_WIFI_50		"nm-signal-50"
#define ICON_CONNECTED_WIFI_75		"nm-signal-75"
#define ICON_CONNECTED_WIFI_100		"nm-signal-100"
#define ICON_CONNECTED_WIFI		ICON_CONNECTED_WIFI_100
#define ICON_CONNECTED_CELLULAR		"nm-device-wwan"
#define ICON_CONNECTED_BLUETOOTH	"bluetooth-active"
#define ICON_CONNECTED_DEFAULT		ICON_CONNECTED_WIRED
#define ICON_DISCONNECTED		"nm-no-connection"

#define ICON_NOTIFICATION_WIFI_FULL		"notification-network-wireless-full"
#define ICON_NOTIFICATION_WIFI_DISCONNECTED	"notification-network-wireless-disconnected"
#define ICON_NOTIFICATION_WIRED_CONNECTED	"notification-network-ethernet-connected"
#define ICON_NOTIFICATION_WIRED_DISCONNECTED	"notification-network-ethernet-disconnected"
#define ICON_NOTIFICATION_CELLULAR_CONNECTED	"notification-gsm-high"
#define ICON_NOTIFICATION_CELLULAR_DISCONNECTED	"notification-gsm-disconnected"

#define CONNECTING_ICON_STAGES 3
#define CONNECTING_ICON_STATES 11
#define MAX_ICON_NAME_LEN 30

typedef enum {
  MANAGER_STATE_UNKNOWN,
  MANAGER_STATE_OFFLINE,
  MANAGER_STATE_ONLINE,
} ManagerState;

#include "service.h"
#include "barcode-menu.h"
#include "service-manager.h"

GType manager_get_type(void);

const gchar *manager_icon_name(BarmanServiceType type, gint signal);
const gchar *manager_accessible_desc(BarmanServiceType type, gint signal);
void manager_notify(Manager *self, const gchar *summary, const gchar *body,
		    const gchar *icon);
UIProxy *manager_get_ui(Manager *self);
BarmanManager *manager_get_barman(Manager *self);
ServiceManager *manager_get_service_manager(Manager *self);
void stop_agent(Manager *self);
void manager_set_debug_level(Manager *self, gint level);
void manager_request_scan(Manager *self);
Manager *manager_new(void);

#endif

