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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 */

#include <config.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>

#include "bm-policy.h"
#include "BarcodeManagerUtils.h"
#include "bm-wifi-ap.h"
#include "bm-activation-request.h"
#include "bm-logging.h"
#include "bm-device-interface.h"
#include "bm-device.h"
#include "bm-device-wifi.h"
#include "bm-device-ethernet.h"
#include "bm-device-modem.h"
#include "bm-dbus-manager.h"
#include "bm-setting-ip4-config.h"
#include "bm-setting-connection.h"
#include "bm-system.h"
#include "bm-dns-manager.h"
#include "bm-vpn-manager.h"
#include "bm-policy-hostname.h"

struct NMPolicy {
	NMManager *manager;
	guint update_state_id;
	GSList *pending_activation_checks;
	GSList *signal_ids;
	GSList *dev_signal_ids;

	NMVPNManager *vpn_manager;
	gulong vpn_activated_id;
	gulong vpn_deactivated_id;

	BMDevice *default_device4;
	BMDevice *default_device6;

	HostnameThread *lookup;

	char *orig_hostname; /* hostname at NM start time */
	char *cur_hostname;  /* hostname we want to assign */
};

#define INVALID_TAG "invalid"

static const char *
get_connection_id (BMConnection *connection)
{
	BMSettingConnection *s_con;

	g_return_val_if_fail (BM_IS_CONNECTION (connection), NULL);

	s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);
	g_return_val_if_fail (s_con != NULL, NULL);

	return bm_setting_connection_get_id (s_con);
}

static BMDevice *
get_best_ip4_device (NMManager *manager, NMActRequest **out_req)
{
	GSList *devices, *iter;
	BMDevice *best = NULL;
	int best_prio = G_MAXINT;

	g_return_val_if_fail (manager != NULL, NULL);
	g_return_val_if_fail (BM_IS_MANAGER (manager), NULL);
	g_return_val_if_fail (out_req != NULL, NULL);
	g_return_val_if_fail (*out_req == NULL, NULL);

	devices = bm_manager_get_devices (manager);
	for (iter = devices; iter; iter = g_slist_next (iter)) {
		BMDevice *dev = BM_DEVICE (iter->data);
		NMActRequest *req;
		BMConnection *connection;
		NMIP4Config *ip4_config;
		BMSettingIP4Config *s_ip4;
		int prio;
		guint i;
		gboolean can_default = FALSE;
		const char *method = NULL;

		if (bm_device_get_state (dev) != BM_DEVICE_STATE_ACTIVATED)
			continue;

		ip4_config = bm_device_get_ip4_config (dev);
		if (!ip4_config)
			continue;

		req = bm_device_get_act_request (dev);
		g_assert (req);
		connection = bm_act_request_get_connection (req);
		g_assert (connection);

		/* Never set the default route through an IPv4LL-addressed device */
		s_ip4 = (BMSettingIP4Config *) bm_connection_get_setting (connection, BM_TYPE_SETTING_IP4_CONFIG);
		if (s_ip4)
			method = bm_setting_ip4_config_get_method (s_ip4);

		if (s_ip4 && !strcmp (method, BM_SETTING_IP4_CONFIG_METHOD_LINK_LOCAL))
			continue;

		/* Make sure at least one of this device's IP addresses has a gateway */
		for (i = 0; i < bm_ip4_config_get_num_addresses (ip4_config); i++) {
			NMIP4Address *addr;

			addr = bm_ip4_config_get_address (ip4_config, i);
			if (bm_ip4_address_get_gateway (addr)) {
				can_default = TRUE;
				break;
			}
		}

		if (!can_default && !BM_IS_DEVICE_MODEM (dev))
			continue;

		/* 'never-default' devices can't ever be the default */
		if (   (s_ip4 && bm_setting_ip4_config_get_never_default (s_ip4))
		    || bm_ip4_config_get_never_default (ip4_config))
			continue;

		prio = bm_device_get_priority (dev);
		if (prio > 0 && prio < best_prio) {
			best = dev;
			best_prio = prio;
			*out_req = req;
		}
	}

	return best;
}

static BMDevice *
get_best_ip6_device (NMManager *manager, NMActRequest **out_req)
{
	GSList *devices, *iter;
	BMDevice *best = NULL;
	int best_prio = G_MAXINT;

	g_return_val_if_fail (manager != NULL, NULL);
	g_return_val_if_fail (BM_IS_MANAGER (manager), NULL);
	g_return_val_if_fail (out_req != NULL, NULL);
	g_return_val_if_fail (*out_req == NULL, NULL);

	devices = bm_manager_get_devices (manager);
	for (iter = devices; iter; iter = g_slist_next (iter)) {
		BMDevice *dev = BM_DEVICE (iter->data);
		NMActRequest *req;
		BMConnection *connection;
		NMIP6Config *ip6_config;
		BMSettingIP6Config *s_ip6;
		int prio;
		guint i;
		gboolean can_default = FALSE;
		const char *method = NULL;

		if (bm_device_get_state (dev) != BM_DEVICE_STATE_ACTIVATED)
			continue;

		ip6_config = bm_device_get_ip6_config (dev);
		if (!ip6_config)
			continue;

		req = bm_device_get_act_request (dev);
		g_assert (req);
		connection = bm_act_request_get_connection (req);
		g_assert (connection);

		/* Never set the default route through an IPv4LL-addressed device */
		s_ip6 = (BMSettingIP6Config *) bm_connection_get_setting (connection, BM_TYPE_SETTING_IP6_CONFIG);
		if (s_ip6)
			method = bm_setting_ip6_config_get_method (s_ip6);

		if (method && !strcmp (method, BM_SETTING_IP6_CONFIG_METHOD_LINK_LOCAL))
			continue;

		/* Make sure at least one of this device's IP addresses has a gateway */
		for (i = 0; i < bm_ip6_config_get_num_addresses (ip6_config); i++) {
			NMIP6Address *addr;

			addr = bm_ip6_config_get_address (ip6_config, i);
			if (bm_ip6_address_get_gateway (addr)) {
				can_default = TRUE;
				break;
			}
		}

		if (!can_default && !BM_IS_DEVICE_MODEM (dev))
			continue;

		/* 'never-default' devices can't ever be the default */
		if (s_ip6 && bm_setting_ip6_config_get_never_default (s_ip6))
			continue;

		prio = bm_device_get_priority (dev);
		if (prio > 0 && prio < best_prio) {
			best = dev;
			best_prio = prio;
			*out_req = req;
		}
	}

	return best;
}

static void
_set_hostname (NMPolicy *policy,
               gboolean change_hostname,
               const char *new_hostname,
               const char *msg)
{
	if (change_hostname) {
		NMDnsManager *dns_mgr;

		g_free (policy->cur_hostname);
		policy->cur_hostname = g_strdup (new_hostname);

		dns_mgr = bm_dns_manager_get (NULL);
		bm_dns_manager_set_hostname (dns_mgr, policy->cur_hostname);
		g_object_unref (dns_mgr);
	}

	if (bm_policy_set_system_hostname (policy->cur_hostname, msg))
		bm_utils_call_dispatcher ("hostname", NULL, NULL, NULL);
}

static void
lookup_callback (HostnameThread *thread,
                 int result,
                 const char *hostname,
                 gpointer user_data)
{
	NMPolicy *policy = (NMPolicy *) user_data;
	char *msg;

	/* Update the hostname if the calling lookup thread is the in-progress one */
	if (!hostname_thread_is_dead (thread) && (thread == policy->lookup)) {
		policy->lookup = NULL;
		if (!hostname) {
			/* Fall back to localhost.localdomain */
			msg = g_strdup_printf ("address lookup failed: %d", result);
			_set_hostname (policy, TRUE, NULL, msg);
			g_free (msg);
		} else
			_set_hostname (policy, TRUE, hostname, "from address lookup");
	}
	hostname_thread_free (thread);
}

static void
update_system_hostname (NMPolicy *policy, BMDevice *best4, BMDevice *best6)
{
	char *configured_hostname = NULL;
	NMActRequest *best_req4 = NULL;
	NMActRequest *best_req6 = NULL;
	const char *dhcp_hostname, *p;

	g_return_if_fail (policy != NULL);

	if (policy->lookup) {
		hostname_thread_kill (policy->lookup);
		policy->lookup = NULL;
	}

	/* Hostname precedence order:
	 *
	 * 1) a configured hostname (from system-settings)
	 * 2) automatic hostname from the default device's config (DHCP, VPN, etc)
	 * 3) the original hostname when NM started
	 * 4) reverse-DNS of the best device's IPv4 address
	 *
	 */

	/* Try a persistent hostname first */
	g_object_get (G_OBJECT (policy->manager), BM_MANAGER_HOSTNAME, &configured_hostname, NULL);
	if (configured_hostname) {
		_set_hostname (policy, TRUE, configured_hostname, "from system configuration");
		g_free (configured_hostname);
		return;
	}

	/* Try automatically determined hostname from the best device's IP config */
	if (!best4)
		best4 = get_best_ip4_device (policy->manager, &best_req4);
	if (!best6)
		best6 = get_best_ip6_device (policy->manager, &best_req6);

	if (!best4 && !best6) {
		/* No best device; fall back to original hostname or if there wasn't
		 * one, 'localhost.localdomain'
		 */
		_set_hostname (policy, TRUE, policy->orig_hostname, "no default device");
		return;
	}

	if (best4) {
		NMDHCP4Config *dhcp4_config;

		/* Grab a hostname out of the device's DHCP4 config */
		dhcp4_config = bm_device_get_dhcp4_config (best4);
		if (dhcp4_config) {
			p = dhcp_hostname = bm_dhcp4_config_get_option (dhcp4_config, "host_name");
			if (dhcp_hostname && strlen (dhcp_hostname)) {
				/* Sanity check; strip leading spaces */
				while (*p) {
					if (!isblank (*p++)) {
						_set_hostname (policy, TRUE, dhcp_hostname, "from DHCPv4");
						return;
					}
				}
				bm_log_warn (LOGD_DNS, "DHCPv4-provided hostname '%s' looks invalid; ignoring it",
					         dhcp_hostname);
			}
		}
	} else if (best6) {
		NMDHCP6Config *dhcp6_config;

		/* Grab a hostname out of the device's DHCP4 config */
		dhcp6_config = bm_device_get_dhcp6_config (best6);
		if (dhcp6_config) {
			p = dhcp_hostname = bm_dhcp6_config_get_option (dhcp6_config, "host_name");
			if (dhcp_hostname && strlen (dhcp_hostname)) {
				/* Sanity check; strip leading spaces */
				while (*p) {
					if (!isblank (*p++)) {
						_set_hostname (policy, TRUE, dhcp_hostname, "from DHCPv6");
						return;
					}
				}
				bm_log_warn (LOGD_DNS, "DHCPv6-provided hostname '%s' looks invalid; ignoring it",
					         dhcp_hostname);
			}
		}
	}

	/* If no automatically-configured hostname, try using the hostname from
	 * when NM started up.
	 */
	if (policy->orig_hostname) {
		_set_hostname (policy, TRUE, policy->orig_hostname, "from system startup");
		return;
	}

	/* No configured hostname, no automatically determined hostname, and no
	 * bootup hostname. Start reverse DNS of the current IPv4 or IPv6 address.
	 */
	if (best4) {
		NMIP4Config *ip4_config;
		NMIP4Address *addr4;

		ip4_config = bm_device_get_ip4_config (best4);
		if (   !ip4_config
		    || (bm_ip4_config_get_num_nameservers (ip4_config) == 0)
		    || (bm_ip4_config_get_num_addresses (ip4_config) == 0)) {
			/* No valid IP4 config (!!); fall back to localhost.localdomain */
			_set_hostname (policy, TRUE, NULL, "no IPv4 config");
			return;
		}

		addr4 = bm_ip4_config_get_address (ip4_config, 0);
		g_assert (addr4); /* checked for > 1 address above */

		/* Start the hostname lookup thread */
		policy->lookup = hostname4_thread_new (bm_ip4_address_get_address (addr4), lookup_callback, policy);
	} else if (best6) {
		NMIP6Config *ip6_config;
		NMIP6Address *addr6;

		ip6_config = bm_device_get_ip6_config (best6);
		if (   !ip6_config
		    || (bm_ip6_config_get_num_nameservers (ip6_config) == 0)
		    || (bm_ip6_config_get_num_addresses (ip6_config) == 0)) {
			/* No valid IP6 config (!!); fall back to localhost.localdomain */
			_set_hostname (policy, TRUE, NULL, "no IPv6 config");
			return;
		}

		addr6 = bm_ip6_config_get_address (ip6_config, 0);
		g_assert (addr6); /* checked for > 1 address above */

		/* Start the hostname lookup thread */
		policy->lookup = hostname6_thread_new (bm_ip6_address_get_address (addr6), lookup_callback, policy);
	}

	if (!policy->lookup) {
		/* Fall back to 'localhost.localdomain' */
		_set_hostname (policy, TRUE, NULL, "error starting hostname thread");
	}
}

static void
update_ip4_routing_and_dns (NMPolicy *policy, gboolean force_update)
{
	NMDnsIPConfigType dns_type = BM_DNS_IP_CONFIG_TYPE_BEST_DEVICE;
	BMDevice *best = NULL;
	NMActRequest *best_req = NULL;
	NMDnsManager *dns_mgr;
	GSList *devices = NULL, *iter, *vpns;
	NMIP4Config *ip4_config = NULL;
	NMIP4Address *addr;
	const char *ip_iface = NULL;
	BMConnection *connection = NULL;
	BMSettingConnection *s_con = NULL;
	const char *connection_id;

	best = get_best_ip4_device (policy->manager, &best_req);
	if (!best)
		goto out;
	if (!force_update && (best == policy->default_device4))
		goto out;

	/* If a VPN connection is active, it is preferred */
	vpns = bm_vpn_manager_get_active_connections (policy->vpn_manager);
	for (iter = vpns; iter; iter = g_slist_next (iter)) {
		NMVPNConnection *candidate = BM_VPN_CONNECTION (iter->data);
		BMConnection *vpn_connection;
		BMSettingIP4Config *s_ip4;
		gboolean can_default = TRUE;
		NMVPNConnectionState vpn_state;

		/* If it's marked 'never-default', don't make it default */
		vpn_connection = bm_vpn_connection_get_connection (candidate);
		g_assert (vpn_connection);

		/* Check the active IP4 config from the VPN service daemon */
		ip4_config = bm_vpn_connection_get_ip4_config (candidate);
		if (ip4_config && bm_ip4_config_get_never_default (ip4_config))
			can_default = FALSE;

		/* Check the user's preference from the BMConnection */
		s_ip4 = (BMSettingIP4Config *) bm_connection_get_setting (vpn_connection, BM_TYPE_SETTING_IP4_CONFIG);
		if (s_ip4 && bm_setting_ip4_config_get_never_default (s_ip4))
			can_default = FALSE;

		vpn_state = bm_vpn_connection_get_vpn_state (candidate);
		if (can_default && (vpn_state == BM_VPN_CONNECTION_STATE_ACTIVATED)) {
			NMIP4Config *parent_ip4;
			BMDevice *parent;

			ip_iface = bm_vpn_connection_get_ip_iface (candidate);
			connection = bm_vpn_connection_get_connection (candidate);
			addr = bm_ip4_config_get_address (ip4_config, 0);

			parent = bm_vpn_connection_get_parent_device (candidate);
			parent_ip4 = bm_device_get_ip4_config (parent);

			bm_system_replace_default_ip4_route_vpn (ip_iface,
			                                         bm_ip4_address_get_gateway (addr),
			                                         bm_vpn_connection_get_ip4_internal_gateway (candidate),
			                                         bm_ip4_config_get_mss (ip4_config),
			                                         bm_device_get_ip_iface (parent),
			                                         bm_ip4_config_get_mss (parent_ip4));

			dns_type = BM_DNS_IP_CONFIG_TYPE_VPN;
		}
		g_object_unref (candidate);
	}
	g_slist_free (vpns);

	/* The best device gets the default route if a VPN connection didn't */
	if (!ip_iface || !ip4_config) {
		connection = bm_act_request_get_connection (best_req);
		ip_iface = bm_device_get_ip_iface (best);
		ip4_config = bm_device_get_ip4_config (best);
		g_assert (ip4_config);
		addr = bm_ip4_config_get_address (ip4_config, 0);

		bm_system_replace_default_ip4_route (ip_iface, bm_ip4_address_get_gateway (addr), bm_ip4_config_get_mss (ip4_config));

		dns_type = BM_DNS_IP_CONFIG_TYPE_BEST_DEVICE;
	}

	if (!ip_iface || !ip4_config) {
		bm_log_warn (LOGD_CORE, "couldn't determine IP interface (%p) or IPv4 config (%p)!",
		             ip_iface, ip4_config);
		goto out;
	}

	/* Update the default active connection.  Only mark the new default
	 * active connection after setting default = FALSE on all other connections
	 * first.  The order is important, we don't want two connections marked
	 * default at the same time ever.
	 */
	devices = bm_manager_get_devices (policy->manager);
	for (iter = devices; iter; iter = g_slist_next (iter)) {
		BMDevice *dev = BM_DEVICE (iter->data);
		NMActRequest *req;

		req = bm_device_get_act_request (dev);
		if (req && (req != best_req))
			bm_act_request_set_default (req, FALSE);
	}

	dns_mgr = bm_dns_manager_get (NULL);
	bm_dns_manager_add_ip4_config (dns_mgr, ip_iface, ip4_config, dns_type);
	g_object_unref (dns_mgr);

	/* Now set new default active connection _after_ updating DNS info, so that
	 * if the connection is shared dnsmasq picks up the right stuff.
	 */
	if (best_req)
		bm_act_request_set_default (best_req, TRUE);

	if (connection)
		s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);

	connection_id = s_con ? bm_setting_connection_get_id (s_con) : NULL;
	if (connection_id) {
		bm_log_info (LOGD_CORE, "Policy set '%s' (%s) as default for IPv4 routing and DNS.", connection_id, ip_iface);
	} else {
		bm_log_info (LOGD_CORE, "Policy set (%s) as default for IPv4 routing and DNS.", ip_iface);
	}

out:
	policy->default_device4 = best;
}

static void
update_ip6_routing_and_dns (NMPolicy *policy, gboolean force_update)
{
	NMDnsIPConfigType dns_type = BM_DNS_IP_CONFIG_TYPE_BEST_DEVICE;
	BMDevice *best = NULL;
	NMActRequest *best_req = NULL;
	NMDnsManager *dns_mgr;
	GSList *devices = NULL, *iter;
#if NOT_YET
	GSList *vpns;
#endif
	NMIP6Config *ip6_config = NULL;
	NMIP6Address *addr;
	const char *ip_iface = NULL;
	BMConnection *connection = NULL;
	BMSettingConnection *s_con = NULL;
	const char *connection_id;

	best = get_best_ip6_device (policy->manager, &best_req);
	if (!best)
		goto out;
	if (!force_update && (best == policy->default_device6))
		goto out;

#if NOT_YET
	/* If a VPN connection is active, it is preferred */
	vpns = bm_vpn_manager_get_active_connections (policy->vpn_manager);
	for (iter = vpns; iter; iter = g_slist_next (iter)) {
		NMVPNConnection *candidate = BM_VPN_CONNECTION (iter->data);
		BMConnection *vpn_connection;
		BMSettingIP6Config *s_ip6;
		gboolean can_default = TRUE;
		NMVPNConnectionState vpn_state;

		/* If it's marked 'never-default', don't make it default */
		vpn_connection = bm_vpn_connection_get_connection (candidate);
		g_assert (vpn_connection);
		s_ip6 = (BMSettingIP6Config *) bm_connection_get_setting (vpn_connection, BM_TYPE_SETTING_IP6_CONFIG);
		if (s_ip6 && bm_setting_ip6_config_get_never_default (s_ip6))
			can_default = FALSE;

		vpn_state = bm_vpn_connection_get_vpn_state (candidate);
		if (can_default && (vpn_state == BM_VPN_CONNECTION_STATE_ACTIVATED)) {
			NMIP6Config *parent_ip6;
			BMDevice *parent;

			ip_iface = bm_vpn_connection_get_ip_iface (candidate);
			connection = bm_vpn_connection_get_connection (candidate);
			ip6_config = bm_vpn_connection_get_ip6_config (candidate);
			addr = bm_ip6_config_get_address (ip6_config, 0);

			parent = bm_vpn_connection_get_parent_device (candidate);
			parent_ip6 = bm_device_get_ip6_config (parent);

			bm_system_replace_default_ip6_route_vpn (ip_iface,
			                                         bm_ip6_address_get_gateway (addr),
			                                         bm_vpn_connection_get_ip4_internal_gateway (candidate),
			                                         bm_ip6_config_get_mss (ip4_config),
			                                         bm_device_get_ip_iface (parent),
			                                         bm_ip6_config_get_mss (parent_ip4));

			dns_type = BM_DNS_IP_CONFIG_TYPE_VPN;
		}
		g_object_unref (candidate);
	}
	g_slist_free (vpns);
#endif

	/* The best device gets the default route if a VPN connection didn't */
	if (!ip_iface || !ip6_config) {
		connection = bm_act_request_get_connection (best_req);
		ip_iface = bm_device_get_ip_iface (best);
		ip6_config = bm_device_get_ip6_config (best);
		g_assert (ip6_config);
		addr = bm_ip6_config_get_address (ip6_config, 0);

		bm_system_replace_default_ip6_route (ip_iface, bm_ip6_address_get_gateway (addr));

		dns_type = BM_DNS_IP_CONFIG_TYPE_BEST_DEVICE;
	}

	if (!ip_iface || !ip6_config) {
		bm_log_warn (LOGD_CORE, "couldn't determine IP interface (%p) or IPv6 config (%p)!",
		             ip_iface, ip6_config);
		goto out;
	}

	/* Update the default active connection.  Only mark the new default
	 * active connection after setting default = FALSE on all other connections
	 * first.  The order is important, we don't want two connections marked
	 * default at the same time ever.
	 */
	devices = bm_manager_get_devices (policy->manager);
	for (iter = devices; iter; iter = g_slist_next (iter)) {
		BMDevice *dev = BM_DEVICE (iter->data);
		NMActRequest *req;

		req = bm_device_get_act_request (dev);
		if (req && (req != best_req))
			bm_act_request_set_default6 (req, FALSE);
	}

	dns_mgr = bm_dns_manager_get (NULL);
	bm_dns_manager_add_ip6_config (dns_mgr, ip_iface, ip6_config, dns_type);
	g_object_unref (dns_mgr);

	/* Now set new default active connection _after_ updating DNS info, so that
	 * if the connection is shared dnsmasq picks up the right stuff.
	 */
	if (best_req)
		bm_act_request_set_default6 (best_req, TRUE);

	if (connection)
		s_con = (BMSettingConnection *) bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION);

	connection_id = s_con ? bm_setting_connection_get_id (s_con) : NULL;
	if (connection_id) {
		bm_log_info (LOGD_CORE, "Policy set '%s' (%s) as default for IPv6 routing and DNS.", connection_id, ip_iface);
	} else {
		bm_log_info (LOGD_CORE, "Policy set (%s) as default for IPv6 routing and DNS.", ip_iface);
	}

out:
	policy->default_device6 = best;
}

static void
update_routing_and_dns (NMPolicy *policy, gboolean force_update)
{
	update_ip4_routing_and_dns (policy, force_update);
	update_ip6_routing_and_dns (policy, force_update);

	/* Update the system hostname */
	update_system_hostname (policy, policy->default_device4, policy->default_device6);
}

typedef struct {
	NMPolicy *policy;
	BMDevice *device;
	guint id;
} ActivateData;

static gboolean
auto_activate_device (gpointer user_data)
{
	ActivateData *data = (ActivateData *) user_data;
	NMPolicy *policy;
	BMConnection *best_connection;
	char *specific_object = NULL;
	GSList *connections, *iter;

	g_assert (data);
	policy = data->policy;

	// FIXME: if a device is already activating (or activated) with a connection
	// but another connection now overrides the current one for that device,
	// deactivate the device and activate the new connection instead of just
	// bailing if the device is already active
	if (bm_device_get_act_request (data->device))
		goto out;

	/* System connections first, then user connections */
	connections = bm_manager_get_connections (policy->manager, BM_CONNECTION_SCOPE_SYSTEM);
	if (bm_manager_auto_user_connections_allowed (policy->manager))
		connections = g_slist_concat (connections, bm_manager_get_connections (policy->manager, BM_CONNECTION_SCOPE_USER));

	/* Remove connections that are in the invalid list. */
	iter = connections;
	while (iter) {
		BMConnection *iter_connection = BM_CONNECTION (iter->data);
		GSList *next = g_slist_next (iter);

		if (g_object_get_data (G_OBJECT (iter_connection), INVALID_TAG)) {
			connections = g_slist_remove_link (connections, iter);
			g_object_unref (iter_connection);
			g_slist_free (iter);
		}
		iter = next;
	}

	best_connection = bm_device_get_best_auto_connection (data->device, connections, &specific_object);
	if (best_connection) {
		GError *error = NULL;

		if (!bm_manager_activate_connection (policy->manager,
		                                     best_connection,
		                                     specific_object,
		                                     bm_device_get_path (data->device),
		                                     FALSE,
		                                     &error)) {
			BMSettingConnection *s_con;

			s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (best_connection, BM_TYPE_SETTING_CONNECTION));
			g_assert (s_con);

			bm_log_info (LOGD_DEVICE, "Connection '%s' auto-activation failed: (%d) %s",
			             bm_setting_connection_get_id (s_con), error->code, error->message);
			g_error_free (error);
		}
	}

	g_slist_foreach (connections, (GFunc) g_object_unref, NULL);
	g_slist_free (connections);

 out:
	/* Remove this call's handler ID */
	policy->pending_activation_checks = g_slist_remove (policy->pending_activation_checks, data);
	g_object_unref (data->device);
	g_free (data);

	return FALSE;
}

/*****************************************************************************/

static void
vpn_connection_activated (NMVPNManager *manager,
                          NMVPNConnection *vpn,
                          gpointer user_data)
{
	update_routing_and_dns ((NMPolicy *) user_data, TRUE);
}

static void
vpn_connection_deactivated (NMVPNManager *manager,
                            NMVPNConnection *vpn,
                            NMVPNConnectionState state,
                            NMVPNConnectionStateReason reason,
                            gpointer user_data)
{
	update_routing_and_dns ((NMPolicy *) user_data, TRUE);
}

static void
global_state_changed (NMManager *manager, BMState state, gpointer user_data)
{
}

static void
hostname_changed (NMManager *manager, GParamSpec *pspec, gpointer user_data)
{
	update_system_hostname ((NMPolicy *) user_data, NULL, NULL);
}

static void
sleeping_changed (NMManager *manager, GParamSpec *pspec, gpointer user_data)
{
	gboolean sleeping = FALSE, enabled = FALSE;
	GSList *connections, *iter;

	g_object_get (G_OBJECT (manager), BM_MANAGER_SLEEPING, &sleeping, NULL);
	g_object_get (G_OBJECT (manager), BM_MANAGER_NETWORKING_ENABLED, &enabled, NULL);

	/* Clear the invalid flag on all connections so they'll get retried on wakeup */
	if (sleeping || !enabled) {
		connections = bm_manager_get_connections (manager, BM_CONNECTION_SCOPE_SYSTEM);
		connections = g_slist_concat (connections, bm_manager_get_connections (manager, BM_CONNECTION_SCOPE_USER));
		for (iter = connections; iter; iter = g_slist_next (iter))
			g_object_set_data (G_OBJECT (iter->data), INVALID_TAG, NULL);
		g_slist_free (connections);
	}
}

static void
schedule_activate_check (NMPolicy *policy, BMDevice *device, guint delay_seconds)
{
	ActivateData *data;
	GSList *iter;
	BMDeviceState state;

	if (bm_manager_get_state (policy->manager) == BM_STATE_ASLEEP)
		return;

	state = bm_device_interface_get_state (BM_DEVICE_INTERFACE (device));
	if (state < BM_DEVICE_STATE_DISCONNECTED)
		return;

	if (!bm_device_autoconnect_allowed (device))
		return;

	for (iter = policy->pending_activation_checks; iter; iter = g_slist_next (iter)) {
		/* Only one pending activation check at a time */
		if (((ActivateData *) iter->data)->device == device)
			return;
	}

	data = g_malloc0 (sizeof (ActivateData));
	g_return_if_fail (data != NULL);

	data->policy = policy;
	data->device = g_object_ref (device);
	data->id = delay_seconds ? g_timeout_add_seconds (delay_seconds, auto_activate_device, data) : g_idle_add (auto_activate_device, data);
	policy->pending_activation_checks = g_slist_append (policy->pending_activation_checks, data);
}

static BMConnection *
get_device_connection (BMDevice *device)
{
	NMActRequest *req;

	req = bm_device_get_act_request (device);
	if (!req)
		return NULL;

	return bm_act_request_get_connection (req);
}

static void
device_state_changed (BMDevice *device,
                      BMDeviceState new_state,
                      BMDeviceState old_state,
                      BMDeviceStateReason reason,
                      gpointer user_data)
{
	NMPolicy *policy = (NMPolicy *) user_data;
	BMConnection *connection = get_device_connection (device);

	switch (new_state) {
	case BM_DEVICE_STATE_FAILED:
		/* Mark the connection invalid if it failed during activation so that
		 * it doesn't get automatically chosen over and over and over again.
		 */
		if (connection && IS_ACTIVATING_STATE (old_state)) {
			g_object_set_data (G_OBJECT (connection), INVALID_TAG, GUINT_TO_POINTER (TRUE));
			bm_log_info (LOGD_DEVICE, "Marking connection '%s' invalid.", get_connection_id (connection));
			bm_connection_clear_secrets (connection);
		}
		schedule_activate_check (policy, device, 3);
		break;
	case BM_DEVICE_STATE_ACTIVATED:
		if (connection) {
			/* Clear the invalid tag on the connection */
			g_object_set_data (G_OBJECT (connection), INVALID_TAG, NULL);

			/* And clear secrets so they will always be requested from the
			 * settings service when the next connection is made.
			 */
			bm_connection_clear_secrets (connection);
		}

		update_routing_and_dns (policy, FALSE);
		break;
	case BM_DEVICE_STATE_UNMANAGED:
	case BM_DEVICE_STATE_UNAVAILABLE:
	case BM_DEVICE_STATE_DISCONNECTED:
		update_routing_and_dns (policy, FALSE);
		schedule_activate_check (policy, device, 0);
		break;
	default:
		break;
	}
}

static void
device_ip_config_changed (BMDevice *device,
                          GParamSpec *pspec,
                          gpointer user_data)
{
	update_routing_and_dns ((NMPolicy *) user_data, TRUE);
}

static void
wireless_networks_changed (BMDeviceWifi *device, NMAccessPoint *ap, gpointer user_data)
{
	schedule_activate_check ((NMPolicy *) user_data, BM_DEVICE (device), 0);
}

typedef struct {
	gulong id;
	BMDevice *device;
} DeviceSignalID;

static GSList *
add_device_signal_id (GSList *list, gulong id, BMDevice *device)
{
	DeviceSignalID *data;

	data = g_malloc0 (sizeof (DeviceSignalID));
	if (!data)
		return list;

	data->id = id;
	data->device = device;
	return g_slist_append (list, data);
}

static void
device_added (NMManager *manager, BMDevice *device, gpointer user_data)
{
	NMPolicy *policy = (NMPolicy *) user_data;
	gulong id;

	id = g_signal_connect (device, "state-changed",
	                       G_CALLBACK (device_state_changed),
	                       policy);
	policy->dev_signal_ids = add_device_signal_id (policy->dev_signal_ids, id, device);

	id = g_signal_connect (device, "notify::" BM_DEVICE_INTERFACE_IP4_CONFIG,
	                       G_CALLBACK (device_ip_config_changed),
	                       policy);
	policy->dev_signal_ids = add_device_signal_id (policy->dev_signal_ids, id, device);

	id = g_signal_connect (device, "notify::" BM_DEVICE_INTERFACE_IP6_CONFIG,
	                       G_CALLBACK (device_ip_config_changed),
	                       policy);
	policy->dev_signal_ids = add_device_signal_id (policy->dev_signal_ids, id, device);

	if (BM_IS_DEVICE_WIFI (device)) {
		id = g_signal_connect (device, "access-point-added",
		                       G_CALLBACK (wireless_networks_changed),
		                       policy);
		policy->dev_signal_ids = add_device_signal_id (policy->dev_signal_ids, id, device);

		id = g_signal_connect (device, "access-point-removed",
		                       G_CALLBACK (wireless_networks_changed),
		                       policy);
		policy->dev_signal_ids = add_device_signal_id (policy->dev_signal_ids, id, device);
	}
}

static void
device_removed (NMManager *manager, BMDevice *device, gpointer user_data)
{
	NMPolicy *policy = (NMPolicy *) user_data;
	GSList *iter;

	/* Clear any idle callbacks for this device */
	iter = policy->pending_activation_checks;
	while (iter) {
		ActivateData *data = (ActivateData *) iter->data;
		GSList *next = g_slist_next (iter);

		if (data->device == device) {
			g_source_remove (data->id);
			g_object_unref (data->device);
			g_free (data);
			policy->pending_activation_checks = g_slist_delete_link (policy->pending_activation_checks, iter);
		}
		iter = next;
	}

	/* Clear any signal handlers for this device */
	iter = policy->dev_signal_ids;
	while (iter) {
		DeviceSignalID *data = (DeviceSignalID *) iter->data;
		GSList *next = g_slist_next (iter);

		if (data->device == device) {
			g_signal_handler_disconnect (data->device, data->id);
			g_free (data);
			policy->dev_signal_ids = g_slist_delete_link (policy->dev_signal_ids, iter);
		}
		iter = next;
	}

	update_routing_and_dns (policy, FALSE);
}

static void
schedule_activate_all (NMPolicy *policy)
{
	GSList *iter, *devices;

	devices = bm_manager_get_devices (policy->manager);
	for (iter = devices; iter; iter = g_slist_next (iter))
		schedule_activate_check (policy, BM_DEVICE (iter->data), 0);
}

static void
connections_added (NMManager *manager,
                   BMConnectionScope scope,
                   gpointer user_data)
{
	schedule_activate_all ((NMPolicy *) user_data);
}

static void
connection_added (NMManager *manager,
                  BMConnection *connection,
                  BMConnectionScope scope,
                  gpointer user_data)
{
	schedule_activate_all ((NMPolicy *) user_data);
}

static void
connection_updated (NMManager *manager,
                    BMConnection *connection,
                    BMConnectionScope scope,
                    gpointer user_data)
{
	/* Clear the invalid tag on the connection if it got updated. */
	g_object_set_data (G_OBJECT (connection), INVALID_TAG, NULL);

	schedule_activate_all ((NMPolicy *) user_data);
}

static void
connection_removed (NMManager *manager,
                    BMConnection *connection,
                    BMConnectionScope scope,
                    gpointer user_data)
{
	BMSettingConnection *s_con;
	GPtrArray *list;
	int i;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (connection, BM_TYPE_SETTING_CONNECTION));
	if (!s_con)
		return;

	list = bm_manager_get_active_connections_by_connection (manager, connection);
	if (!list)
		return;

	for (i = 0; i < list->len; i++) {
		char *path = g_ptr_array_index (list, i);
		GError *error = NULL;

		if (!bm_manager_deactivate_connection (manager, path, BM_DEVICE_STATE_REASON_CONNECTION_REMOVED, &error)) {
			bm_log_warn (LOGD_DEVICE, "Connection '%s' disappeared, but error deactivating it: (%d) %s",
			             bm_setting_connection_get_id (s_con), error->code, error->message);
			g_error_free (error);
		}
		g_free (path);
	}
	g_ptr_array_free (list, TRUE);
}

static void
manager_user_permissions_changed (NMManager *manager, NMPolicy *policy)
{
	schedule_activate_all (policy);
}

NMPolicy *
bm_policy_new (NMManager *manager, NMVPNManager *vpn_manager)
{
	NMPolicy *policy;
	static gboolean initialized = FALSE;
	gulong id;
	char hostname[HOST_NAME_MAX + 2];

	g_return_val_if_fail (BM_IS_MANAGER (manager), NULL);
	g_return_val_if_fail (initialized == FALSE, NULL);

	policy = g_malloc0 (sizeof (NMPolicy));
	policy->manager = g_object_ref (manager);
	policy->update_state_id = 0;

	/* Grab hostname on startup and use that if nothing provides one */
	memset (hostname, 0, sizeof (hostname));
	if (gethostname (&hostname[0], HOST_NAME_MAX) == 0) {
		/* only cache it if it's a valid hostname */
		if (strlen (hostname) && strcmp (hostname, "localhost") && strcmp (hostname, "localhost.localdomain"))
			policy->orig_hostname = g_strdup (hostname);
	}

	policy->vpn_manager = g_object_ref (vpn_manager);
	id = g_signal_connect (policy->vpn_manager, "connection-activated",
	                       G_CALLBACK (vpn_connection_activated), policy);
	policy->vpn_activated_id = id;
	id = g_signal_connect (policy->vpn_manager, "connection-deactivated",
	                       G_CALLBACK (vpn_connection_deactivated), policy);
	policy->vpn_deactivated_id = id;

	id = g_signal_connect (manager, "state-changed",
	                       G_CALLBACK (global_state_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "notify::" BM_MANAGER_HOSTNAME,
	                       G_CALLBACK (hostname_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "notify::" BM_MANAGER_SLEEPING,
	                       G_CALLBACK (sleeping_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "notify::" BM_MANAGER_NETWORKING_ENABLED,
	                       G_CALLBACK (sleeping_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "device-added",
	                       G_CALLBACK (device_added), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "device-removed",
	                       G_CALLBACK (device_removed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	/* Large batch of connections added, manager doesn't want us to
	 * process each one individually.
	 */
	id = g_signal_connect (manager, "connections-added",
	                       G_CALLBACK (connections_added), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	/* Single connection added */
	id = g_signal_connect (manager, "connection-added",
	                       G_CALLBACK (connection_added), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "connection-updated",
	                       G_CALLBACK (connection_updated), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "connection-removed",
	                       G_CALLBACK (connection_removed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	id = g_signal_connect (manager, "user-permissions-changed",
	                       G_CALLBACK (manager_user_permissions_changed), policy);
	policy->signal_ids = g_slist_append (policy->signal_ids, (gpointer) id);

	return policy;
}

void
bm_policy_destroy (NMPolicy *policy)
{
	GSList *iter;

	g_return_if_fail (policy != NULL);

	/* Tell any existing hostname lookup thread to die, it'll get cleaned up
	 * by the lookup thread callback.
	  */
	if (policy->lookup) {
		hostname_thread_kill (policy->lookup);
		policy->lookup = NULL;
	}

	for (iter = policy->pending_activation_checks; iter; iter = g_slist_next (iter)) {
		ActivateData *data = (ActivateData *) iter->data;

		g_source_remove (data->id);
		g_object_unref (data->device);
		g_free (data);
	}
	g_slist_free (policy->pending_activation_checks);

	g_signal_handler_disconnect (policy->vpn_manager, policy->vpn_activated_id);
	g_signal_handler_disconnect (policy->vpn_manager, policy->vpn_deactivated_id);
	g_object_unref (policy->vpn_manager);

	for (iter = policy->signal_ids; iter; iter = g_slist_next (iter))
		g_signal_handler_disconnect (policy->manager, (gulong) iter->data);
	g_slist_free (policy->signal_ids);

	for (iter = policy->dev_signal_ids; iter; iter = g_slist_next (iter)) {
		DeviceSignalID *data = (DeviceSignalID *) iter->data;

		g_signal_handler_disconnect (data->device, data->id);
		g_free (data);
	}
	g_slist_free (policy->dev_signal_ids);

	g_free (policy->orig_hostname);
	g_free (policy->cur_hostname);

	g_object_unref (policy->manager);
	g_free (policy);
}

