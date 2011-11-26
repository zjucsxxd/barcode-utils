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

#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <glib-object.h>
#include <glib.h>

#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define TYPE_SERVICE service_get_type()

#define SERVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SERVICE, Service))

#define SERVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SERVICE, ServiceClass))

#define IS_SERVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SERVICE))

#define IS_SERVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SERVICE))

#define SERVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SERVICE, ServiceClass))

typedef struct {
  DbusmenuMenuitem parent;
} Service;

typedef struct {
  DbusmenuMenuitemClass parent_class;
} ServiceClass;

#include "manager.h"

GType service_get_type(void);

Service *service_new(BarmanService *service, Manager *ns);
const gchar *service_get_path(Service *self);
BarmanServiceType service_get_service_type(Service *self);
BarmanServiceState service_get_state(Service *self);
gint service_get_strength(Service *self);
const gchar *service_get_name(Service *self);

G_END_DECLS

#endif
