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

#include "barman-manager.h"

#include <string.h>

#include "barman-manager-xml.h"
#include "barman-technology-xml.h"
#include "barman.h"
#include "marshal.h"
#include "barman-service.h"

G_DEFINE_TYPE(BarmanManager, barman_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE((o), BARMAN_TYPE_MANAGER, \
			       BarmanManagerPrivate))

typedef struct _BarmanManagerPrivate BarmanManagerPrivate;

struct _BarmanManagerPrivate {
  /*
   * I'm not sure if a hashtable is the best choise, especially because we
   * need traverse it quite often. But currently the number of services is
   * so low that it doesn't really matter.
   */
  GHashTable *services;
  GHashTable *technologies;

  GDBusProxy *proxy;
  gboolean connected;
  gboolean initialised;

  guint watch_id;

  /* used only by get_services() */
  BarmanService **array;

  /* current default service, visible to clients */
  BarmanService *default_service;

  /*
   * the new default service but waiting for it to be ready, not visible to
   * clients
   */
  BarmanService *pending_default_service;

  BarmanTechnologyState wifi_state;
  BarmanTechnologyState ethernet_state;
  BarmanTechnologyState cellular_state;
  BarmanTechnologyState bluetooth_state;

  gboolean offline_mode;
};

struct technology_type_string
{
  const gchar *str;
  BarmanTechnologyType type;
};

static const struct technology_type_string technology_type_map[] = {
  { "ethernet",		BARMAN_TECHNOLOGY_TYPE_ETHERNET },
  { "wifi",		BARMAN_TECHNOLOGY_TYPE_WIFI },
  { "bluetooth",	BARMAN_TECHNOLOGY_TYPE_BLUETOOTH },
  { "cellular",		BARMAN_TECHNOLOGY_TYPE_CELLULAR },
};

struct technology_state_string
{
  const gchar *str;
  BarmanTechnologyState state;
};

static const struct technology_state_string technology_state_map[] = {
  { "offline",		BARMAN_TECHNOLOGY_STATE_OFFLINE },
  { "available",	BARMAN_TECHNOLOGY_STATE_AVAILABLE },
  { "enabled",		BARMAN_TECHNOLOGY_STATE_ENABLED },
  { "connected",	BARMAN_TECHNOLOGY_STATE_CONNECTED },
};

struct technology  {
  GDBusProxy *proxy;
  BarmanTechnologyType type;
  BarmanTechnologyState state;
};

enum {
  SERVICE_ADDED_SIGNAL,
  SERVICE_REMOVED_SIGNAL,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

enum
{
  /* reserved */
  PROP_0,

  PROP_CONNECTED,
  PROP_DEFAULT_SERVICE,
  PROP_WIFI_STATE,
  PROP_ETHERNET_STATE,
  PROP_CELLULAR_STATE,
  PROP_BLUETOOTH_STATE,
  PROP_OFFLINE_MODE,
};

static const gchar *technology_type2str(BarmanTechnologyType type)
{
  const struct technology_type_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(technology_type_map); i++) {
    s = &technology_type_map[i];
    if (s->type == type)
      return s->str;
  }

  g_warning("%s(): unknown technology type %d", __func__, type);

  return "unknown";
}

static void enable_technology_cb(GObject *object, GAsyncResult *res,
				 gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(user_data);
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  GError *error = NULL;

  g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_simple_async_result_set_from_error(simple, error);
    g_error_free(error);
    goto out;
  }

 out:
  g_simple_async_result_complete(simple);
  g_object_unref(simple);
}

void barman_manager_enable_technology(BarmanManager *self,
				       BarmanTechnologyType type,
				       GCancellable *cancellable,
				       GAsyncReadyCallback callback,
				       gpointer user_data)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GSimpleAsyncResult *simple;
  GVariant *parameters;
  const gchar *s;

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(priv != NULL);

  simple = g_simple_async_result_new(G_OBJECT(self), callback,
				     user_data,
				     barman_manager_enable_technology);

  s = technology_type2str(type);
  parameters = g_variant_new("(s)", s);

  /* FIXME: cancel the call on dispose */
  g_dbus_proxy_call(priv->proxy, "EnableTechnology", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, CONNECT_TIMEOUT, cancellable,
  		    enable_technology_cb, simple);
}

void barman_manager_enable_technology_finish(BarmanManager *self,
					      GAsyncResult *res,
					      GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(res);

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(g_simple_async_result_get_source_tag(simple) ==
		   barman_manager_enable_technology);

  g_simple_async_result_propagate_error(simple, error);
}

static void disable_technology_cb(GObject *object, GAsyncResult *res,
				 gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(user_data);
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  GError *error = NULL;

  g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_simple_async_result_set_from_error(simple, error);
    g_error_free(error);
    goto out;
  }

 out:
  g_simple_async_result_complete(simple);
  g_object_unref(simple);
}

void barman_manager_disable_technology(BarmanManager *self,
				       BarmanTechnologyType type,
				       GCancellable *cancellable,
				       GAsyncReadyCallback callback,
				       gpointer user_data)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GSimpleAsyncResult *simple;
  GVariant *parameters;
  const gchar *s;

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(priv != NULL);

  simple = g_simple_async_result_new(G_OBJECT(self), callback,
				     user_data,
				     barman_manager_disable_technology);

  s = technology_type2str(type);
  parameters = g_variant_new("(s)", s);

  /* FIXME: cancel the call on dispose */
  g_dbus_proxy_call(priv->proxy, "DisableTechnology", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, CONNECT_TIMEOUT, cancellable,
  		    disable_technology_cb, simple);
}

void barman_manager_disable_technology_finish(BarmanManager *self,
					       GAsyncResult *res,
					       GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(res);

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(g_simple_async_result_get_source_tag(simple) ==
		   barman_manager_disable_technology);

  g_simple_async_result_propagate_error(simple, error);
}

static void connect_service_cb(GObject *object, GAsyncResult *res,
			       gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(user_data);
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  GError *error = NULL;

  g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_simple_async_result_set_from_error(simple, error);
    g_error_free(error);
    goto out;
  }

 out:
  g_simple_async_result_complete(simple);
  g_object_unref(simple);
}

void barman_manager_connect_service(BarmanManager *self,
				     BarmanServiceType type,
				     BarmanServiceMode mode,
				     BarmanServiceSecurity security,
				     guint8 *ssid,
				     guint ssid_len,
				     GCancellable *cancellable,
				     GAsyncReadyCallback callback,
				     gpointer user_data)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  gchar ssid_buf[ssid_len + 1];
  GSimpleAsyncResult *simple;
  GVariantBuilder *builder;
  GVariant *parameters;

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(priv != NULL);

  simple = g_simple_async_result_new(G_OBJECT(self), callback,
                                      user_data,
				     barman_manager_connect_service);

  /*
   * FIXME: ssid can contain any character possible, including null, but
   * for now null terminate the buffer with null and send it as a string to
   * barman. But this needs to be properly fixed at some point.
   */
  memcpy(ssid_buf, ssid, ssid_len);
  ssid_buf[ssid_len] = '\0';

  builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(builder, "{sv}", "Type",
			g_variant_new_string(barman_service_type2str(type)));
  g_variant_builder_add(builder, "{sv}", "Mode",
			g_variant_new_string(barman_service_mode2str(mode)));
  g_variant_builder_add(builder, "{sv}", "Security",
			g_variant_new_string(barman_service_security2str(security)));
  g_variant_builder_add(builder, "{sv}", "SSID",
			g_variant_new_string(ssid_buf));

  parameters = g_variant_new("(a{sv})", builder);

  /* FIXME: cancel the call on dispose */
  g_dbus_proxy_call(priv->proxy, "ConnectService", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, CONNECT_TIMEOUT, cancellable,
  		    connect_service_cb, simple);

  /* FIXME: doesn't builder leak here? */
}

void barman_manager_connect_service_finish(BarmanManager *self,
					    GAsyncResult *res,
					    GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(res);

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(g_simple_async_result_get_source_tag(simple) ==
		   barman_manager_connect_service);

  g_simple_async_result_propagate_error(simple, error);
}

gboolean barman_manager_get_connected(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->connected;
}

static void iterate_create_list(gpointer key, gpointer value,
				gpointer user_data)
{
  GList **l = user_data;
  BarmanService *service = value;

  *l = g_list_append(*l, service);
}

/**
 * barman_manager_get_services:
 *
 * @self: self
 *
 * Returns: (array zero-terminated=1) (array length=false) (element-type Barman.Service) (transfer none): Null terminated list of services,
 * owned by BarmanManager. Caller must create a reference if it uses them.
 */
BarmanService **barman_manager_get_services(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  BarmanService *service;
  GList *l;
  gint i;

  g_return_val_if_fail(BARMAN_IS_MANAGER(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  if (!priv->connected)
    return NULL;

  if (priv->services == NULL)
    return NULL;

  l = NULL;

  g_hash_table_foreach(priv->services, iterate_create_list, &l);

  g_free(priv->array);

  /* allocate space for the pointers and the null termination */
  priv->array = g_malloc((sizeof(BarmanService *) * g_list_length(l)) + 1);

  if (priv->array == NULL)
    return NULL;

  for (i = 0; l != NULL; l = l->next) {
    service = l->data;

    /*
     * return only services which are ready yet, there will be
     * a separate service added signal for the ones which are not ready yet
     */
    if (!barman_service_is_ready(service))
      continue;

    priv->array[i++] = service;
  }

  priv->array[i] = NULL;

  g_list_free(l);
  l = NULL;

  return priv->array;
}

/**
 * barman_manager_get_service:
 *
 * @self: self
 * @path: path
 *
 * Returns: (transfer none): The service or null if not found.
 */
BarmanService *barman_manager_get_service(BarmanManager *self,
					    const gchar *path)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return g_hash_table_lookup(priv->services, path);
}

/**
 * barman_manager_get_default_service:
 *
 * @self: self
 *
 * Returns: (transfer none): The service or null if not found.
 */
BarmanService *barman_manager_get_default_service(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->default_service;
}

BarmanTechnologyState barman_manager_get_wifi_state(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self),
		       BARMAN_TECHNOLOGY_STATE_UNKNOWN);
  g_return_val_if_fail(priv != NULL, BARMAN_TECHNOLOGY_STATE_UNKNOWN);

  return priv->wifi_state;
}

BarmanTechnologyState barman_manager_get_ethernet_state(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self),
		       BARMAN_TECHNOLOGY_STATE_UNKNOWN);
  g_return_val_if_fail(priv != NULL, BARMAN_TECHNOLOGY_STATE_UNKNOWN);

  return priv->ethernet_state;
}

BarmanTechnologyState barman_manager_get_cellular_state(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self),
		       BARMAN_TECHNOLOGY_STATE_UNKNOWN);
  g_return_val_if_fail(priv != NULL, BARMAN_TECHNOLOGY_STATE_UNKNOWN);

  return priv->cellular_state;
}

BarmanTechnologyState barman_manager_get_bluetooth_state(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self),
		       BARMAN_TECHNOLOGY_STATE_UNKNOWN);
  g_return_val_if_fail(priv != NULL, BARMAN_TECHNOLOGY_STATE_UNKNOWN);

  return priv->bluetooth_state;
}

gboolean barman_manager_get_offline_mode(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_MANAGER(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->offline_mode;
}

void barman_manager_set_offline_mode(BarmanManager *self, gboolean mode)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(priv != NULL);

  g_object_set(self, "offline-mode", mode, NULL);
}

static void update_default_service(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  gboolean changed = FALSE;
  BarmanService *pending;

  g_return_if_fail(priv != NULL);

  pending = priv->pending_default_service;

  if (pending == NULL) {
    if (priv->default_service == NULL)
      goto out;

    /* remove current default service because there aren't any services */
    g_object_unref(priv->default_service);
    priv->default_service = NULL;
    changed = TRUE;
    goto check;
  }

  /* no need to do anything if the service is already the default */
  if (priv->default_service == pending)
    goto check;

  if (!barman_service_is_ready(pending))
    /* wait for the ready signal from the service */
    goto check;

  switch(barman_service_get_state(pending)) {
  case BARMAN_SERVICE_STATE_IDLE:
  case BARMAN_SERVICE_STATE_FAILURE:
  case BARMAN_SERVICE_STATE_ASSOCIATION:
  case BARMAN_SERVICE_STATE_CONFIGURATION:
  case BARMAN_SERVICE_STATE_DISCONNECT:
    /* pending isn't connected and we can't use it as default, yet  */
    goto check;
  case BARMAN_SERVICE_STATE_READY:
  case BARMAN_SERVICE_STATE_LOGIN:
  case BARMAN_SERVICE_STATE_ONLINE:
    break;
  }

  /* pending service is now ready, let's switch to it */
  if (priv->default_service != NULL)
    g_object_unref(priv->default_service);

  /* the ownership moves from pending_default_service to default_service */
  priv->default_service = pending;
  priv->pending_default_service = NULL;
  changed = TRUE;

 check:
  if (priv->default_service == NULL)
    goto out;

  /*
   * Let's check the state of the default service and whether it needs to
   * be removed.
   */
  switch(barman_service_get_state(priv->default_service)) {
  case BARMAN_SERVICE_STATE_IDLE:
  case BARMAN_SERVICE_STATE_FAILURE:
  case BARMAN_SERVICE_STATE_ASSOCIATION:
  case BARMAN_SERVICE_STATE_CONFIGURATION:
  case BARMAN_SERVICE_STATE_DISCONNECT:
    /* remove the default service because it's not valid anymore */
    g_object_unref(priv->default_service);
    priv->default_service = NULL;
    changed = TRUE;
    break;
  case BARMAN_SERVICE_STATE_READY:
  case BARMAN_SERVICE_STATE_LOGIN:
  case BARMAN_SERVICE_STATE_ONLINE:
    break;
  }

 out:
  /* don't send any signals until connected to barmand */
  if (priv->connected && changed)
    g_object_notify(G_OBJECT(self), "default-service");
}

static void iterate_service_ready(gpointer key, gpointer value,
				  gpointer user_data)
{
  gboolean *not_ready = user_data;
  BarmanService *service = value;

  if (!barman_service_is_ready(service))
    *not_ready = TRUE;
}

static gboolean all_services_ready(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  gboolean not_ready = FALSE;

  g_hash_table_foreach(priv->services, iterate_service_ready, &not_ready);

  return !not_ready;
}

static void service_ready(BarmanService *service, GParamSpec *pspec,
			  gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  if (!barman_service_is_ready(service))
    return;

  if (priv->pending_default_service != NULL &&
      priv->pending_default_service == service)
    update_default_service(self);

  /*
   * Send service added signals only after we are connected. The client
   * needs to call get_services() when it has received the connected
   * signal.
   */
  if (!priv->connected) {
    if (all_services_ready(self)) {
      priv->connected = TRUE;
      g_object_notify(G_OBJECT(self), "connected");
    }

    return;
  }

  g_signal_emit(self, signals[SERVICE_ADDED_SIGNAL], 0, service);
}

static void service_state_notify(BarmanService *service, GParamSpec *pspec,
				 gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  if (priv->pending_default_service == service ||
      priv->default_service == service)
    update_default_service(self);
}

static BarmanService *create_service(BarmanManager *self,
				      const gchar *path)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  BarmanService *service;

  service = barman_service_new(path);

  g_hash_table_insert(priv->services, g_strdup(path), service);

  /* FIXME: disconnect ready handler */
  g_signal_connect(service, "notify::ready", G_CALLBACK(service_ready),
		   self);
  g_signal_connect(service, "notify::state",
		   G_CALLBACK(service_state_notify), self);

  return service;
}

static void remove_service(BarmanManager *self, BarmanService *service)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  const gchar *path = barman_service_get_path(service);

  /*
   * Send service removed signals only when connected, connected
   * property with false value implies that services are lost.
   */
  if (priv->connected)
    g_signal_emit(self, signals[SERVICE_REMOVED_SIGNAL], 0, path);

  g_signal_handlers_disconnect_by_func(service,
				       G_CALLBACK(service_state_notify),
				       self);

  g_object_unref(service);
}

static GHashTable *create_services_list(BarmanManager *self)
{
  return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static void notify_service_removal(gpointer key, gpointer value,
				   gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanService *service = BARMAN_SERVICE(value);

  remove_service(self, service);
}

static void remove_services_list(BarmanManager *self, GHashTable *list)
{
  g_hash_table_foreach(list, notify_service_removal, self);
  g_hash_table_destroy(list);
}

static void update_services(BarmanManager *self, GVariant *variant)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GHashTable *old_services;
  BarmanService *service;
  gchar *path;
  GVariantIter iter;
  gint count;

  old_services = priv->services;
  priv->services = create_services_list(self);

  g_variant_iter_init(&iter, variant);
  count = 0;

  while (g_variant_iter_next(&iter, "o", &path)) {
    /* try find an existing service */
    if (old_services != NULL) {
      service = g_hash_table_lookup(old_services, path);

      if (service != NULL) {
	/* there's already an object for this service */
	g_hash_table_remove(old_services, path);
	g_hash_table_insert(priv->services, path, service);
	goto next;
      }
    }

    service = create_service(self, path);
    g_free(path);

  next:
    if (count++ == 0) {
      if (priv->pending_default_service != NULL)
	g_object_unref(priv->pending_default_service);

      priv->pending_default_service = g_object_ref(service);
      update_default_service(self);
    }
  }

  /* remove services which are not visible anymore */
  if (old_services != NULL) {
    remove_services_list(self, old_services);
    old_services = NULL;
  }
}

static BarmanTechnologyType technology_str2type(const gchar *type)
{
  const struct technology_type_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(technology_type_map); i++) {
    s = &technology_type_map[i];
    if (g_strcmp0(s->str, type) == 0)
      return s->type;
  }

  g_warning("unknown technology type %s", type);

  return BARMAN_TECHNOLOGY_TYPE_UNKNOWN;
}

static BarmanTechnologyState technology_str2state(const gchar *state)
{
  const struct technology_state_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(technology_state_map); i++) {
    s = &technology_state_map[i];
    if (g_strcmp0(s->str, state) == 0)
      return s->state;
  }

  g_warning("unknown technology state %s", state);

  return BARMAN_TECHNOLOGY_STATE_UNKNOWN;
}

static void technology_notify_state(BarmanManager *self,
				    struct technology *tech)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  BarmanTechnologyState *state;
  const gchar *property = NULL;

  g_return_if_fail(tech != NULL);
  g_return_if_fail(tech->state != BARMAN_TECHNOLOGY_STATE_UNKNOWN);

  switch (tech->type) {
  case BARMAN_TECHNOLOGY_TYPE_UNKNOWN:
    /* type is not yet retrieved, can happen with get_properties() */
    return;
  case BARMAN_TECHNOLOGY_TYPE_ETHERNET:
    state = &priv->ethernet_state;
    property = "ethernet-state";
    break;
  case BARMAN_TECHNOLOGY_TYPE_WIFI:
    state = &priv->wifi_state;
    property = "wifi-state";
    break;
  case BARMAN_TECHNOLOGY_TYPE_BLUETOOTH:
    state = &priv->bluetooth_state;
    property = "bluetooth-state";
    break;
  case BARMAN_TECHNOLOGY_TYPE_CELLULAR:
    state = &priv->cellular_state;
    property = "cellular-state";
    break;
  default:
    g_warning("%s(): unknown type %d", __func__, tech->type);
    return;
  }

  if (*state == tech->state)
    return;

  g_return_if_fail(property != NULL);

  *state = tech->state;
  g_object_notify(G_OBJECT(self), property);
}

static void technology_state_updated(BarmanManager *self,
				     struct technology *tech,
				     GVariant *value)
{
  const gchar *s;

  s = g_variant_get_string(value, NULL);
  tech->state = technology_str2state(s);

  technology_notify_state(self, tech);

  g_variant_unref(value);
}

static void technology_type_updated(BarmanManager *self,
				    struct technology *tech,
				    GVariant *value)
{
  const gchar *s;

  s = g_variant_get_string(value, NULL);
  tech->type = technology_str2type(s);

  g_variant_unref(value);
}

static void technology_property_updated(BarmanManager *self,
					GDBusProxy *proxy,
					const gchar *property,
					GVariant *value)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  struct technology *tech;

  tech = g_hash_table_lookup(priv->technologies,
			     g_dbus_proxy_get_object_path(proxy));

  if (tech == NULL) {
    g_warning("Did not find tech for %s %s update",
	      g_dbus_proxy_get_object_path(proxy), property);
    g_variant_unref(value);
    return;
  }

  if (g_strcmp0(property, BARMAN_TECHNOLOGY_PROPERTY_STATE) == 0)
    technology_state_updated(self, tech, value);
  else if (g_strcmp0(property, BARMAN_TECHNOLOGY_PROPERTY_TYPE) == 0)
    technology_type_updated(self, tech, value);
  else
    /* unknown property */
    g_variant_unref(value);
}

static void technology_property_changed(BarmanManager *self,
				  GDBusProxy *proxy,
				  GVariant *parameters)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GVariant *value;
  gchar *name;

  g_return_if_fail(priv != NULL);

  g_variant_get(parameters, "(sv)", &name, &value);

  /* callee takes the ownership of the variant */
  technology_property_updated(self, proxy, name, value);

  g_free(name);
}

static void technology_g_signal(GDBusProxy *proxy,
			  const gchar *sender_name,
			  const gchar *signal_name,
			  GVariant *parameters,
			  gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);

  /*
   * gdbus documentation is not clear who owns the variant so take it, just
   * in case
   */
  g_variant_ref(parameters);

  if (g_strcmp0(signal_name, "PropertyChanged") == 0)
    technology_property_changed(self, proxy, parameters);

  g_variant_unref(parameters);
}


static void technology_get_properties_cb(GObject *source_object,
					 GAsyncResult *res,
					 gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GDBusProxy *proxy = G_DBUS_PROXY(source_object);
  GVariant *result, *value, *dict;
  struct technology *tech;
  GError *error = NULL;
  GVariantIter iter;
  gchar *name;

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv != NULL);
  g_return_if_fail(proxy != NULL);

  result = g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_warning("Failed to get barman properties: %s", error->message);
    g_error_free(error);
    return;
  }

  if (result == NULL)
    return;

  /* get the dict */
  dict = g_variant_get_child_value(result, 0);

  /* go through the dict */
  g_variant_iter_init(&iter, dict);

  while (g_variant_iter_next(&iter, "{sv}", &name, &value)) {
    /* callee owns the variant now */
    technology_property_updated(self, proxy, name, value);

    g_free(name);
  }

  tech = g_hash_table_lookup(priv->technologies,
			     g_dbus_proxy_get_object_path(proxy));

  if (tech == NULL) {
    g_warning("Did not find tech for %s during the first state notify",
	      g_dbus_proxy_get_object_path(proxy));
    goto out;
  }

  /*
   * this needs to come after all properties are updated so that the type
   * is known
   */
  technology_notify_state(self, tech);

 out:
  g_variant_unref(dict);
  g_variant_unref(result);
}


static void technology_get_properties(BarmanManager *self, GDBusProxy *proxy)
{
  g_return_if_fail(self != NULL);
  g_return_if_fail(proxy != NULL);

  g_dbus_proxy_call(proxy, "GetProperties", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    technology_get_properties_cb, self);

  /* FIXME: howto cancel the call during dispose? */
}

static void technology_create_proxy_cb(GObject *source_object,
				       GAsyncResult *res,
				       gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GDBusNodeInfo *introspection_data;
  GDBusInterfaceInfo *info;
  GError *error = NULL;
  GDBusProxy *proxy;

  g_return_if_fail(priv != NULL);

  proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get technology proxy: %s", error->message);
    g_error_free(error);
    return;
  }

  if (proxy == NULL) {
    g_warning("Failed to get technology proxy, but no errors");
    return;
  }

  g_signal_connect(proxy, "g-signal", G_CALLBACK(technology_g_signal), self);

  introspection_data = g_dbus_node_info_new_for_xml(barman_technology_xml,
						    NULL);
  g_return_if_fail(introspection_data != NULL);

  info = introspection_data->interfaces[0];
  g_dbus_proxy_set_interface_info(proxy, info);

  technology_get_properties(self, proxy);
}

static struct technology *technology_added(BarmanManager *self,
					   const gchar *path)
{
  struct technology *tech;

  tech = g_malloc0(sizeof(*tech));

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
			   NULL, BARMAN_SERVICE_NAME, path,
			   BARMAN_TECHNOLOGY_INTERFACE, NULL,
			   technology_create_proxy_cb, self);

  return tech;
}

static void technology_removed(BarmanManager *self, struct technology *tech)
{
  tech->state = BARMAN_TECHNOLOGY_STATE_UNAVAILABLE;
  technology_notify_state(self, tech);

  if (tech->proxy != NULL)
    g_object_unref(tech->proxy);

  /* a precaution */
  memset(tech, 0, sizeof(*tech));

  g_free(tech);
}

static void technology_notify_removal(gpointer key, gpointer value,
				      gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  struct technology *tech = value;

  g_return_if_fail(self != NULL);
  g_return_if_fail(tech != NULL);

  technology_removed(self, tech);
}

static GHashTable *create_technology_table(BarmanManager *self)
{
  return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static void destroy_technology_table(BarmanManager *self, GHashTable *list)
{
  g_hash_table_foreach(list, technology_notify_removal, self);
  g_hash_table_destroy(list);
}

static void technologies_updated(BarmanManager *self, GVariant *variant)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  struct technology *tech;
  GHashTable *old_techs;
  gchar *path;
  GVariantIter iter;

  g_return_if_fail(priv->technologies != NULL);

  old_techs = priv->technologies;
  priv->technologies = create_technology_table(self);

  g_variant_iter_init(&iter, variant);

  while (g_variant_iter_next(&iter, "o", &path)) {
    /* try find an existing proxy */
    if ((tech = g_hash_table_lookup(old_techs, path)) != NULL) {
	/* there's already an object for this service */
	g_hash_table_remove(old_techs, path);
    } else {
      tech = technology_added(self, path);
    }

    g_hash_table_insert(priv->technologies, path, tech);

    /* hashtable now owns path */
  }

  /* remove services which are not visible anymore */
  destroy_technology_table(self, old_techs);

  g_variant_unref(variant);
}

static void offline_mode_updated(BarmanManager *self, GVariant *value)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  gboolean mode;

  mode = g_variant_get_boolean(value);

  if (priv->offline_mode == mode)
    return;

  priv->offline_mode = mode;
  g_object_notify(G_OBJECT(self), "offline-mode");

  g_variant_unref(value);
}

static void update_property(BarmanManager *self, const gchar *property,
			    GVariant *variant)
{
  if (g_strcmp0(property, BARMAN_PROPERTY_SERVICES) == 0) {
    update_services(self, variant);
  } else if (g_strcmp0(property, BARMAN_PROPERTY_TECHNOLOGIES) == 0) {
    technologies_updated(self, variant);
  } else if (g_strcmp0(property, BARMAN_PROPERTY_OFFLINE_MODE) == 0) {
    offline_mode_updated(self, variant);
  } else {
    /* unknown property */
    g_variant_unref(variant);
    variant = NULL;
  }
}

static void property_changed(BarmanManager *self, GVariant *parameters)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GVariant *variant;
  gchar *name;

  g_return_if_fail(priv != NULL);

  g_variant_get(parameters, "(sv)", &name, &variant);

  /* callee takes the ownership of the variant */
  update_property(self, name, variant);

  g_free(name);
}

static void barman_manager_g_signal(GDBusProxy *proxy,
				   const gchar *sender_name,
				   const gchar *signal_name,
				   GVariant *parameters,
				   gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);

  /*
   * gdbus documentation is not clear who owns the variant so take it, just
   * in case
   */
  g_variant_ref(parameters);

  if (g_strcmp0(signal_name, "PropertyChanged") == 0)
    property_changed(self, parameters);

  g_variant_unref(parameters);
}

static void get_properties_cb(GObject *source_object, GAsyncResult *res,
			      gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GVariant *result, *variant, *dict;
  GError *error = NULL;
  GVariantIter iter, dict_iter;
  gchar *name;

  g_return_if_fail(self != NULL);
  g_return_if_fail(priv != NULL);

  result = g_dbus_proxy_call_finish(priv->proxy, res, &error);

  if (error != NULL) {
    g_warning("Failed to get barman properties: %s", error->message);
    g_error_free(error);
    return;
  }

  if (result == NULL)
    return;

  g_variant_iter_init(&iter, result);

  /* get the dict */
  dict = g_variant_iter_next_value(&iter);

  /* go through the dict */
  g_variant_iter_init(&dict_iter, dict);

  while (g_variant_iter_next(&dict_iter, "{sv}", &name, &variant)) {
    /* callee owns the variant now */
    update_property(self, name, variant);

    g_free(name);
  }

  if (!priv->initialised)
    priv->initialised = TRUE;

  if (!priv->connected && g_hash_table_size(priv->services) == 0) {
    /* no services so we can change to connected immediately */
    priv->connected = TRUE;
    g_object_notify(G_OBJECT(self), "connected");
  }

  g_variant_unref(dict);
  g_variant_unref(result);
}


static void get_properties(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(self != NULL);

  g_dbus_proxy_call(priv->proxy, "GetProperties", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    get_properties_cb, self);
}

static void barman_appeared(GDBusConnection *connection, const gchar *name,
			     const gchar *name_owner, gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);

  get_properties(self);
}


static void barman_vanished(GDBusConnection *connection, const gchar *name,
			     gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  /* if barman is not available, send a disconnected signal during init */

  if (priv->initialised && !priv->connected)
    return;

  priv->connected = FALSE;

  if (priv->services != NULL) {
    remove_services_list(self, priv->services);
    priv->services = NULL;
  }

  if (!priv->initialised)
    priv->initialised = TRUE;

  g_object_notify(G_OBJECT(self), "connected");
}


static void create_proxy_cb(GObject *source_object, GAsyncResult *res,
			    gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GDBusNodeInfo *introspection_data;
  GDBusInterfaceInfo *info;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  priv->proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get barman proxy: %s", error->message);
    g_error_free(error);
    return;
  }

  if (priv->proxy == NULL) {
    g_warning("Failed to get barman proxy, but no errors");
    return;
  }

  g_signal_connect(priv->proxy, "g-signal", G_CALLBACK(barman_manager_g_signal),
		   self);

  introspection_data = g_dbus_node_info_new_for_xml(barman_manager_xml,
						    NULL);
  g_return_if_fail(introspection_data != NULL);

  info = introspection_data->interfaces[0];
  g_dbus_proxy_set_interface_info(priv->proxy, info);

  priv->watch_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM, BARMAN_SERVICE_NAME,
				    G_BUS_NAME_WATCHER_FLAGS_NONE,
				    barman_appeared,
				    barman_vanished,
				    self,
				    NULL);
}

static void set_dbus_property_cb(GObject *object,
				 GAsyncResult *res,
				 gpointer user_data)
{
  BarmanManager *self = BARMAN_MANAGER(user_data);
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  GError *error = NULL;

  g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_warning("BarmanManager SetProperty() failed: %s", error->message);
    g_error_free(error);
  }

  /* trick to avoid destroying self during async call */
  g_object_unref(self);
}

static void set_dbus_property(BarmanManager *self,
			      const gchar *property,
			      GVariant *value)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GVariant *parameters;

  g_return_if_fail(BARMAN_IS_MANAGER(self));
  g_return_if_fail(priv != NULL);

  parameters = g_variant_new("(sv)", property, value);

  g_dbus_proxy_call(priv->proxy, "SetProperty", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    set_dbus_property_cb, g_object_ref(self));
}

static void update_offline_mode(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);
  GVariant *value;

  value = g_variant_new("b", priv->offline_mode);

  set_dbus_property(self, "OfflineMode", value);
}

static void set_property(GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
  BarmanManager *self = BARMAN_MANAGER(object);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_CONNECTED:
      priv->connected = g_value_get_boolean(value);
      break;
  case PROP_DEFAULT_SERVICE:
      priv->default_service = g_value_get_object(value);
      break;
  case PROP_WIFI_STATE:
    priv->wifi_state = g_value_get_uint(value);
    break;
  case PROP_ETHERNET_STATE:
    priv->ethernet_state = g_value_get_uint(value);
    break;
  case PROP_CELLULAR_STATE:
    priv->cellular_state = g_value_get_uint(value);
    break;
  case PROP_BLUETOOTH_STATE:
    priv->bluetooth_state = g_value_get_uint(value);
    break;
  case PROP_OFFLINE_MODE:
    priv->offline_mode = g_value_get_boolean(value);
    update_offline_mode(self);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void get_property(GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
  BarmanManager *self = BARMAN_MANAGER(object);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  switch(property_id) {
  case PROP_CONNECTED:
    g_value_set_boolean(value, priv->connected);
    break;
  case PROP_DEFAULT_SERVICE:
    g_value_set_object(value, priv->default_service);
    break;
  case PROP_WIFI_STATE:
    g_value_set_uint(value, priv->wifi_state);
    break;
  case PROP_ETHERNET_STATE:
    g_value_set_uint(value, priv->ethernet_state);
    break;
  case PROP_CELLULAR_STATE:
    g_value_set_uint(value, priv->cellular_state);
    break;
  case PROP_BLUETOOTH_STATE:
    g_value_set_uint(value, priv->bluetooth_state);
    break;
  case PROP_OFFLINE_MODE:
    g_value_set_boolean(value, priv->offline_mode);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void barman_manager_dispose(GObject *object)
{
  BarmanManager *self = BARMAN_MANAGER(object);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  if (priv->services != NULL) {
    remove_services_list(self, priv->services);
    priv->services = NULL;
  }

  if (priv->technologies != NULL) {
    destroy_technology_table(self, priv->technologies);
    priv->technologies = NULL;
  }

  if (priv->proxy != NULL) {
    g_object_unref(priv->proxy);
    priv->proxy = NULL;
  }

  if (priv->watch_id != 0) {
    g_bus_unwatch_name(priv->watch_id);
    priv->watch_id = 0;
  }

  if (priv->default_service != NULL) {
    g_object_unref(priv->default_service);
    priv->default_service = NULL;
  }

  if (priv->pending_default_service != NULL) {
    g_object_unref(priv->pending_default_service);
    priv->pending_default_service = NULL;
  }

  G_OBJECT_CLASS(barman_manager_parent_class)->dispose(object);
}

static void barman_manager_finalize(GObject *object)
{
  BarmanManager *self = BARMAN_MANAGER(object);
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  g_free(priv->array);
  priv->array = NULL;

  G_OBJECT_CLASS(barman_manager_parent_class)->finalize(object);
}

static void barman_manager_class_init(BarmanManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec;
  guint id;

  g_type_class_add_private(klass, sizeof(BarmanManagerPrivate));

  gobject_class->dispose = barman_manager_dispose;
  gobject_class->finalize = barman_manager_finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  id = g_signal_new("service-added",
		    G_TYPE_FROM_CLASS(klass),
		    G_SIGNAL_RUN_LAST,
		    0, NULL, NULL,
		    _marshal_VOID__BOXED,
		    G_TYPE_NONE, 1, BARMAN_TYPE_SERVICE);
  signals[SERVICE_ADDED_SIGNAL] = id;

  id = g_signal_new("service-removed",
		    G_TYPE_FROM_CLASS(klass),
		    G_SIGNAL_RUN_LAST,
		    0, NULL, NULL,
		    _marshal_VOID__STRING,
		    G_TYPE_NONE, 1, G_TYPE_STRING);
  signals[SERVICE_REMOVED_SIGNAL] = id;

  pspec = g_param_spec_boolean("connected",
			       "BarmanManager's connected property",
			       "Set connected state", FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_CONNECTED, pspec);

  pspec = g_param_spec_object("default-service",
			     "BarmanManager's default service",
			     "Set default service",
			     BARMAN_TYPE_SERVICE,
			     G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_DEFAULT_SERVICE, pspec);


  pspec = g_param_spec_uint("wifi-state",
			    "BarmanManager's wifi state property",
			    "The state of the technology",
			    0, G_MAXUINT,
			    BARMAN_TECHNOLOGY_STATE_UNKNOWN,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_WIFI_STATE, pspec);

  pspec = g_param_spec_uint("ethernet-state",
			    "BarmanManager's ethernet state property",
			    "The state of the technology",
			    0, G_MAXUINT,
			    BARMAN_TECHNOLOGY_STATE_UNKNOWN,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_ETHERNET_STATE, pspec);

  pspec = g_param_spec_uint("cellular-state",
			    "BarmanManager's cellular state property",
			    "The state of the technology",
			    0, G_MAXUINT,
			    BARMAN_TECHNOLOGY_STATE_UNKNOWN,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_CELLULAR_STATE, pspec);

  pspec = g_param_spec_uint("bluetooth-state",
			    "BarmanManager's bluetooth state property",
			    "The state of the technology",
			    0, G_MAXUINT,
			    BARMAN_TECHNOLOGY_STATE_UNKNOWN,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_BLUETOOTH_STATE, pspec);

  pspec = g_param_spec_boolean("offline-mode",
			       "BarmanManager's offline mode property",
			       "Set offline mode", FALSE,
			       G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_OFFLINE_MODE, pspec);
}

static void barman_manager_init(BarmanManager *self)
{
  BarmanManagerPrivate *priv = GET_PRIVATE(self);

  priv->services = NULL;
  priv->technologies = create_technology_table(self);

  priv->proxy = NULL;
  priv->connected = FALSE;
  priv->initialised = FALSE;

  priv->default_service = NULL;
  priv->pending_default_service = NULL;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
			   NULL, BARMAN_SERVICE_NAME, BARMAN_MANAGER_PATH,
			   BARMAN_MANAGER_INTERFACE, NULL,
			   create_proxy_cb, self);
}

BarmanManager *barman_manager_new(void)
{
  return g_object_new(BARMAN_TYPE_MANAGER, NULL);
}
