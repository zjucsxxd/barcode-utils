/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Applet -- Display barcode scanners and allow user control
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2011 Jakob Flierl
 */

#ifndef BMA_GCONF_SETTINGS_H
#define BMA_GCONF_SETTINGS_H

#include <bm-connection.h>
#include <bm-settings-service.h>

#include "bma-gconf-connection.h"

G_BEGIN_DECLS

#define BMA_TYPE_GCONF_SETTINGS            (bma_gconf_settings_get_type ())
#define BMA_GCONF_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BMA_TYPE_GCONF_SETTINGS, BMAGConfSettings))
#define BMA_GCONF_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BMA_TYPE_GCONF_SETTINGS, BMAGConfSettingsClass))
#define BMA_IS_GCONF_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BMA_TYPE_GCONF_SETTINGS))
#define BMA_IS_GCONF_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BMA_TYPE_GCONF_SETTINGS))
#define BMA_GCONF_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BMA_TYPE_GCONF_SETTINGS, BMAGConfSettingsClass))

typedef struct {
	BMSettingsService parent;
} BMAGConfSettings;

typedef struct {
	BMSettingsServiceClass parent;

} BMAGConfSettingsClass;

GType bma_gconf_settings_get_type (void);

BMAGConfSettings *bma_gconf_settings_new (DBusGConnection *bus);

BMAGConfConnection *bma_gconf_settings_add_connection (BMAGConfSettings *self,
                                                       BMConnection *connection);

G_END_DECLS

#endif /* BMA_GCONF_SETTINGS_H */
