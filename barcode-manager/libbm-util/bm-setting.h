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

#ifndef BM_SETTING_H
#define BM_SETTING_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define BM_TYPE_SETTING            (bm_setting_get_type ())
#define BM_SETTING(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_SETTING, BMSetting))
#define BM_SETTING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_SETTING, BMSettingClass))
#define BM_IS_SETTING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_SETTING))
#define BM_IS_SETTING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_SETTING))
#define BM_SETTING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_SETTING, BMSettingClass))

/**
 * BMSettingError:
 * @BM_SETTING_ERROR_UNKNOWN: unknown or unclassified error
 * @BM_SETTING_ERROR_PROPERTY_NOT_FOUND: a property required by the operation
 *   was not found; for example, an attempt to update an invalid secret
 * @BM_SETTING_ERROR_PROPERTY_NOT_SECRET: an operation which requires a secret
 *   was attempted on a non-secret property
 * @BM_SETTING_ERROR_PROPERTY_TYPE_MISMATCH: the operation requires a property
 *   of a specific type, or the value couldn't be transformed to the same type
 *   as the property being acted upon
 *
 * Describes errors that may result from operations involving a #BMSetting.
 *
 **/
typedef enum
{
	BM_SETTING_ERROR_UNKNOWN = 0,
	BM_SETTING_ERROR_PROPERTY_NOT_FOUND,
	BM_SETTING_ERROR_PROPERTY_NOT_SECRET,
	BM_SETTING_ERROR_PROPERTY_TYPE_MISMATCH
} BMSettingError;

#define BM_TYPE_SETTING_ERROR (bm_setting_error_get_type ()) 
GType bm_setting_error_get_type (void);

#define BM_SETTING_ERROR bm_setting_error_quark ()
GQuark bm_setting_error_quark (void);


/* The property of the #BMSetting should be serialized */
#define BM_SETTING_PARAM_SERIALIZE    (1 << (0 + G_PARAM_USER_SHIFT))

/* The property of the #BMSetting is required for the setting to be valid */
#define BM_SETTING_PARAM_REQUIRED     (1 << (1 + G_PARAM_USER_SHIFT))

/* The property of the #BMSetting is a secret */
#define BM_SETTING_PARAM_SECRET       (1 << (2 + G_PARAM_USER_SHIFT))

/* The property of the #BMSetting should be ignored during comparisons that
 * use the %BM_SETTING_COMPARE_FLAG_FUZZY flag.
 */
#define BM_SETTING_PARAM_FUZZY_IGNORE (1 << (3 + G_PARAM_USER_SHIFT))

#define BM_SETTING_NAME "name"

/**
 * BMSetting:
 *
 * The BMSetting struct contains only private data.
 * It should only be accessed through the functions described below.
 */
typedef struct {
	GObject parent;
} BMSetting;

typedef struct {
	GObjectClass parent;

	/* Virtual functions */
	gboolean    (*verify)            (BMSetting  *setting,
	                                  GSList     *all_settings,
	                                  GError     **error);

	GPtrArray  *(*need_secrets)      (BMSetting  *setting);

	gboolean    (*update_one_secret) (BMSetting  *setting,
	                                  const char *key,
	                                  GValue     *value,
	                                  GError    **error);

	/* Padding for future expansion */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
} BMSettingClass;

typedef void (*BMSettingValueIterFn) (BMSetting *setting,
                                      const char *key,
                                      const GValue *value,
                                      GParamFlags flags,
                                      gpointer user_data);


GType bm_setting_get_type (void);

GHashTable *bm_setting_to_hash       (BMSetting *setting);
BMSetting  *bm_setting_new_from_hash (GType setting_type,
                                      GHashTable *hash);

BMSetting *bm_setting_duplicate      (BMSetting *setting);

const char *bm_setting_get_name      (BMSetting *setting);

gboolean    bm_setting_verify        (BMSetting *setting,
                                      GSList    *all_settings,
                                      GError    **error);

/**
 * BMSettingCompareFlags:
 * @BM_SETTING_COMPARE_FLAG_EXACT: match all properties exactly
 * @BM_SETTING_COMPARE_FLAG_FUZZY: match only important attributes, like SSID,
 *   type, security settings, etc.  Does not match, for example, connection ID
 *   or UUID.
 * @BM_SETTING_COMPARE_FLAG_IGNORE_ID: ignore the connection's ID
 * @BM_SETTING_COMPARE_FLAG_IGNORE_SECRETS: ignore secrets
 *
 * These flags modify the comparison behavior when comparing two settings or
 * two connections.
 *
 **/
typedef enum {
	BM_SETTING_COMPARE_FLAG_EXACT = 0x00000000,
	BM_SETTING_COMPARE_FLAG_FUZZY = 0x00000001,
	BM_SETTING_COMPARE_FLAG_IGNORE_ID = 0x00000002,
	BM_SETTING_COMPARE_FLAG_IGNORE_SECRETS = 0x00000004
} BMSettingCompareFlags;

gboolean    bm_setting_compare       (BMSetting *a,
                                      BMSetting *b,
                                      BMSettingCompareFlags flags);

/**
 * BMSettingDiffResult:
 * @BM_SETTING_DIFF_RESULT_UNKNOWN: unknown result
 * @BM_SETTING_DIFF_RESULT_IN_A: the property is present in setting A
 * @BM_SETTING_DIFF_RESULT_IN_B: the property is present in setting B
 *
 * These values indicate the result of a setting difference operation.
 **/
typedef enum {
	BM_SETTING_DIFF_RESULT_UNKNOWN = 0x00000000,
	BM_SETTING_DIFF_RESULT_IN_A =    0x00000001,
	BM_SETTING_DIFF_RESULT_IN_B =    0x00000002,
} BMSettingDiffResult;

gboolean    bm_setting_diff          (BMSetting *a,
                                      BMSetting *b,
                                      BMSettingCompareFlags flags,
                                      gboolean invert_results,
                                      GHashTable **results);

void        bm_setting_enumerate_values (BMSetting *setting,
                                         BMSettingValueIterFn func,
                                         gpointer user_data);

char       *bm_setting_to_string      (BMSetting *setting);

/* Secrets */
void        bm_setting_clear_secrets  (BMSetting *setting);
GPtrArray  *bm_setting_need_secrets   (BMSetting *setting);
gboolean    bm_setting_update_secrets (BMSetting *setting,
                                       GHashTable *secrets,
                                       GError **error);

G_END_DECLS

#endif /* BM_SETTING_H */

