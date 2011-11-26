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

#ifndef _NETWORK_MENU_H_
#define _NETWORK_MENU_H_

#include <glib-object.h>
#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define TYPE_NETWORK_MENU network_menu_get_type()

#define NETWORK_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_NETWORK_MENU, BarcodeMenu))

#define NETWORK_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_NETWORK_MENU, BarcodeMenuClass))

#define IS_NETWORK_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_NETWORK_MENU))

#define IS_NETWORK_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_NETWORK_MENU))

#define NETWORK_MENU_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_NETWORK_MENU, BarcodeMenuClass))

typedef struct {
  DbusmenuMenuitem parent;
} BarcodeMenu;

typedef struct {
  DbusmenuMenuitemClass parent_class;
} BarcodeMenuClass;

GType network_menu_get_type(void);

#include "manager.h"

BarcodeMenu *network_menu_new(Manager *ns);
void network_menu_enable(BarcodeMenu *self);
void network_menu_disable(BarcodeMenu *self);

G_END_DECLS

#endif
