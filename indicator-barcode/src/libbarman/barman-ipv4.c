/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Canonical Ltd.
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

#include "barman-ipv4.h"

#include <string.h>

G_DEFINE_TYPE(BarmanIPv4, barman_ipv4, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE((o), BARMAN_TYPE_IPV4, \
			       BarmanIPv4Private))

typedef struct _BarmanIPv4Private BarmanIPv4Private;

struct _BarmanIPv4Private {
  BarmanIPv4Method method;
  gchar *address;
  gchar *netmask;
  gchar *gateway;
};

enum
{
  /* reserved */
  PROP_0,

  PROP_METHOD,
  PROP_ADDRESS,
  PROP_NETMASK,
  PROP_GATEWAY,
};

struct ipv4_method_entry
{
  const gchar *str;
  BarmanIPv4Method method;
};

static const struct ipv4_method_entry method_map[] = {
  { "off",		BARMAN_IPV4_METHOD_OFF },
  { "manual",		BARMAN_IPV4_METHOD_MANUAL },
  { "fixed",		BARMAN_IPV4_METHOD_FIXED },
  { "dhcp",		BARMAN_IPV4_METHOD_DHCP },
};

static BarmanIPv4Method str2method(const gchar *state)
{
  const struct ipv4_method_entry *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(method_map); i++) {
    s = &method_map[i];
    if (g_strcmp0(s->str, state) == 0)
      return s->method;
  }

  g_warning("unknown ipv4 method: %s", state);

  return BARMAN_IPV4_METHOD_DHCP;
}

static const gchar *method2str(BarmanIPv4Method method)
{
  const struct ipv4_method_entry *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(method_map); i++) {
    s = &method_map[i];
    if (s->method == method)
      return s->str;
  }

  g_warning("%s(): unknown method %d", __func__, method);

  return "dhcp";
}

BarmanIPv4Method barman_ipv4_get_method(BarmanIPv4 *self)
{
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV4(self), BARMAN_IPV4_METHOD_OFF);
  g_return_val_if_fail(priv != NULL, BARMAN_IPV4_METHOD_OFF);

  return priv->method;
}

const gchar *barman_ipv4_get_method_as_string(BarmanIPv4 *self)
{
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV4(self), BARMAN_IPV4_METHOD_OFF);
  g_return_val_if_fail(priv != NULL, BARMAN_IPV4_METHOD_OFF);

  return method2str(priv->method);
}


const gchar *barman_ipv4_get_address(BarmanIPv4 *self)
{
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV4(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->address;
}

const gchar *barman_ipv4_get_netmask(BarmanIPv4 *self)
{
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV4(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->netmask;
}

const gchar *barman_ipv4_get_gateway(BarmanIPv4 *self)
{
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV4(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->gateway;
}

static void barman_ipv4_set_property(GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec)
{
  BarmanIPv4 *self = BARMAN_IPV4(object);
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_METHOD:
    priv->method = g_value_get_uint(value);
    break;
  case PROP_ADDRESS:
    g_free(priv->address);
    priv->address = g_value_dup_string(value);
    break;
  case PROP_NETMASK:
    g_free(priv->netmask);
    priv->netmask = g_value_dup_string(value);
    break;
  case PROP_GATEWAY:
    g_free(priv->gateway);
    priv->gateway = g_value_dup_string(value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void barman_ipv4_get_property(GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec)
{
  BarmanIPv4 *self = BARMAN_IPV4(object);
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_METHOD:
    g_value_set_uint(value, priv->method);
    break;
  case PROP_ADDRESS:
    g_value_set_string(value, priv->address);
    break;
  case PROP_NETMASK:
    g_value_set_string(value, priv->netmask);
    break;
  case PROP_GATEWAY:
    g_value_set_string(value, priv->gateway);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void barman_ipv4_dispose(GObject *object)
{
  G_OBJECT_CLASS(barman_ipv4_parent_class)->dispose(object);
}

static void barman_ipv4_finalize(GObject *object)
{
  BarmanIPv4 *self = BARMAN_IPV4(object);
  BarmanIPv4Private *priv = GET_PRIVATE(self);

  g_free(priv->address);
  g_free(priv->netmask);
  g_free(priv->gateway);

  G_OBJECT_CLASS(barman_ipv4_parent_class)->finalize(object);
}

static void barman_ipv4_class_init(BarmanIPv4Class *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec;

  g_type_class_add_private(klass, sizeof(BarmanIPv4Private));

  gobject_class->dispose = barman_ipv4_dispose;
  gobject_class->finalize = barman_ipv4_finalize;
  gobject_class->set_property = barman_ipv4_set_property;
  gobject_class->get_property = barman_ipv4_get_property;

  pspec = g_param_spec_uint("method",
			    "BarmanIPv4's method property",
			    "The ipv4 method",
			    0, G_MAXUINT,
			    BARMAN_IPV4_METHOD_OFF,
			    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_METHOD, pspec);

  pspec = g_param_spec_string("address",
			      "BarmanIPv4's address",
			      "IPv4 address",
			      NULL,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_ADDRESS, pspec);

  pspec = g_param_spec_string("netmask",
			      "BarmanIPv4's netmask",
			      "IPv4 netmask",
			      NULL,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_NETMASK, pspec);

  pspec = g_param_spec_string("gateway",
			      "BarmanIPv4's gateway",
			      "IPv4 gateway",
			      NULL,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_GATEWAY, pspec);
}

static void barman_ipv4_init(BarmanIPv4 *self)
{
}

BarmanIPv4 *barman_ipv4_new(BarmanIPv4Method method,
			      const gchar *address,
			      const gchar *netmask,
			      const gchar *gateway,
			      GError **error)
{
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);

  return g_object_new(BARMAN_TYPE_IPV4,
		      "method", method,
		      "address", address,
		      "netmask", netmask,
		      "gateway", gateway,
		      NULL);
}

BarmanIPv4 *barman_ipv4_new_with_strings(const gchar *method,
					   const gchar *address,
					   const gchar *netmask,
					   const gchar *gateway,
					   GError **error)
{
  const struct ipv4_method_entry *s;
  gboolean valid = FALSE;
  guint i;

  g_return_val_if_fail(error == NULL || *error == NULL, NULL);

  /* check that method is valid and if not, throw an error */
  for (i = 0; i < G_N_ELEMENTS(method_map); i++) {
    s = &method_map[i];
    if (g_strcmp0(s->str, method) == 0) {
      valid = TRUE;
      break;
    }
  }

  if (valid == FALSE) {
      g_set_error(error, BARMAN_IPV4_ERROR,
		  BARMAN_IPV4_ERROR_INVALID_METHOD,
		  "Invalid ipv4 method: %s", method);
      return NULL;
  }

  return barman_ipv4_new(str2method(method), address, netmask, gateway,
			  error);
}
