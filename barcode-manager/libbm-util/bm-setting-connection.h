/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * Dan Williams <dcbw@redhat.com>
 * Tambet Ingo <tambet@gmail.com>
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
 * (C) Copyright 2007 - 2008 Red Hat, Inc.
 * (C) Copyright 2007 - 2008 Novell, Inc.
 */

#ifndef BM_SETTING_CONNECTION_H
#define BM_SETTING_CONNECTION_H

#include "bm-setting.h"

G_BEGIN_DECLS

#define BM_TYPE_SETTING_CONNECTION            (bm_setting_connection_get_type ())
#define BM_SETTING_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SETTING_CONNECTION, BMSettingConnection))
#define BM_SETTING_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_SETTING_CONNECTION, BMSettingConnectionClass))
#define BM_IS_SETTING_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SETTING_CONNECTION))
#define BM_IS_SETTING_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_SETTING_CONNECTION))
#define BM_SETTING_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_SETTING_CONNECTION, BMSettingConnectionClass))

#define BM_SETTING_CONNECTION_SETTING_NAME "connection"

/**
 * BMSettingConnectionError:
 * @BM_SETTING_CONNECTION_ERROR_UNKNOWN: unknown or unclassified error
 * @BM_SETTING_CONNECTION_ERROR_INVALID_PROPERTY: the property's value is
 *   invalid
 * @BM_SETTING_CONNECTION_ERROR_MISSING_PROPERTY: a required property is not
 *   present
 * @BM_SETTING_CONNECTION_ERROR_TYPE_SETTING_NOT_FOUND: the #BMSetting object
 *   referenced by the setting name contained in the
 *   #BMSettingConnection:type property was not present in the #BMConnection
 *
 * Describes errors that may result from operations involving a
 * #BMSettingConnection.
 *
 **/
typedef enum
{
	BM_SETTING_CONNECTION_ERROR_UNKNOWN = 0,
	BM_SETTING_CONNECTION_ERROR_INVALID_PROPERTY,
	BM_SETTING_CONNECTION_ERROR_MISSING_PROPERTY,
	BM_SETTING_CONNECTION_ERROR_TYPE_SETTING_NOT_FOUND
} BMSettingConnectionError;

#define BM_TYPE_SETTING_CONNECTION_ERROR (bm_setting_connection_error_get_type ()) 
GType bm_setting_connection_error_get_type (void);

#define BM_SETTING_CONNECTION_ERROR bm_setting_connection_error_quark ()
GQuark bm_setting_connection_error_quark (void);

#define BM_SETTING_CONNECTION_ID          "id"
#define BM_SETTING_CONNECTION_UUID        "uuid"
#define BM_SETTING_CONNECTION_TYPE        "type"
#define BM_SETTING_CONNECTION_AUTOCONNECT "autoconnect"
#define BM_SETTING_CONNECTION_TIMESTAMP   "timestamp"
#define BM_SETTING_CONNECTION_READ_ONLY   "read-only"

/**
 * BMSettingConnection:
 *
 * The BMSettingConnection struct contains only private data.
 * It should only be accessed through the functions described below.
 */
typedef struct {
	BMSetting parent;
} BMSettingConnection;

typedef struct {
	BMSettingClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
} BMSettingConnectionClass;

GType bm_setting_connection_get_type (void);

BMSetting * bm_setting_connection_new                 (void);
const char *bm_setting_connection_get_id              (BMSettingConnection *setting);
const char *bm_setting_connection_get_uuid            (BMSettingConnection *setting);
const char *bm_setting_connection_get_connection_type (BMSettingConnection *setting);
gboolean    bm_setting_connection_get_autoconnect     (BMSettingConnection *setting);
guint64     bm_setting_connection_get_timestamp       (BMSettingConnection *setting);
gboolean    bm_setting_connection_get_read_only       (BMSettingConnection *setting);

G_END_DECLS

#endif /* BM_SETTING_CONNECTION_H */
