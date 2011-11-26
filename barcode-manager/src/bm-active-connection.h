/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- barcode scanner manager
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
 * Copyright (C) 2008 - 2010 Red Hat, Inc.
 */

#ifndef BM_ACTIVE_CONNECTION_H
#define BM_ACTIVE_CONNECTION_H

#include <glib-object.h>
#include "bm-connection.h"

#define BM_ACTIVE_CONNECTION_SERVICE_NAME "service-name"
#define BM_ACTIVE_CONNECTION_CONNECTION "connection"
#define BM_ACTIVE_CONNECTION_SPECIFIC_OBJECT "specific-object"
#define BM_ACTIVE_CONNECTION_DEVICES "devices"
#define BM_ACTIVE_CONNECTION_STATE "state"
#define BM_ACTIVE_CONNECTION_DEFAULT "default"
#define BM_ACTIVE_CONNECTION_DEFAULT6 "default6"
#define BM_ACTIVE_CONNECTION_VPN "vpn"

char *bm_active_connection_get_next_object_path (void);

void bm_active_connection_scope_to_value (BMConnection *connection, GValue *value);

void bm_active_connection_install_properties (GObjectClass *object_class,
                                              guint prop_service_name,
                                              guint prop_connection,
                                              guint prop_specific_object,
                                              guint prop_devices,
                                              guint prop_state,
                                              guint prop_default,
                                              guint prop_default6,
                                              guint prop_vpn);

#endif /* BM_ACTIVE_CONNECTION_H */
