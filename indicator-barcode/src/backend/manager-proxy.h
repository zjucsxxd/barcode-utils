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

#ifndef _MANAGER_PROXY_H_
#define _MANAGER_PROXY_H_

#include <glib.h>
#include <glib-object.h>

#include "manager.h"

G_BEGIN_DECLS

#define MANAGER_PROXY_TYPE         (manager_proxy_get_type ())
#define MANAGER_PROXY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MANAGER_PROXY_TYPE, ManagerProxy))
#define MANAGER_PROXY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MANAGER_PROXY_TYPE, ManagerProxyClass))
#define IS_MANAGER_PROXY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MANAGER_PROXY_TYPE))
#define IS_MANAGER_PROXY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MANAGER_PROXY_TYPE))
#define MANAGER_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MANAGER_PROXY_TYPE, ManagerProxyClass))


typedef struct _ManagerProxy      ManagerProxy;
typedef struct _ManagerProxyClass ManagerProxyClass;
typedef struct _BarcodeData             BarcodeData;

struct _BarcodeData
{
  ManagerProxy *service;
};

struct _ManagerProxy
{
  GObject parent;
};

struct _ManagerProxyClass
{
  GObjectClass parent_class;
};

GType manager_proxy_get_type(void) G_GNUC_CONST;

void manager_proxy_set_icon(ManagerProxy* self, const gchar *icon_name);
void manager_proxy_set_accessible_desc(ManagerProxy* self, const gchar *accessible_desc);
void manager_proxy_get_icon(ManagerProxy* self, gchar **icon_name);
void manager_proxy_request_scan (ManagerProxy* self);
gboolean manager_proxy_set_debug(ManagerProxy* self, gint level,
				 GError **error);
ManagerProxy *manager_proxy_new(Manager *manager);

G_END_DECLS

#endif
