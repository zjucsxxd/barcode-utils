/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- Network link manager
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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2008 Red Hat, Inc.
 */

#ifndef BM_DEVICE_PRIVATE_H
#define BM_DEVICE_PRIVATE_H

#include "bm-device.h"

void bm_device_set_ip_iface (BMDevice *self, const char *iface);

void bm_device_activate_schedule_stage3_ip_config_start (BMDevice *device);

void bm_device_state_changed (BMDevice *device,
                              BMDeviceState state,
                              BMDeviceStateReason reason);

gboolean bm_device_hw_bring_up (BMDevice *self, gboolean wait, gboolean *no_firmware);

void bm_device_hw_take_down (BMDevice *self, gboolean block);

void bm_device_handle_autoip4_event (BMDevice *self,
                                     const char *event,
                                     const char *address);

gboolean bm_device_ip_config_should_fail (BMDevice *self, gboolean ip6);

gboolean bm_device_get_firmware_missing (BMDevice *self);

void bm_device_set_firmware_missing (BMDevice *self, gboolean missing);

#endif	/* BM_DEVICE_PRIVATE_H */
