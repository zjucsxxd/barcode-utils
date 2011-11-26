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

public class WirelessBox : DeviceBox {

    /* A Gtk.VBox which holds all of the widgets to control a wireless
      device. It contains an InfoBox and ToggleSwitch to control and
      monitor the state of the wireless device, and a
      WirelessConnectionView to connect to and remember details for
      wireless networks. */

    private string datadir;
    private Barman.Manager barman;
    private InfoBox infobox;
    private ToggleSwitch toggleswitch;
    private Gtk.Label label_status;
    private Gtk.Builder builder;
    private WirelessConnectionView treeview_connections;
    private Gtk.VBox vbox_connections;
    private Gtk.Button button_connect;
    private Gtk.Button button_forget;
    private Gtk.Button button_edit;
    private Gtk.ScrolledWindow scrolledwindow_connections;

    public WirelessBox(Barman.Manager barman, string datadir) {
        this.set_spacing(12);
        this.datadir = datadir;
        this.barman = barman;

        this.barman.notify["wifi-state"].connect(this.wifi_state_changed);

        // Infobox and Togglswitch
        /// Creation
        this.infobox = new InfoBox(false, 12);
        this.toggleswitch = new ToggleSwitch();
        this.label_status = new Gtk.Label(null);
        /// Padding and alignment
        this.label_status.set_alignment(0, 0.5f);
        this.infobox.set_border_width(10);
        /// Packing
        this.infobox.pack_start(this.label_status, true, true);
        this.infobox.pack_start(this.toggleswitch, false, true);
        this.pack_start(this.infobox, false, false);
        /// Connect signals;
        this.toggleswitch.toggled.connect(this.on_toggleswitch_toggled);

        // Network Settings
        /// Creation
        this.builder = Utils.Gui.new_builder(datadir+"ui/wireless_box.ui");
        this.get_widgets();
        this.connect_signals();
        /// Packing
        this.pack_start(this.vbox_connections, true, true);

        // Connection View
        /// Creation
        var connection_store = new WirelessConnectionStore();
        this.treeview_connections = new WirelessConnectionView(connection_store,
                                                this.button_connect);
        /// Packing
        this.scrolledwindow_connections.add(this.treeview_connections);
        /// Connect signals
        this.treeview_connections.get_selection().changed.connect(this.on_treeview_connections_selection_changed);
        /// Population
        var services = barman.get_services();
        foreach (var service in services) {
			if (service.get_service_type() == Barman.ServiceType.WIFI)
				this.add_service(service);
        }

        this.update_widget_states(this.barman.get_wifi_state());
        this.on_treeview_connections_selection_changed();
    }

    private void get_widgets() {
        this.vbox_connections = this.builder.get_object("vbox_connections") as Gtk.VBox;
        this.button_connect = this.builder.get_object("button_connect") as Gtk.Button;
        this.button_forget = this.builder.get_object("button_forget") as Gtk.Button;
        this.button_edit = this.builder.get_object("button_edit") as Gtk.Button;
        this.scrolledwindow_connections = this.builder.get_object("scrolledwindow_connections") as Gtk.ScrolledWindow;
    }

    private void connect_signals() {
        this.button_connect.clicked.connect(this.on_button_connect_clicked);
        this.button_edit.clicked.connect(this.on_button_edit_clicked);
    }

    private void update_widget_states(Barman.TechnologyState state) {
        bool device_editable = false;
        bool settings_editable = false;
        bool toggleswitch_state = false;
        string status_text = "";

        switch (state) {
            case Barman.TechnologyState.ENABLED:
                device_editable = true;
                settings_editable = true;
                toggleswitch_state = true;
                status_text = ("Wi-fi is on but not connected to the Internet.");
                break;
            case Barman.TechnologyState.CONNECTED:
                device_editable = true;
                settings_editable = true;
                toggleswitch_state = true;
                status_text = ("Wi-fi is on and connected to the Internet.");
                break;
            case Barman.TechnologyState.AVAILABLE:
                device_editable = true;
                settings_editable = false;
                toggleswitch_state = false;
                status_text = ("The Wi-fi device is powered off.");
                break;
            case Barman.TechnologyState.OFFLINE:
                device_editable = true;
                settings_editable = false;
                toggleswitch_state = false;
                status_text = ("Wi-fi is offline.");
                break;
        }

        this.vbox_connections.set_sensitive(settings_editable);
        this.toggleswitch.set_sensitive(device_editable);
        this.toggleswitch.set_active(toggleswitch_state);
        this.label_status.set_text(status_text);
    }

    private void add_service(Barman.Service service) {
        var name = service.name;
        var signal = (float) service.strength / 100;
        var last_used = "N/A";

        var liststore = this.treeview_connections.get_model() as WirelessConnectionStore;
        liststore.add_service(service, name, signal, last_used);
    }
    
    private void wifi_state_changed(ParamSpec p) {
        this.update_widget_states(this.barman.get_wifi_state());
    }

    private void on_toggleswitch_toggled() {
        if (this.toggleswitch.get_active()) {
            this.barman.enable_technology(Barman.TechnologyType.WIFI, null);
        } else {
            this.barman.disable_technology(Barman.TechnologyType.WIFI, null);
        }
    }

   private void on_button_connect_clicked() {
        // var connection = this.treeview_connections.get_selected_connection();
        // connection.connect_("password");
    }
    
   private void on_button_edit_clicked() {
        var connection = this.treeview_connections.get_selected_connection();
        var dialog = new EditConnectionDialog(connection, this.datadir);
        //FIXME: make this work
        //dialog.response.connect(() => { delete dialog; });
        dialog.run();
    }

    private void on_treeview_connections_selection_changed() {
        bool path_is_selected = (this.treeview_connections.get_selected_connection() != null);
        
        this.button_connect.set_sensitive(path_is_selected);
        this.button_forget.set_sensitive(path_is_selected);
        this.button_edit.set_sensitive(path_is_selected);
    }
}
