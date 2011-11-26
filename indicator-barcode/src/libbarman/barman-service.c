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

#include "barman-service.h"

#include <glib/gi18n.h>

#include "barman-service-xml.h"
#include "barman.h"
#include "marshal.h"

G_DEFINE_TYPE(BarmanService, barman_service, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE((o), BARMAN_TYPE_SERVICE, \
			       BarmanServicePrivate))

typedef struct _BarmanServicePrivate BarmanServicePrivate;

struct _BarmanServicePrivate {
  GDBusProxy *proxy;

  gchar *path;
  gboolean ready;

  /* barman service properties */
  BarmanServiceState state;
  gchar *error;
  gchar *name;
  BarmanServiceType type;
  BarmanServiceMode mode;
  BarmanServiceSecurity security;
  gboolean login_required;
  gchar *passphrase;
  gboolean passphrase_required;
  guint8 strength;
  gboolean favorite;
  gboolean immutable;
  gboolean autoconnect;
  gboolean setup_required;
  gchar *apn;
  gchar *mcc;
  gchar *mnc;
  gboolean roaming;
  gchar **nameservers;
  gchar **nameservers_configuration;
  gchar **domains;
  gchar **domains_configuration;
  BarmanIPv4 *ipv4;
  BarmanIPv4 *ipv4_configuration;
  BarmanIPv6 *ipv6;
  BarmanIPv6 *ipv6_configuration;
};

enum
{
  /* reserved */
  PROP_0,

  PROP_PATH,
  PROP_READY,

  PROP_STATE,
  PROP_ERROR,
  PROP_NAME,
  PROP_TYPE,
  PROP_MODE,
  PROP_SECURITY,
  PROP_LOGIN_REQUIRED,
  PROP_PASSPHRASE, /* rw */
  PROP_PASSPHRASE_REQUIRED,
  PROP_STRENGTH,
  PROP_FAVORITE,
  PROP_IMMUTABLE,
  PROP_AUTOCONNECT, /* rw */
  PROP_SETUP_REQUIRED,
  PROP_APN, /* rw */
  PROP_MCC,
  PROP_MNC,
  PROP_ROAMING,
  PROP_NAMESERVERS,
  PROP_NAMESERVERS_CONFIGURATION, /* rw */
  PROP_DOMAINS,
  PROP_DOMAINS_CONFIGURATION, /* rw */
  PROP_IPV4,
  PROP_IPV4_CONFIGURATION, /* rw */
  PROP_IPV6,
  PROP_IPV6_CONFIGURATION, /* rw */
  /* PROP_PROXY, */
  /* PROP_PROXY_CONFIGURATION, /\* rw *\/ */
  /* PROP_PROVIDER, */
  /* PROP_ETHERNET, */
};

#define SERVICE_NAME_UNKNOWN _("<unnamed>")
#define DISCONNECT_TIMEOUT 10000
#define MAX_NAMESERVERS 100
#define MAX_DOMAINS 100

struct service_state_string
{
  const gchar *str;
  BarmanServiceState state;
};

static const struct service_state_string service_state_map[] = {
  { "idle",		BARMAN_SERVICE_STATE_IDLE },
  { "failure",		BARMAN_SERVICE_STATE_FAILURE },
  { "association",	BARMAN_SERVICE_STATE_ASSOCIATION },
  { "configuration",	BARMAN_SERVICE_STATE_CONFIGURATION },
  { "ready",		BARMAN_SERVICE_STATE_READY },
  { "login",		BARMAN_SERVICE_STATE_LOGIN },
  { "online",		BARMAN_SERVICE_STATE_ONLINE },
  { "disconnect",	BARMAN_SERVICE_STATE_DISCONNECT },
};

struct service_security_string
{
  const gchar *str;
  BarmanServiceSecurity security;
};

static const struct service_security_string service_security_map[] = {
  { "unknown",		BARMAN_SERVICE_SECURITY_UNKNOWN },
  { "none",		BARMAN_SERVICE_SECURITY_NONE },
  { "wep",		BARMAN_SERVICE_SECURITY_WEP },
  { "psk",		BARMAN_SERVICE_SECURITY_PSK },
  { "wpa",		BARMAN_SERVICE_SECURITY_PSK },
  { "rsn",		BARMAN_SERVICE_SECURITY_PSK },
  { "ieee8021x",	BARMAN_SERVICE_SECURITY_IEEE8021X },
};

struct service_type_string
{
  const gchar *str;
  BarmanServiceType type;
};

static const struct service_type_string service_type_map[] = {
  { "ethernet",		BARMAN_SERVICE_TYPE_ETHERNET },
  { "wifi",		BARMAN_SERVICE_TYPE_WIFI },
  { "bluetooth",	BARMAN_SERVICE_TYPE_BLUETOOTH },
  { "cellular",		BARMAN_SERVICE_TYPE_CELLULAR },
};

struct service_mode_string
{
  const gchar *str;
  BarmanServiceMode mode;
};

static const struct service_mode_string service_mode_map[] = {
  { "managed",		BARMAN_SERVICE_MODE_MANAGED },
  { "adhoc",		BARMAN_SERVICE_MODE_ADHOC },
  { "gprs",		BARMAN_SERVICE_MODE_GPRS },
  { "edge",		BARMAN_SERVICE_MODE_EDGE },
  { "umts",		BARMAN_SERVICE_MODE_UMTS },
};

static void connect_cb(GObject *object, GAsyncResult *res,
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

void barman_service_connect(BarmanService *self,
			     GAsyncReadyCallback callback,
			     gpointer user_data)
{
  GSimpleAsyncResult *simple;
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  simple = g_simple_async_result_new(G_OBJECT(self), callback,
                                      user_data, barman_service_connect);

  /* FIXME: cancel the call on dispose */
  g_dbus_proxy_call(priv->proxy, "Connect", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, CONNECT_TIMEOUT, NULL,
  		    connect_cb, simple);
}

void barman_service_connect_finish(BarmanService *self,
				    GAsyncResult *res,
				    GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(res);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(g_simple_async_result_get_source_tag(simple) ==
		   barman_service_connect);

  g_simple_async_result_propagate_error (simple, error);
}

static void disconnect_cb(GObject *object, GAsyncResult *res,
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

void barman_service_disconnect(BarmanService *self,
				GAsyncReadyCallback callback,
				gpointer user_data)
{
  GSimpleAsyncResult *simple;
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  simple = g_simple_async_result_new(G_OBJECT(self), callback,
				     user_data, barman_service_disconnect);

  /* FIXME: cancel the call on dispose */
  g_dbus_proxy_call(priv->proxy, "Disconnect", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, CONNECT_TIMEOUT, NULL,
  		    disconnect_cb, simple);
}

void barman_service_disconnect_finish(BarmanService *self,
				       GAsyncResult *res,
				       GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(res);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(g_simple_async_result_get_source_tag(simple) ==
		   barman_service_disconnect);

  g_simple_async_result_propagate_error (simple, error);
}

/* property accessors */
BarmanServiceState barman_service_get_state(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), BARMAN_SERVICE_STATE_FAILURE);
  g_return_val_if_fail(priv != NULL, BARMAN_SERVICE_STATE_FAILURE);

  return priv->state;
}

const gchar *barman_service_get_error(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->error;
}

const gchar *barman_service_get_name(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->name;
}

BarmanServiceType barman_service_get_service_type(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), BARMAN_SERVICE_TYPE_ETHERNET);
  g_return_val_if_fail(priv != NULL, BARMAN_SERVICE_TYPE_ETHERNET);

  return priv->type;
}

/* FIXME: when adding wps support this should return a list */
BarmanServiceSecurity barman_service_get_security(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self),
		       BARMAN_SERVICE_SECURITY_UNKNOWN);
  g_return_val_if_fail(priv != NULL, BARMAN_SERVICE_SECURITY_UNKNOWN);

  return priv->security;
}

gboolean barman_service_get_login_required(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->login_required;
}

const gchar *barman_service_get_passphrase(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->passphrase != NULL ? priv->passphrase : "";
}

void barman_service_set_passphrase(BarmanService *self,
				    const gchar *passphrase)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  g_object_set(self, "passphrase", passphrase, NULL);
}

gboolean barman_service_get_passphrase_required(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->passphrase_required;
}

guint8 barman_service_get_strength(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), 0);
  g_return_val_if_fail(priv != NULL, 0);

  return priv->strength;
}

gboolean barman_service_get_favorite(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->favorite;
}

gboolean barman_service_get_immutable(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->immutable;
}

gboolean barman_service_get_autoconnect(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->autoconnect;
}

void barman_service_set_autoconnect(BarmanService *self,
				     gboolean autoconnect)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  g_object_set(self, "autoconnect", autoconnect, NULL);
}


gboolean barman_service_get_setup_required(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->setup_required;
}

const gchar *barman_service_get_apn(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->apn;
}

void barman_service_set_apn(BarmanService *self, const gchar *apn)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  g_object_set(self, "apn", apn, NULL);
}


const gchar *barman_service_get_mcc(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->mcc;
}

const gchar *barman_service_get_mnc(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->mnc;
}

gboolean barman_service_get_roaming(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->roaming;
}

/**
 * barman_service_get_nameservers:
 *
 * @self: self
 *
 * Returns: (array zero-terminated=1) (array length=false) (transfer none):
 * Null terminated list of strings, owned by BarmanService. Caller must
 * copy if it uses them.
 */
gchar **barman_service_get_nameservers(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->nameservers;
}

/**
 * barman_service_get_nameservers_configuration:
 *
 * @self: self
 *
 * Returns: (array zero-terminated=1) (array length=false) (transfer none):
 * Null terminated list of strings, owned by BarmanService. Caller must
 * copy if it uses them.
 */
gchar **barman_service_get_nameservers_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->nameservers_configuration;
}

/**
 * barman_service_set_nameservers_configuration:
 *
 * @self: self
 * @nameservers: (array zero-terminated=1) (transfer none): Null terminated list of strings, owned by caller.
 */
void barman_service_set_nameservers_configuration(BarmanService *self,
						   gchar **nameservers)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  /* FIXME: figure out and _document_ how to clear the nameservers */
  if (nameservers == NULL)
    return;

  g_object_set(self, "nameservers-configuration", nameservers, NULL);
}

/**
 * barman_service_get_domains:
 *
 * @self: self
 *
 * Returns: (array zero-terminated=1) (array length=false) (transfer none): Null terminated list of strings,
 * owned by BarmanManager. Caller must copy if it uses them.
 */
gchar **barman_service_get_domains(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->domains;
}

/**
 * barman_service_get_domains_configuration:
 *
 * @self: self
 *
 * Returns: (array zero-terminated=1) (array length=false) (transfer none):
 * Null terminated list of strings, owned by BarmanService. Caller must
 * copy if it uses them.
 */
gchar **barman_service_get_domains_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->domains_configuration;
}

/**
 * barman_service_set_domains_configuration:
 *
 * @self: self
 * @domains: (array zero-terminated=1) (transfer none): Null terminated list of strings, owned by caller.
 */
void barman_service_set_domains_configuration(BarmanService *self,
					       gchar **domains)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  /* FIXME: figure out and _document_ how to clear the domains */
  if (domains == NULL)
    return;

  g_object_set(self, "domains-configuration", domains, NULL);
}

/**
 * barman_service_get_ipv4:
 *
 * @self: self
 *
 * Returns: (transfer none): Current ipv4 settings. The caller must
 * take a reference.
 */
BarmanIPv4 *barman_service_get_ipv4(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->ipv4;
}

/**
 * barman_service_get_ipv4_configuration:
 *
 * @self: self
 *
 * Returns: (transfer none): Current configured ipv4 settings. The caller must
 * take a reference.
 */
BarmanIPv4 *barman_service_get_ipv4_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->ipv4_configuration;
}

void barman_service_set_ipv4_configuration(BarmanService *self,
					    BarmanIPv4 *ipv4)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  g_object_set(self, "ipv4-configuration", ipv4, NULL);
}

/**
 * barman_service_get_ipv6:
 *
 * @self: self
 *
 * Returns: (transfer none): Current ipv6 settings. The caller must
 * take a reference.
 */
BarmanIPv6 *barman_service_get_ipv6(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->ipv6;
}

/**
 * barman_service_get_ipv6_configuration:
 *
 * @self: self
 *
 * Returns: (transfer none): Current configured ipv6 settings. The caller must
 * take a reference.
 */
BarmanIPv6 *barman_service_get_ipv6_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->ipv6_configuration;
}

void barman_service_set_ipv6_configuration(BarmanService *self,
					    BarmanIPv6 *ipv6)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  g_object_set(self, "ipv6-configuration", ipv6, NULL);
}

const gchar *barman_service_get_path(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->path;
}

gboolean barman_service_is_ready(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_SERVICE(self), FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);

  return priv->ready;
}

BarmanServiceState barman_service_str2state(const gchar *state)
{
  const struct service_state_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_state_map); i++) {
    s = &service_state_map[i];
    if (g_strcmp0(s->str, state) == 0)
      return s->state;
  }

  g_warning("unknown service state %s", state);

  return BARMAN_SERVICE_STATE_IDLE;
}

const gchar *barman_service_state2str(BarmanServiceState state)
{
  const struct service_state_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_state_map); i++) {
    s = &service_state_map[i];
    if (s->state == state)
      return s->str;
  }

  g_warning("%s(): unknown state %d", __func__, state);

  return "unknown";
}

BarmanServiceType barman_service_str2type(const gchar *type)
{
  const struct service_type_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_type_map); i++) {
    s = &service_type_map[i];
    if (g_strcmp0(s->str, type) == 0)
      return s->type;
  }

  g_warning("unknown service type %s", type);

  return BARMAN_SERVICE_TYPE_ETHERNET;
}

const gchar *barman_service_type2str(BarmanServiceType type)
{
  const struct service_type_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_type_map); i++) {
    s = &service_type_map[i];
    if (s->type == type)
      return s->str;
  }

  g_warning("%s(): unknown type %d", __func__, type);

  return "unknown";
}

BarmanServiceSecurity barman_service_str2security(const gchar *security)
{
  const struct service_security_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_security_map); i++) {
    s = &service_security_map[i];
    if (g_strcmp0(s->str, security) == 0)
      return s->security;
  }

  g_warning("unknown service security %s", security);

  return BARMAN_SERVICE_SECURITY_UNKNOWN;
}

const gchar *barman_service_security2str(BarmanServiceSecurity security)
{
  const struct service_security_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_security_map); i++) {
    s = &service_security_map[i];
    if (s->security == security)
      return s->str;
  }

  g_warning("%s(): unknown security %d", __func__, security);

  return "unknown";
}

BarmanServiceMode barman_service_str2mode(const gchar *mode)
{
  const struct service_mode_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_mode_map); i++) {
    s = &service_mode_map[i];
    if (g_strcmp0(s->str, mode) == 0)
      return s->mode;
  }

  g_warning("unknown service mode %s", mode);

  return BARMAN_SERVICE_MODE_MANAGED;
}

const gchar *barman_service_mode2str(BarmanServiceMode mode)
{
  const struct service_mode_string *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(service_mode_map); i++) {
    s = &service_mode_map[i];
    if (s->mode == mode)
      return s->str;
  }

  g_warning("%s(): unknown mode %d", __func__, mode);

  return "unknown";
}

static void state_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  const gchar *s;

  s = g_variant_get_string(variant, NULL);
  priv->state = barman_service_str2state(s);

  g_object_notify(G_OBJECT(self), "state");

  g_variant_unref(variant);
}

static void error_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->error);

  priv->error = g_variant_dup_string(variant, NULL);
  g_object_notify(G_OBJECT(self), "error");

  g_variant_unref(variant);
}

static void name_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->name);

  priv->name = g_variant_dup_string(variant, NULL);
  g_object_notify(G_OBJECT(self), "name");

  g_variant_unref(variant);
}

static void type_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  const gchar *s;

  s = g_variant_get_string(variant, NULL);
  priv->type = barman_service_str2type(s);

  g_object_notify(G_OBJECT(self), "type");

  g_variant_unref(variant);
}

static void mode_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  const gchar *s;

  s = g_variant_get_string(variant, NULL);
  priv->security = barman_service_str2mode(s);

  g_object_notify(G_OBJECT(self), "mode");

  g_variant_unref(variant);
}

static void security_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  const gchar **strv;
  guint i;

  strv = g_variant_get_strv(variant, NULL);
  priv->security = BARMAN_SERVICE_SECURITY_UNKNOWN;

  /*
   * For now just take the first known security method from the list and
   * use that. When adding wps support, we need to provide the whole list
   * to the client.
   */
  for (i = 0; i < g_strv_length((gchar **)strv); i++) {
    priv->security = barman_service_str2security(strv[i]);

    if (priv->security != BARMAN_SERVICE_SECURITY_UNKNOWN)
      break;
  }

  g_object_notify(G_OBJECT(self), "security");

  g_free(strv);
  g_variant_unref(variant);
}

static void login_required_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->login_required = g_variant_get_boolean(variant);

  g_object_notify(G_OBJECT(self), "login-required");

  g_variant_unref(variant);
}

static void passphrase_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->passphrase);
  priv->passphrase = g_variant_dup_string(variant, NULL);

  g_object_notify(G_OBJECT(self), "passphrase");

  g_variant_unref(variant);
}

static void passphrase_required_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->passphrase_required = g_variant_get_boolean(variant);

  g_object_notify(G_OBJECT(self), "passphrase-required");

  g_variant_unref(variant);
}

static void strength_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->strength = g_variant_get_byte(variant);

  g_object_notify(G_OBJECT(self), "strength");

  g_variant_unref(variant);
}

static void favorite_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->favorite = g_variant_get_boolean(variant);

  g_object_notify(G_OBJECT(self), "favorite");

  g_variant_unref(variant);
}

static void immutable_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->immutable = g_variant_get_boolean(variant);

  g_object_notify(G_OBJECT(self), "immutable");

  g_variant_unref(variant);
}

static void autoconnect_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->autoconnect = g_variant_get_boolean(variant);

  g_object_notify(G_OBJECT(self), "autoconnect");

  g_variant_unref(variant);
}

static void setup_required_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->setup_required = g_variant_get_boolean(variant);
  g_object_notify(G_OBJECT(self), "setup-required");

  g_variant_unref(variant);
}

static void apn_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->apn);
  priv->apn = g_variant_dup_string(variant, NULL);

  g_object_notify(G_OBJECT(self), "apn");

  g_variant_unref(variant);
}

static void mcc_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->mcc);
  priv->mcc = g_variant_dup_string(variant, NULL);

  g_object_notify(G_OBJECT(self), "mcc");

  g_variant_unref(variant);
}

static void mnc_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->mnc);
  priv->mnc = g_variant_dup_string(variant, NULL);

  g_object_notify(G_OBJECT(self), "mnc");

  g_variant_unref(variant);
}

static void roaming_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  priv->roaming = g_variant_get_boolean(variant);

  g_object_notify(G_OBJECT(self), "roaming");

  g_variant_unref(variant);
}

static void nameservers_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_strfreev(priv->nameservers);
  priv->nameservers = g_variant_dup_strv(variant, NULL);

  g_object_notify(G_OBJECT(self), "nameservers");
}

static void nameservers_configuration_updated(BarmanService *self,
					      GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_strfreev(priv->nameservers_configuration);
  priv->nameservers_configuration = g_variant_dup_strv(variant, NULL);

  g_object_notify(G_OBJECT(self), "nameservers-configuration");
}

static void domains_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_strfreev(priv->domains);
  priv->domains = g_variant_dup_strv(variant, NULL);

  g_object_notify(G_OBJECT(self), "domains");
}

static void domains_configuration_updated(BarmanService *self,
					  GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_strfreev(priv->domains_configuration);
  priv->domains_configuration = g_variant_dup_strv(variant, NULL);

  g_object_notify(G_OBJECT(self), "domains-configuration");
}

static void ipv4_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  gchar *method = NULL, *address = NULL, *netmask = NULL, *gateway = NULL,
    *key;
  GError *error = NULL;
  GVariantIter iter;
  GVariant *value;

  if (priv->ipv4 != NULL) {
    g_object_unref(priv->ipv4);
    priv->ipv4 = NULL;
  }

  g_variant_iter_init(&iter, variant);

  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    if (g_strcmp0(key, "Method") == 0)
      method = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Address") == 0)
      address = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Netmask") == 0)
      netmask = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Gateway") == 0)
      gateway = g_variant_dup_string(value, NULL);
    else
      g_warning("Unknown ipv4 property: %s", key);

    g_free(key);
    g_variant_unref(value);
  }

  if (method == NULL && address == NULL &&
      netmask == NULL && gateway == NULL)
    /* some cases, like with bluetooth, ipv4 dict is just empty */
    goto out;

  priv->ipv4 = barman_ipv4_new_with_strings(method, address, netmask,
					     gateway, &error);
  if (error != NULL) {
    g_warning("Received invalid ipv4 settings: %s", error->message);
    g_error_free(error);
    return;
  }

 out:
  g_object_notify(G_OBJECT(self), "ipv4");

  g_free(method);
  g_free(address);
  g_free(netmask);
  g_free(gateway);
}

static void ipv4_configuration_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  gchar *method = NULL, *address = NULL, *netmask = NULL, *gateway = NULL,
    *key;
  GError *error = NULL;
  GVariantIter iter;
  GVariant *value;

  if (priv->ipv4_configuration != NULL) {
    g_object_unref(priv->ipv4_configuration);
    priv->ipv4_configuration = NULL;
  }

  g_variant_iter_init(&iter, variant);

  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    if (g_strcmp0(key, "Method") == 0)
      method = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Address") == 0)
      address = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Netmask") == 0)
      netmask = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Gateway") ==  0)
      gateway = g_variant_dup_string(value, NULL);
    else
      g_warning("Unknown ipv4 property: %s", key);

    g_free(key);
    g_variant_unref(value);
  }

  if (method == NULL && address == NULL &&
      netmask == NULL && gateway == NULL)
    /* some cases, like with bluetooth, ipv4 dict is just empty */
    goto out;

  priv->ipv4_configuration = barman_ipv4_new_with_strings(method,
							   address,
							   netmask,
							   gateway,
							   &error);
  if (error != NULL) {
    g_warning("Received invalid ipv4 configuration settings: %s",
	      error->message);
    g_error_free(error);
    return;
  }

 out:
  g_object_notify(G_OBJECT(self), "ipv4-configuration");

  g_free(method);
  g_free(address);
  g_free(netmask);
  g_free(gateway);
}

static void ipv6_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  gchar *method = NULL, *address = NULL, *gateway = NULL, *key;
  guchar prefix_len = 0;
  GError *error = NULL;
  GVariantIter iter;
  GVariant *value;

  if (priv->ipv6 != NULL) {
    g_object_unref(priv->ipv6);
    priv->ipv6 = NULL;
  }

  g_variant_iter_init(&iter, variant);

  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    if (g_strcmp0(key, "Method") == 0)
      method = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Address") == 0)
      address = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "PrefixLength") == 0)
      prefix_len = g_variant_get_byte(value);
    else if (g_strcmp0(key, "Gateway") == 0)
      gateway = g_variant_dup_string(value, NULL);
    else
      g_warning("Unknown ipv6 property: %s", key);

    g_free(key);
    g_variant_unref(value);
  }

  if (method == NULL && address == NULL &&
      prefix_len == 0 && gateway == NULL)
    /* some cases, like with bluetooth, ipv6 dict is just empty */
    goto out;

  priv->ipv6 = barman_ipv6_new_with_strings(method, address, prefix_len,
					     gateway, &error);
  if (error != NULL) {
    g_warning("Received invalid ipv6 settings: %s", error->message);
    g_error_free(error);
    return;
  }

 out:
  g_object_notify(G_OBJECT(self), "ipv6");

  g_free(method);
  g_free(address);
  g_free(gateway);
}

static void ipv6_configuration_updated(BarmanService *self, GVariant *variant)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  gchar *method = NULL, *address = NULL, *gateway = NULL, *key;
  guchar prefix_len = 0;
  GError *error = NULL;
  GVariantIter iter;
  GVariant *value;

  if (priv->ipv6_configuration != NULL) {
    g_object_unref(priv->ipv6_configuration);
    priv->ipv6_configuration = NULL;
  }

  g_variant_iter_init(&iter, variant);

  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    if (g_strcmp0(key, "Method") == 0)
      method = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "Address") == 0)
      address = g_variant_dup_string(value, NULL);
    else if (g_strcmp0(key, "PrefixLength") == 0)
      prefix_len = g_variant_get_byte(value);
    else if (g_strcmp0(key, "Gateway") ==  0)
      gateway = g_variant_dup_string(value, NULL);
    else
      g_warning("Unknown ipv6 property: %s", key);

    g_free(key);
    g_variant_unref(value);
  }

  if (method == NULL && address == NULL &&
      prefix_len == 0 && gateway == NULL)
    /* some cases, like with bluetooth, ipv6 dict is just empty */
    goto out;

  priv->ipv6_configuration = barman_ipv6_new_with_strings(method,
							   address,
							   prefix_len,
							   gateway,
							   &error);
  if (error != NULL) {
    g_warning("Received invalid ipv6 configuration settings: %s",
	      error->message);
    g_error_free(error);
    return;
  }

 out:
  g_object_notify(G_OBJECT(self), "ipv6-configuration");

  g_free(method);
  g_free(address);
  g_free(gateway);
}

struct property_map
{
  const gchar *property;
  void (*handler)(BarmanService *self, GVariant *variant);
};

struct property_map property_table[] = {
  { BARMAN_PROPERTY_STATE,		state_updated },
  { BARMAN_PROPERTY_ERROR,		error_updated },
  { BARMAN_PROPERTY_NAME,		name_updated },
  { BARMAN_PROPERTY_TYPE,		type_updated },
  { BARMAN_PROPERTY_MODE,		mode_updated },
  { BARMAN_PROPERTY_SECURITY,		security_updated },
  { BARMAN_PROPERTY_LOGIN_REQUIRED,	login_required_updated },
  { BARMAN_PROPERTY_PASSPHRASE,	passphrase_updated },
  { BARMAN_PROPERTY_PASSPHRASE_REQUIRED, passphrase_required_updated },
  { BARMAN_PROPERTY_STRENGTH,		strength_updated },
  { BARMAN_PROPERTY_FAVORITE,		favorite_updated },
  { BARMAN_PROPERTY_IMMUTABLE,		immutable_updated },
  { BARMAN_PROPERTY_AUTOCONNECT,	autoconnect_updated },
  { BARMAN_PROPERTY_SETUP_REQUIRED,	setup_required_updated },
  { BARMAN_PROPERTY_APN,		apn_updated },
  { BARMAN_PROPERTY_MCC,		mcc_updated },
  { BARMAN_PROPERTY_MNC,		mnc_updated },
  { BARMAN_PROPERTY_ROAMING,		roaming_updated },
  { BARMAN_PROPERTY_NAMESERVERS,	nameservers_updated },
  { BARMAN_PROPERTY_NAMESERVERS_CONFIGURATION, nameservers_configuration_updated },
  { BARMAN_PROPERTY_DOMAINS,		domains_updated },
  { BARMAN_PROPERTY_DOMAINS_CONFIGURATION, domains_configuration_updated },
  { BARMAN_PROPERTY_IPV4,		ipv4_updated },
  { BARMAN_PROPERTY_IPV4_CONFIGURATION, ipv4_configuration_updated },
  { BARMAN_PROPERTY_IPV6,		ipv6_updated },
  { BARMAN_PROPERTY_IPV6_CONFIGURATION, ipv6_configuration_updated },
};

static void property_updated(BarmanService *self, const gchar *property,
			    GVariant *variant)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS(property_table); i++) {
    struct property_map *p = &property_table[i];

    if (g_strcmp0(property, p->property) == 0) {
      p->handler(self, variant);
      /* callee owns the variant now */
      return;
    }
  }

  /* unknown property */
  g_variant_unref(variant);
}

static void property_changed(BarmanService *self, GVariant *parameters)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariant *variant;
  gchar *name;

  g_return_if_fail(priv != NULL);

  g_variant_get(parameters, "(sv)", &name, &variant);

  /* callee takes the ownership of the variant */
  property_updated(self, name, variant);

  g_free(name);
}

static void barman_service_g_signal(GDBusProxy *proxy,
				   const gchar *sender_name,
				   const gchar *signal_name,
				   GVariant *parameters,
				   gpointer user_data)
{
  BarmanService *self = BARMAN_SERVICE(user_data);

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
  BarmanService *self = BARMAN_SERVICE(user_data);
  BarmanServicePrivate *priv = GET_PRIVATE(self);
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
    property_updated(self, name, variant);

    g_free(name);
  }

  g_variant_unref(dict);
  dict = NULL;

  g_variant_unref(result);
  result = NULL;

  if (!priv->ready) {
    priv->ready = TRUE;
    g_object_notify(G_OBJECT(self), "ready");
  }
}

static void get_properties(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(self != NULL);

  g_dbus_proxy_call(priv->proxy, "GetProperties", NULL,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    get_properties_cb, self);
}

static void create_proxy_cb(GObject *source_object, GAsyncResult *res,
			    gpointer user_data)
{
  BarmanService *self = BARMAN_SERVICE(user_data);
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GDBusNodeInfo *introspection_data;
  GDBusInterfaceInfo *info;
  GError *error = NULL;

  g_return_if_fail(priv != NULL);

  priv->proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get service proxy for %s: %s",
	      priv->path, error->message);
    g_error_free(error);
    return;
  }

  if (priv->proxy == NULL) {
    g_warning("Failed to get service proxy for %s, but no errors", priv->path);
    return;
  }

  g_signal_connect(priv->proxy, "g-signal",
		   G_CALLBACK(barman_service_g_signal),
		   self);

  introspection_data = g_dbus_node_info_new_for_xml(barman_service_xml,
						    NULL);
  g_return_if_fail(introspection_data != NULL);

  info = introspection_data->interfaces[0];
  g_dbus_proxy_set_interface_info(priv->proxy, info);

  get_properties(self);
}

static void set_service_property_cb(GObject *object, GAsyncResult *res,
				    gpointer user_data)
{
  BarmanService *self = BARMAN_SERVICE(user_data);
  GDBusProxy *proxy = G_DBUS_PROXY(object);
  GError *error = NULL;

  g_dbus_proxy_call_finish(proxy, res, &error);

  if (error != NULL) {
    g_warning("BarmanService SetProperty() failed: %s", error->message);
    g_error_free(error);
  }

  /* trick to avoid destroying self during async call */
  g_object_unref(self);
}

static void set_service_property(BarmanService *self, const gchar *property,
				 GVariant *value)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariant *parameters;

  g_return_if_fail(BARMAN_IS_SERVICE(self));
  g_return_if_fail(priv != NULL);

  parameters = g_variant_new("(sv)", property, value);

  g_dbus_proxy_call(priv->proxy, "SetProperty", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    set_service_property_cb, g_object_ref(self));
}

static void update_passphrase(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariant *value;

  if (priv->passphrase == NULL)
    return;

  value = g_variant_new("s", priv->passphrase);

  set_service_property(self, "Passphrase", value);
}

static void update_autoconnect(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariant *value;

  value = g_variant_new("b", priv->autoconnect);

  set_service_property(self, "AutoConnect", value);
}

static void update_apn(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariant *value;

  if (priv->apn == NULL)
    return;

  value = g_variant_new("s", priv->apn);

  set_service_property(self, "APN", value);
}

static void update_nameservers_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariantBuilder builder;
  GVariant *value;
  guint len, i;
  gchar **p;

  if (priv->nameservers_configuration == NULL)
    /* FIXME: how to clear the nameservers? */
    return;

  len = g_strv_length(priv->nameservers_configuration);
  p = priv->nameservers_configuration;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

  for (i = 0; i < len; i++) {
    g_variant_builder_add(&builder, "s", *p);
    p++;
  }

  value = g_variant_new("as", &builder);

  set_service_property(self, "Nameservers.Configuration", value);
}

static void update_domains_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariantBuilder builder;
  GVariant *value;
  guint len, i;
  gchar **p;

  if (priv->domains_configuration == NULL)
    /* FIXME: how to clear the domains? */
    return;

  len = g_strv_length(priv->domains_configuration);
  p = priv->domains_configuration;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

  for (i = 0; i < len; i++) {
    g_variant_builder_add(&builder, "s", *p);
    p++;
  }

  value = g_variant_new("as", &builder);

  set_service_property(self, "Domains.Configuration", value);
}

static void update_ipv4_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariantBuilder builder;
  BarmanIPv4 *ipv4;
  GVariant *value;

  if (priv->ipv4_configuration == NULL)
    return;

  ipv4 = priv->ipv4_configuration;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));

  g_variant_builder_add(&builder, "{ss}", "Method",
			barman_ipv4_get_method_as_string(ipv4));
  g_variant_builder_add(&builder, "{ss}", "Address",
  			barman_ipv4_get_address(ipv4));
  g_variant_builder_add(&builder, "{ss}", "Netmask",
  			barman_ipv4_get_netmask(ipv4));
  g_variant_builder_add(&builder, "{ss}", "Gateway",
  			barman_ipv4_get_gateway(ipv4));

  value = g_variant_new("a{ss}", &builder);

  set_service_property(self, "IPv4.Configuration", value);
}

static void update_ipv6_configuration(BarmanService *self)
{
  BarmanServicePrivate *priv = GET_PRIVATE(self);
  GVariantBuilder builder;
  gchar prefix_length[10];
  BarmanIPv6 *ipv6;
  GVariant *value;

  if (priv->ipv6_configuration == NULL)
    return;

  ipv6 = priv->ipv6_configuration;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));

  g_variant_builder_add(&builder, "{ss}", "Method",
			barman_ipv6_get_method_as_string(ipv6));
  g_variant_builder_add(&builder, "{ss}", "Address",
  			barman_ipv6_get_address(ipv6));

  g_snprintf(prefix_length, sizeof(prefix_length), "%d",
	     barman_ipv6_get_prefix_length(ipv6));
  g_variant_builder_add(&builder, "{ss}", "PrefixLength", prefix_length);
  g_variant_builder_add(&builder, "{ss}", "Gateway",
  			barman_ipv6_get_gateway(ipv6));

  value = g_variant_new("a{ss}", &builder);

  set_service_property(self, "IPv6.Configuration", value);
}

static void barman_service_set_property(GObject *object,
					       guint property_id,
					       const GValue *value,
					       GParamSpec *pspec)
{
  BarmanService *self = BARMAN_SERVICE(object);
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_PATH:
    g_free(priv->path);
    priv->path = g_value_dup_string(value);
    break;
  case PROP_READY:
    priv->ready = g_value_get_boolean(value);
    break;
  case PROP_STATE:
    priv->state = g_value_get_uint(value);
    break;
  case PROP_ERROR:
    g_free(priv->error);
    priv->error = g_value_dup_string(value);
    break;
  case PROP_NAME:
    g_free(priv->name);
    priv->name = g_value_dup_string(value);
    break;
  case PROP_TYPE:
    priv->type = g_value_get_uint(value);
    break;
  case PROP_MODE:
    priv->mode = g_value_get_uint(value);
    break;
  case PROP_SECURITY:
    priv->security = g_value_get_uint(value);
    break;
  case PROP_LOGIN_REQUIRED:
    priv->login_required = g_value_get_boolean(value);
    break;
  case PROP_PASSPHRASE:
    priv->passphrase = g_value_dup_string(value);
    update_passphrase(self);
    break;
  case PROP_PASSPHRASE_REQUIRED:
    priv->passphrase_required = g_value_get_boolean(value);
    break;
  case PROP_STRENGTH:
    priv->strength = g_value_get_uchar(value);
    break;
  case PROP_FAVORITE:
    priv->favorite = g_value_get_boolean(value);
    break;
  case PROP_IMMUTABLE:
    priv->immutable = g_value_get_boolean(value);
    break;
  case PROP_AUTOCONNECT:
    priv->autoconnect = g_value_get_boolean(value);
    update_autoconnect(self);
    break;
  case PROP_SETUP_REQUIRED:
    priv->setup_required = g_value_get_boolean(value);
    break;
  case PROP_APN:
    priv->apn = g_value_dup_string(value);
    update_apn(self);
    break;
  case PROP_MCC:
    priv->mcc = g_value_dup_string(value);
    break;
  case PROP_MNC:
    priv->mnc = g_value_dup_string(value);
    break;
  case PROP_ROAMING:
    priv->roaming = g_value_get_boolean(value);
    break;
  case PROP_NAMESERVERS:
    g_strfreev(priv->nameservers);
    priv->nameservers = g_value_get_boxed(value);
    break;
  case PROP_NAMESERVERS_CONFIGURATION:
    g_strfreev(priv->nameservers_configuration);
    priv->nameservers_configuration = g_strdupv(g_value_get_boxed(value));
    update_nameservers_configuration(self);
    break;
  case PROP_DOMAINS:
    g_strfreev(priv->domains);
    priv->domains = g_value_get_boxed(value);
    break;
  case PROP_DOMAINS_CONFIGURATION:
    g_strfreev(priv->domains_configuration);
    priv->domains_configuration = g_strdupv(g_value_get_boxed(value));
    update_domains_configuration(self);
    break;
  case PROP_IPV4:
    if (priv->ipv4 != NULL)
      g_object_unref(priv->ipv4);
    priv->ipv4 = g_value_dup_object(value);
    break;
  case PROP_IPV4_CONFIGURATION:
    if (priv->ipv4_configuration != NULL)
      g_object_unref(priv->ipv4_configuration);
    priv->ipv4_configuration = g_value_dup_object(value);
    update_ipv4_configuration(self);
    break;
  case PROP_IPV6:
    if (priv->ipv6 != NULL)
      g_object_unref(priv->ipv6);
    priv->ipv6 = g_value_dup_object(value);
    break;
  case PROP_IPV6_CONFIGURATION:
    if (priv->ipv6_configuration != NULL)
      g_object_unref(priv->ipv6_configuration);
    priv->ipv6_configuration = g_value_dup_object(value);
    update_ipv6_configuration(self);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void barman_service_get_property(GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec)
{
  BarmanService *self = BARMAN_SERVICE(object);
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_PATH:
    g_value_set_string(value, priv->path);
    break;
  case PROP_READY:
    g_value_set_boolean(value, priv->ready);
    break;
  case PROP_STATE:
    g_value_set_uint(value, priv->state);
    break;
  case PROP_ERROR:
    g_value_set_string(value, priv->error);
    break;
  case PROP_NAME:
    g_value_set_string(value, priv->name);
    break;
  case PROP_TYPE:
    g_value_set_uint(value, priv->type);
    break;
  case PROP_MODE:
    g_value_set_uint(value, priv->mode);
    break;
  case PROP_SECURITY:
    g_value_set_uint(value, priv->security);
    break;
  case PROP_LOGIN_REQUIRED:
    g_value_set_boolean(value, priv->login_required);
    break;
  case PROP_PASSPHRASE:
    g_value_set_string(value, priv->passphrase);
    break;
  case PROP_PASSPHRASE_REQUIRED:
    g_value_set_boolean(value, priv->passphrase_required);
    break;
  case PROP_STRENGTH:
    g_value_set_uchar(value, priv->strength);
    break;
  case PROP_FAVORITE:
    g_value_set_boolean(value, priv->favorite);
    break;
  case PROP_IMMUTABLE:
    g_value_set_boolean(value, priv->immutable);
    break;
  case PROP_AUTOCONNECT:
    g_value_set_boolean(value, priv->autoconnect);
    break;
  case PROP_SETUP_REQUIRED:
    g_value_set_boolean(value, priv->setup_required);
    break;
  case PROP_APN:
    g_value_set_string(value, priv->apn);
    break;
  case PROP_MCC:
    g_value_set_string(value, priv->mcc);
    break;
  case PROP_MNC:
    g_value_set_string(value, priv->mnc);
    break;
  case PROP_ROAMING:
    g_value_set_boolean(value, priv->roaming);
    break;
  case PROP_NAMESERVERS:
    g_value_set_boxed(value, priv->nameservers);
    break;
  case PROP_NAMESERVERS_CONFIGURATION:
    g_value_set_boxed(value, priv->nameservers_configuration);
    break;
  case PROP_DOMAINS:
    g_value_set_boxed(value, priv->domains);
    break;
  case PROP_DOMAINS_CONFIGURATION:
    g_value_set_boxed(value, priv->domains_configuration);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void barman_service_dispose(GObject *object)
{
  BarmanService *self = BARMAN_SERVICE(object);
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  G_OBJECT_CLASS(barman_service_parent_class)->dispose(object);

  if (priv->proxy != NULL) {
    g_object_unref(priv->proxy);
    priv->proxy = NULL;
  }

  if (priv->ipv4 != NULL) {
    g_object_unref(priv->ipv4);
    priv->ipv4 = NULL;
  }

  if (priv->ipv4_configuration != NULL) {
    g_object_unref(priv->ipv4_configuration);
    priv->ipv4_configuration = NULL;
  }

  if (priv->ipv6 != NULL) {
    g_object_unref(priv->ipv6);
    priv->ipv6 = NULL;
  }

  if (priv->ipv6_configuration != NULL) {
    g_object_unref(priv->ipv6_configuration);
    priv->ipv6_configuration = NULL;
  }
}

static void barman_service_finalize(GObject *object)
{
  BarmanService *self = BARMAN_SERVICE(object);
  BarmanServicePrivate *priv = GET_PRIVATE(self);

  g_free(priv->path);
  g_free(priv->error);
  g_free(priv->name);
  g_free(priv->passphrase);
  g_free(priv->apn);
  g_free(priv->mcc);
  g_free(priv->mnc);
  g_strfreev(priv->nameservers);
  g_strfreev(priv->nameservers_configuration);
  g_strfreev(priv->domains);
  g_strfreev(priv->domains_configuration);

  G_OBJECT_CLASS(barman_service_parent_class)->finalize(object);
}

static void barman_service_class_init(BarmanServiceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec;

  g_type_class_add_private(klass, sizeof(BarmanServicePrivate));

  gobject_class->dispose = barman_service_dispose;
  gobject_class->finalize = barman_service_finalize;
  gobject_class->set_property = barman_service_set_property;
  gobject_class->get_property = barman_service_get_property;

  pspec = g_param_spec_string("path",
			      "BarmanService's DBus path",
			      "Set DBus path for the service",
			      NULL,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_PATH, pspec);

  pspec = g_param_spec_boolean("ready",
			       "BarmanService's ready property",
			       "Informs when object is ready for use",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_READY, pspec);

  pspec = g_param_spec_uint("state",
			    "BarmanService's state property",
			    "The state of the service",
			    0, G_MAXUINT,
			    BARMAN_SERVICE_STATE_IDLE,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_STATE, pspec);

  pspec = g_param_spec_string("error",
			      "BarmanService's error property",
			      "Last error code",
			      NULL,
			      G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_ERROR, pspec);

  pspec = g_param_spec_string("name",
			      "BarmanService's name property",
			      "Name of the service",
			      SERVICE_NAME_UNKNOWN,
			      G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_NAME, pspec);

  pspec = g_param_spec_uint("type",
			    "BarmanService's type property",
			    "Type of the service",
			    0, G_MAXUINT,
			    BARMAN_SERVICE_TYPE_ETHERNET,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_TYPE, pspec);

  pspec = g_param_spec_uint("mode",
			    "BarmanService's mode property",
			    "Mode of the service",
			    0, G_MAXUINT,
			    BARMAN_SERVICE_MODE_MANAGED,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_MODE, pspec);

  pspec = g_param_spec_uint("security",
			    "BarmanService's security property",
			    "Security type of the service",
			    0, G_MAXUINT,
			    BARMAN_SERVICE_SECURITY_UNKNOWN,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_SECURITY, pspec);

  pspec = g_param_spec_boolean("login-required",
			       "BarmanService's login-required property",
			       "Does network require separate login",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_LOGIN_REQUIRED, pspec);

  pspec = g_param_spec_string("passphrase",
			      "BarmanService's passphrase",
			      "Set passphrase for the service",
			      NULL,
			      G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_PASSPHRASE, pspec);

  pspec = g_param_spec_boolean("passphrase-required",
			       "BarmanService's passphrase-required property",
			       "Does network require passphrase",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_PASSPHRASE_REQUIRED,
				  pspec);

  pspec = g_param_spec_uint("strength",
			    "BarmanService's strength property",
			    "Current signal strength of the service",
			    0, G_MAXUINT8,
			    0,
			    G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_SECURITY, pspec);

  pspec = g_param_spec_boolean("favorite",
			       "BarmanService's favorite property",
			       "Is this service saved",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_FAVORITE, pspec);

  pspec = g_param_spec_boolean("immutable",
			       "BarmanService's immutable property",
			       "Is this service immutable",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_IMMUTABLE,
				  pspec);

  pspec = g_param_spec_boolean("autoconnect",
			       "BarmanService's autoconnect property",
			       "Can service connect automatically",
			       FALSE,
			       G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_AUTOCONNECT, pspec);

  pspec = g_param_spec_boolean("setup-required",
			       "BarmanService's setup-required property",
			       "Is user configuration needed, eg APN",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_SETUP_REQUIRED, pspec);

  pspec = g_param_spec_string("apn",
			      "BarmanService's APN",
			      "Set APN for the service",
			      NULL,
			      G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_APN, pspec);

  pspec = g_param_spec_string("mcc",
			      "BarmanService's Mobile Country Code",
			      "Set MCC for the service",
			      NULL,
			      G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_MCC, pspec);

  pspec = g_param_spec_string("mnc",
			      "BarmanService's Mobile Network Code",
			      "Set MNC for the service",
			      NULL,
			      G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_MNC, pspec);

  pspec = g_param_spec_boolean("roaming",
			       "BarmanService's roaming property",
			       "Is the service on a foreign network",
			       FALSE,
			       G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_ROAMING, pspec);

  pspec = g_param_spec_boxed("nameservers",
			     "BarmanService's nameservers property",
			     "Null terminated list of nameserver strings "
			     "in priority order",
			     G_TYPE_STRV,
			     G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_NAMESERVERS, pspec);

  pspec = g_param_spec_boxed("nameservers-configuration",
			     "BarmanService's nameservers-configuration property",
			     "Null terminated list of nameserver strings set by the user, "
			     "in priority order",
			     G_TYPE_STRV,
			     G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_NAMESERVERS_CONFIGURATION,
				  pspec);

  pspec = g_param_spec_boxed("domains",
			     "BarmanService's domains property",
			     "Null terminated list of strings of domains currently in use "
			     "in priority order",
			     G_TYPE_STRV,
			     G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_DOMAINS, pspec);

  pspec = g_param_spec_boxed("domains-configuration",
			     "BarmanService's domains-configuration property",
			     "Null terminated list of string of domains set by the user, "
			     "in priority order",
			     G_TYPE_STRV,
			     G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_DOMAINS_CONFIGURATION,
				  pspec);

  pspec = g_param_spec_object("ipv4",
			     "BarmanService's ipv4 property",
			     "BarmanIPv4 object",
			     BARMAN_TYPE_IPV4,
			     G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_IPV4, pspec);

  pspec = g_param_spec_object("ipv4-configuration",
			     "BarmanService's ipv4-configuration property",
			     "BarmanIPv4 object",
			     BARMAN_TYPE_IPV4,
			     G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_IPV4_CONFIGURATION,
				  pspec);

  pspec = g_param_spec_object("ipv6",
			     "BarmanService's ipv6 property",
			     "BarmanIPv6 object",
			     BARMAN_TYPE_IPV6,
			     G_PARAM_READABLE);
  g_object_class_install_property(gobject_class, PROP_IPV6, pspec);

  pspec = g_param_spec_object("ipv6-configuration",
			     "BarmanService's ipv6-configuration property",
			     "BarmanIPv6 object",
			     BARMAN_TYPE_IPV6,
			     G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class, PROP_IPV6_CONFIGURATION,
				  pspec);
}

static void barman_service_init(BarmanService *self)
{
}

BarmanService *barman_service_new(const gchar *path)
{
  BarmanService *self;

  g_return_val_if_fail(path != NULL, NULL);

  self = g_object_new(BARMAN_TYPE_SERVICE, "path", path, NULL);

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
  			   NULL, BARMAN_SERVICE_NAME, path,
  			   BARMAN_SERVICE_INTERFACE, NULL,
  			   create_proxy_cb, self);

  return self;
}
