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

#ifndef BMA_GCONF_CONNECTION_H
#define BMA_GCONF_CONNECTION_H

#include <gconf/gconf-client.h>
#include <dbus/dbus-glib.h>
#include <bm-connection.h>
#include <bm-exported-connection.h>
#include <bm-settings-connection-interface.h>

G_BEGIN_DECLS

#define BMA_TYPE_GCONF_CONNECTION            (bma_gconf_connection_get_type ())
#define BMA_GCONF_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BMA_TYPE_GCONF_CONNECTION, BMAGConfConnection))
#define BMA_GCONF_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BMA_TYPE_GCONF_CONNECTION, BMAGConfConnectionClass))
#define BMA_IS_GCONF_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BMA_TYPE_GCONF_CONNECTION))
#define BMA_IS_GCONF_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BMA_TYPE_GCONF_CONNECTION))
#define BMA_GCONF_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BMA_TYPE_GCONF_CONNECTION, BMAGConfConnectionClass))

#define BMA_GCONF_CONNECTION_CLIENT "client"
#define BMA_GCONF_CONNECTION_DIR    "dir"

typedef struct {
	BMExportedConnection parent;
} BMAGConfConnection;

typedef void (*BMANewSecretsRequestedFunc) (BMSettingsConnectionInterface *connection,
                                            GHashTable *settings,
                                            GError *error,
                                            gpointer user_data);

typedef struct {
	BMExportedConnectionClass parent;

	/* Signals */
	void (*new_secrets_requested)  (BMAGConfConnection *self,
	                                const char *setting_name,
	                                const char **hints,
	                                gboolean ask_user,
	                                BMANewSecretsRequestedFunc callback,
	                                gpointer callback_data);
} BMAGConfConnectionClass;

GType bma_gconf_connection_get_type (void);

BMAGConfConnection *bma_gconf_connection_new  (GConfClient *client,
                                               const char *conf_dir);

BMAGConfConnection *bma_gconf_connection_new_from_connection (GConfClient *client,
                                                              const char *conf_dir,
                                                              BMConnection *connection);

gboolean bma_gconf_connection_gconf_changed (BMAGConfConnection *self);

const char *bma_gconf_connection_get_gconf_path (BMAGConfConnection *self);

void bma_gconf_connection_update (BMAGConfConnection *self,
                                  gboolean ignore_secrets);

G_END_DECLS

#endif /* BMA_GCONF_CONNECTION_H */
