/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Jakob Flierl <jakob.flierl@gmail.com>
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

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libdbusmenu-gtk/menu.h>
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>
#include <libindicator/indicator-image-helper.h>

#include "dbus-shared-names.h"
#include "marshal.h"
#include "service-menuitem.h"
#include "tech-menuitem.h"
#include "indicator-barcode-service-xml.h"

#define INDICATOR_DEFAULT_ICON		"nm-no-connection"

#define INDICATOR_BARCODE_TYPE            (indicator_barcode_get_type())
#define INDICATOR_BARCODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), INDICATOR_BARCODE_TYPE, IndicatorBarcode))
#define INDICATOR_BARCODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), INDICATOR_BARCODE_TYPE, IndicatorBarcodeClass))
#define IS_INDICATOR_BARCODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), INDICATOR_BARCODE_TYPE))
#define IS_INDICATOR_BARCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), INDICATOR_BARCODE_TYPE))
#define INDICATOR_BARCODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), INDICATOR_BARCODE_TYPE, IndicatorBarcodeClass))

typedef struct _IndicatorBarcode      IndicatorBarcode;
typedef struct _IndicatorBarcodeClass IndicatorBarcodeClass;

struct _IndicatorBarcodeClass
{
  IndicatorObjectClass parent_class;
};

struct _IndicatorBarcode
{
  IndicatorObject parent;

  /* FIXME: move to a private struct */
  IndicatorServiceManager *service;
  GDBusProxy *backend_proxy;
  GtkImage *indicator_image;
  const gchar *accessible_desc;
  guint watch_id;
};

/* needed to to workaround a warning in INDICATOR_SET_TYPE() */
GType get_type(void);

GType indicator_barcode_get_type(void);
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_BARCODE_TYPE)

static void indicator_barcode_dispose(GObject *object);
static void indicator_barcode_finalize(GObject *object);
G_DEFINE_TYPE(IndicatorBarcode, indicator_barcode, INDICATOR_OBJECT_TYPE);

static void set_icon(IndicatorBarcode *self, const gchar *name)
{
  if (name == NULL || strlen(name) == 0)
    return;

  g_debug("%s(%s)", __func__, name);

  indicator_image_helper_update(self->indicator_image, name);
}

static void set_accessible_desc(IndicatorBarcode *self, const gchar *desc)
{
  if (desc == NULL || strlen(desc) == 0)
    return;

  g_debug("%s(%s)", __func__, desc);

  self->accessible_desc = g_strdup(desc);

  g_signal_emit(G_OBJECT(self),
                INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE_ID,
                0,
                (IndicatorObjectEntry *)indicator_object_get_entries(INDICATOR_OBJECT(self))->data,
                TRUE);
}

static gboolean confirm_menu_open (IndicatorObject *io)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(io);

  g_return_val_if_fail(IS_INDICATOR_BARCODE (io), FALSE);
  g_return_val_if_fail(self->backend_proxy != NULL, FALSE);

  g_dbus_proxy_call(self->backend_proxy, "Scan", NULL, G_DBUS_CALL_FLAGS_NONE,
		    -1, NULL, NULL, NULL);

  return FALSE;
}

static void menu_visibility_changed (GtkWidget *widget, gpointer *user_data)
{
	static guint id = 0;

	g_return_if_fail (GTK_IS_WIDGET (widget));

	/* confirm the menu is open after 2s, or cancel the timer
	   if its closed in the meantime */
	if (gtk_widget_get_visible (widget))
		id = g_timeout_add_seconds (2, (GSourceFunc) confirm_menu_open,
									user_data);
	else if (id > 0)
		g_source_remove (id);
}

static gboolean new_service_menuitem(DbusmenuMenuitem *newitem,
				     DbusmenuMenuitem *parent,
				     DbusmenuClient   *client,
				     gpointer user_data)
{
  GtkWidget *smi;

  g_debug("%s()", __func__);

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
  /* Note: not checking parent, it's reasonable for it to be NULL */

  smi = service_menuitem_new();

  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem,
				  GTK_MENU_ITEM(smi), parent);

  service_menuitem_set_dbusmenu(SERVICE_MENUITEM(smi), newitem);

  return TRUE;
}

static gboolean new_tech_menuitem(DbusmenuMenuitem *newitem,
				  DbusmenuMenuitem *parent,
				  DbusmenuClient   *client,
				  gpointer user_data)
{
  GtkWidget *smi;

  g_debug("%s()", __func__);

  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
  /* Note: not checking parent, it's reasonable for it to be NULL */

  smi = tech_menuitem_new();

  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem,
				  GTK_MENU_ITEM(smi), parent);

  tech_menuitem_set_dbusmenu(TECH_MENUITEM(smi), newitem);

  return TRUE;
}

static GtkLabel *get_label(IndicatorObject * io)
{
  return NULL;
}

static GtkImage *get_icon(IndicatorObject * io)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(io);

  self->indicator_image = indicator_image_helper(INDICATOR_DEFAULT_ICON);
  gtk_widget_show(GTK_WIDGET(self->indicator_image));

  return self->indicator_image;
}

static const gchar *get_accessible_desc(IndicatorObject * io)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(io);

  return self->accessible_desc;
}

/* Indicator based function to get the menu for the whole
   applet.  This starts up asking for the parts of the menu
   from the various services. */
static GtkMenu *get_menu(IndicatorObject *io)
{
  DbusmenuGtkMenu *menu = dbusmenu_gtkmenu_new(INDICATOR_BARCODE_DBUS_NAME,
					       INDICATOR_BARCODE_DBUS_OBJECT);
  DbusmenuGtkClient * client = dbusmenu_gtkmenu_get_client(menu);

  g_signal_connect (GTK_MENU (menu),
		    "map", G_CALLBACK (menu_visibility_changed), io);
  g_signal_connect (GTK_MENU (menu),
		    "hide", G_CALLBACK (menu_visibility_changed), io);

  dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client),
  				   SERVICE_MENUITEM_NAME, new_service_menuitem);

  dbusmenu_client_add_type_handler(DBUSMENU_CLIENT(client),
  				   TECH_MENUITEM_NAME, new_tech_menuitem);

  g_debug("%s()", __func__);

  return GTK_MENU(menu);
}

static void icon_changed(IndicatorBarcode *self, GVariant *parameters)
{
  const gchar *name;
  GVariant *value;

  value = g_variant_get_child_value(parameters, 0);
  name = g_variant_get_string(value, NULL);

  set_icon(self, name);

  g_variant_unref(value);
}

static void accessible_desc_changed(IndicatorBarcode *self, GVariant *parameters)
{
  const gchar *desc;
  GVariant *value;

  value = g_variant_get_child_value(parameters, 0);
  desc = g_variant_get_string(value, NULL);

  set_accessible_desc(self, desc);

  g_variant_unref(value);
}

static void backend_signal_cb(GDBusProxy *proxy,
			      const gchar *sender_name,
			      const gchar *signal_name,
			      GVariant *parameters,
			      gpointer user_data)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(user_data);

  /*
   * gdbus documentation is not clear who owns the variant so take it, just
   * in case
   */
  g_variant_ref(parameters);

  if (g_strcmp0(signal_name, INDICATOR_BARCODE_SIGNAL_ICON_CHANGED) == 0) {
    icon_changed(self, parameters);
  } else if (g_strcmp0(signal_name, INDICATOR_BARCODE_SIGNAL_ACCESSIBLE_DESC_CHANGED) == 0) {
    accessible_desc_changed(self, parameters);
  }

  g_variant_unref(parameters);
}

static void get_icon_cb(GObject *object, GAsyncResult *res,
		       gpointer user_data)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(user_data);
  GVariant *result, *value;
  GError *error = NULL;
  const gchar *name;

  g_return_if_fail(self != NULL);

  result = g_dbus_proxy_call_finish(self->backend_proxy, res, &error);

  if (error != NULL) {
    g_debug("GetIcon call failed: %s", error->message);
    g_error_free(error);
    return;
  }

  if (result == NULL)
    return;

  value = g_variant_get_child_value(result, 0);
  name = g_variant_get_string(value, NULL);

  set_icon(self, name);

  g_variant_unref(value);
  g_variant_unref(result);
}

static void backend_appeared(GDBusConnection *connection, const gchar *name,
			     const gchar *name_owner, gpointer user_data)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(user_data);

  g_dbus_proxy_call(self->backend_proxy, "GetIcon", NULL,
		    G_DBUS_CALL_FLAGS_NONE, -1, NULL, get_icon_cb, self);
}

static void backend_vanished(GDBusConnection *connection, const gchar *name,
			     gpointer user_data)
{
  /* FIXME: change icon to show that we are not connected to backend */
}

static void create_backend_proxy_cb(GObject *source_object, GAsyncResult *res,
				    gpointer user_data)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(user_data);
  GDBusNodeInfo *intro;
  GDBusInterfaceInfo *info;
  GError *error = NULL;

  g_return_if_fail(self != NULL);

  self->backend_proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get backend proxy: %s", error->message);
    g_error_free(error);
    return;
  }

  if (self->backend_proxy == NULL) {
    g_warning("Failed to get backend proxy, but no errors");
    return;
  }

  g_signal_connect(self->backend_proxy, "g-signal",
		   G_CALLBACK(backend_signal_cb), self);

  intro = g_dbus_node_info_new_for_xml(indicator_barcode_service_xml,
				       NULL);
  g_return_if_fail(intro != NULL);

  info = intro->interfaces[0];
  g_dbus_proxy_set_interface_info(self->backend_proxy, info);

  self->watch_id = g_bus_watch_name(G_BUS_TYPE_SESSION, 
				    INDICATOR_BARCODE_DBUS_NAME,
				    G_BUS_NAME_WATCHER_FLAGS_NONE,
				    backend_appeared,
				    backend_vanished,
				    self,
				    NULL);
}

static void create_backend_proxy (IndicatorBarcode *self)
{
  g_return_if_fail (IS_INDICATOR_BARCODE (self));

  if (self->backend_proxy != NULL)
    return;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
			   G_DBUS_PROXY_FLAGS_NONE,
			   NULL,
			   INDICATOR_BARCODE_BACKEND_NAME,
			   INDICATOR_BARCODE_BACKEND_MANAGER_PATH,
			   INDICATOR_BARCODE_BACKEND_MANAGER_INTERFACE,
			   NULL,
			   create_backend_proxy_cb,
			   self);
}

static void indicator_connected(IndicatorBarcode *self)
{
  g_return_if_fail (IS_INDICATOR_BARCODE (self));

  create_backend_proxy(self);
}

static void connection_changed(IndicatorServiceManager *sm,
			       gboolean connected,
			       gpointer user_data)
{
  IndicatorBarcode *self = user_data;

  if (connected) {
    indicator_connected(self);
  } else {
    //TODO : will need to handle this scenario
  }

  return;
}

static void indicator_barcode_class_init(IndicatorBarcodeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  IndicatorObjectClass *io_class;

  object_class->dispose = indicator_barcode_dispose;
  object_class->finalize = indicator_barcode_finalize;

  io_class = INDICATOR_OBJECT_CLASS(klass);
  io_class->get_label = get_label;
  io_class->get_image = get_icon;
  io_class->get_menu = get_menu;
  io_class->get_accessible_desc = get_accessible_desc;
}

static void indicator_barcode_init(IndicatorBarcode *self)
{
  self->service = NULL;

  self->service = indicator_service_manager_new_version(INDICATOR_BARCODE_DBUS_NAME,
							INDICATOR_BARCODE_DBUS_VERSION);
  g_signal_connect(G_OBJECT(self->service),
		   INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE,
		   G_CALLBACK(connection_changed),
		   self);

  return;
}

static void indicator_barcode_dispose(GObject *object)
{
  IndicatorBarcode *self = INDICATOR_BARCODE(object);

  if (self->service != NULL) {
    g_object_unref(G_OBJECT(self->service));
    self->service = NULL;
  }

  G_OBJECT_CLASS(indicator_barcode_parent_class)->dispose(object);

  return;
}

static void indicator_barcode_finalize(GObject *object)
{
  G_OBJECT_CLASS(indicator_barcode_parent_class)->finalize(object);
  return;
}
