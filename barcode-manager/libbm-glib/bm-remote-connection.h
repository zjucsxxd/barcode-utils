/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access network status & information from glib applications
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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2009 Red Hat, Inc.
 */

#ifndef __BM_REMOTE_CONNECTION_H__
#define __BM_REMOTE_CONNECTION_H__

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <bm-connection.h>

G_BEGIN_DECLS

#define BM_TYPE_REMOTE_CONNECTION            (bm_remote_connection_get_type ())
#define BM_REMOTE_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_REMOTE_CONNECTION, NMRemoteConnection))
#define BM_REMOTE_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_REMOTE_CONNECTION, NMRemoteConnectionClass))
#define BM_IS_REMOTE_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_REMOTE_CONNECTION))
#define BM_IS_REMOTE_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_REMOTE_CONNECTION))
#define BM_REMOTE_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_REMOTE_CONNECTION, NMRemoteConnectionClass))

typedef struct {
	BMConnection parent;
} NMRemoteConnection;

typedef struct {
	BMConnectionClass parent_class;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} NMRemoteConnectionClass;

GType bm_remote_connection_get_type (void);

NMRemoteConnection *bm_remote_connection_new (DBusGConnection *bus,
                                              BMConnectionScope scope,
                                              const char *path);
G_END_DECLS

#endif  /* __BM_REMOTE_CONNECTION__ */

