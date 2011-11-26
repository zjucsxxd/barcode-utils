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
 * Copyright (C) 2004 - 2010 Red Hat, Inc.
 * Copyright (C) 2005 - 2008 Novell, Inc.
 */

#ifndef NETWORK_MANAGER_SYSTEM_H
#define NETWORK_MANAGER_SYSTEM_H

#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>

#include <net/ethernet.h>

#include <glib.h>
#include "bm-device.h"
#include "bm-ip4-config.h"

/* Prototypes for system/distribution dependent functions,
 * implemented in the backend files in backends/ directory
 */

void			bm_system_device_flush_routes				(BMDevice *dev, int family);
void			bm_system_device_flush_routes_with_iface	(const char *iface, int family);

gboolean		bm_system_replace_default_ip4_route   (const char *iface,
                                                       guint32 gw,
                                                       guint32 mss);

gboolean		bm_system_replace_default_ip6_route   (const char *iface,
                                                       const struct in6_addr *gw);

gboolean		bm_system_replace_default_ip4_route_vpn (const char *iface,
                                                         guint32 ext_gw,
                                                         guint32 int_gw,
                                                         guint32 mss,
                                                         const char *parent_iface,
                                                         guint32 parent_mss);

struct rtnl_route *bm_system_add_ip4_vpn_gateway_route (BMDevice *parent_device, NMIP4Config *vpn_config);


void			bm_system_device_flush_addresses			(BMDevice *dev, int family);
void			bm_system_device_flush_addresses_with_iface	(const char *iface);

void			bm_system_enable_loopback				(void);
void			bm_system_update_dns					(void);

gboolean		bm_system_apply_ip4_config              (const char *iface,
                                                         NMIP4Config *config,
                                                         int priority,
                                                         NMIP4ConfigCompareFlags flags);

int             bm_system_set_ip6_route                 (int ifindex,
                                                         const struct in6_addr *ip6_dest,
                                                         guint32 ip6_prefix,
                                                         const struct in6_addr *ip6_gateway,
                                                         guint32 metric,
                                                         int mss,
                                                         int protocol,
                                                         int table,
                                                         struct rtnl_route **out_route);

gboolean		bm_system_apply_ip6_config              (const char *iface,
                                                         NMIP6Config *config,
                                                         int priority,
                                                         NMIP6ConfigCompareFlags flags);

gboolean		bm_system_device_set_up_down				(BMDevice *dev,
                                                             gboolean up,
                                                             gboolean *no_firmware);
gboolean		bm_system_device_set_up_down_with_iface		(const char *iface,
                                                             gboolean up,
                                                             gboolean *no_firmware);

gboolean        bm_system_device_is_up (BMDevice *device);
gboolean        bm_system_device_is_up_with_iface (const char *iface);

gboolean		bm_system_device_set_mtu (const char *iface, guint32 mtu);
gboolean		bm_system_device_set_mac (const char *iface, const struct ether_addr *mac);

#endif
