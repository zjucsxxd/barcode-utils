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

#include "barman-ipv6.h"

#include <string.h>

G_DEFINE_TYPE(BarmanIPv6, barman_ipv6, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE((o), BARMAN_TYPE_IPV6, \
			       BarmanIPv6Private))

typedef struct _BarmanIPv6Private BarmanIPv6Private;

struct _BarmanIPv6Private {
  BarmanIPv6Method method;
  gchar *address;
  guchar prefix_length;
  gchar *gateway;
};

enum
{
  /* reserved */
  PROP_0,

  PROP_METHOD,
  PROP_ADDRESS,
  PROP_PREFIX_LENGTH,
  PROP_GATEWAY,
};

struct ipv6_method_entry
{
  const gchar *str;
  BarmanIPv6Method method;
};

static const struct ipv6_method_entry method_map[] = {
  { "off",		BARMAN_IPV6_METHOD_OFF },
  { "manual",		BARMAN_IPV6_METHOD_MANUAL },
  { "fixed",		BARMAN_IPV6_METHOD_FIXED },
  { "auto",		BARMAN_IPV6_METHOD_AUTO },
};

static BarmanIPv6Method str2method(const gchar *state)
{
  const struct ipv6_method_entry *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(method_map); i++) {
    s = &method_map[i];
    if (g_strcmp0(s->str, state) == 0)
      return s->method;
  }

  g_warning("unknown ipv6 method: %s", state);

  return BARMAN_IPV6_METHOD_AUTO;
}

static const gchar *method2str(BarmanIPv6Method method)
{
  const struct ipv6_method_entry *s;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(method_map); i++) {
    s = &method_map[i];
    if (s->method == method)
      return s->str;
  }

  g_warning("%s(): unknown ipv6 method %d", __func__, method);

  return "auto";
}

BarmanIPv6Method barman_ipv6_get_method(BarmanIPv6 *self)
{
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV6(self), BARMAN_IPV6_METHOD_OFF);
  g_return_val_if_fail(priv != NULL, BARMAN_IPV6_METHOD_OFF);

  return priv->method;
}

const gchar *barman_ipv6_get_method_as_string(BarmanIPv6 *self)
{
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV6(self), BARMAN_IPV6_METHOD_OFF);
  g_return_val_if_fail(priv != NULL, BARMAN_IPV6_METHOD_OFF);

  return method2str(priv->method);
}


const gchar *barman_ipv6_get_address(BarmanIPv6 *self)
{
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV6(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->address;
}

guchar barman_ipv6_get_prefix_length(BarmanIPv6 *self)
{
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV6(self), 0);
  g_return_val_if_fail(priv != NULL, 0);

  return priv->prefix_length;
}

const gchar *barman_ipv6_get_gateway(BarmanIPv6 *self)
{
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_val_if_fail(BARMAN_IS_IPV6(self), NULL);
  g_return_val_if_fail(priv != NULL, NULL);

  return priv->gateway;
}

static void barman_ipv6_set_property(GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec)
{
  BarmanIPv6 *self = BARMAN_IPV6(object);
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_METHOD:
    priv->method = g_value_get_uint(value);
    break;
  case PROP_ADDRESS:
    g_free(priv->address);
    priv->address = g_value_dup_string(value);
    break;
  case PROP_PREFIX_LENGTH:
    priv->prefix_length = g_value_get_uchar(value);
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

static void barman_ipv6_get_property(GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec)
{
  BarmanIPv6 *self = BARMAN_IPV6(object);
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  switch(property_id) {
  case PROP_METHOD:
    g_value_set_uint(value, priv->method);
    break;
  case PROP_ADDRESS:
    g_value_set_string(value, priv->address);
    break;
  case PROP_PREFIX_LENGTH:
    g_value_set_uchar(value, priv->prefix_length);
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

static void barman_ipv6_dispose(GObject *object)
{
  G_OBJECT_CLASS(barman_ipv6_parent_class)->dispose(object);
}

static void barman_ipv6_finalize(GObject *object)
{
  BarmanIPv6 *self = BARMAN_IPV6(object);
  BarmanIPv6Private *priv = GET_PRIVATE(self);

  g_free(priv->address);
  g_free(priv->gateway);

  G_OBJECT_CLASS(barman_ipv6_parent_class)->finalize(object);
}

static void barman_ipv6_class_init(BarmanIPv6Class *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec;

  g_type_class_add_private(klass, sizeof(BarmanIPv6Private));

  gobject_class->dispose = barman_ipv6_dispose;
  gobject_class->finalize = barman_ipv6_finalize;
  gobject_class->set_property = barman_ipv6_set_property;
  gobject_class->get_property = barman_ipv6_get_property;

  pspec = g_param_spec_uint("method",
			    "BarmanIPv6's method property",
			    "The ipv6 method",
			    0, G_MAXUINT,
			    BARMAN_IPV6_METHOD_OFF,
			    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_METHOD, pspec);

  pspec = g_param_spec_string("address",
			      "BarmanIPv6's address",
			      "IPv6 address",
			      NULL,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_ADDRESS, pspec);

  pspec = g_param_spec_uchar("prefix-length",
			     "BarmanIPv6's prefix length",
			     "IPv6 prefix length",
			     0,
			     128,
			     0,
			     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_PREFIX_LENGTH, pspec);

  pspec = g_param_spec_string("gateway",
			      "BarmanIPv6's gateway",
			      "IPv6 gateway",
			      NULL,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property(gobject_class, PROP_GATEWAY, pspec);
}

static void barman_ipv6_init(BarmanIPv6 *self)
{
}

BarmanIPv6 *barman_ipv6_new(BarmanIPv6Method method,
			      const gchar *address,
			      guchar prefix_length,
			      const gchar *gateway,
			      GError **error)
{
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);

  return g_object_new(BARMAN_TYPE_IPV6,
		      "method", method,
		      "address", address,
		      "prefix-length", prefix_length,
		      "gateway", gateway,
		      NULL);
}

BarmanIPv6 *barman_ipv6_new_with_strings(const gchar *method,
					   const gchar *address,
					   guchar prefix_length,
					   const gchar *gateway,
					   GError **error)
{
  const struct ipv6_method_entry *s;
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
      g_set_error(error, BARMAN_IPV6_ERROR,
		  BARMAN_IPV6_ERROR_INVALID_METHOD,
		  "Invalid ipv6 method: %s", method);
      return NULL;
  }

  return barman_ipv6_new(str2method(method), address, prefix_length, gateway,
			  error);
}
