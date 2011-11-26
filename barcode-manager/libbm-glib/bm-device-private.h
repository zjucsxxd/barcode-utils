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
 * Copyright (C) 2007 - 2008 Red Hat, Inc.
 */

#ifndef BM_DEVICE_PRIVATE_H
#define BM_DEVICE_PRIVATE_H

//#include <BarcodeManager.h>

#include <dbus/dbus-glib.h>

DBusGConnection *bm_device_get_connection       (BMDevice *device);
const char      *bm_device_get_path             (BMDevice *device);
DBusGProxy      *bm_device_get_properties_proxy (BMDevice *device);

/* static methods */
BMDeviceType     bm_device_type_for_path (DBusGConnection *connection,
										  const char *path);

#endif /* BM_DEVICE_PRIVATE_H */
