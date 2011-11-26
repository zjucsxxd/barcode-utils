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

class DeviceView : Gtk.TreeView {
    
    public signal void reordered();

    private bool dragging = false;
    
    public DeviceView (DeviceStore store) {
        this.set_model(store);
        
        // Device (main) Column (contains pin icon, type icon and label)
        var column = new Gtk.TreeViewColumn();
        column.set_title(("Device"));
        var cr_pin = new PinCellRenderer();
        var cr_icon = new Gtk.CellRendererPixbuf();
        var cr_label = new Gtk.CellRendererText();
        column.pack_start(cr_pin, false);
        column.set_attributes(cr_pin, "color", DeviceStoreCol.PIN);
        column.pack_start(cr_icon, false);
        column.set_attributes(cr_icon, "pixbuf", DeviceStoreCol.ICON);
        column.pack_start(cr_label, true);
        column.set_attributes(cr_label, "markup", DeviceStoreCol.LABEL);
        cr_label.set_property("ellipsize", Pango.EllipsizeMode.END);
        this.append_column(column);

        this.set_headers_visible(false);
        /* FIXME: when get gtk errors if we enable reordering and
           try to get the selected path
        this.set_reorderable(true);*/

        this.drag_begin.connect(this.on_drag_begin);
        this.drag_end.connect(this.on_drag_end);
        this.drag_failed.connect(this.on_drag_failed);
    }
        
    private void on_drag_begin(Gdk.DragContext drag_context) {
        this.dragging = true;
    }

    private bool on_drag_failed(Gdk.DragContext drag_context, Gtk.DragResult drag_result) {
        this.dragging = false;
        return true;
    }

    private void on_drag_end(Gdk.DragContext drag_context) {
        if (this.dragging) {
            this.reordered();
        }
    }

/* FIXME: unused for now so comment it out 

    public Device? get_selected_device() {
        Gtk.TreeModel model;
        Gtk.TreeIter? iter;
        this.get_selection().get_selected(out model, out iter);
        if (iter == null) {
            return null;
        } else {
            GLib.Value value;
            model.get_value(iter, 0, out value);
            return value.get_object() as Device;
        }
        
    }
*/

}

class WirelessConnectionView : Gtk.TreeView {

    public WirelessConnectionView(WirelessConnectionStore store, Gtk.Widget style_widget) {
        this.set_model(store);
        
        // Network Column (contains label)
        var network_column = new Gtk.TreeViewColumn();
        network_column.set_title(("Network"));
        var cr_label_network = new Gtk.CellRendererText();
        network_column.pack_start(cr_label_network, true);
        network_column.set_attributes(cr_label_network, "markup", WirelessConnectionStoreCol.NETWORK);
        network_column.set_expand(true);
        this.append_column(network_column);
        
        // Signal Column (contains signal indicator)
        var signal_column = new Gtk.TreeViewColumn();
        signal_column.set_title(("Signal"));
        var cr_signal = new SignalStrengthCellRenderer(style_widget, this);
        signal_column.pack_start(cr_signal, true);
        signal_column.set_attributes(cr_signal, "strength", 2);
        signal_column.set_clickable(true);
        signal_column.set_sort_column_id(2);
        this.append_column(signal_column);
        
        // Last Used Column (contains label)
        var last_used_column = new Gtk.TreeViewColumn();
        last_used_column.set_title(("Last Used"));
        var cr_label_last_used = new Gtk.CellRendererText();
        last_used_column.pack_start(cr_label_last_used, true);
        last_used_column.set_attributes(cr_label_last_used, "markup", WirelessConnectionStoreCol.LAST_USED);
        this.append_column(last_used_column);
    }

    public Barman.Service? get_selected_connection() {
        Gtk.TreeModel model;
        Gtk.TreeIter? iter;
        this.get_selection().get_selected(out model, out iter);
        bool iter_is_valid = (model as WirelessConnectionStore).iter_is_valid(iter);
        if (iter != null && iter_is_valid) {
            GLib.Value value;
            model.get_value(iter, WirelessConnectionStoreCol.CONNECTION_OBJ, out value);
            return value.get_object() as Barman.Service;
        }
        return null;
    }
}
