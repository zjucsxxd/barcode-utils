/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Jakob Flierl
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

public class ConnectionsPage : Object {

    public Gtk.Alignment alignment;
    public Gtk.Notebook notebook_right;
    public bool viewed = false;

    private DeviceView treeview_devices;

    public ConnectionsPage(string datadir, Gtk.IconTheme icons,
                            Gtk.Alignment alignment,
                            Gtk.ScrolledWindow scrolledwindow_devices,
						   Gtk.Notebook notebook_right,
						   Barman.Manager barman) {
                                
        this.alignment = alignment;
        this.notebook_right = notebook_right;

        // Create LHS
        /// Create device view
        var device_store = new DeviceStore(icons);
        this.treeview_devices = new DeviceView(device_store);
        /// Pack it
        scrolledwindow_devices.add(this.treeview_devices);
        /// Connect signals to callbacks
        this.treeview_devices.get_selection().changed.connect(
            this.on_treeview_devices_selection_changed);
        this.treeview_devices.reordered.connect(
            this.on_treeview_devices_reordered);

        var usb = new USBDevice(barman);
        var usb_box = new WiredBox(barman, datadir);
        notebook_right.append_page(usb_box, null);
        this.add_device(wifi, usb_box);

        var serial = new SerialDevice(barman);
        var serial_box = new WiredBox(barman, datadir);
        notebook_right.append_page(serial_box, null);
        this.add_device(serial, serial_box);

        var bluetooth = new BluetoothDevice(barman);
        var bluetooth_box = new BluetoothBox(barman, datadir);
        notebook_right.append_page(bluetooth_box, null);
        this.add_device(bluetooth, bluetooth_box);

        // Order the RHS notebook pages inaccordance with the deviceview
        this.on_treeview_devices_reordered();
    }

    public void add_device(Device device, DeviceBox box) {
        var liststore = this.treeview_devices.get_model() as DeviceStore;
        liststore.add_device(device, box);
    }

    public void on_first_expose() {
        if (!this.viewed) {
            this.treeview_devices.get_selection().select_path(new Gtk.TreePath.first());
            this.viewed = true;
        }
    }

    private void on_treeview_devices_selection_changed() {
        var selection = this.treeview_devices.get_selection();
        Gtk.TreeModel model;
        Gtk.TreeIter? iter;
        selection.get_selected(out model, out iter);

        if (iter != null) {
            int i = model.get_path(iter).get_indices()[0];
            this.notebook_right.set_current_page(i);
        }
    }
    
    private void on_treeview_devices_reordered() {
        var i = 0;
        var model = this.treeview_devices.get_model();
        model.foreach((model, path, iter) => {
            GLib.Value value;
            model.get_value(iter, DeviceStoreCol.SETTINGS_BOX, out value);
            Gtk.VBox page = value.get_object() as Gtk.VBox;
            this.notebook_right.reorder_child(page, i);
            i += 1;
            return false;
        });
    }

}
