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

#ifndef _BARMAN_IPV4_H_
#define _BARMAN_IPV4_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define BARMAN_TYPE_IPV4 barman_ipv4_get_type()

#define BARMAN_IPV4(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), BARMAN_TYPE_IPV4, \
			      BarmanIPv4))

#define BARMAN_IPV4_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), BARMAN_TYPE_IPV4, \
			   BarmanIPv4Class))

#define BARMAN_IS_IPV4(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), BARMAN_TYPE_IPV4))

#define BARMAN_IS_IPV4_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), BARMAN_TYPE_IPV4))

#define BARMAN_IPV4_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), BARMAN_TYPE_IPV4, \
			     BarmanIPv4Class))

typedef struct {
  GObject parent;
} BarmanIPv4;

typedef struct {
  GObjectClass parent_class;
} BarmanIPv4Class;

#define BARMAN_IPV4_ERROR barman_ipv4_error_quark()

static inline GQuark barman_ipv4_error_quark(void)
{
  return g_quark_from_static_string ("barman-ipv4-error-quark");
}

typedef enum {
  BARMAN_IPV4_ERROR_INVALID_METHOD,
  BARMAN_IPV4_ERROR_INVALID_SETTINGS,
} BarmanIPv4Error;

typedef enum {
  BARMAN_IPV4_METHOD_OFF,
  BARMAN_IPV4_METHOD_MANUAL,
  BARMAN_IPV4_METHOD_FIXED,
  BARMAN_IPV4_METHOD_DHCP,
} BarmanIPv4Method;

GType barman_ipv4_get_type(void);

BarmanIPv4Method barman_ipv4_get_method(BarmanIPv4 *self);
const gchar *barman_ipv4_get_method_as_string(BarmanIPv4 *self);
const gchar *barman_ipv4_get_address(BarmanIPv4 *self);
const gchar *barman_ipv4_get_netmask(BarmanIPv4 *self);
const gchar *barman_ipv4_get_gateway(BarmanIPv4 *self);
BarmanIPv4 *barman_ipv4_new(BarmanIPv4Method method,
			      const gchar *address,
			      const gchar *netmask,
			      const gchar *gateway,
			      GError **error);
BarmanIPv4 *barman_ipv4_new_with_strings(const gchar *method,
					   const gchar *address,
					   const gchar *netmask,
					   const gchar *gateway,
					   GError **error);

#endif
