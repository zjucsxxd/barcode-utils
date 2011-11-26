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
 * Copyright (C) 2005 - 2008 Red Hat, Inc.
 * Copyright (C) 2005 - 2008 Novell, Inc.
 */

#ifndef _LIB_BM_H_
#define _LIB_BM_H_

#ifndef BM_DISABLE_DEPRECATED

#include <glib.h>

G_BEGIN_DECLS

typedef enum libbm_glib_state
{
	LIBBM_NO_DBUS = 0,
	LIBBM_NO_NETWORKMANAGER,
	LIBBM_NO_NETWORK_CONNECTION,
	LIBBM_ACTIVE_NETWORK_CONNECTION,
	LIBBM_INVALID_CONTEXT
} libbm_glib_state G_GNUC_DEPRECATED;

typedef struct libbm_glib_ctx libbm_glib_ctx G_GNUC_DEPRECATED;


typedef void (*libbm_glib_callback_func) (libbm_glib_ctx *libbm_ctx, gpointer user_data) G_GNUC_DEPRECATED;


G_GNUC_DEPRECATED libbm_glib_ctx *  libbm_glib_init                (void);
G_GNUC_DEPRECATED void              libbm_glib_shutdown            (libbm_glib_ctx *ctx);

G_GNUC_DEPRECATED libbm_glib_state  libbm_glib_get_network_state   (const libbm_glib_ctx *ctx);

G_GNUC_DEPRECATED guint             libbm_glib_register_callback   (libbm_glib_ctx *ctx, libbm_glib_callback_func func, gpointer user_data, GMainContext *g_main_ctx);
G_GNUC_DEPRECATED void              libbm_glib_unregister_callback (libbm_glib_ctx *ctx, guint id);

G_END_DECLS

#endif /* BM_DISABLE_DEPRECATED */

#endif /* _LIB_BM_H_ */
