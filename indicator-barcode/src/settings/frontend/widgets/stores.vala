/*
 * indicator-barcode - user interface for barman
 * Copyright: Jakob Flierl <jakob.flierl@gmail.com>
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

public enum DeviceStoreCol {
    DEVICE_OBJ,
    SETTINGS_BOX,
    PIN,
    ICON,
    LABEL
}

class DeviceStore : Gtk.ListStore {

    private Gtk.IconTheme icons;
    
    private Gtk.TreeIter? device_iter;
    
    public DeviceStore(Gtk.IconTheme icons) {
        Type[] column_types;
        column_types = {typeof(Device), typeof(Gtk.VBox),
                                typeof(string), typeof(Gdk.Pixbuf),
                                typeof(string)};
        this.set_column_types(column_types);
        this.icons = icons;
    }

    public void add_device(Device device, Gtk.VBox settings_box) {
        device.notify["state"].connect(this.on_device_state_changed);
        
        var pin_color = this.get_pin_color(device.state);
        var caption = device.state.get_name();
        var label = device.type_.get_name();
        var icon = device.type_.get_icon_name();
        var markup = this.get_markup(label, caption);

        Gdk.Pixbuf icon_pixbuf = null;
        try {
            icon_pixbuf = this.icons.load_icon(icon, 22, Gtk.IconLookupFlags.USE_BUILTIN);
        } catch (Error e) {
            var fallback_icon = DeviceType.GENERIC.get_icon_name();
            logger.debug(@"Can't find '$icon' icon in your Gtk IconTheme, using '$fallback_icon' instead.");
            try {
                icon_pixbuf = this.icons.load_icon(fallback_icon, 22,
                                                   Gtk.IconLookupFlags.USE_BUILTIN);
            } catch (Error e) {
                logger.info("Failed to load fallback icon.");
            }
        }

        Gtk.TreeIter iter;
        this.append(out iter);
        this.set(iter, 0, device, 1, settings_box, 2, pin_color, 3, icon_pixbuf, 4, markup);
    }

    private void on_device_state_changed(GLib.Object object, ParamSpec p) {
        var device = object as Device;
        var iter = this.get_iter_for_device(device);
        var pin_color = this.get_pin_color(device.state);
        var caption = device.state.get_name();
        var label = device.type_.get_name();
        
        var markup = this.get_markup(label, caption);
        
        this.set_value(iter, DeviceStoreCol.PIN, pin_color);
        this.set_value(iter, DeviceStoreCol.LABEL, markup);
    }

    private Gtk.TreeIter get_iter_for_device(Device device) {
        this.device_iter = null;
        this.foreach((model, path, iter) => {
            GLib.Value value;
            model.get_value(iter, DeviceStoreCol.DEVICE_OBJ, out value);
            if ((value.get_object() as Device) == device) {
                this.device_iter = iter;
                return true;
            }
            return false;
        });
        return this.device_iter;
    }

    private string get_pin_color(DeviceState state) {
        switch (state) {
        case DeviceState.OFF:
            return "grey";
        case DeviceState.OFFLINE:
            return "red";
        case DeviceState.CONNECTED:
            return "yellow";
        case DeviceState.ONLINE:
            return "green";
        default:
            assert_not_reached();
        }
    }

    private string get_markup(string label, string caption) {
        return @"$label\n<span font_size='small'>$caption</span>";
    }
}


public enum WirelessConnectionStoreCol {
    CONNECTION_OBJ,
    NETWORK,
    SIGNAL,
    LAST_USED
}

class WirelessConnectionStore : Gtk.ListStore {

    public WirelessConnectionStore() {
        Type[] column_types;
        column_types = {typeof(Barman.Service), typeof(string),
                                typeof(double), typeof(string)};
        this.set_column_types(column_types);
    }

    public void add_service(Barman.Service service, string network,
                                double signal, string last_used) {
        Gtk.TreeIter iter;
        this.append(out iter);
        this.set(iter, 0, service, 1, network, 2, signal, 3, last_used);
    }
}


