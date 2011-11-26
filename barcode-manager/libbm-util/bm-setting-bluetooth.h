/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * Bastien Nocera <hadess@hadess.net>
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
 * (C) Copyright 2007 - 2009 Red Hat, Inc.
 * (C) Copyright 2007 - 2008 Novell, Inc.
 */

#ifndef BM_SETTING_BLUETOOTH_H
#define BM_SETTING_BLUETOOTH_H

#include "bm-setting.h"

G_BEGIN_DECLS

#define BM_TYPE_SETTING_BLUETOOTH            (bm_setting_bluetooth_get_type ())
#define BM_SETTING_BLUETOOTH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SETTING_BLUETOOTH, BMSettingBluetooth))
#define BM_SETTING_BLUETOOTH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_SETTING_BLUETOOTH, BMSettingBluetoothClass))
#define BM_IS_SETTING_BLUETOOTH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SETTING_BLUETOOTH))
#define BM_IS_SETTING_BLUETOOTH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_SETTING_BLUETOOTH))
#define BM_SETTING_BLUETOOTH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_SETTING_BLUETOOTH, BMSettingBluetoothClass))

#define BM_SETTING_BLUETOOTH_SETTING_NAME "bluetooth"

typedef enum
{
	BM_SETTING_BLUETOOTH_ERROR_UNKNOWN = 0,
	BM_SETTING_BLUETOOTH_ERROR_INVALID_PROPERTY,
	BM_SETTING_BLUETOOTH_ERROR_MISSING_PROPERTY,
	BM_SETTING_BLUETOOTH_ERROR_TYPE_SETTING_NOT_FOUND,
} BMSettingBluetoothError;

#define BM_TYPE_SETTING_BLUETOOTH_ERROR (bm_setting_bluetooth_error_get_type ()) 
GType bm_setting_bluetooth_error_get_type (void);

#define BM_SETTING_BLUETOOTH_ERROR bm_setting_bluetooth_error_quark ()
GQuark bm_setting_bluetooth_error_quark (void);

#define BM_SETTING_BLUETOOTH_BDADDR    "bdaddr"
#define BM_SETTING_BLUETOOTH_TYPE      "type"

#define BM_SETTING_BLUETOOTH_TYPE_DUN  "dun"
#define BM_SETTING_BLUETOOTH_TYPE_PANU "panu"

typedef struct {
	BMSetting parent;
} BMSettingBluetooth;

typedef struct {
	BMSettingClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
} BMSettingBluetoothClass;

GType bm_setting_bluetooth_get_type (void);

BMSetting *       bm_setting_bluetooth_new                 (void);
const GByteArray *bm_setting_bluetooth_get_bdaddr          (BMSettingBluetooth *setting);
const char *      bm_setting_bluetooth_get_connection_type (BMSettingBluetooth *setting);

G_END_DECLS

#endif /* BM_SETTING_BLUETOOTH_H */
