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
 * Copyright (C) 2011 Jakob Flierl
 */

#ifndef BM_MANAGER_AUTH_H
#define BM_MANAGER_AUTH_H

#include <polkit/polkit.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "bm-dbus-manager.h"

#define BM_AUTH_PERMISSION_ENABLE_DISABLE_NETWORK "org.freedesktop.NetworkManager.enable-disable-network"
#define BM_AUTH_PERMISSION_SLEEP_WAKE             "org.freedesktop.NetworkManager.sleep-wake"
#define BM_AUTH_PERMISSION_ENABLE_DISABLE_WIFI    "org.freedesktop.NetworkManager.enable-disable-wifi"
#define BM_AUTH_PERMISSION_ENABLE_DISABLE_WWAN    "org.freedesktop.NetworkManager.enable-disable-wwan"
#define BM_AUTH_PERMISSION_USE_USER_CONNECTIONS   "org.freedesktop.NetworkManager.use-user-connections"
#define BM_AUTH_PERMISSION_NETWORK_CONTROL        "org.freedesktop.NetworkManager.network-control"


typedef struct BMAuthChain BMAuthChain;

typedef enum {
	BM_AUTH_CALL_RESULT_UNKNOWN,
	BM_AUTH_CALL_RESULT_YES,
	BM_AUTH_CALL_RESULT_AUTH,
	BM_AUTH_CALL_RESULT_NO,
} BMAuthCallResult;

typedef void (*BMAuthChainResultFunc) (BMAuthChain *chain,
                                       GError *error,
                                       DBusGMethodInvocation *context,
                                       gpointer user_data);

typedef void (*BMAuthChainCallFunc) (BMAuthChain *chain,
                                     const char *permission,
                                     GError *error,
                                     BMAuthCallResult result,
                                     gpointer user_data);

BMAuthChain *bm_auth_chain_new (PolkitAuthority *authority,
                                DBusGMethodInvocation *context,
                                DBusGProxy *proxy,
                                BMAuthChainResultFunc done_func,
                                gpointer user_data);

BMAuthChain *bm_auth_chain_new_raw_message (PolkitAuthority *authority,
                                            DBusMessage *message,
                                            BMAuthChainResultFunc done_func,
                                            gpointer user_data);

gpointer bm_auth_chain_get_data (BMAuthChain *chain, const char *tag);

void bm_auth_chain_set_data (BMAuthChain *chain,
                             const char *tag,
                             gpointer data,
                             GDestroyNotify data_destroy);

gboolean bm_auth_chain_add_call (BMAuthChain *chain,
                                 const char *permission,
                                 gboolean allow_interaction);

void bm_auth_chain_unref (BMAuthChain *chain);

/* Utils */
gboolean bm_auth_get_caller_uid (DBusGMethodInvocation *context,
                                 BMDBusManager *dbus_mgr,
                                 gulong *out_uid,
                                 const char **out_error_desc);

gboolean bm_auth_uid_authorized (gulong uid,
                                 BMDBusManager *dbus_mgr,
                                 DBusGProxy *user_proxy,
                                 const char **out_error_desc);

#endif /* BM_MANAGER_AUTH_H */

