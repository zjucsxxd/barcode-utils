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

#ifndef BM_SETTING_SERIAL_H
#define BM_SETTING_SERIAL_H

#include <bm-setting.h>

G_BEGIN_DECLS

#define BM_TYPE_SETTING_SERIAL            (bm_setting_serial_get_type ())
#define BM_SETTING_SERIAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SETTING_SERIAL, BMSettingSerial))
#define BM_SETTING_SERIAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_SETTING_SERIAL, BMSettingSerialClass))
#define BM_IS_SETTING_SERIAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SETTING_SERIAL))
#define BM_IS_SETTING_SERIAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_SETTING_SERIAL))
#define BM_SETTING_SERIAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_SETTING_SERIAL, BMSettingSerialClass))

#define BM_SETTING_SERIAL_SETTING_NAME "serial"

typedef enum
{
	BM_SETTING_SERIAL_ERROR_UNKNOWN = 0,
	BM_SETTING_SERIAL_ERROR_INVALID_PROPERTY,
	BM_SETTING_SERIAL_ERROR_MISSING_PROPERTY,
	BM_SETTING_SERIAL_ERROR_MISSING_PPP_SETTING
} BMSettingSerialError;

#define BM_TYPE_SETTING_SERIAL_ERROR (bm_setting_serial_error_get_type ()) 
GType bm_setting_serial_error_get_type (void);

#define BM_SETTING_SERIAL_ERROR bm_setting_serial_error_quark ()
GQuark bm_setting_serial_error_quark (void);

#define BM_SETTING_SERIAL_BAUD "baud"
#define BM_SETTING_SERIAL_BITS "bits"
#define BM_SETTING_SERIAL_PARITY "parity"
#define BM_SETTING_SERIAL_STOPBITS "stopbits"
#define BM_SETTING_SERIAL_SEND_DELAY "send-delay"

typedef struct {
	BMSetting parent;
} BMSettingSerial;

typedef struct {
	BMSettingClass parent;

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
} BMSettingSerialClass;

GType bm_setting_serial_get_type (void);

BMSetting *bm_setting_serial_new            (void);
guint      bm_setting_serial_get_baud       (BMSettingSerial *setting);
guint      bm_setting_serial_get_bits       (BMSettingSerial *setting);
char       bm_setting_serial_get_parity     (BMSettingSerial *setting);
guint      bm_setting_serial_get_stopbits   (BMSettingSerial *setting);
guint64    bm_setting_serial_get_send_delay (BMSettingSerial *setting);

G_END_DECLS

#endif /* BM_SETTING_SERIAL_H */
