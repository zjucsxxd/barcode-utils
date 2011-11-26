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

#include "service.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <glib/gi18n.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/client.h>

#include "manager.h"
#include "barman.h"
#include "barcode-menu.h"
#include "dbus-shared-names.h"
#include "marshal.h"

#define NETWORK_AGENT LIBEXECDIR"/indicator-barcode-agent"

G_DEFINE_TYPE(Service, service, DBUSMENU_TYPE_MENUITEM)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_SERVICE, ServicePrivate))

typedef struct _ServicePrivate ServicePrivate;

struct _ServicePrivate {
  Manager *network_service;
  BarmanService *service;
  BarmanServiceState state;
};

enum {
  STATE_CHANGED,
  STRENGTH_UPDATED,
  ERROR,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

static void service_dispose(GObject *object)
{
  Service *self = SERVICE(object);
  ServicePrivate *priv = GET_PRIVATE(self);

  if (priv->service != NULL) {
    g_object_unref(priv->service);
    priv->service = NULL;
  }

  if (priv->network_service != NULL) {
    g_object_unref(priv->network_service);
    priv->network_service = NULL;
  }

  G_OBJECT_CLASS (service_parent_class)->dispose (object);
}

static void service_finalize(GObject *object)
{
  Service *self = SERVICE(object);

  g_debug("[%p] service finalize", self);

  G_OBJECT_CLASS (service_parent_class)->finalize (object);
}

static void service_class_init(ServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ServicePrivate));

  object_class->dispose = service_dispose;
  object_class->finalize = service_finalize;

  signals[STATE_CHANGED] = g_signal_new("state-changed",
					G_TYPE_FROM_CLASS(klass),
					G_SIGNAL_RUN_LAST,
					0, NULL, NULL,
					_marshal_VOID__VOID,
					G_TYPE_NONE, 0);

  signals[STRENGTH_UPDATED] = g_signal_new("strength-updated",
					G_TYPE_FROM_CLASS(klass),
					G_SIGNAL_RUN_LAST,
					0, NULL, NULL,
					_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}

static void service_init(Service *self)
{
}

static const gchar *get_disconnected_icon(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(priv != NULL, NULL);

  switch (barman_service_get_service_type(priv->service)) {
  case BARMAN_SERVICE_TYPE_WIFI:
    return ICON_NOTIFICATION_WIFI_DISCONNECTED;
  case BARMAN_SERVICE_TYPE_ETHERNET:
    return ICON_NOTIFICATION_WIRED_DISCONNECTED;
  case BARMAN_SERVICE_TYPE_CELLULAR:
    return ICON_NOTIFICATION_CELLULAR_DISCONNECTED;
  default:
    return ICON_NOTIFICATION_WIRED_DISCONNECTED;
  }
}

static void service_connect_cb(GObject *object, GAsyncResult *res,
				  gpointer user_data)
{
  Service *self = user_data;
  ServicePrivate *priv = GET_PRIVATE(self);
  const gchar *summary, *msg, *icon;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  barman_service_connect_finish(priv->service, res, &error);

  if (error == NULL)
    /* method call had no errors */
    return;

  /* FIXME: don't show errors if passphrase is requested through agent */

  g_warning("[%p] failed to connect service %s: %s",
	    self, barman_service_get_path(priv->service), error->message);

  summary = barman_service_get_name(priv->service);
  msg = _("Connection Failed");
  icon = get_disconnected_icon(self);

  manager_notify(priv->network_service, summary, msg, icon);

  g_error_free(error);
}

static void service_connect(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  g_debug("[%p] connect", self);

  barman_service_connect(priv->service, service_connect_cb, self);
}

static void service_disconnect_cb(GObject *object, GAsyncResult *res,
				  gpointer user_data)
{
  Service *self = SERVICE(user_data);
  ServicePrivate *priv = GET_PRIVATE(self);
  const gchar *summary, *msg, *icon;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  barman_service_disconnect_finish(priv->service, res, &error);

  if (error == NULL)
    /* method call had no errors */
    return;

  g_warning("[%p] failed to disconnect service %s: %s",
	    self, barman_service_get_path(priv->service), error->message);

  summary = barman_service_get_name(priv->service);
  msg = _("Disconnection Failed");
  icon = get_disconnected_icon(self);

  manager_notify(priv->network_service, summary, msg, icon);

  g_error_free(error);
}

static void service_disconnect(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->service != NULL);

  g_debug("[%p] disconnect", self);

  barman_service_disconnect(priv->service, service_disconnect_cb, self);
}

static void service_item_activated(DbusmenuMenuitem *mi, guint timestamp,
				   gpointer user_data)
{
  Service *self = user_data;
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch (barman_service_get_state(priv->service)) {
  case BARMAN_SERVICE_STATE_IDLE:
  case BARMAN_SERVICE_STATE_FAILURE:
  case BARMAN_SERVICE_STATE_DISCONNECT:
    service_connect(self);
    break;
  case BARMAN_SERVICE_STATE_ASSOCIATION:
  case BARMAN_SERVICE_STATE_CONFIGURATION:
  case BARMAN_SERVICE_STATE_READY:
  case BARMAN_SERVICE_STATE_LOGIN:
  case BARMAN_SERVICE_STATE_ONLINE:
    service_disconnect(self);
    break;
  default:
    break;
  }
}

static void service_update_label(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv);

  dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self),
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 barman_service_get_name(priv->service));
}

static void service_update_checkmark(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);
  gint state;

  g_return_if_fail(priv);

  switch (barman_service_get_state(priv->service)) {
  case BARMAN_SERVICE_STATE_READY:
  case BARMAN_SERVICE_STATE_LOGIN:
  case BARMAN_SERVICE_STATE_ONLINE:
    state = DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
    break;
  case BARMAN_SERVICE_STATE_IDLE:
  case BARMAN_SERVICE_STATE_FAILURE:
  case BARMAN_SERVICE_STATE_ASSOCIATION:
  case BARMAN_SERVICE_STATE_CONFIGURATION:
  default:
    state = DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED;
    break;
  }

  dbusmenu_menuitem_property_set_int(DBUSMENU_MENUITEM(self),
				     DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
				     state);
}

static void show_connect_error(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);
  const gchar *error_type = "general-connect-error";
  BarmanServiceSecurity security;
  BarmanServiceState state;
  BarmanServiceType type;

  g_return_if_fail(priv != NULL);

  type = barman_service_get_service_type(priv->service);
  security = barman_service_get_security(priv->service);
  state = barman_service_get_state(priv->service);

  if (type == BARMAN_SERVICE_TYPE_WIFI) {
    if (security == BARMAN_SERVICE_SECURITY_NONE) {
      error_type = "wifi-open-connect-error";
    }
    else if (security == BARMAN_SERVICE_SECURITY_WEP) {
      if (state == BARMAN_SERVICE_STATE_CONFIGURATION)
	error_type = "wifi-wep-dhcp-error";
      else
	error_type = "wifi-wep-connect-error";
    }
    else if (security == BARMAN_SERVICE_SECURITY_PSK) {
      if (state == BARMAN_SERVICE_STATE_CONFIGURATION)
	error_type = "wifi-wpa-dhcp-error";
      else
	error_type = "wifi-wpa-connect-error";
    }
  }

  /* FIXME: show error dialogs */
}

static void state_updated(BarmanService *service, GParamSpec *pspec,
			  gpointer user_data)
{
  Service *self = SERVICE(user_data);
  ServicePrivate *priv = GET_PRIVATE(self);
  BarmanServiceState new_state;
  BarmanServiceType type;
  const gchar *icon;

  g_return_if_fail(priv != NULL);

  new_state = barman_service_get_state(priv->service);

  g_debug("[%p] %s -> %s", self,
	  barman_service_state2str(priv->state),
	  barman_service_state2str(new_state));

  /* only show state transitions */
  if (priv->state == new_state)
    return;

  /* barman is buggy and in certain cases goes from online to ready */
  if (priv->state == BARMAN_SERVICE_STATE_ONLINE &&
      new_state == BARMAN_SERVICE_STATE_READY) {
    g_debug("[%p] ignoring online -> ready transition", self);
    return;
  }

  type = barman_service_get_service_type(priv->service);

  switch (new_state) {
  case BARMAN_SERVICE_STATE_READY:
    if (type == BARMAN_SERVICE_TYPE_WIFI)
      icon = ICON_NOTIFICATION_WIFI_FULL;
    else if (type == BARMAN_SERVICE_TYPE_ETHERNET)
      icon = ICON_NOTIFICATION_WIRED_CONNECTED;
    else if (type == BARMAN_SERVICE_TYPE_CELLULAR)
      icon = ICON_NOTIFICATION_CELLULAR_CONNECTED;
    else
      icon = NULL;
    manager_notify(priv->network_service,
		   barman_service_get_name(priv->service),
		   _("Connection Established"), icon);
    break;
  case BARMAN_SERVICE_STATE_IDLE:
    icon = get_disconnected_icon(self);
    manager_notify(priv->network_service,
		   barman_service_get_name(priv->service),
		   _("Disconnected"), icon);
    break;
  case BARMAN_SERVICE_STATE_FAILURE:
    icon = get_disconnected_icon(self);
    manager_notify(priv->network_service,
		   barman_service_get_name(priv->service),
		   _("Connection Failed"), icon);
    break;
  case BARMAN_SERVICE_STATE_ASSOCIATION:
  case BARMAN_SERVICE_STATE_CONFIGURATION:
  default:
    /* do nothing */
    break;
  }

  if ((priv->state == BARMAN_SERVICE_STATE_ASSOCIATION ||
       priv->state == BARMAN_SERVICE_STATE_CONFIGURATION) &&
      new_state == BARMAN_SERVICE_STATE_FAILURE) {
    show_connect_error(self);
  }

  priv->state = new_state;
  service_update_label(self);
  service_update_checkmark(self);

  g_signal_emit(self, signals[STATE_CHANGED], 0);
}

static void service_update_icon(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);
  const gchar *icon;

  g_return_if_fail(priv != NULL);

  icon = manager_icon_name(barman_service_get_service_type(priv->service),
			   barman_service_get_strength(priv->service));

  dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self),
				 DBUSMENU_MENUITEM_PROP_ICON_NAME,
				 icon);
}

static void strength_updated(BarmanService *service, GParamSpec *pspec,
			 gpointer user_data)
{
  Service *self = SERVICE(user_data);

  service_update_icon(self);

  g_signal_emit(self, signals[STRENGTH_UPDATED], 0);
}

static void setup_required(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);
  BarmanServiceType type;
  gchar *argv[3];
  gchar *cmd = BINDIR "/indicator-barcode-mobile-wizard";

  g_return_if_fail(priv);

  g_debug("[%p] setup required", self);

  type = barman_service_get_service_type(priv->service);

  /* mobile wizard is only for cellular */
  if (type != BARMAN_SERVICE_TYPE_CELLULAR)
    return;

  argv[0] = cmd;
  argv[1] = g_strdup(barman_service_get_path(priv->service));
  argv[2] = 0;

  /*
   * FIXME: for the prototype just start the script, but with the proper UI
   * we use dbus
   */
  g_debug("%s(): starting %s %s", __func__, argv[0], argv[1]);
  g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, NULL, NULL);

  g_free(argv[1]);
}

static void name_updated(BarmanService *service, GParamSpec *pspec,
			 gpointer user_data)
{
  Service *self = SERVICE(user_data);
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  g_debug("[%p] name %s", self, barman_service_get_name(priv->service));

  service_update_label(self);
}

Service *service_new(BarmanService *service, Manager *ns)
{
  ServicePrivate *priv;
  Service *self;
  gint mode;

  g_return_val_if_fail(service != NULL, NULL);
  g_return_val_if_fail(ns != NULL, NULL);

  self = g_object_new(TYPE_SERVICE, NULL);
  priv = GET_PRIVATE(self);

  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  priv->network_service = ns;
  g_object_ref(priv->network_service);

  priv->service = service;
  g_object_ref(priv->service);

  /* FIXME: follow signals from priv->service */

  g_debug("[%p] service new %s", self, barman_service_get_path(priv->service));

  dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self),
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 SERVICE_MENUITEM_NAME);
  dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self),
				 DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
				 DBUSMENU_MENUITEM_TOGGLE_CHECK);

  switch (barman_service_get_security(priv->service)) {
  case BARMAN_SERVICE_SECURITY_WEP:
  case BARMAN_SERVICE_SECURITY_PSK:
  case BARMAN_SERVICE_SECURITY_IEEE8021X:
    mode = SERVICE_MENUITEM_PROP_PROTECTED;
    break;
  case BARMAN_SERVICE_SECURITY_NONE:
  default:
    mode = SERVICE_MENUITEM_PROP_OPEN;
    break;
  }

  dbusmenu_menuitem_property_set_int(DBUSMENU_MENUITEM(self),
				     SERVICE_MENUITEM_PROP_PROTECTION_MODE,
				     mode);

  service_update_label(self);
  service_update_checkmark(self);
  service_update_icon(self);

  g_signal_connect(G_OBJECT(self),
		   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
		   G_CALLBACK(service_item_activated), self);

  g_signal_connect(priv->service, "notify::state", G_CALLBACK(state_updated),
		   self);
  g_signal_connect(priv->service, "notify::name", G_CALLBACK(name_updated),
		   self);
  g_signal_connect(priv->service, "notify::strength",
		   G_CALLBACK(strength_updated),
		   self);

  if (barman_service_get_setup_required(priv->service))
    setup_required(self);

  return self;
}

const gchar *service_get_path(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return barman_service_get_path(priv->service);
}

/* service_get_type() existed already, use "service_type" instead */
BarmanServiceType service_get_service_type(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_SERVICE(self), BARMAN_SERVICE_TYPE_ETHERNET);
  g_return_val_if_fail(priv != NULL, BARMAN_SERVICE_TYPE_ETHERNET);

  return barman_service_get_service_type(priv->service);
}

BarmanServiceState service_get_state(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_SERVICE(self), BARMAN_SERVICE_STATE_FAILURE);
  g_return_val_if_fail(priv != NULL, BARMAN_SERVICE_STATE_FAILURE);

  return barman_service_get_state(priv->service);
}

gint service_get_strength(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_SERVICE(self), 0);
  g_return_val_if_fail(priv != NULL, 0);

  return barman_service_get_strength(priv->service);
}

const gchar *service_get_name(Service *self)
{
  ServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return barman_service_get_name(priv->service);
}
