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
 * Copyright (C) 2007 - 2008 Red Hat, Inc.
 */

#ifndef BM_OBJECT_H
#define BM_OBJECT_H

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define BM_TYPE_OBJECT            (bm_object_get_type ())
#define BM_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_OBJECT, BMObject))
#define BM_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_OBJECT, BMObjectClass))
#define BM_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_OBJECT))
#define BM_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_OBJECT))
#define BM_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_OBJECT, BMObjectClass))

#define BM_OBJECT_DBUS_CONNECTION "dbus-connection"
#define BM_OBJECT_DBUS_PATH "dbus-path"

typedef struct {
	GObject parent;
} BMObject;

typedef struct {
	GObjectClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
} BMObjectClass;

GType bm_object_get_type (void);

DBusGConnection *bm_object_get_connection (BMObject *object);
const char      *bm_object_get_path       (BMObject *object);

G_END_DECLS

#endif /* BM_OBJECT_H */
