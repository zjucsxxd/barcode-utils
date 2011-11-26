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

#include "service-manager.h"

#include <glib.h>

#include "service.h"
#include "barman.h"
#include "manager.h"
#include "marshal.h"

G_DEFINE_TYPE (ServiceManager, service_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_SERVICE_MANAGER, 	\
				ServiceManagerPrivate))

typedef struct _ServiceManagerPrivate ServiceManagerPrivate;

struct _ServiceManagerPrivate {
  GList *services;
  Manager *network_service;
};

enum {
  STATE_CHANGED,
  STRENGTH_UPDATED,
  SERVICES_UPDATED,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

static void service_manager_free_all(GList *list);

static void service_manager_dispose(GObject *object)
{
  G_OBJECT_CLASS (service_manager_parent_class)->dispose (object);
}

static void service_manager_finalize(GObject *object)
{
  ServiceManager *self = SERVICE_MANAGER(object);
  ServiceManagerPrivate *priv = GET_PRIVATE(self);

  service_manager_free_all(priv->services);
  priv->services = NULL;

  G_OBJECT_CLASS (service_manager_parent_class)->finalize (object);
}

static void service_manager_class_init(ServiceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ServiceManagerPrivate));

  object_class->dispose = service_manager_dispose;
  object_class->finalize = service_manager_finalize;

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
					_marshal_VOID__UINT,
					G_TYPE_NONE, 1,
					G_TYPE_UINT);

  signals[SERVICES_UPDATED] = g_signal_new("services-updated",
					   G_TYPE_FROM_CLASS(klass),
					   G_SIGNAL_RUN_LAST,
					   0, NULL, NULL,
					   _marshal_VOID__VOID,
					   G_TYPE_NONE, 0);
}

static void service_manager_init(ServiceManager *self)
{
}

ServiceManager *service_manager_new(Manager *ns)
{
  ServiceManager *self;
  ServiceManagerPrivate *priv;

  self = g_object_new (TYPE_SERVICE_MANAGER, NULL);
  priv = GET_PRIVATE(self);

  priv->network_service = ns;

  return self;
}

static void service_manager_free_all(GList *list)
{
  Service *service;
  GList *iter;

  for (iter = list; iter != NULL; iter = iter->next) {
    service = iter->data;
    g_object_unref(service);
  }

  g_list_free(list);
}

static GList *service_manager_find(GList *list, const gchar *path)
{
  Service *service;
  GList *iter;

  for (iter = list; iter != NULL; iter = iter->next) {
    service = iter->data;
    if (g_strcmp0(path, service_get_path(service)) == 0)
      return iter;
  }

  return NULL;
}

void service_manager_remove_all(ServiceManager *self)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);

  service_manager_free_all(priv->services);
  priv->services = NULL;
}

/*
 * Beginnings of proper sorting of services. Needs a lot more work, for example
 * need to give more priority for favourite services
 */
static gint cmp_services(gconstpointer a, gconstpointer b)
{
  Service *a_service, *b_service;
  BarmanServiceType a_type, b_type;
  gboolean a_connected, b_connected;
  guint a_strength, b_strength;

  a_service = SERVICE(a);
  b_service = SERVICE(b);

  a_type = service_get_service_type(a_service);
  b_type = service_get_service_type(b_service);

  if (service_get_state(a_service) == BARMAN_SERVICE_STATE_READY ||
      service_get_state(a_service) == BARMAN_SERVICE_STATE_ONLINE)
    a_connected = TRUE;
  else
    a_connected = FALSE;

  if (service_get_state(b_service) == BARMAN_SERVICE_STATE_READY ||
      service_get_state(b_service) == BARMAN_SERVICE_STATE_ONLINE)
    b_connected = TRUE;
  else
    b_connected = FALSE;

  a_strength = service_get_strength(a_service);
  b_strength = service_get_strength(b_service);

  /* ethernet */

  if (a_type == BARMAN_SERVICE_TYPE_ETHERNET &&
      b_type == BARMAN_SERVICE_TYPE_ETHERNET)
    return 0;

  if (a_type == BARMAN_SERVICE_TYPE_ETHERNET &&
      b_type == BARMAN_SERVICE_TYPE_WIFI)
    return -1;

  if (a_type == BARMAN_SERVICE_TYPE_ETHERNET &&
      b_type == BARMAN_SERVICE_TYPE_BLUETOOTH)
    return -1;

  if (a_type == BARMAN_SERVICE_TYPE_ETHERNET &&
      b_type == BARMAN_SERVICE_TYPE_CELLULAR)
    return -1;

  /* wifi */

  if (a_type == BARMAN_SERVICE_TYPE_WIFI &&
      b_type == BARMAN_SERVICE_TYPE_ETHERNET)
    return 1;

  if (a_type == BARMAN_SERVICE_TYPE_WIFI &&
      b_type == BARMAN_SERVICE_TYPE_WIFI) {
    if (a_connected && !b_connected)
      return -1;
    else if (!a_connected && b_connected)
      return 1;
    else
      return b_strength - a_strength;
  }

  if (a_type == BARMAN_SERVICE_TYPE_WIFI &&
      b_type == BARMAN_SERVICE_TYPE_BLUETOOTH)
    return -1;

  if (a_type == BARMAN_SERVICE_TYPE_WIFI &&
      b_type == BARMAN_SERVICE_TYPE_CELLULAR)
    return -1;

  /* bluetooth */

  if (a_type == BARMAN_SERVICE_TYPE_BLUETOOTH &&
      b_type == BARMAN_SERVICE_TYPE_ETHERNET)
    return 1;

  if (a_type == BARMAN_SERVICE_TYPE_BLUETOOTH &&
      b_type == BARMAN_SERVICE_TYPE_WIFI)
    return 1;

  if (a_type == BARMAN_SERVICE_TYPE_BLUETOOTH &&
      b_type == BARMAN_SERVICE_TYPE_BLUETOOTH)
    return 0;

  if (a_type == BARMAN_SERVICE_TYPE_BLUETOOTH &&
      b_type == BARMAN_SERVICE_TYPE_CELLULAR)
    return 1;

  /* cellular */

  if (a_type == BARMAN_SERVICE_TYPE_CELLULAR &&
      b_type == BARMAN_SERVICE_TYPE_ETHERNET)
    return 1;

  if (a_type == BARMAN_SERVICE_TYPE_CELLULAR &&
      b_type == BARMAN_SERVICE_TYPE_WIFI)
    return 1;

  if (a_type == BARMAN_SERVICE_TYPE_CELLULAR &&
      b_type == BARMAN_SERVICE_TYPE_BLUETOOTH)
    return -1;

  if (a_type == BARMAN_SERVICE_TYPE_CELLULAR &&
      b_type == BARMAN_SERVICE_TYPE_CELLULAR)
    return b_strength - a_strength;

  return 0;
}

static void service_state_changed(Service *service, gpointer user_data)
{
  ServiceManager *self = SERVICE_MANAGER(user_data);
  ServiceManagerPrivate *priv = GET_PRIVATE(self);

  g_signal_emit(self, signals[STATE_CHANGED], 0);

  /* services need to be sorted again due to the state change */
  priv->services = g_list_sort(priv->services, cmp_services);

  g_signal_emit(self, signals[SERVICES_UPDATED], 0);
}

static void service_strength_updated(Service *service, gpointer user_data)
{
  ServiceManager *self = SERVICE_MANAGER(user_data);

  g_signal_emit(self, signals[STRENGTH_UPDATED], 0,
		service_get_strength(service));
}

void service_manager_update_services(ServiceManager *self,
				     BarmanService **services)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);
  GList *old_services, *link;
  Service *service;
  const gchar *path;
  gint count = 0;

  old_services = priv->services;
  priv->services = NULL;

  for (; *services != NULL; services++) {
    path = barman_service_get_path(*services);

    /* check if we already have this service and reuse it */
    link = service_manager_find(old_services, path);
    if (link != NULL) {
      service = link->data;
      old_services = g_list_delete_link(old_services, link);
      link = NULL;
    } else {
      service = service_new(*services, priv->network_service);
      g_signal_connect(G_OBJECT(service), "state-changed",
      		       G_CALLBACK(service_state_changed), self);
      g_signal_connect(G_OBJECT(service), "strength-updated",
      		       G_CALLBACK(service_strength_updated), self);

      count++;
    }

    if (service == NULL)
      continue;

    priv->services = g_list_insert_sorted(priv->services, service,
					  cmp_services);
  }

  g_debug("services updated (total %d, new %d, removed %d)",
	  g_list_length(priv->services), count, g_list_length(old_services));

  g_signal_emit(self, signals[SERVICES_UPDATED], 0);

  /* remove unavailable services, we don't want to show them anymore */
  service_manager_free_all(old_services);
  old_services = NULL;
}

GList *service_manager_get_wired(ServiceManager *self)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);
  GList *list = NULL, *iter;
  Service *service;

  for (iter = priv->services; iter != NULL; iter = iter->next) {
    service = iter->data;
    if (service_get_service_type(service) == BARMAN_SERVICE_TYPE_ETHERNET) {
      list = g_list_append(list, service);
    }
  }

  return list;
}

GList *service_manager_get_wireless(ServiceManager *self)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);
  GList *list = NULL, *iter;
  Service *service;

  for (iter = priv->services; iter != NULL; iter = iter->next) {
    service = iter->data;
    if (service_get_service_type(service) == BARMAN_SERVICE_TYPE_WIFI) {
      list = g_list_append(list, service);

      /* show only certain number of wireless networks */
      if (g_list_length(list) >= 5)
	break;
    }
  }

  return list;
}

GList *service_manager_get_cellular(ServiceManager *self)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);
  GList *list = NULL, *iter;
  BarmanServiceType type;
  Service *service;

  for (iter = priv->services; iter != NULL; iter = iter->next) {
    service = iter->data;
    type = service_get_service_type(service);

    if (type == BARMAN_SERVICE_TYPE_CELLULAR) {
      list = g_list_append(list, service);
    } else if (type == BARMAN_SERVICE_TYPE_BLUETOOTH) {
      list = g_list_append(list, service);
    }
  }

  return list;
}

gboolean service_manager_is_connecting(ServiceManager *self)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);
  Service *service;
  GList *iter;

  for (iter = priv->services; iter != NULL; iter = iter->next) {
    service = iter->data;
    if (service_get_state(service) == BARMAN_SERVICE_STATE_ASSOCIATION ||
	service_get_state(service) == BARMAN_SERVICE_STATE_CONFIGURATION) {
      return TRUE;
    }
  }

  return FALSE;
}

gboolean service_manager_is_connected(ServiceManager *self)
{
  return service_manager_get_connected(self) > 0;
}

guint service_manager_get_connected(ServiceManager *self)
{
  ServiceManagerPrivate *priv = GET_PRIVATE(self);
  Service *service;
  BarmanServiceState state;
  GList *iter;
  guint result = 0;

  for (iter = priv->services; iter != NULL; iter = iter->next) {
    service = iter->data;
    state = service_get_state(service);

    if (state == BARMAN_SERVICE_STATE_READY ||
	state == BARMAN_SERVICE_STATE_LOGIN ||
	state == BARMAN_SERVICE_STATE_ONLINE) {
      result++;
    }
  }

  return result;
}
