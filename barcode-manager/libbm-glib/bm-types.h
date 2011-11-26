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
 * Copyright (C) 2008 Red Hat, Inc.
 */

#ifndef BM_TYPES_H
#define BM_TYPES_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define BM_TYPE_SSID  (bm_ssid_get_type ())
GType     bm_ssid_get_type (void) G_GNUC_CONST;

#define BM_TYPE_UINT_ARRAY  (bm_uint_array_get_type ())
GType     bm_uint_array_get_type (void) G_GNUC_CONST;

#define BM_TYPE_STRING_ARRAY  (bm_string_array_get_type ())
GType     bm_string_array_get_type (void) G_GNUC_CONST;

#define BM_TYPE_OBJECT_ARRAY  (bm_object_array_get_type ())
GType     bm_object_array_get_type (void) G_GNUC_CONST;

#define BM_TYPE_IP6_ADDRESS_OBJECT_ARRAY  (bm_ip6_address_object_array_get_type ())
GType     bm_ip6_address_object_array_get_type (void) G_GNUC_CONST;

#define BM_TYPE_IP6_ADDRESS_ARRAY  (bm_ip6_address_array_get_type ())
GType     bm_ip6_address_array_get_type (void) G_GNUC_CONST;

#define BM_TYPE_IP6_ROUTE_OBJECT_ARRAY  (bm_ip6_route_object_array_get_type ())
GType     bm_ip6_route_object_array_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* BM_TYPES_H */
