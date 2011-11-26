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
 * (C) Copyright 2011 Jakob Flierl
 */

#ifndef BM_ACTIVATION_REQUEST_H
#define BM_ACTIVATION_REQUEST_H

#include <glib.h>
#include <glib-object.h>
#include "bm-connection.h"
#include "bm-active-connection.h"
#include "bm-secrets-provider-interface.h"

#define BM_TYPE_ACT_REQUEST            (bm_act_request_get_type ())
#define BM_ACT_REQUEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_ACT_REQUEST, NMActRequest))
#define BM_ACT_REQUEST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_ACT_REQUEST, NMActRequestClass))
#define BM_IS_ACT_REQUEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_ACT_REQUEST))
#define BM_IS_ACT_REQUEST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BM_TYPE_ACT_REQUEST))
#define BM_ACT_REQUEST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BM_TYPE_ACT_REQUEST, NMActRequestClass))

typedef struct {
	GObject parent;
} NMActRequest;

typedef struct {
	GObjectClass parent;

	/* Signals */
	void (*secrets_updated)        (NMActRequest *req,
	                                BMConnection *connection,
	                                GSList *updated_settings,
	                                RequestSecretsCaller caller);
	void (*secrets_failed)         (NMActRequest *req,
	                                BMConnection *connection,
	                                const char *setting,
	                                RequestSecretsCaller caller);

	void (*properties_changed) (NMActRequest *req, GHashTable *properties);
} NMActRequestClass;

GType bm_act_request_get_type (void);

NMActRequest *bm_act_request_new          (BMConnection *connection,
                                           const char *specific_object,
                                           gboolean user_requested,
                                           gboolean assumed,
                                           gpointer *device);  /* An BMDevice */

BMConnection *bm_act_request_get_connection     (NMActRequest *req);
const char *  bm_act_request_get_specific_object (NMActRequest *req);

void          bm_act_request_set_specific_object (NMActRequest *req,
                                                  const char *specific_object);

gboolean      bm_act_request_get_user_requested (NMActRequest *req);

const char *  bm_act_request_get_active_connection_path (NMActRequest *req);

void          bm_act_request_set_default (NMActRequest *req, gboolean is_default);

gboolean      bm_act_request_get_default (NMActRequest *req);

void          bm_act_request_set_default6 (NMActRequest *req, gboolean is_default6);

gboolean      bm_act_request_get_default6 (NMActRequest *req);

gboolean      bm_act_request_get_shared (NMActRequest *req);

void          bm_act_request_set_shared (NMActRequest *req, gboolean shared);

void          bm_act_request_add_share_rule (NMActRequest *req,
                                             const char *table,
                                             const char *rule);

GObject *     bm_act_request_get_device (NMActRequest *req);

gboolean      bm_act_request_get_assumed (NMActRequest *req);

gboolean bm_act_request_get_secrets    (NMActRequest *req,
                                        const char *setting_name,
                                        gboolean request_new,
                                        RequestSecretsCaller caller,
                                        const char *hint1,
                                        const char *hint2);

#endif /* BM_ACTIVATION_REQUEST_H */

