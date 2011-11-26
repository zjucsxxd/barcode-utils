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
 * Copyright (C) 2005 - 2010 Red Hat, Inc.
 * Copyright (C) 2005 - 2008 Novell, Inc.
 * Copyright (C) 2005 Ray Strode
 */

#ifndef BM_NETLINK_MONITOR_H
#define BM_NETLINK_MONITOR_H

#include <glib.h>
#include <glib-object.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>

#define BM_TYPE_NETLINK_MONITOR            (bm_netlink_monitor_get_type ())
#define BM_NETLINK_MONITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_NETLINK_MONITOR, NMNetlinkMonitor))
#define BM_NETLINK_MONITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BM_TYPE_NETLINK_MONITOR, NMNetlinkMonitorClass))
#define BM_IS_NETLINK_MONITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_NETLINK_MONITOR))
#define BM_IS_NETLINK_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BM_TYPE_NETLINK_MONITOR))
#define BM_NETLINK_MONITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BM_TYPE_NETLINK_MONITOR, NMNetlinkMonitorClass))

typedef enum {
	BM_NETLINK_MONITOR_ERROR_GENERIC = 0,
	BM_NETLINK_MONITOR_ERROR_NETLINK_ALLOC_HANDLE,
	BM_NETLINK_MONITOR_ERROR_NETLINK_CONNECT,
	BM_NETLINK_MONITOR_ERROR_NETLINK_JOIN_GROUP,
	BM_NETLINK_MONITOR_ERROR_NETLINK_ALLOC_LINK_CACHE,
	BM_NETLINK_MONITOR_ERROR_PROCESSING_MESSAGE,
	BM_NETLINK_MONITOR_ERROR_BAD_ALLOC,
	BM_NETLINK_MONITOR_ERROR_WAITING_FOR_SOCKET_DATA,
	BM_NETLINK_MONITOR_ERROR_LINK_CACHE_UPDATE
} NMNetlinkMonitorError;

typedef struct {
	GObject parent; 
} NMNetlinkMonitor;

typedef struct {
	GObjectClass parent_class;

	/* Signals */
	void (*notification) (NMNetlinkMonitor *monitor, struct nl_msg *msg);
	void (*carrier_on)   (NMNetlinkMonitor *monitor, int index);
	void (*carrier_off)  (NMNetlinkMonitor *monitor, int index);
	void (*error)        (NMNetlinkMonitor *monitor, GError *error);
} NMNetlinkMonitorClass;


#define BM_NETLINK_MONITOR_ERROR      (bm_netlink_monitor_error_quark ())
GType  bm_netlink_monitor_get_type    (void) G_GNUC_CONST;
GQuark bm_netlink_monitor_error_quark (void) G_GNUC_CONST;

NMNetlinkMonitor *bm_netlink_monitor_get (void);

gboolean          bm_netlink_monitor_open_connection  (NMNetlinkMonitor *monitor,
                                                       GError **error);
void              bm_netlink_monitor_close_connection (NMNetlinkMonitor *monitor);
void              bm_netlink_monitor_attach           (NMNetlinkMonitor *monitor);
void              bm_netlink_monitor_detach           (NMNetlinkMonitor *monitor);

gboolean          bm_netlink_monitor_subscribe        (NMNetlinkMonitor *monitor,
                                                       int group,
                                                       GError **error);
void              bm_netlink_monitor_unsubscribe      (NMNetlinkMonitor *monitor,
                                                       int group);

gboolean          bm_netlink_monitor_request_ip6_info (NMNetlinkMonitor *monitor,
                                                       GError **error);

gboolean          bm_netlink_monitor_request_status   (NMNetlinkMonitor *monitor,
                                                       GError **error);
gboolean          bm_netlink_monitor_get_flags_sync   (NMNetlinkMonitor *monitor,
                                                       guint32 ifindex,
                                                       guint32 *ifflags,
                                                       GError **error);

/* Generic utility functions */
int               bm_netlink_iface_to_index     (const char *iface);
char *            bm_netlink_index_to_iface     (int idx);
struct rtnl_link *bm_netlink_index_to_rtnl_link (int idx);
struct nl_handle *bm_netlink_get_default_handle (void);

#endif  /* BM_NETLINK_MONITOR_H */
