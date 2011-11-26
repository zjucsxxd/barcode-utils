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

#ifndef _SERVICE_MANAGER_H_
#define _SERVICE_MANAGER_H_

#include <glib-object.h>

#include "barman-service.h"

G_BEGIN_DECLS

#define TYPE_SERVICE_MANAGER service_manager_get_type()

#define SERVICE_MANAGER(obj)						\
	(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_SERVICE_MANAGER,	\
				    ServiceManager))

#define SERVICE_MANAGER_CLASS(klass)				\
	(G_TYPE_CHECK_CLASS_CAST((klass), TYPE_SERVICE_MANAGER,	\
				 ServiceManagerClass))

#define IS_SERVICE_MANAGER(obj)						\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_SERVICE_MANAGER))

#define IS_SERVICE_MANAGER_CLASS(klass)					\
	(G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_SERVICE_MANAGER))

#define SERVICE_MANAGER_GET_CLASS(obj)				\
	(G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_SERVICE_MANAGER,	\
				   ServiceManagerClass))

typedef struct {
  GObject parent;
} ServiceManager;

typedef struct {
  GObjectClass parent_class;
} ServiceManagerClass;

GType service_manager_get_type(void);

#include "manager.h"

ServiceManager *service_manager_new(Manager *ns);
void service_manager_free(ServiceManager *self);
void service_manager_update_services(ServiceManager *self,
				     BarmanService **services);
void service_manager_remove_all(ServiceManager *self);
GList *service_manager_get_wired(ServiceManager *self);
GList *service_manager_get_wireless(ServiceManager *self);
GList *service_manager_get_cellular(ServiceManager *self);
gboolean service_manager_is_connecting(ServiceManager *self);
gboolean service_manager_is_connected(ServiceManager *self);
guint service_manager_get_connected(ServiceManager *self);

G_END_DECLS

#endif
