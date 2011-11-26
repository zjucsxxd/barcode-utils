/*
 * indicator-barcode - user interface for barman
 * Copyright 2011-2012 Jakob Flierl
 *
 * Authors:
 * Jakob Flierl
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

public abstract class Device : GLib.Object {

    public DeviceType type_ { get; protected set; }
    public DeviceState state { get; protected set; }

    protected Barman.Manager barman;

    protected DeviceState convert_state(Barman.TechnologyState state) {

        switch (state) {
        case Barman.TechnologyState.CONNECTED:
            return DeviceState.CONNECTED;
        case Barman.TechnologyState.ENABLED:
            return DeviceState.ONLINE;
        case Barman.TechnologyState.OFFLINE:
            return DeviceState.OFFLINE;
        case Barman.TechnologyState.AVAILABLE:
        default:
            return DeviceState.OFF;
        }
    }

    protected void update_state(Barman.TechnologyState state) {
        this.state = convert_state(state);
    }
}

public class USBDevice : Device {

    public USBDevice(Barman.Manager barman) {
        this.type_ = DeviceType.USB;
        this.barman = barman;

        this.barman.notify["usb-state"].connect((s, p) => {
                update_state(this.barman.get_usb_state());
            });

        update_state(this.barman.get_usb_state());
    }
}

public class SerialDevice : Device {

    public SerialDevice(Barman.Manager barman) {
        this.type_ = DeviceType.SERIAL;
        this.barman = barman;

        this.barman.notify["serial-state"].connect((s, p) => {
                update_state(this.barman.get_serial_state());
            });

        update_state(this.barman.get_serial_state());
    }
}

public class BluetoothDevice : Device {

    public BluetoothDevice(Barman.Manager barman) {
        this.type_ = DeviceType.BLUETOOTH;
        this.barman = barman;

        this.barman.notify["bluetooth-state"].connect((s, p) => {
                update_state(this.barman.get_bluetooth_state());
            });

        update_state(this.barman.get_bluetooth_state());
    }
}

