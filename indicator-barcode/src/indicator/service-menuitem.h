/*
 * service_menuitem - a menuitem subclass that has the ability to do lots
 *                   of different things depending on it's settings.
 * Note: this is a clone of dbusmenu/libdbusmenu-gtk/genericmenuitem.h
 *
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * Ted Gould <ted@canonical.com>
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

#ifndef __SERVICE_MENUITEM_H__
#define __SERVICE_MENUITEM_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define SERVICE_MENUITEM_TYPE            (service_menuitem_get_type ())
#define SERVICE_MENUITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SERVICE_MENUITEM_TYPE, ServiceMenuitem))
#define SERVICE_MENUITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SERVICE_MENUITEM_TYPE, ServiceMenuitemClass))
#define IS_SERVICE_MENUITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SERVICE_MENUITEM_TYPE))
#define IS_SERVICE_MENUITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SERVICE_MENUITEM_TYPE))
#define SERVICE_MENUITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SERVICE_MENUITEM_TYPE, ServiceMenuitemClass))

typedef struct _ServiceMenuitem           ServiceMenuitem;
typedef struct _ServiceMenuitemClass      ServiceMenuitemClass;
typedef struct _ServiceMenuitemPrivate    ServiceMenuitemPrivate;
typedef enum   _ServiceMenuitemCheckType  ServiceMenuitemCheckType;
typedef enum   _ServiceMenuitemState      ServiceMenuitemState;

/**
	ServiceMenuitemClass:
	@parent_class: Our parent #GtkCheckMenuItemClass
*/
struct _ServiceMenuitemClass {
	GtkCheckMenuItemClass parent_class;
};

/**
	ServiceMenuitem:
	@parent: Our parent #GtkCheckMenuItem
*/
struct _ServiceMenuitem {
	GtkCheckMenuItem parent;
	ServiceMenuitemPrivate * priv;
};

enum _ServiceMenuitemCheckType {
	SERVICE_MENUITEM_CHECK_TYPE_NONE,
	SERVICE_MENUITEM_CHECK_TYPE_CHECKBOX,
	SERVICE_MENUITEM_CHECK_TYPE_RADIO
};

enum _ServiceMenuitemState {
	SERVICE_MENUITEM_STATE_UNCHECKED,
	SERVICE_MENUITEM_STATE_CHECKED,
	SERVICE_MENUITEM_STATE_INDETERMINATE
};

GType        service_menuitem_get_type        (void);
void         service_menuitem_set_check_type  (ServiceMenuitem *         item,
                                              ServiceMenuitemCheckType  check_type);
void         service_menuitem_set_state       (ServiceMenuitem *         item,
                                              ServiceMenuitemState      state);
void         service_menuitem_set_image       (ServiceMenuitem *         item,
                                              GtkWidget *               image);
GtkWidget *  service_menuitem_get_image       (ServiceMenuitem *         item);

void service_menuitem_set_dbusmenu(ServiceMenuitem *self,
					  DbusmenuMenuitem *item);
GtkWidget * service_menuitem_new();

G_END_DECLS

#endif
