/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager system settings service
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
 * (C) Copyright 2008 Novell, Inc.
 */

#ifndef BM_SYSCONFIG_CONNECTION_H
#define BM_SYSCONFIG_CONNECTION_H

#include <bm-connection.h>
#include <bm-exported-connection.h>

G_BEGIN_DECLS

#define BM_TYPE_SYSCONFIG_CONNECTION            (bm_sysconfig_connection_get_type ())
#define BM_SYSCONFIG_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SYSCONFIG_CONNECTION, NMSysconfigConnection))
#define BM_SYSCONFIG_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_SYSCONFIG_CONNECTION, NMSysconfigConnectionClass))
#define BM_IS_SYSCONFIG_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SYSCONFIG_CONNECTION))
#define BM_IS_SYSCONFIG_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_SYSCONFIG_CONNECTION))
#define BM_SYSCONFIG_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_SYSCONFIG_CONNECTION, NMSysconfigConnectionClass))

typedef struct {
	NMExportedConnection parent;
} NMSysconfigConnection;

typedef struct {
	NMExportedConnectionClass parent;
} NMSysconfigConnectionClass;

GType bm_sysconfig_connection_get_type (void);

/* Called by a system-settings plugin to update a connection is out of sync
 * with it's backing storage.
 */
gboolean bm_sysconfig_connection_update (NMSysconfigConnection *self,
                                         BMConnection *new_settings,
                                         gboolean signal_update,
                                         GError **error);

G_END_DECLS

#endif /* BM_SYSCONFIG_CONNECTION_H */
