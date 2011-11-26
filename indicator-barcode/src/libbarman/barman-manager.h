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

#ifndef _BARMAN_PROXY_H_
#define _BARMAN_PROXY_H_

#include <glib-object.h>
#include <gio/gio.h>

#include "barman-service.h"

G_BEGIN_DECLS

#define BARMAN_TYPE_MANAGER barman_manager_get_type()

#define BARMAN_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), BARMAN_TYPE_MANAGER, BarmanManager))

#define BARMAN_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), BARMAN_TYPE_MANAGER, BarmanManagerClass))

#define BARMAN_IS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), BARMAN_TYPE_MANAGER))

#define BARMAN_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), BARMAN_TYPE_MANAGER))

#define BARMAN_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), BARMAN_TYPE_MANAGER, BarmanManagerClass))

typedef struct {
  GObject parent;
} BarmanManager;

typedef struct {
  GObjectClass parent_class;
} BarmanManagerClass;

typedef enum {
  BARMAN_TECHNOLOGY_TYPE_UNKNOWN,
  BARMAN_TECHNOLOGY_TYPE_ETHERNET,
  BARMAN_TECHNOLOGY_TYPE_WIFI,
  BARMAN_TECHNOLOGY_TYPE_BLUETOOTH,
  BARMAN_TECHNOLOGY_TYPE_CELLULAR,
} BarmanTechnologyType;

typedef enum {
  BARMAN_TECHNOLOGY_STATE_UNKNOWN,
  BARMAN_TECHNOLOGY_STATE_UNAVAILABLE,
  BARMAN_TECHNOLOGY_STATE_AVAILABLE,
  BARMAN_TECHNOLOGY_STATE_OFFLINE,
  BARMAN_TECHNOLOGY_STATE_ENABLED,
  BARMAN_TECHNOLOGY_STATE_CONNECTED,
} BarmanTechnologyState;

GType barman_manager_get_type(void);

void barman_manager_enable_technology(BarmanManager *self,
				       BarmanTechnologyType type,
				       GCancellable *cancellable,
				       GAsyncReadyCallback callback,
				       gpointer user_data);
void barman_manager_enable_technology_finish(BarmanManager *self,
					      GAsyncResult *res,
					      GError **error);
void barman_manager_disable_technology(BarmanManager *self,
					BarmanTechnologyType type,
					GCancellable *cancellable,
					GAsyncReadyCallback callback,
					gpointer user_data);
void barman_manager_disable_technology_finish(BarmanManager *self,
					       GAsyncResult *res,
					       GError **error);
void barman_manager_connect_service(BarmanManager *self,
				     BarmanServiceType type,
				     BarmanServiceMode mode,
				     BarmanServiceSecurity security,
				     guint8 *ssid,
				     guint ssid_len,
				     GCancellable *cancellable,
				     GAsyncReadyCallback callback,
				     gpointer user_data);
void barman_manager_connect_service_finish(BarmanManager *self,
					    GAsyncResult *res,
					    GError **error);
gboolean barman_manager_get_connected(BarmanManager *self);
BarmanService **barman_manager_get_services(BarmanManager *self);
BarmanService *barman_manager_get_service(BarmanManager *self,
					    const gchar *path);
BarmanService *barman_manager_get_default_service(BarmanManager *self);
BarmanTechnologyState barman_manager_get_wifi_state(BarmanManager *self);
BarmanTechnologyState barman_manager_get_usb_state(BarmanManager *self);
BarmanTechnologyState barman_manager_get_cellular_state(BarmanManager *self);
BarmanTechnologyState barman_manager_get_bluetooth_state(BarmanManager *self);
gboolean barman_manager_get_offline_mode(BarmanManager *self);
void barman_manager_set_offline_mode(BarmanManager *self, gboolean mode);
BarmanManager *barman_manager_new(void);

#endif
