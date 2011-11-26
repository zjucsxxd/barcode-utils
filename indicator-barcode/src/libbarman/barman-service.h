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

#ifndef _BARMAN_SERVICE_H_
#define _BARMAN_SERVICE_H_

#include "barman-ipv4.h"
#include "barman-ipv6.h"

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define BARMAN_TYPE_SERVICE barman_service_get_type()

#define BARMAN_SERVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), BARMAN_TYPE_SERVICE, \
			      BarmanService))

#define BARMAN_SERVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), BARMAN_TYPE_SERVICE, \
			   BarmanServiceClass))

#define BARMAN_IS_SERVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), BARMAN_TYPE_SERVICE))

#define BARMAN_IS_SERVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), BARMAN_TYPE_SERVICE))

#define BARMAN_SERVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), BARMAN_TYPE_SERVICE, \
			     BarmanServiceClass))

typedef struct {
  GObject parent;
} BarmanService;

typedef struct {
  GObjectClass parent_class;
} BarmanServiceClass;

typedef enum {
  BARMAN_SERVICE_STATE_IDLE,
  BARMAN_SERVICE_STATE_FAILURE,
  BARMAN_SERVICE_STATE_ASSOCIATION,
  BARMAN_SERVICE_STATE_CONFIGURATION,
  BARMAN_SERVICE_STATE_READY,
  BARMAN_SERVICE_STATE_LOGIN,
  BARMAN_SERVICE_STATE_ONLINE,
  BARMAN_SERVICE_STATE_DISCONNECT,
} BarmanServiceState;

typedef enum {
  BARMAN_SERVICE_SECURITY_UNKNOWN,
  BARMAN_SERVICE_SECURITY_NONE,
  BARMAN_SERVICE_SECURITY_WEP,
  BARMAN_SERVICE_SECURITY_PSK,
  BARMAN_SERVICE_SECURITY_IEEE8021X,
} BarmanServiceSecurity;

typedef enum {
  BARMAN_SERVICE_TYPE_ETHERNET,
  BARMAN_SERVICE_TYPE_WIFI,
  BARMAN_SERVICE_TYPE_BLUETOOTH,
  BARMAN_SERVICE_TYPE_CELLULAR,
} BarmanServiceType;

typedef enum {
  BARMAN_SERVICE_MODE_MANAGED,
  BARMAN_SERVICE_MODE_ADHOC,
  BARMAN_SERVICE_MODE_GPRS,
  BARMAN_SERVICE_MODE_EDGE,
  BARMAN_SERVICE_MODE_UMTS,
} BarmanServiceMode;

#define CONNECT_TIMEOUT 120000

GType barman_service_get_type(void);

/* property accessors */
BarmanServiceState barman_service_get_state(BarmanService *self);
const gchar *barman_service_get_error(BarmanService *self);
const gchar *barman_service_get_name(BarmanService *self);
BarmanServiceType barman_service_get_service_type(BarmanService *self);
BarmanServiceSecurity barman_service_get_security(BarmanService *self);
gboolean barman_service_get_login_required(BarmanService *self);
const gchar *barman_service_get_passphrase(BarmanService *self);
void barman_service_set_passphrase(BarmanService *self,
				    const gchar *passphrase);
gboolean barman_service_get_passphrase_required(BarmanService *self);
guint8 barman_service_get_strength(BarmanService *self);
gboolean barman_service_get_favorite(BarmanService *self);
gboolean barman_service_get_immutable(BarmanService *self);
gboolean barman_service_get_autoconnect(BarmanService *self);
void barman_service_set_autoconnect(BarmanService *self,
				     gboolean autoconnect);
gboolean barman_service_get_setup_required(BarmanService *self);
const gchar *barman_service_get_apn(BarmanService *self);
void barman_service_set_apn(BarmanService *self, const gchar *apn);
const gchar *barman_service_get_mcc(BarmanService *self);
const gchar *barman_service_get_mnc(BarmanService *self);
gboolean barman_service_get_roaming(BarmanService *self);
gchar **barman_service_get_nameservers(BarmanService *self);
gchar **barman_service_get_nameservers_configuration(BarmanService *self);
void barman_service_set_nameservers_configuration(BarmanService *self,
						   gchar **nameservers);
gchar **barman_service_get_domains(BarmanService *self);
gchar **barman_service_get_domains_configuration(BarmanService *self);
void barman_service_set_domains_configuration(BarmanService *self,
					       gchar **domains);
BarmanIPv4 *barman_service_get_ipv4(BarmanService *self);
BarmanIPv4 *barman_service_get_ipv4_configuration(BarmanService *self);
void barman_service_set_ipv4_configuration(BarmanService *self,
					    BarmanIPv4 *ipv4);
BarmanIPv6 *barman_service_get_ipv6(BarmanService *self);
BarmanIPv6 *barman_service_get_ipv6_configuration(BarmanService *self);
void barman_service_set_ipv6_configuration(BarmanService *self,
					    BarmanIPv6 *ipv6);

/* methods */
void barman_service_connect(BarmanService *self,
			     GAsyncReadyCallback callback,
			     gpointer user_data);
void barman_service_connect_finish(BarmanService *self,
				    GAsyncResult *res,
				    GError **error);

void barman_service_disconnect(BarmanService *self,
				GAsyncReadyCallback callback,
				gpointer user_data);
void barman_service_disconnect_finish(BarmanService *self,
				       GAsyncResult *res,
				       GError **error);

const gchar *barman_service_get_path(BarmanService *self);
gboolean barman_service_is_ready(BarmanService *self);
BarmanService *barman_service_new(const gchar *path);

/* helper functions */
BarmanServiceState barman_service_str2state(const gchar *state);
const gchar *barman_service_state2str(BarmanServiceState state);
BarmanServiceType barman_service_str2type(const gchar *type);
const gchar *barman_service_type2str(BarmanServiceType type);
BarmanServiceSecurity barman_service_str2security(const gchar *security);
const gchar *barman_service_security2str(BarmanServiceSecurity security);
BarmanServiceMode barman_service_str2mode(const gchar *mode);
const gchar *barman_service_mode2str(BarmanServiceMode mode);

#endif
