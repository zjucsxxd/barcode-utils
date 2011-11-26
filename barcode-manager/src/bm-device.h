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
 * Copyright (C) 2006 - 2008 Novell, Inc.
 */

#ifndef BM_DEVICE_H
#define BM_DEVICE_H

#include <glib-object.h>
#include <dbus/dbus.h>
#include <netinet/in.h>

#include "BarcodeManager.h"
#include "bm-activation-request.h"
#include "bm-ip4-config.h"
#include "bm-ip6-config.h"
#include "bm-dhcp4-config.h"
#include "bm-dhcp6-config.h"
#include "bm-connection.h"

typedef enum NMActStageReturn
{
	BM_ACT_STAGE_RETURN_FAILURE = 0,
	BM_ACT_STAGE_RETURN_SUCCESS,
	BM_ACT_STAGE_RETURN_POSTPONE,
	BM_ACT_STAGE_RETURN_STOP         /* This activation chain is done */
} NMActStageReturn;


G_BEGIN_DECLS

#define BM_TYPE_DEVICE			(bm_device_get_type ())
#define BM_DEVICE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BM_TYPE_DEVICE, BMDevice))
#define BM_DEVICE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  BM_TYPE_DEVICE, BMDeviceClass))
#define BM_IS_DEVICE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BM_TYPE_DEVICE))
#define BM_IS_DEVICE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  BM_TYPE_DEVICE))
#define BM_DEVICE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  BM_TYPE_DEVICE, BMDeviceClass))

typedef struct {
	GObject parent;
} BMDevice;

typedef struct {
	GObjectClass parent;

	/* Hardware state, ie IFF_UP */
	gboolean        (*hw_is_up)      (BMDevice *self);
	gboolean        (*hw_bring_up)   (BMDevice *self, gboolean *no_firmware);
	void            (*hw_take_down)  (BMDevice *self);

	/* Additional stuff required to operate the device, like a 
	 * connection to the supplicant, Bluez, etc
	 */
	gboolean        (*is_up)         (BMDevice *self);
	gboolean        (*bring_up)      (BMDevice *self);
	void            (*take_down)     (BMDevice *self);

	void        (* update_hw_address) (BMDevice *self);
	void        (* update_permanent_hw_address) (BMDevice *self);
	void        (* update_initial_hw_address) (BMDevice *self);

	guint32		(* get_type_capabilities)	(BMDevice *self);
	guint32		(* get_generic_capabilities)	(BMDevice *self);

	gboolean	(* is_available) (BMDevice *self);

	BMConnection * (* get_best_auto_connection) (BMDevice *self,
	                                             GSList *connections,
	                                             char **specific_object);

	void        (* connection_secrets_updated) (BMDevice *self,
	                                            BMConnection *connection,
	                                            GSList *updated_settings,
	                                            RequestSecretsCaller caller);

	gboolean    (* check_connection_compatible) (BMDevice *self,
	                                             BMConnection *connection,
	                                             GError **error);

	NMActStageReturn	(* act_stage1_prepare)	(BMDevice *self,
	                                             BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage2_config)	(BMDevice *self,
	                                             BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage3_ip4_config_start) (BMDevice *self,
														 BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage3_ip6_config_start) (BMDevice *self,
														 BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage4_get_ip4_config)	(BMDevice *self,
														 NMIP4Config **config,
	                                                     BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage4_get_ip6_config)	(BMDevice *self,
														 NMIP6Config **config,
	                                                     BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage4_ip4_config_timeout)	(BMDevice *self,
	                                                         NMIP4Config **config,
	                                                         BMDeviceStateReason *reason);
	NMActStageReturn	(* act_stage4_ip6_config_timeout)	(BMDevice *self,
	                                                         NMIP6Config **config,
	                                                         BMDeviceStateReason *reason);
	void			(* deactivate)			(BMDevice *self);
	void			(* deactivate_quickly)	(BMDevice *self);

	gboolean		(* can_interrupt_activation)		(BMDevice *self);

	gboolean        (* spec_match_list)     (BMDevice *self, const GSList *specs);

	BMConnection *  (* connection_match_config) (BMDevice *self, const GSList *connections);
} BMDeviceClass;


GType bm_device_get_type (void);

const char *    bm_device_get_path (BMDevice *dev);
void            bm_device_set_path (BMDevice *dev, const char *path);

const char *	bm_device_get_udi		(BMDevice *dev);
const char *	bm_device_get_iface		(BMDevice *dev);
int             bm_device_get_ifindex	(BMDevice *dev);
const char *	bm_device_get_ip_iface	(BMDevice *dev);
int             bm_device_get_ip_ifindex(BMDevice *dev);
const char *	bm_device_get_driver	(BMDevice *dev);
const char *	bm_device_get_type_desc (BMDevice *dev);

BMDeviceType	bm_device_get_device_type	(BMDevice *dev);
guint32		bm_device_get_capabilities	(BMDevice *dev);
guint32		bm_device_get_type_capabilities	(BMDevice *dev);

int			bm_device_get_priority (BMDevice *dev);

guint32			bm_device_get_ip4_address	(BMDevice *dev);
void				bm_device_update_ip4_address	(BMDevice *dev);

NMDHCP4Config * bm_device_get_dhcp4_config (BMDevice *dev);
NMDHCP6Config * bm_device_get_dhcp6_config (BMDevice *dev);

NMIP4Config *	bm_device_get_ip4_config	(BMDevice *dev);
NMIP6Config *	bm_device_get_ip6_config	(BMDevice *dev);

void *		bm_device_get_system_config_data	(BMDevice *dev);

NMActRequest *	bm_device_get_act_request	(BMDevice *dev);

gboolean		bm_device_is_available (BMDevice *dev);

BMConnection * bm_device_get_best_auto_connection (BMDevice *dev,
                                                   GSList *connections,
                                                   char **specific_object);

void			bm_device_activate_schedule_stage1_device_prepare		(BMDevice *device);
void			bm_device_activate_schedule_stage2_device_config		(BMDevice *device);
void			bm_device_activate_schedule_stage4_ip4_config_get		(BMDevice *device);
void			bm_device_activate_schedule_stage4_ip4_config_timeout	(BMDevice *device);
void			bm_device_activate_schedule_stage4_ip6_config_get		(BMDevice *device);
void			bm_device_activate_schedule_stage4_ip6_config_timeout	(BMDevice *device);
gboolean		bm_device_deactivate_quickly	(BMDevice *dev);
gboolean		bm_device_is_activating		(BMDevice *dev);
gboolean		bm_device_can_interrupt_activation		(BMDevice *self);
gboolean		bm_device_autoconnect_allowed	(BMDevice *self);

BMDeviceState bm_device_get_state (BMDevice *device);

gboolean bm_device_get_managed (BMDevice *device);
void bm_device_set_managed (BMDevice *device,
                            gboolean managed,
                            BMDeviceStateReason reason);

void bm_device_set_dhcp_timeout (BMDevice *device, guint32 timeout);
void bm_device_set_dhcp_anycast_address (BMDevice *device, guint8 *addr);

void bm_device_clear_autoconnect_inhibit (BMDevice *device);

G_END_DECLS

#endif	/* BM_DEVICE_H */
