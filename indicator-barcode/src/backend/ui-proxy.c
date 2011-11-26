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

#include "ui-proxy.h"

#include "indicator-barcode-agent-xml.h"
#include "dbus-shared-names.h"

G_DEFINE_TYPE(UIProxy, ui_proxy, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE((o), TYPE_UI_PROXY, \
			       UIProxyPrivate))

typedef struct _UIProxyPrivate UIProxyPrivate;

struct _UIProxyPrivate {
  GDBusProxy *proxy;
  guint watch_id;

  gboolean connected;
  gboolean start_requested;
};

enum
{
  /* reserved */
  PROP_0,

  PROP_CONNECTED,
};

static void call_cb(GObject *object, GAsyncResult *res, gpointer user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  UIProxy *self = UI_PROXY(user_data);
  GError *error = NULL;
  GVariant *result;

  result = g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_warning("ui dbus call failed: %s", error->message);
    g_error_free(error);
    return;
  }

  if (result == NULL)
    return;

  g_variant_unref(result);
  g_object_unref(self);
}

void ui_proxy_start(UIProxy *self)
{
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(IS_UI_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (priv->proxy == NULL) {
    /*
     * The current way to create UIProxy object is racy because we don't
     * signal when dbus is ready for use, so it can happen that gdbus proxy
     * is not created when start is called. Postpone start to happen after
     * the proxy is created.
     */
    priv->start_requested = TRUE;
    return;
  }

  g_dbus_proxy_call(priv->proxy, "Start", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    call_cb, g_object_ref(self));
}

void ui_proxy_stop(UIProxy *self)
{
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(IS_UI_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (!priv->connected) {
    g_warning("Not connected to UI, cancelling Stop request");
    return;
  }

  g_dbus_proxy_call(priv->proxy, "Stop", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    call_cb, g_object_ref(self));
}

void ui_proxy_show_connect_error(UIProxy *self, const gchar *error_id)
{
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(IS_UI_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (!priv->connected) {
    g_warning("Not connected to UI, cancelling ShowConnectError request");
    return;
  }

  g_dbus_proxy_call(priv->proxy, "ShowConnectError", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    call_cb, g_object_ref(self));
}

void ui_proxy_show_wireless_connect(UIProxy *self)
{
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(IS_UI_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (!priv->connected) {
    g_warning("Not connected to UI, cancelling ShowWirelessConnect request");
    return;
  }

  g_dbus_proxy_call(priv->proxy, "ShowWirelessConnect", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    call_cb, g_object_ref(self));
}

void ui_proxy_set_debug(UIProxy *self, guint level)
{
  UIProxyPrivate *priv = GET_PRIVATE(self);
  GVariant *parameters;

  g_return_if_fail(IS_UI_PROXY(self));
  g_return_if_fail(priv != NULL);

  if (!priv->connected) {
    g_warning("Not connected to UI, cancelling SetDebug request");
    return;
  }

  parameters = g_variant_new("(i)", level);

  g_dbus_proxy_call(priv->proxy, "SetDebug", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    call_cb, g_object_ref(self));
}

static void ask_pin_cb(GObject *object, GAsyncResult *res, gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(user_data);
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  GError *error = NULL;
  GVariant *result;

  result = g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_simple_async_result_set_from_error(simple, error);
    g_error_free(error);
    goto out;
  }

  g_simple_async_result_set_op_res_gpointer(simple, result,
					    (GDestroyNotify) g_variant_unref);

 out:
  g_simple_async_result_complete(simple);
  g_object_unref(simple);
}

void ui_proxy_ask_pin(UIProxy *self, const gchar *type,
		      GCancellable *cancellable, GAsyncReadyCallback callback,
		      gpointer user_data)
{
  GSimpleAsyncResult *simple;
  UIProxyPrivate *priv = GET_PRIVATE(self);
  GVariant *parameters;

  g_return_if_fail(IS_UI_PROXY(self));
  g_return_if_fail(priv != NULL);

  simple = g_simple_async_result_new(G_OBJECT(self), callback,
                                      user_data, ui_proxy_ask_pin);

  if (!priv->connected) {
    g_simple_async_result_set_error(simple, 0, 0,
				    "Not connected to UI, cancelling AskPin request");
    return;
  }

  parameters = g_variant_new("(s)", type);

  g_dbus_proxy_call(priv->proxy, "AskPin", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, -1, cancellable,
  		    ask_pin_cb, simple);
}

gchar *ui_proxy_ask_pin_finish(UIProxy *self, GAsyncResult *res, GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(res);
  gchar *pin = NULL;
  GVariantIter iter;
  GVariant *result, *value;

  g_return_val_if_fail(IS_UI_PROXY(self), NULL);
  g_return_val_if_fail(g_simple_async_result_get_source_tag(simple) ==
		       ui_proxy_ask_pin, NULL);

  if (g_simple_async_result_propagate_error(simple, error))
    return NULL;

  result = g_simple_async_result_get_op_res_gpointer(simple);

  g_variant_iter_init(&iter, result);
  value = g_variant_iter_next_value(&iter);

  g_variant_get(value, "s", &pin);

  /* simple still owns variant and unrefs it automatically */

  /* caller now owns pin */
  return pin;
}

gboolean ui_proxy_is_connected(UIProxy *self)
{
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(IS_UI_PROXY(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->connected;
}

static void service_appeared(GDBusConnection *connection, const gchar *name,
			     const gchar *name_owner, gpointer user_data)
{
  UIProxy *self = UI_PROXY(user_data);
  UIProxyPrivate *priv = GET_PRIVATE(self);

  if (priv->connected)
    return;

  priv->connected = TRUE;
  g_object_notify(G_OBJECT(self), "connected");
}


static void service_vanished(GDBusConnection *connection, const gchar *name,
			     gpointer user_data)
{
  UIProxy *self = UI_PROXY(user_data);
  UIProxyPrivate *priv = GET_PRIVATE(self);

  if (!priv->connected)
    return;

  priv->connected = FALSE;
  g_object_notify(G_OBJECT(self), "connected");
}

static void create_proxy_cb(GObject *source_object, GAsyncResult *res,
			    gpointer user_data)
{
  UIProxy *self = UI_PROXY(user_data);
  UIProxyPrivate *priv = GET_PRIVATE(self);
  GDBusNodeInfo *node_info;
  GDBusInterfaceInfo *info;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  priv->proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get proxy: %s", error->message);
    g_error_free(error);
    return;
  }

  if (priv->proxy == NULL) {
    g_warning("Failed to get proxy, but no errors");
    return;
  }

  node_info = g_dbus_node_info_new_for_xml(indicator_barcode_agent_xml,
					   NULL);
  g_return_if_fail(node_info != NULL);

  info = node_info->interfaces[0];
  g_dbus_proxy_set_interface_info(priv->proxy, info);

  priv->watch_id = g_bus_watch_name(G_BUS_TYPE_SESSION,
				    INDICATOR_BARCODE_AGENT_NAME,
				    G_BUS_NAME_WATCHER_FLAGS_NONE,
				    service_appeared,
				    service_vanished,
				    self,
				    NULL);


  if (priv->start_requested) {
    ui_proxy_start(self);
    priv->start_requested = FALSE;
  }
}

static void ui_proxy_set_property(GObject *object,
					 guint property_id,
					 const GValue *value,
					 GParamSpec *pspec)
{
  UIProxy *self = UI_PROXY(object);
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_CONNECTED:
    priv->connected = g_value_get_boolean(value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void ui_proxy_get_property(GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec)
{
  UIProxy *self = UI_PROXY(object);
  UIProxyPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_CONNECTED:
    g_value_set_boolean(value, priv->connected);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void ui_proxy_dispose(GObject *object)
{
  UIProxy *self = UI_PROXY(object);
  UIProxyPrivate *priv = GET_PRIVATE(self);

  if (priv->proxy != NULL) {
    g_object_unref(priv->proxy);
    priv->proxy = NULL;
  }

  if (priv->watch_id != 0) {
    g_bus_unwatch_name(priv->watch_id);
    priv->watch_id = 0;
  }

  G_OBJECT_CLASS(ui_proxy_parent_class)->dispose(object);
}

static void ui_proxy_finalize(GObject *object)
{
  G_OBJECT_CLASS(ui_proxy_parent_class)->finalize(object);
}

static void ui_proxy_class_init(UIProxyClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec;

  g_type_class_add_private(klass, sizeof(UIProxyPrivate));

  gobject_class->dispose = ui_proxy_dispose;
  gobject_class->finalize = ui_proxy_finalize;
  gobject_class->set_property = ui_proxy_set_property;
  gobject_class->get_property = ui_proxy_get_property;

  pspec = g_param_spec_boolean("connected",
			       "UiProxy's connected property",
			       "Informs when object is connected for use",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_CONNECTED, pspec);
}

static void ui_proxy_init(UIProxy *self)
{
}

UIProxy *ui_proxy_new(void)
{
  UIProxy *self;

  self = g_object_new(TYPE_UI_PROXY, NULL);

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
			   INDICATOR_BARCODE_AGENT_NAME,
			   INDICATOR_BARCODE_AGENT_OBJECT,
  			   INDICATOR_BARCODE_AGENT_INTERFACE,
			   NULL, create_proxy_cb, self);

  return self;
}
