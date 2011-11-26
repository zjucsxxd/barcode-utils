/* BarcodeManager -- Barcode scanners manager
 *
 * Jakob Flierl <jakob.flierl@gmail.com>
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

#ifndef BARCODE_MANAGER_H
#define BARCODE_MANAGER_H

#include "bm-version.h"

/*
 * dbus services details
 */
#define	BM_DBUS_SERVICE                     "org.freedesktop.BarcodeManager"

#define	BM_DBUS_PATH                        "/org/freedesktop/BarcodeManager"
#define	BM_DBUS_INTERFACE                   "org.freedesktop.BarcodeManager"
#define	BM_DBUS_INTERFACE_DEVICE            BM_DBUS_INTERFACE ".Device"
#define BM_DBUS_INTERFACE_DEVICE_BLUETOOTH  BM_DBUS_INTERFACE_DEVICE ".Bluetooth"
#define BM_DBUS_INTERFACE_SERIAL_DEVICE     BM_DBUS_INTERFACE_DEVICE ".Serial"
#define BM_DBUS_INTERFACE_USB_DEVICE        BM_DBUS_INTERFACE_DEVICE ".Usb"
#define BM_DBUS_INTERFACE_ACTIVE_CONNECTION BM_DBUS_INTERFACE ".Connection.Active"


#define BM_DBUS_SERVICE_USER_SETTINGS     "org.freedesktop.BarcodeManagerUserSettings"
#define BM_DBUS_SERVICE_SYSTEM_SETTINGS   "org.freedesktop.BarcodeManagerSystemSettings"
#define BM_DBUS_IFACE_SETTINGS            "org.freedesktop.BarcodeManagerSettings"
#define BM_DBUS_IFACE_SETTINGS_SYSTEM     "org.freedesktop.BarcodeManagerSettings.System"
#define BM_DBUS_PATH_SETTINGS             "/org/freedesktop/BarcodeManagerSettings"

#define BM_DBUS_IFACE_SETTINGS_CONNECTION "org.freedesktop.BarcodeManagerSettings.Connection"
#define BM_DBUS_PATH_SETTINGS_CONNECTION  "/org/freedesktop/BarcodeManagerSettings/Connection"

/*
 * Types of BarcodeManager states
 */
typedef enum BMState
{
	BM_STATE_UNKNOWN = 0,
	BM_STATE_ASLEEP,
	BM_STATE_CONNECTING,
	BM_STATE_CONNECTED,
	BM_STATE_DISCONNECTED
} BMState;


/*
 * Types of BarcodeManager devices
 */
typedef enum BMDeviceType
{
	BM_DEVICE_TYPE_UNKNOWN = 0,
	BM_DEVICE_TYPE_SERIAL,
	BM_DEVICE_TYPE_USB,
	BM_DEVICE_TYPE_BT  /* Bluetooth */
} BNDeviceType;

/*
 * General device capability bits
 *
 */
#define BM_DEVICE_CAP_NONE               0x00000000
#define BM_DEVICE_CAP_BM_SUPPORTED       0x00000001
#define BM_DEVICE_CAP_CARRIER_DETECT     0x00000002

/**
 * BMBluetoothCapabilities:
 * @BM_BT_CAPABILITY_NONE: device has no usable capabilities
 * @BM_BT_CAPABILITY_DUN:  device provides Dial-Up Networking capability
 * @BM_BT_CAPABILITY_PAN:  device provides Personal Area Networking capability
 *
 * #BMBluetoothCapabilities values indicate the usable capabilities of a
 * Bluetooth device.
 */
typedef enum {
	BM_BT_CAPABILITY_NONE = 0x00000000,
	BM_BT_CAPABILITY_DUN  = 0x00000001,
	BM_BT_CAPABILITY_NAP  = 0x00000002,
} BMBluetoothCapabilities;


/*
 * Device states
 */
typedef enum
{
	BM_DEVICE_STATE_UNKNOWN = 0,

	/* Initial state of all devices and the only state for devices not
	 * managed by BarcodeManager.
	 *
	 * Allowed next states:
	 *   UNAVAILABLE:  the device is now managed by BarcodeManager
	 */
	BM_DEVICE_STATE_UNMANAGED = 1,

	/* Indicates the device is not yet ready for use, but is managed by
	 * BarcodeManager.  For Ethernet devices, the device may not have an
	 * active carrier.  For WiFi devices, the device may not have it's radio
	 * enabled.
	 *
	 * Allowed next states:
	 *   UNMANAGED:  the device is no longer managed by BarcodeManager
	 *   DISCONNECTED:  the device is now ready for use
	 */
	BM_DEVICE_STATE_UNAVAILABLE = 2,

	/* Indicates the device does not have an activate connection to anything.
	 *
	 * Allowed next states:
	 *   UNMANAGED:  the device is no longer managed by BarcodeManager
	 *   UNAVAILABLE:  the device is no longer ready for use (rfkill, no carrier, etc)
	 *   PREPARE:  the device has started activation
	 */
	BM_DEVICE_STATE_DISCONNECTED = 3,

	/* Indicate states in device activation.
	 *
	 * Allowed next states:
	 *   UNMANAGED:  the device is no longer managed by BarcodeManager
	 *   UNAVAILABLE:  the device is no longer ready for use (rfkill, no carrier, etc)
	 *   FAILED:  an error ocurred during activation
	 *   NEED_AUTH:  authentication/secrets are needed
	 *   ACTIVATED:  (IP_CONFIG only) activation was successful
	 *   DISCONNECTED:  the device's connection is no longer valid, or BarcodeManager went to sleep
	 */
	BM_DEVICE_STATE_PREPARE = 4,
	BM_DEVICE_STATE_CONFIG = 5,
	BM_DEVICE_STATE_NEED_AUTH = 6,
	BM_DEVICE_STATE_IP_CONFIG = 7,

	/* Indicates the device is part of an active network connection.
	 *
	 * Allowed next states:
	 *   UNMANAGED:  the device is no longer managed by BarcodeManager
	 *   UNAVAILABLE:  the device is no longer ready for use (rfkill, no carrier, etc)
	 *   FAILED:  a DHCP lease was not renewed, or another error
	 *   DISCONNECTED:  the device's connection is no longer valid, or BarcodeManager went to sleep
	 */
	BM_DEVICE_STATE_ACTIVATED = 8,

	/* Indicates the device's activation failed.
	 *
	 * Allowed next states:
	 *   UNMANAGED:  the device is no longer managed by BarcodeManager
	 *   UNAVAILABLE:  the device is no longer ready for use (rfkill, no carrier, etc)
	 *   DISCONNECTED:  the device's connection is ready for activation, or BarcodeManager went to sleep
	 */
	BM_DEVICE_STATE_FAILED = 9
} BMDeviceState;


/*
 * Device state change reason codes
 */
typedef enum {
	/* No reason given */
	BM_DEVICE_STATE_REASON_NONE = 0,

	/* Unknown error */
	BM_DEVICE_STATE_REASON_UNKNOWN = 1,

	/* Device is now managed */
	BM_DEVICE_STATE_REASON_NOW_MANAGED = 2,

	/* Device is now unmanaged */
	BM_DEVICE_STATE_REASON_NOW_UNMANAGED = 3,

	/* The device could not be readied for configuration */
	BM_DEVICE_STATE_REASON_CONFIG_FAILED = 4,

	/* Secrets were required, but not provided */
	BM_DEVICE_STATE_REASON_NO_SECRETS = 7,

	/* Necessary firmware for the device may be missing */
	BM_DEVICE_STATE_REASON_FIRMWARE_MISSING = 35,

	/* The device was removed */
	BM_DEVICE_STATE_REASON_REMOVED = 36,

	/* BarcodeManager went to sleep */
	BM_DEVICE_STATE_REASON_SLEEPING = 37,

	/* The device's active connection disappeared */
	BM_DEVICE_STATE_REASON_CONNECTION_REMOVED = 38,

	/* Device disconnected by user or client */
	BM_DEVICE_STATE_REASON_USER_REQUESTED = 39,

	/* The Bluetooth connection failed or timed out */
	BM_DEVICE_STATE_REASON_BT_FAILED = 44,

	/* Unused */
	BM_DEVICE_STATE_REASON_LAST = 0xFFFF
} BMDeviceStateReason;


typedef enum {
	BM_ACTIVE_CONNECTION_STATE_UNKNOWN = 0,

	/* Indicates the connection is activating */
	BM_ACTIVE_CONNECTION_STATE_ACTIVATING,

	/* Indicates the connection is currently active */
	BM_ACTIVE_CONNECTION_STATE_ACTIVATED
} BMActiveConnectionState;

#endif /* BARCODE_MANAGER_H */

