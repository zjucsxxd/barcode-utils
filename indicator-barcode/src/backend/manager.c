/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Jakob Flierl
 *
 * Authors:
 * Jakob Flierl <jakob.flierl@gmail.com>
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

/*
 * FIXME: libbarman todo list:
 *
 * o technology handling:
 * -  g_signal_emit(self, signals[TECHNOLOGY_STATE_CHANGED], 0,
 * -               BARMAN_TECHNOLOGY_WIFI, wifi);
 * -  g_signal_emit(self, signals[TECHNOLOGY_STATE_CHANGED], 0,
 * -               BARMAN_TECHNOLOGY_ETHERNET, ethernet);
 * -  g_signal_emit(self, signals[TECHNOLOGY_STATE_CHANGED], 0,
 * -               BARMAN_TECHNOLOGY_CELLULAR, cellular);
 *
 *
 * o implement_request_scan()
 *
 * o handle wifi enable/disable errors:
 *
 * -  g_warning("Failed to enable wireless: %s", error->message);
 * -
 * -  summary = _("Wireless");
 * -  msg = _("Failed to enable wireless");
 * -  icon = ICON_NOTIFICATION_WIFI_FULL;
 *
 * -  g_warning("Failed to disable wireless: %s", error->message);
 * -
 * -  summary = _("Wireless");
 * -  msg = _("Failed to disable wireless");
 * -  icon = ICON_NOTIFICATION_WIFI_FULL;
 */

#include "manager.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include "marshal.h"
#include "barman-manager.h"
#include "barman-service.h"
#include "service.h"
#include "barman.h"
#include "dbus-shared-names.h"
#include "service-manager.h"
#include "barcode-menu.h"
#include "log.h"
#include "manager-proxy.h"
#include "ui-proxy.h"

G_DEFINE_TYPE(Manager, manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE((o), TYPE_MANAGER, ManagerPrivate))

typedef struct _ManagerPrivate ManagerPrivate;

struct _ManagerPrivate {
  ManagerProxy *ns_dbus;
  ServiceManager *service_manager;

  BarmanManager *barman;
  BarmanService *default_service;

  UIProxy *ui;

  NotifyNotification *notification;
  guint animation_timer;
  gint animation_counter;
  gint connecting_stage;
  gint connecting_state;
  gchar *connecting_icons[CONNECTING_ICON_STAGES][CONNECTING_ICON_STATES];
  gint signal_strength;
  ManagerState manager_state;
  BarcodeMenu *network_menu;
};

enum {
  TECHNOLOGY_STATE_CHANGED,
  TECHNOLOGY_AVAILABILITY_CHANGED,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

#define CONNECTING_ANIMATION_TIMEOUT 100

static void stop_connecting_animation(Manager *self);
static void start_agent(Manager *self);

static void manager_dispose(GObject *object)
{
  Manager *self = MANAGER(object);
  ManagerPrivate *priv = GET_PRIVATE(self);

  if (priv->network_menu != NULL) {
    g_object_unref(priv->network_menu);
    priv->network_menu = NULL;
  }

  stop_agent(self);

  G_OBJECT_CLASS(manager_parent_class)->dispose(object);
}

static void manager_finalize(GObject *object)
{
  Manager *manager = MANAGER(object);
  ManagerPrivate *priv = GET_PRIVATE(manager);
  int i, j;

  for (i = 0; i < CONNECTING_ICON_STAGES; i++) {
    for (j = 0; j < CONNECTING_ICON_STATES; j++) {
      g_free(priv->connecting_icons[i][j]);
      priv->connecting_icons[i][j] = NULL;
    }
  }

  G_OBJECT_CLASS(manager_parent_class)->finalize(object);
}

static void manager_class_init(ManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  guint signal;

  g_type_class_add_private(klass, sizeof(ManagerPrivate));

  object_class->dispose = manager_dispose;
  object_class->finalize = manager_finalize;

  signal = g_signal_new("technology-state-changed", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST, 0, NULL, NULL,
			_marshal_VOID__STRING_BOOLEAN,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  signals[TECHNOLOGY_STATE_CHANGED] = signal;

  signal = g_signal_new("technology-availability-updated",
			G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
			0, NULL, NULL, _marshal_VOID__STRING_BOOLEAN,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  signals[TECHNOLOGY_AVAILABILITY_CHANGED] = signal;

}

static void manager_init(Manager *self)
{
}

const gchar *manager_icon_name(BarmanServiceType type, gint signal)
{
  switch (type) {
  case BARMAN_SERVICE_TYPE_ETHERNET:
    return ICON_CONNECTED_WIRED;
  case BARMAN_SERVICE_TYPE_WIFI:
    if (signal > 75)
       return ICON_CONNECTED_WIFI_100;
    else if (signal > 50)
      return ICON_CONNECTED_WIFI_75;
    else if (signal > 25)
      return ICON_CONNECTED_WIFI_50;
    else if (signal > 0)
      return ICON_CONNECTED_WIFI_25;
    else
      return ICON_CONNECTED_WIFI_0;
  case BARMAN_SERVICE_TYPE_CELLULAR:
    return ICON_CONNECTED_CELLULAR;
  case BARMAN_SERVICE_TYPE_BLUETOOTH:
    return ICON_CONNECTED_BLUETOOTH;
  default:
    return ICON_CONNECTED_DEFAULT;
  }
}

const gchar *manager_accessible_desc(BarmanServiceType type, gint signal)
{
  switch (type) {
  case BARMAN_SERVICE_TYPE_ETHERNET:
    return _("Network (Wired)");
  case BARMAN_SERVICE_TYPE_WIFI:
    return g_strdup_printf(_("Network (Wireless, %d%%)"), signal);
  case BARMAN_SERVICE_TYPE_CELLULAR:
    return _("Network (Mobile)");
  case BARMAN_SERVICE_TYPE_BLUETOOTH:
    return _("Network (Bluetooth)");
  default:
    return _("Network (Connected)");
  }
}

static void set_animation_icon(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);
  gchar *icon;

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv->ns_dbus != NULL);
  g_return_if_fail(priv->connecting_stage < CONNECTING_ICON_STAGES);
  g_return_if_fail(priv->connecting_state < CONNECTING_ICON_STATES);

  icon = priv->connecting_icons[priv->connecting_stage][priv->connecting_state];
  manager_proxy_set_icon(priv->ns_dbus, icon);
}

static gboolean connecting_animation_timeout(gpointer user_data)
{
  Manager *self = MANAGER(user_data);
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(self != NULL, FALSE);

  priv->connecting_state++;
  if (priv->connecting_state >= CONNECTING_ICON_STATES)
    priv->connecting_state = 0;

  set_animation_icon(self);

  priv->animation_counter++;

  /* fail safe in case animation is not stopped properly */
  if (priv->animation_counter > 3000) {
    g_warning("connecting animation running for too long!");
    stop_connecting_animation(self);
    return FALSE;
  }

  return TRUE;
}

static void start_connecting_animation(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  if (priv->animation_timer != 0)
    return;

  priv->connecting_stage = 0;
  priv->connecting_state = 0;
  priv->animation_counter = 0;

  set_animation_icon(self);

  priv->animation_timer = g_timeout_add(CONNECTING_ANIMATION_TIMEOUT,
					connecting_animation_timeout,
					self);

  if (priv->animation_timer == 0)
    g_warning("failed to add timeout for icon animation");
}

static void stop_connecting_animation(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  if (priv->animation_timer == 0)
	  return;

  g_source_remove(priv->animation_timer);
  priv->animation_timer = 0;
}

void manager_notify(Manager *self, const gchar *summary, const gchar *body,
		    const gchar *icon)
{
  ManagerPrivate *priv = GET_PRIVATE(self);
  GError *error = NULL;
  gboolean result;

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv != NULL);

  if (priv->notification == NULL) {
    priv->notification = notify_notification_new(summary, body, icon);
  } else {
    notify_notification_update(priv->notification, summary, body, icon);
  }

  result = notify_notification_show(priv->notification, &error);
  if (!result) {
    g_warning("Failed to show notification '%s/%s': %s", summary, body,
	      error->message);
    g_error_free(error);
  }
}

static void update_icon(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);
  BarmanServiceType type;
  const gchar *icon_name;
  const gchar *accessible_desc;

  g_return_if_fail(self);

  if (service_manager_is_connecting(priv->service_manager)) {
    start_connecting_animation(self);
    manager_proxy_set_accessible_desc(priv->ns_dbus, _("Network (Connecting)"));
    return;
  }

  stop_connecting_animation(self);

  if (!service_manager_get_connected(priv->service_manager)) {
    icon_name = ICON_DISCONNECTED;
    accessible_desc = _("Network (Disconnected)");
    goto done;
  }

  if (priv->default_service == NULL) {
    icon_name = ICON_DISCONNECTED;
    accessible_desc = _("Network (Disconnected)");
    goto done;
  }

  type = barman_service_get_service_type(priv->default_service);

  icon_name = manager_icon_name(type, priv->signal_strength);
  accessible_desc = manager_accessible_desc(type, priv->signal_strength);

 done:
  manager_proxy_set_icon(priv->ns_dbus, icon_name);
  manager_proxy_set_accessible_desc(priv->ns_dbus, accessible_desc);
}

static void state_changed(ServiceManager *sm, gpointer user_data)
{
  Manager *self = MANAGER(user_data);
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv != NULL);

  update_icon(self);
}

static void update_default_service(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);
  const gchar *path;

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv != NULL);

  if (priv->default_service != NULL)
    g_object_unref(priv->default_service);

  priv->default_service = barman_manager_get_default_service(priv->barman);

  if (priv->default_service == NULL)
    goto done;

  g_object_ref(priv->default_service);

  priv->signal_strength = barman_service_get_strength(priv->default_service);

 done:
  update_icon(self);

  if (priv->default_service != NULL)
    path = barman_service_get_path(priv->default_service);
  else
    path = "<none>";

  g_debug("default service %s",  path);
}

/* FIXME: should follow priv->default_service strength */
static void strength_updated(ServiceManager *sm, guint strength,
			     gpointer user_data)
{
  Manager *self = MANAGER(user_data);
  ManagerPrivate *priv = GET_PRIVATE(self);

  priv->signal_strength = strength;

  update_icon(self);
}

static void default_service_notify(BarmanManager *barman, GParamSpec *pspec,
				   gpointer user_data)
{
  Manager *self = MANAGER(user_data);

  update_default_service(self);
}

static void update_services(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);
  BarmanService **services;

  g_return_if_fail(priv != NULL);

  services = barman_manager_get_services(priv->barman);

  service_manager_update_services(priv->service_manager, services);

  update_icon(self);
}

static void barman_service_added(BarmanManager *proxy,
				  BarmanService *service,
				  gpointer user_data)
{
  Manager *self = MANAGER(user_data);

  update_services(self);
}

static void barman_service_removed(BarmanManager *proxy, const gchar *path,
				    gpointer user_data)
{
  Manager *self = MANAGER(user_data);

  update_services(self);
}

static void barman_connected(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  g_debug("connected to barman");

  start_agent(self);

  update_services(self);

  update_default_service(self);

  /* FIXME: create update_technologies() */

  network_menu_enable(priv->network_menu);
}

static void barman_disconnected(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_debug("disconnected from barman");

  g_return_if_fail(priv != NULL);

  network_menu_disable(priv->network_menu);
  service_manager_remove_all(priv->service_manager);
  stop_connecting_animation(self);
  manager_proxy_set_icon(priv->ns_dbus, ICON_DISCONNECTED);
  manager_proxy_set_accessible_desc(priv->ns_dbus, _("Network (Disconnected)"));
  stop_agent(self);
}

static void barman_connected_notify(BarmanManager *barman, GParamSpec *pspec,
				     gpointer user_data)
{
  Manager *self = MANAGER(user_data);

  if (barman_manager_get_connected(barman))
    barman_connected(self);
  else
    barman_disconnected(self);
}

static void create_barman(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  priv->barman = barman_manager_new();

  g_signal_connect(priv->barman, "notify::connected",
		   G_CALLBACK(barman_connected_notify),
		   self);
  g_signal_connect(priv->barman, "service-added",
		   G_CALLBACK(barman_service_added),
		   self);
  g_signal_connect(priv->barman, "service-removed",
		   G_CALLBACK(barman_service_removed),
		   self);

  g_signal_connect(priv->barman, "notify::default-service",
		   G_CALLBACK(default_service_notify),
		   self);
}

static void ui_connected_notify(UIProxy *ui, GParamSpec *pspec,
				gpointer user_data)
{
  if (ui_proxy_is_connected(ui))
    g_debug("ui connected");
  else
    g_debug("ui disconnected");

  /* FIXME: start ui again if we didn't stop it */
}

static void create_ui_proxy(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv != NULL);

  /* this should not be called agent_proxy is already created */
  g_return_if_fail(priv->ui == NULL);

  priv->ui = ui_proxy_new();

  if (priv->ui == NULL) {
    g_warning("failed to create ui proxy");
    return;
  }

  g_signal_connect(priv->ui, "notify::connected",
		   G_CALLBACK(ui_connected_notify),
		   self);

  g_debug("ui proxy created");
}

void start_agent(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->ui != NULL);

  ui_proxy_start(priv->ui);
}

void stop_agent(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->ui != NULL);

  ui_proxy_stop(priv->ui);
}

void manager_request_scan(Manager *self)
{
  /* FIXME: implement barman_manager_request_scan() */
}

void manager_set_debug_level(Manager *self, gint level)
{
  g_return_if_fail(IS_MANAGER(self));

  log_set_debug(level > 0);
}

UIProxy *manager_get_ui(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_MANAGER(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->ui;
}

BarmanManager *manager_get_barman(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->barman;
}

ServiceManager *manager_get_service_manager(Manager *self)
{
  ManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->service_manager;
}


Manager *manager_new(void)
{
  Manager *self;
  ManagerPrivate *priv;
  gchar *icon;
  int i, j;

  self = g_object_new(TYPE_MANAGER, NULL);
  priv = GET_PRIVATE(self);

  create_barman(self);

  priv->ns_dbus = manager_proxy_new(self);
  priv->service_manager = service_manager_new(self);
  priv->network_menu = network_menu_new(self);

  g_signal_connect(G_OBJECT(priv->service_manager), "state-changed",
		   G_CALLBACK(state_changed), self);
  g_signal_connect(G_OBJECT(priv->service_manager), "strength-updated",
		   G_CALLBACK(strength_updated), self);

  create_ui_proxy(self);

  /* create icon names */
  for (i = 0; i < CONNECTING_ICON_STAGES; i++) {
    for (j = 0; j < CONNECTING_ICON_STATES; j++) {
      icon = g_malloc(MAX_ICON_NAME_LEN);
      snprintf(icon, MAX_ICON_NAME_LEN,
               "nm-stage%02d-connecting%02d", i + 1, j + 1);
      priv->connecting_icons[i][j] = icon;
    }
  }

  if (barman_manager_get_connected(priv->barman))
    barman_connected(self);

  /* otherwise wait for the connected signal */

  return self;
}
