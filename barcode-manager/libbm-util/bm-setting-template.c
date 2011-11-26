/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
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
 * (C) Copyright 2007 - 2008 Novell, Inc.
 */

/* This file is just a template - it's not built nor included in the tarball.
   It's sole purpose is to make the process of creating new settings easier.
   Just replace 'template' with new setting name (preserving the case),
   remove this comment, and you're almost done.
*/

#include "bm-setting-template.h"

G_DEFINE_TYPE (BMSettingTemplate, bm_setting_template, BM_TYPE_SETTING)

enum {
	PROP_0,

	LAST_PROP
};

BMSetting *
bm_setting_template_new (void)
{
	return (BMSetting *) g_object_new (BM_TYPE_SETTING_TEMPLATE, NULL);
}

static gboolean
verify (BMSetting *setting, GSList *all_settings)
{
	BMSettingTemplate *self = BM_SETTING_TEMPLATE (setting);
}

static void
bm_setting_template_init (BMSettingTemplate *setting)
{
	g_object_set (setting, BM_SETTING_NAME, BM_SETTING_TEMPLATE_SETTING_NAME, NULL);
}

static void
finalize (GObject *object)
{
	BMSettingTemplate *self = BM_SETTING_TEMPLATE (object);

	G_OBJECT_CLASS (bm_setting_template_parent_class)->finalize (object);
}

static void
set_property (GObject *object, guint prop_id,
		    const GValue *value, GParamSpec *pspec)
{
	BMSettingTemplate *setting = BM_SETTING_TEMPLATE (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object, guint prop_id,
		    GValue *value, GParamSpec *pspec)
{
	BMSettingTemplate *setting = BM_SETTING_TEMPLATE (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bm_setting_template_class_init (BMSettingTemplateClass *setting_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (setting_class);
	BMSettingClass *parent_class = BM_SETTING_CLASS (setting_class);

	/* virtual methods */
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->finalize     = finalize;
	parent_class->verify       = verify;

	/* Properties */
}
