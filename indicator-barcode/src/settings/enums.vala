/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Jakob Flierl <jakob.flierl@gmail.com>
 *
 * Authors:
 * Jakob Flierl <jakob.flierl@gmail.com>
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

public enum DeviceType {
    WIRED,
    WIRELESS,
    BLUETOOTH,
    USB,
    SERIAL,
    GENERIC; // Shouldn't be used manually
    
    public string get_name() {
        switch (this) {
            case WIRED:
                return ("Wired");
            case WIRELESS:
                return ("Wireless");
            case BLUETOOTH:
                return ("Bluetooth");
            case USB:
                return ("USB");
            case SERIAL:
                return ("Serial");
            case GENERIC:
                return ("Wired");
            default:
                assert_not_reached();
        }
    }
    
    public string get_lname() {
        switch (this) {
            case WIRED:
                return "wired";
            case WIRELESS:
                return "wireless";
            case BLUETOOTH:
                return "bluetooth";
            case USB:
                return "usb";
            case SERIAL:
                return "serial";
            default:
                assert_not_reached();
        }
    }
    
    public string get_icon_name() {
        switch (this) {
            case WIRED:
                return "barcode-wired";
            case WIRELESS:
                return "barcode-wireless";
            case BLUETOOTH:
                return "barcode-bluetooth";
            case USB:
                return "barcode-usb";
            case SERIAL:
                return "barcode-serial";
            case GENERIC:
                return "barcode-generic";
            default:
                assert_not_reached();
        }
    }
}

public enum DeviceState {
    OFF,
    OFFLINE,
    CONNECTED,
    ONLINE;

    public string get_name() {
        switch (this) {
            case OFF:
                return ("Off");
            case OFFLINE:
                return ("Offline");
            case CONNECTED:
                return ("Connected");
            case ONLINE:
                return ("Online");
            default:
             assert_not_reached();
            }
    }
}
