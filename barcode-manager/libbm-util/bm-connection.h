/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * Jakob Flierl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2011 Jakob Flierl
 */

#ifndef BM_CONNECTION_H
#define BM_CONNECTION_H

#include <glib.h>
#include <glib-object.h>
#include <bm-setting.h>

G_BEGIN_DECLS

#define BM_TYPE_CONNECTION            (bm_connection_get_type ())
#define BM_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_CONNECTION, BMConnection))
#define BM_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_CONNECTION, BMConnectionClass))
#define BM_IS_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_CONNECTION))
#define BM_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_CONNECTION))
#define BM_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_CONNECTION, BMConnectionClass))

/**
 * BMConnectionScope:
 * @BM_CONNECTION_SCOPE_UNKNOWN: scope not known or not yet set
 * @BM_CONNECTION_SCOPE_SYSTEM: connection is provided by the system settings
 *   service
 * @BM_CONNECTION_SCOPE_USER: connection is provided by a user settings service
 *
 * Connection scope indicated what settings service, if any, provides the
 * connection.
 *
 **/
typedef enum {
	BM_CONNECTION_SCOPE_UNKNOWN = 0,
	BM_CONNECTION_SCOPE_SYSTEM,
	BM_CONNECTION_SCOPE_USER
} BMConnectionScope;


/**
 * BMConnectionError:
 * @BM_CONNECTION_ERROR_UNKNOWN: unknown or unclassified error
 * @BM_CONNECTION_ERROR_CONNECTION_SETTING_NOT_FOUND: the #BMConnection object
 *   did not contain the required #BMSettingConnection object, which must be
 *   present for all connections
 *
 * Describes errors that may result from operations involving a #BMConnection.
 *
 **/
typedef enum
{
	BM_CONNECTION_ERROR_UNKNOWN = 0,
	BM_CONNECTION_ERROR_CONNECTION_SETTING_NOT_FOUND
} BMConnectionError;

#define BM_TYPE_CONNECTION_ERROR (bm_connection_error_get_type ()) 
GType bm_connection_error_get_type (void);

#define BM_CONNECTION_ERROR bm_connection_error_quark ()
GQuark bm_connection_error_quark (void);

#define BM_CONNECTION_SCOPE "scope"
#define BM_CONNECTION_PATH "path"

/**
 * BMConnection:
 *
 * The BMConnection struct contains only private data.
 * It should only be accessed through the functions described below.
 */
typedef struct {
	GObject parent;
} BMConnection;

typedef struct {
	GObjectClass parent;

	/* Signals */
	void (*secrets_updated) (BMConnection *connection, const char * setting);
} BMConnectionClass;

GType bm_connection_get_type (void);

BMConnection *bm_connection_new           (void);

BMConnection *bm_connection_new_from_hash (GHashTable *hash, GError **error);

BMConnection *bm_connection_duplicate     (BMConnection *connection);

void          bm_connection_add_setting   (BMConnection *connection,
								   BMSetting    *setting);

void          bm_connection_remove_setting (BMConnection *connection,
                                            GType         setting_type);

BMSetting    *bm_connection_get_setting   (BMConnection *connection,
                                           GType         setting_type);

BMSetting    *bm_connection_get_setting_by_name (BMConnection *connection,
									    const char *name);

gboolean      bm_connection_replace_settings (BMConnection *connection,
                                              GHashTable *new_settings,
                                              GError **error);

gboolean      bm_connection_compare       (BMConnection *a,
                                           BMConnection *b,
                                           BMSettingCompareFlags flags);

gboolean      bm_connection_diff          (BMConnection *a,
                                           BMConnection *b,
                                           BMSettingCompareFlags flags,
                                           GHashTable **out_settings);

gboolean      bm_connection_verify        (BMConnection *connection, GError **error);

const char *  bm_connection_need_secrets  (BMConnection *connection,
                                           GPtrArray **hints);

void          bm_connection_clear_secrets (BMConnection *connection);

gboolean      bm_connection_update_secrets (BMConnection *connection,
                                            const char *setting_name,
                                            GHashTable *setting_secrets,
                                            GError **error);

void             bm_connection_set_scope (BMConnection *connection,
                                                 BMConnectionScope scope);

BMConnectionScope bm_connection_get_scope (BMConnection *connection);

void             bm_connection_set_path (BMConnection *connection,
                                         const char *path);

const char *     bm_connection_get_path (BMConnection *connection);

void          bm_connection_for_each_setting_value (BMConnection *connection,
										  BMSettingValueIterFn func,
										  gpointer user_data);

GHashTable   *bm_connection_to_hash       (BMConnection *connection);
void          bm_connection_dump          (BMConnection *connection);

BMSetting    *bm_connection_create_setting (const char *name);

GType bm_connection_lookup_setting_type (const char *name);

GType bm_connection_lookup_setting_type_by_quark (GQuark error_quark);

G_END_DECLS

#endif /* BM_CONNECTION_H */
