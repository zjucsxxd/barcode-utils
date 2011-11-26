/*
 * tech_menuitem - a menuitem subclass that has the ability to do lots
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

#ifndef __TECH_MENUITEM_H__
#define __TECH_MENUITEM_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define TECH_MENUITEM_TYPE            (tech_menuitem_get_type ())
#define TECH_MENUITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TECH_MENUITEM_TYPE, TechMenuitem))
#define TECH_MENUITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TECH_MENUITEM_TYPE, TechMenuitemClass))
#define IS_TECH_MENUITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TECH_MENUITEM_TYPE))
#define IS_TECH_MENUITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TECH_MENUITEM_TYPE))
#define TECH_MENUITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TECH_MENUITEM_TYPE, TechMenuitemClass))

typedef struct _TechMenuitem           TechMenuitem;
typedef struct _TechMenuitemClass      TechMenuitemClass;
typedef struct _TechMenuitemPrivate    TechMenuitemPrivate;
typedef enum   _TechMenuitemCheckType  TechMenuitemCheckType;
typedef enum   _TechMenuitemState      TechMenuitemState;

/**
	TechMenuitemClass:
	@parent_class: Our parent #GtkCheckMenuItemClass
*/
struct _TechMenuitemClass {
	GtkCheckMenuItemClass parent_class;
};

/**
	TechMenuitem:
	@parent: Our parent #GtkCheckMenuItem
*/
struct _TechMenuitem {
	GtkCheckMenuItem parent;
	TechMenuitemPrivate * priv;
};

enum _TechMenuitemCheckType {
	TECH_MENUITEM_CHECK_TYPE_NONE,
	TECH_MENUITEM_CHECK_TYPE_CHECKBOX,
	TECH_MENUITEM_CHECK_TYPE_RADIO
};

enum _TechMenuitemState {
	TECH_MENUITEM_STATE_UNCHECKED,
	TECH_MENUITEM_STATE_CHECKED,
	TECH_MENUITEM_STATE_INDETERMINATE
};

GType        tech_menuitem_get_type        (void);
void         tech_menuitem_set_check_type  (TechMenuitem *         item,
                                              TechMenuitemCheckType  check_type);
void         tech_menuitem_set_state       (TechMenuitem *         item,
                                              TechMenuitemState      state);
void         tech_menuitem_set_image       (TechMenuitem *         item,
                                              GtkWidget *               image);
GtkWidget *  tech_menuitem_get_image       (TechMenuitem *         item);

void tech_menuitem_set_dbusmenu(TechMenuitem *self,
					  DbusmenuMenuitem *item);
GtkWidget * tech_menuitem_new();

G_END_DECLS

#endif
