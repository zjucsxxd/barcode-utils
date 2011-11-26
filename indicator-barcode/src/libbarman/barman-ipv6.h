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

#ifndef _BARMAN_IPV6_H_
#define _BARMAN_IPV6_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define BARMAN_TYPE_IPV6 barman_ipv6_get_type()

#define BARMAN_IPV6(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), BARMAN_TYPE_IPV6, \
			      BarmanIPv6))

#define BARMAN_IPV6_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), BARMAN_TYPE_IPV6, \
			   BarmanIPv6Class))

#define BARMAN_IS_IPV6(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), BARMAN_TYPE_IPV6))

#define BARMAN_IS_IPV6_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), BARMAN_TYPE_IPV6))

#define BARMAN_IPV6_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), BARMAN_TYPE_IPV6, \
			     BarmanIPv6Class))

typedef struct {
  GObject parent;
} BarmanIPv6;

typedef struct {
  GObjectClass parent_class;
} BarmanIPv6Class;

#define BARMAN_IPV6_ERROR barman_ipv6_error_quark()

static inline GQuark barman_ipv6_error_quark(void)
{
  return g_quark_from_static_string ("barman-ipv6-error-quark");
}

typedef enum {
  BARMAN_IPV6_ERROR_INVALID_METHOD,
  BARMAN_IPV6_ERROR_INVALID_SETTINGS,
} BarmanIPv6Error;

typedef enum {
  BARMAN_IPV6_METHOD_OFF,
  BARMAN_IPV6_METHOD_MANUAL,
  BARMAN_IPV6_METHOD_FIXED,
  BARMAN_IPV6_METHOD_AUTO,
} BarmanIPv6Method;

GType barman_ipv6_get_type(void);

BarmanIPv6Method barman_ipv6_get_method(BarmanIPv6 *self);
const gchar *barman_ipv6_get_method_as_string(BarmanIPv6 *self);
const gchar *barman_ipv6_get_address(BarmanIPv6 *self);
guchar barman_ipv6_get_prefix_length(BarmanIPv6 *self);
const gchar *barman_ipv6_get_gateway(BarmanIPv6 *self);
BarmanIPv6 *barman_ipv6_new(BarmanIPv6Method method,
			      const gchar *address,
			      guchar prefix_length,
			      const gchar *gateway,
			      GError **error);
BarmanIPv6 *barman_ipv6_new_with_strings(const gchar *method,
					   const gchar *address,
					   guchar prefix_length,
					   const gchar *gateway,
					   GError **error);

#endif
