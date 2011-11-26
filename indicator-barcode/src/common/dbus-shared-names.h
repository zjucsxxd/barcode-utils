/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Jakob Flierl <jakob.flierl@gmail.com>
 *
 * Authors:
 * Jakob Flierl <jakob.flierl@gmail.com>
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

#ifndef __DBUS_SHARED_NAMES_H__
#define __DBUS_SHARED_NAMES_H__ 1

#define INDICATOR_BARCODE_DBUS_NAME  "me.koppi.indicator.barcode"
#define INDICATOR_BARCODE_DBUS_OBJECT "/me/koppi/indicator/barcode/menu"
#define INDICATOR_BARCODE_SERVICE_DBUS_OBJECT "/"
#define INDICATOR_BARCODE_SERVICE_DBUS_INTERFACE "me.koppi.indicator.barcode.BackendManager"
#define INDICATOR_BARCODE_DBUS_VERSION  0

#define INDICATOR_BARCODE_BACKEND_NAME  "me.koppi.indicator.barcode.backend"
#define INDICATOR_BARCODE_BACKEND_MANAGER_PATH "/me/koppi/indicator/barcode/backend/manager"
#define INDICATOR_BARCODE_BACKEND_INTERFACE "me.koppi.indicator.barcode.backend"
#define INDICATOR_BARCODE_BACKEND_MANAGER_INTERFACE \
  INDICATOR_BARCODE_BACKEND_INTERFACE ".Manager"

#define INDICATOR_BARCODE_AGENT_NAME  "me.koppi.indicator.barcode.agent"
#define INDICATOR_BARCODE_AGENT_OBJECT "/me/koppi/indicator/barcode/agent"
#define INDICATOR_BARCODE_AGENT_INTERFACE "me.koppi.indicator.barcode.agent"

#define INDICATOR_BARCODE_SIGNAL_ICON_CHANGED "IconChanged"
#define INDICATOR_BARCODE_SIGNAL_ACCESSIBLE_DESC_CHANGED "AccessibleDescChanged"

#define SERVICE_MENUITEM_NAME			"service-item"
#define SERVICE_MENUITEM_PROP_PROTECTION_MODE	"protection"

#define SERVICE_MENUITEM_PROP_OPEN		0
#define SERVICE_MENUITEM_PROP_PROTECTED		1

#define TECH_MENUITEM_NAME			"tech-item"

#endif /* __DBUS_SHARED_NAMES_H__ */
