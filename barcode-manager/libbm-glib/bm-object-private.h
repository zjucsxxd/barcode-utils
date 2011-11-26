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

#ifndef BM_OBJECT_PRIVATE_H
#define BM_OBJECT_PRIVATE_H

#include <glib.h>
#include <glib-object.h>
#include "bm-object.h"

typedef gboolean (*PropChangedMarshalFunc) (BMObject *, GParamSpec *, GValue *, gpointer);
typedef GObject * (*BMObjectCreatorFunc) (DBusGConnection *, const char *);

typedef struct {
	const char *name;
	PropChangedMarshalFunc func;
	gpointer field;
} NMPropertiesChangedInfo;


void             _bm_object_handle_properties_changed (BMObject *object,
                                                      DBusGProxy *proxy,
                                                      const NMPropertiesChangedInfo *info);

gboolean _bm_object_demarshal_generic (BMObject *object, GParamSpec *pspec, GValue *value, gpointer field);

void _bm_object_queue_notify (BMObject *object, const char *property);

/* DBus property accessors */

gboolean _bm_object_get_property (BMObject *object,
								 const char *interface,
								 const char *prop_name,
								 GValue *value);

void _bm_object_set_property (BMObject *object,
							 const char *interface,
							 const char *prop_name,
							 GValue *value);

char *_bm_object_get_string_property (BMObject *object,
									 const char *interface,
									 const char *prop_name);

char *_bm_object_get_object_path_property (BMObject *object,
										  const char *interface,
										  const char *prop_name);

gint32 _bm_object_get_int_property (BMObject *object,
								   const char *interface,
								   const char *prop_name);

guint32 _bm_object_get_uint_property (BMObject *object,
									 const char *interface,
									 const char *prop_name);

gboolean _bm_object_get_boolean_property (BMObject *object,
										const char *interface,
										const char *prop_name);

gint8 _bm_object_get_byte_property (BMObject *object,
								   const char *interface,
								   const char *prop_name);

gdouble _bm_object_get_double_property (BMObject *object,
									   const char *interface,
									   const char *prop_name);

GByteArray *_bm_object_get_byte_array_property (BMObject *object,
											   const char *interface,
											   const char *prop_name);

static inline const GPtrArray *
handle_ptr_array_return (GPtrArray *array)
{
	/* zero-length is special-case; return NULL */
	if (!array || !array->len)
		return NULL;
	return array;
}

#endif /* BM_OBJECT_PRIVATE_H */
