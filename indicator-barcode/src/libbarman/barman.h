/*
 * indicator-barcode - user interface for barman
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * Kalle Valo <kalle.valo@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INDIDICATOR_BARMAN_H_
#define _INDIDICATOR_BARMAN_H_

#define BARMAN_SERVICE_NAME		"net.barman"
#define BARMAN_PATH			"/org/moblin/barman"

#define BARMAN_DEBUG_INTERFACE		BARMAN_SERVICE_NAME ".Debug"
#define BARMAN_ERROR_INTERFACE		BARMAN_SERVICE_NAME ".Error"
#define BARMAN_AGENT_INTERFACE		BARMAN_SERVICE_NAME ".Agent"
#define BARMAN_COUNTER_INTERFACE	BARMAN_SERVICE_NAME ".Counter"

#define BARMAN_MANAGER_INTERFACE	BARMAN_SERVICE_NAME ".Manager"
#define BARMAN_MANAGER_PATH		"/"

#define BARMAN_TASK_INTERFACE		BARMAN_SERVICE_NAME ".Task"
#define BARMAN_PROFILE_INTERFACE	BARMAN_SERVICE_NAME ".Profile"
#define BARMAN_SERVICE_INTERFACE	BARMAN_SERVICE_NAME ".Service"
#define BARMAN_DEVICE_INTERFACE	BARMAN_SERVICE_NAME ".Device"
#define BARMAN_NETWORK_INTERFACE	BARMAN_SERVICE_NAME ".Network"
#define BARMAN_PROVIDER_INTERFACE	BARMAN_SERVICE_NAME ".Provider"
#define BARMAN_TECHNOLOGY_INTERFACE	BARMAN_SERVICE_NAME ".Technology"

#define BARMAN_SIGNAL_PROPERTY_CHANGED	"PropertyChanged"

#define BARMAN_TECHNOLOGY_ETHERNET	"ethernet"
#define BARMAN_TECHNOLOGY_WIFI		"wifi"
#define BARMAN_TECHNOLOGY_CELLULAR	"cellular"
#define BARMAN_TECHNOLOGY_BLUETOOTH	"bluetooth"

#define BARMAN_PROPERTY_PASSPHRASE		"Passphrase"
#define BARMAN_PROPERTY_DEFAULT_TECHNOLOGY	"DefaultTechnology"
#define BARMAN_PROPERTY_ENABLED_TECHNOLOGIES	"EnabledTechnologies"
#define BARMAN_PROPERTY_SERVICES		"Services"
#define BARMAN_PROPERTY_NAME			"Name"
#define BARMAN_PROPERTY_TYPE			"Type"
#define BARMAN_PROPERTY_STATE			"State"
#define BARMAN_PROPERTY_STRENGTH		"Strength"
#define BARMAN_PROPERTY_SETUP_REQUIRED		"SetupRequired"
#define BARMAN_PROPERTY_SECURITY		"Security"
#define BARMAN_PROPERTY_ERROR			"Error"
#define BARMAN_PROPERTY_MODE			"Mode"
#define BARMAN_PROPERTY_LOGIN_REQUIRED		"LoginRequired"
#define BARMAN_PROPERTY_PASSPHRASE_REQUIRED	"PassphraseRequired"
#define BARMAN_PROPERTY_FAVORITE		"Favorite"
#define BARMAN_PROPERTY_IMMUTABLE		"Immutable"
#define BARMAN_PROPERTY_AUTOCONNECT		"AutoConnect"
#define BARMAN_PROPERTY_APN			"APN"
#define BARMAN_PROPERTY_MCC			"MCC"
#define BARMAN_PROPERTY_MNC			"MNC"
#define BARMAN_PROPERTY_ROAMING		"Roaming"
#define BARMAN_PROPERTY_NAMESERVERS		"Nameservers"
#define BARMAN_PROPERTY_NAMESERVERS_CONFIGURATION "Nameservers.Configuration"
#define BARMAN_PROPERTY_DOMAINS		"Domains"
#define BARMAN_PROPERTY_DOMAINS_CONFIGURATION	"Domains.Configuration"
#define BARMAN_PROPERTY_IPV4			"IPv4"
#define BARMAN_PROPERTY_IPV4_CONFIGURATION	"IPv4.Configuration"
#define BARMAN_PROPERTY_IPV6			"IPv6"
#define BARMAN_PROPERTY_IPV6_CONFIGURATION	"IPv6.Configuration"
#define BARMAN_PROPERTY_TECHNOLOGIES		"Technologies"
#define BARMAN_PROPERTY_OFFLINE_MODE		"OfflineMode"

#define BARMAN_TECHNOLOGY_PROPERTY_STATE	"State"
#define BARMAN_TECHNOLOGY_PROPERTY_TYPE	"Type"

#define BARMAN_STATE_ONLINE			"online"
#define BARMAN_STATE_CONNECTING		"connecting"
#define BARMAN_STATE_OFFLINE			"offline"

#define BARMAN_SECURITY_NONE			"none"
#define BARMAN_SECURITY_WEP			"wep"
#define BARMAN_SECURITY_PSK			"psk"
#define BARMAN_SECURITY_WPA			"wpa"
#define BARMAN_SECURITY_RSN			"rsn"
#define BARMAN_SECURITY_IEEE8021X		"ieee8021x"

#endif
