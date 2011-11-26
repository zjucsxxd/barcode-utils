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
 * Copyright (C) 2010 Red Hat, Inc.
 */

#ifndef BM_MANAGER_AUTH_H
#define BM_MANAGER_AUTH_H

#include <polkit/polkit.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "bm-dbus-manager.h"

#define BM_AUTH_PERMISSION_ENABLE_DISABLE_NETWORK "org.freedesktop.BarcodeManager.enable-disable-network"
#define BM_AUTH_PERMISSION_SLEEP_WAKE             "org.freedesktop.BarcodeManager.sleep-wake"
#define BM_AUTH_PERMISSION_ENABLE_DISABLE_WIFI    "org.freedesktop.BarcodeManager.enable-disable-wifi"
#define BM_AUTH_PERMISSION_ENABLE_DISABLE_WWAN    "org.freedesktop.BarcodeManager.enable-disable-wwan"
#define BM_AUTH_PERMISSION_USE_USER_CONNECTIONS   "org.freedesktop.BarcodeManager.use-user-connections"
#define BM_AUTH_PERMISSION_NETWORK_CONTROL        "org.freedesktop.BarcodeManager.network-control"


typedef struct NMAuthChain NMAuthChain;

typedef enum {
	BM_AUTH_CALL_RESULT_UNKNOWN,
	BM_AUTH_CALL_RESULT_YES,
	BM_AUTH_CALL_RESULT_AUTH,
	BM_AUTH_CALL_RESULT_NO,
} NMAuthCallResult;

typedef void (*NMAuthChainResultFunc) (NMAuthChain *chain,
                                       GError *error,
                                       DBusGMethodInvocation *context,
                                       gpointer user_data);

typedef void (*NMAuthChainCallFunc) (NMAuthChain *chain,
                                     const char *permission,
                                     GError *error,
                                     NMAuthCallResult result,
                                     gpointer user_data);

NMAuthChain *bm_auth_chain_new (PolkitAuthority *authority,
                                DBusGMethodInvocation *context,
                                DBusGProxy *proxy,
                                NMAuthChainResultFunc done_func,
                                gpointer user_data);

NMAuthChain *bm_auth_chain_new_raw_message (PolkitAuthority *authority,
                                            DBusMessage *message,
                                            NMAuthChainResultFunc done_func,
                                            gpointer user_data);

gpointer bm_auth_chain_get_data (NMAuthChain *chain, const char *tag);

void bm_auth_chain_set_data (NMAuthChain *chain,
                             const char *tag,
                             gpointer data,
                             GDestroyNotify data_destroy);

gboolean bm_auth_chain_add_call (NMAuthChain *chain,
                                 const char *permission,
                                 gboolean allow_interaction);

void bm_auth_chain_unref (NMAuthChain *chain);

/* Utils */
gboolean bm_auth_get_caller_uid (DBusGMethodInvocation *context,
                                 NMDBusManager *dbus_mgr,
                                 gulong *out_uid,
                                 const char **out_error_desc);

gboolean bm_auth_uid_authorized (gulong uid,
                                 NMDBusManager *dbus_mgr,
                                 DBusGProxy *user_proxy,
                                 const char **out_error_desc);

#endif /* BM_MANAGER_AUTH_H */

