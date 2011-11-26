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

#include "barcode-menu.h"

#include <glib/gi18n.h>

#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/server.h>

#include "service.h"
#include "service-manager.h"
#include "dbus-shared-names.h"
#include "barman.h"
#include "manager.h"
#include "barman-manager.h"

G_DEFINE_TYPE (BarcodeMenu, network_menu, DBUSMENU_TYPE_MENUITEM)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_NETWORK_MENU, BarcodeMenuPrivate))

typedef struct _BarcodeMenuPrivate BarcodeMenuPrivate;

struct _BarcodeMenuPrivate {
  DbusmenuMenuitem *offline;
  DbusmenuMenuitem *offline_separator;
  DbusmenuMenuitem *wired_header;
  DbusmenuMenuitem *wired_separator;
  DbusmenuMenuitem *wireless_header;
  DbusmenuMenuitem *wireless_list_top;
  DbusmenuMenuitem *wireless_other;
  DbusmenuMenuitem *wireless_separator;
  DbusmenuMenuitem *cellular_header;
  DbusmenuMenuitem *cellular_separator;
  DbusmenuMenuitem *network_settings;

  DbusmenuMenuitem *disabled_menuitem;

  GList *wired_services;
  GList *wireless_services;
  GList *cellular_services;

  gboolean wired_enabled;
  gboolean wireless_enabled;
  gboolean cellular_enabled;
  gboolean offline_enabled;

  gboolean wired_shown;
  gboolean wireless_shown;
  gboolean cellular_shown;

  gboolean menu_enabled;

  Manager *network_service;
  BarmanManager *barman;
};

static void
network_menu_dispose (GObject *object)
{
  BarcodeMenu *self = NETWORK_MENU(object);
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);

 /* FIXME: unref all objects from BarcodeMenuPrivate */

  if (priv->barman != NULL) {
    g_object_unref(priv->barman);
    priv->barman = NULL;
  }

  G_OBJECT_CLASS (network_menu_parent_class)->dispose (object);
}

static void
network_menu_finalize (GObject *object)
{
  G_OBJECT_CLASS (network_menu_parent_class)->finalize (object);
}

static void
network_menu_class_init (BarcodeMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BarcodeMenuPrivate));

  object_class->dispose = network_menu_dispose;
  object_class->finalize = network_menu_finalize;
}

static void
network_menu_init (BarcodeMenu *self)
{
}

void network_menu_disable(BarcodeMenu *self)
{
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  Service *service;
  GList *iter;

  if (!priv->menu_enabled)
	  return;

  while ((iter = g_list_first(priv->wired_services)) != NULL) {
    service = iter->data;
    dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(self),
				   DBUSMENU_MENUITEM(service));
    priv->wired_services = g_list_delete_link(priv->wired_services,
						 iter);
  }

  while ((iter = g_list_first(priv->wireless_services)) != NULL) {
    service = iter->data;
    dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(self),
				   DBUSMENU_MENUITEM(service));
    priv->wireless_services = g_list_delete_link(priv->wireless_services,
						 iter);
  }

  while ((iter = g_list_first(priv->cellular_services)) != NULL) {
    service = iter->data;
    dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(self),
				   DBUSMENU_MENUITEM(service));
    priv->cellular_services = g_list_delete_link(priv->cellular_services,
						 iter);
  }

  /* hide all menuitems */
  dbusmenu_menuitem_property_set_bool(priv->offline,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->offline_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->wired_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_header,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_list_top,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_other,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->cellular_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->network_settings,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);

  dbusmenu_menuitem_property_set_bool(priv->disabled_menuitem,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);

  priv->menu_enabled = FALSE;
}

static void network_menu_update_wired(BarcodeMenu *self)
{
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  ServiceManager *service_manager;
  Service *service;
  GList *iter;
  guint pos;

  /* remove old services */
  while ((iter = g_list_first(priv->wired_services)) != NULL) {
    service = iter->data;
    dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(self),
				   DBUSMENU_MENUITEM(service));
    priv->wired_services = g_list_delete_link(priv->wired_services,
					      iter);
  }

  /* get new wired services */
  service_manager = manager_get_service_manager(priv->network_service);
  priv->wired_services = service_manager_get_wired(service_manager);

  if (priv->wired_services == NULL) {
    /* no wired services to show */
    priv->wired_shown = FALSE;
    return;
  }

  pos = dbusmenu_menuitem_get_position(priv->wired_header,
				       DBUSMENU_MENUITEM(self));
  pos++;

  for (iter = priv->wired_services; iter != NULL; iter = iter->next) {
    service = iter->data;
    dbusmenu_menuitem_child_add_position(DBUSMENU_MENUITEM(self),
					 DBUSMENU_MENUITEM(service), pos);
    pos++;
  }

    priv->wired_shown = TRUE;
}

static void network_menu_update_wireless(BarcodeMenu *self)
{
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  ServiceManager *service_manager;
  Service *service;
  GList *iter;
  guint pos;

  /* remove old services */
  while ((iter = g_list_first(priv->wireless_services)) != NULL) {
    service = iter->data;
    dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(self),
				   DBUSMENU_MENUITEM(service));
    priv->wireless_services = g_list_delete_link(priv->wireless_services,
						 iter);
  }

  /* get new wireless services */
  service_manager = manager_get_service_manager(priv->network_service);
  priv->wireless_services = service_manager_get_wireless(service_manager);

  /*
   * Even there are now wireless services, there are other menuitems and
   * separator is still needed.
   */
  priv->wireless_shown = TRUE;

  if (priv->wireless_services == NULL) {
    /* no wireless services to show */
    dbusmenu_menuitem_property_set_bool(priv->wireless_list_top,
					DBUSMENU_MENUITEM_PROP_VISIBLE,
					TRUE);
    return;
  }

  /* turn off the "No network detected" item */
  dbusmenu_menuitem_property_set_bool(priv->wireless_list_top,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);


  pos = dbusmenu_menuitem_get_position(priv->wireless_list_top,
					     DBUSMENU_MENUITEM(self));

  for (iter = priv->wireless_services; iter != NULL; iter = iter->next) {
    service = iter->data;
    dbusmenu_menuitem_child_add_position(DBUSMENU_MENUITEM(self),
					 DBUSMENU_MENUITEM(service), pos);
    pos++;
  }
}

static void network_menu_update_cellular(BarcodeMenu *self)
{
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  ServiceManager *service_manager;
  Service *service;
  GList *iter;
  guint pos;

  /* remove old services */
  while ((iter = g_list_first(priv->cellular_services)) != NULL) {
    service = iter->data;
    dbusmenu_menuitem_child_delete(DBUSMENU_MENUITEM(self),
				   DBUSMENU_MENUITEM(service));
    priv->cellular_services = g_list_delete_link(priv->cellular_services,
						 iter);
  }

  /* get new cellular services */
  service_manager = manager_get_service_manager(priv->network_service);
  priv->cellular_services = service_manager_get_cellular(service_manager);

  if (priv->cellular_services == NULL) {
    /* no cellular services to show */
    priv->cellular_shown = FALSE;
    return;
  }

  pos = dbusmenu_menuitem_get_position(priv->cellular_header,
				       DBUSMENU_MENUITEM(self));
  pos++;

  for (iter = priv->cellular_services; iter != NULL; iter = iter->next) {
    service = iter->data;
    dbusmenu_menuitem_child_add_position(DBUSMENU_MENUITEM(self),
					 DBUSMENU_MENUITEM(service), pos);
    pos++;
  }

  priv->cellular_shown = TRUE;
}

static void update_services(BarcodeMenu *self)
{
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  gboolean show;

  g_return_if_fail(priv != NULL);

  network_menu_update_wired(self);
  network_menu_update_wireless(self);
  network_menu_update_cellular(self);

  show = priv->wired_shown && (priv->wireless_shown || priv->cellular_shown);
  dbusmenu_menuitem_property_set_bool(priv->wired_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE,
				      show);

  show = priv->wireless_shown && priv->cellular_shown;
  dbusmenu_menuitem_property_set_bool(priv->wireless_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE,
				      show);

  /* cellular_separator is always shown */
  dbusmenu_menuitem_property_set_bool(priv->cellular_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE,
				      TRUE);
}

void network_menu_enable(BarcodeMenu *self)
{
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);

  if (priv->menu_enabled)
    return;

  /* show all menuitems which should be always visible */
  dbusmenu_menuitem_property_set_bool(priv->offline,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
  dbusmenu_menuitem_property_set_bool(priv->offline_separator,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_header,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_list_top,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
  dbusmenu_menuitem_property_set_bool(priv->wireless_other,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
  dbusmenu_menuitem_property_set_bool(priv->network_settings,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);

  dbusmenu_menuitem_property_set_bool(priv->disabled_menuitem,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);

  update_services(self);

  priv->menu_enabled = TRUE;
}

static void services_updated(ServiceManager *sm, gpointer user_data)
{
  BarcodeMenu *self = NETWORK_MENU(user_data);
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(IS_NETWORK_MENU(self));
  g_return_if_fail(priv != NULL);

  /* don't update services if menu is disabled */
  if (!priv->menu_enabled)
    return;

  update_services(self);
}

static void connect_state_changed(ServiceManager *sm, gpointer user_data)
{
  BarcodeMenu *self = NETWORK_MENU(user_data);

  g_return_if_fail(IS_NETWORK_MENU(self));
  
}

static void wifi_state_changed(BarmanManager *barman, GParamSpec *pspec,
			       gpointer user_data)
{
  BarcodeMenu *self = NETWORK_MENU(user_data);
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  BarmanTechnologyState state;

  g_return_if_fail(IS_NETWORK_MENU(self));
  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->wireless_header != NULL);

  state = barman_manager_get_wifi_state(priv->barman);

  switch (state) {
  case BARMAN_TECHNOLOGY_STATE_ENABLED:
  case BARMAN_TECHNOLOGY_STATE_CONNECTED:
    priv->wireless_enabled = TRUE;
    break;
  case BARMAN_TECHNOLOGY_STATE_UNKNOWN:
  case BARMAN_TECHNOLOGY_STATE_UNAVAILABLE:
  case BARMAN_TECHNOLOGY_STATE_AVAILABLE:
  case BARMAN_TECHNOLOGY_STATE_OFFLINE:
  default:
    priv->wireless_enabled = FALSE;
    break;
  }

  dbusmenu_menuitem_property_set_int(priv->wireless_header,
				     DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
				     priv->wireless_enabled);
}

static void toggle_wireless_cb(DbusmenuMenuitem *mi, guint timestamp,
			       gpointer user_data)
{
  BarcodeMenu *self = NETWORK_MENU(user_data);
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->barman != NULL);

  if (priv->wireless_enabled)
    barman_manager_disable_technology(priv->barman,
				       BARMAN_TECHNOLOGY_TYPE_WIFI,
				       NULL,
				       NULL,
				       NULL);
  else
    barman_manager_enable_technology(priv->barman,
				      BARMAN_TECHNOLOGY_TYPE_WIFI,
				      NULL,
				      NULL,
				      NULL);
}

static void offline_mode_changed(BarmanManager *barman, GParamSpec *pspec,
				 gpointer user_data)
{
  BarcodeMenu *self = NETWORK_MENU(user_data);
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(IS_NETWORK_MENU(self));
  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->barman != NULL);
  g_return_if_fail(priv->offline != NULL);

  priv->offline_enabled = barman_manager_get_offline_mode(priv->barman);

  dbusmenu_menuitem_property_set_int(priv->offline,
				     DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
				     priv->offline_enabled);
}

static void toggle_offline_cb(DbusmenuMenuitem *mi, guint timestamp,
			       gpointer user_data)
{
  BarcodeMenu *self = NETWORK_MENU(user_data);
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->barman != NULL);

  barman_manager_set_offline_mode(priv->barman, !priv->offline_enabled);
}

static void network_settings_activated(DbusmenuMenuitem *mi, guint timestamp,
				       gpointer user_data)
{
  gchar *argv[2];

  argv[0] = BINDIR "/indicator-barcode-settings";
  argv[1] = NULL;

  g_debug("%s(): starting %s", __func__, argv[0]);
  g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, NULL, NULL);
}

static void wireless_other_activated(DbusmenuMenuitem *mi, guint timestamp,
				       gpointer user_data)
{
  BarcodeMenu *self = user_data;
  BarcodeMenuPrivate *priv = GET_PRIVATE(self);
  Manager *ns = priv->network_service;
  UIProxy *ui;

  ui = manager_get_ui(ns);

  if (ui == NULL)
    return;

  ui_proxy_show_wireless_connect(ui);
}

BarcodeMenu *network_menu_new(Manager *ns)
{
  BarcodeMenu *self;
  BarcodeMenuPrivate *priv;
  DbusmenuServer *server;
  ServiceManager *sm;

  self = g_object_new(TYPE_NETWORK_MENU, NULL);
  priv = GET_PRIVATE(self);

  priv->network_service = ns;
  g_return_val_if_fail(manager_get_barman(ns) != NULL, NULL);
  priv->barman = g_object_ref(manager_get_barman(ns));

  sm = manager_get_service_manager(ns);
  g_signal_connect(G_OBJECT(sm), "services-updated",
		   G_CALLBACK(services_updated), self);
  g_signal_connect(G_OBJECT(sm), "state-changed",
		   G_CALLBACK(connect_state_changed), self);

  g_signal_connect(G_OBJECT(priv->barman), "notify::wifi-state",
		   G_CALLBACK(wifi_state_changed), self);
  g_signal_connect(G_OBJECT(priv->barman), "notify::offline-mode",
		   G_CALLBACK(offline_mode_changed), self);

  server = dbusmenu_server_new(INDICATOR_BARCODE_DBUS_OBJECT);
  dbusmenu_server_set_root(server, DBUSMENU_MENUITEM(self));
  dbusmenu_menuitem_property_set_bool(DBUSMENU_MENUITEM(self),
				      DBUSMENU_MENUITEM_PROP_VISIBLE,
				      TRUE);

  /* offline mode */
  priv->offline = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set(priv->offline,
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 TECH_MENUITEM_NAME);
  dbusmenu_menuitem_property_set(priv->offline,
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 _("Flight Mode"));
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self), priv->offline);
  g_signal_connect(G_OBJECT(priv->offline),
		   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
		   G_CALLBACK(toggle_offline_cb), self);

  priv->offline_separator = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(priv->offline_separator,
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->offline_separator);

  /* Wired section of the menu */
  priv->wired_header = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set_bool(priv->wired_header,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self), priv->wired_header);

  priv->wired_separator = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(priv->wired_separator,
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self), priv->wired_separator);

  /* Wireless section of the menu */
  priv->wireless_header = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set(priv->wireless_header,
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 TECH_MENUITEM_NAME);
  dbusmenu_menuitem_property_set(priv->wireless_header,
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 _("Wi-Fi"));
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self), priv->wireless_header);
  g_signal_connect(G_OBJECT(priv->wireless_header),
		   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
		   G_CALLBACK(toggle_wireless_cb), self);

  priv->wireless_list_top = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set(priv->wireless_list_top,
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 _("No network detected"));
  dbusmenu_menuitem_property_set_bool(priv->wireless_list_top,
				      DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->wireless_list_top);

  priv->wireless_other = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set(priv->wireless_other,
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 _("Other Network..."));
  g_signal_connect(G_OBJECT(priv->wireless_other),
		   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
		   G_CALLBACK(wireless_other_activated), self);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->wireless_other);

  /* FIXME: what is this? */
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->wireless_separator);

  priv->wireless_separator = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set(priv->wireless_separator,
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->wireless_separator);

  /* Mobile section */
  priv->cellular_header = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set_bool(priv->cellular_header,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->cellular_header);

  priv->cellular_separator = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(priv->cellular_separator,
				 DBUSMENU_MENUITEM_PROP_TYPE,
				 DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->cellular_separator);

  /* Network settings */
  priv->network_settings = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set(priv->network_settings,
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 _("Network Settings..."));
  g_signal_connect(G_OBJECT(priv->network_settings),
		   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
		   G_CALLBACK(network_settings_activated), self);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->network_settings);

  /* "barmand not available" message */
  priv->disabled_menuitem = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(priv->disabled_menuitem,
				 DBUSMENU_MENUITEM_PROP_LABEL,
				 _("barmand not available"));
  dbusmenu_menuitem_property_set_bool(priv->disabled_menuitem,
				      DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);
  dbusmenu_menuitem_property_set_bool(priv->disabled_menuitem,
				      DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  dbusmenu_menuitem_child_append(DBUSMENU_MENUITEM(self),
				 priv->disabled_menuitem);

  /*
   * first start menu disabled, Manager will enable it when everything
   * is ready
   */
  priv->menu_enabled = TRUE;
  network_menu_disable(self);

  return self;
}
