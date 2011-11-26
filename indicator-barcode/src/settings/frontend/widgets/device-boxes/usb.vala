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

class MobileBox : DeviceBox {

    /* A Gtk.VBox which holds all of the widgets to control a mobile
      device. It contains an InfoBox and ToggleSwitch to control and
      monitor the state of the mobile device, and a set of widgets
      to enter details about the mobile network to connect to. */

    private string datadir;
    private Barman.Manager barman;
    private InfoBox infobox;
    private ToggleSwitch toggleswitch;
    private Gtk.Label label_status;
    private Gtk.Builder builder;
    private Gtk.VBox vbox_connections;
    private Gtk.Button button_connect;
    private Gtk.Button button_forget;
    private Gtk.Button button_edit;
    private Gtk.ScrolledWindow scrolledwindow_connections;
    private Barman.Service service;

    public MobileBox(Barman.Manager barman, string datadir) {
        this.set_spacing(12);
        this.datadir = datadir;
        this.barman = barman;

        this.barman.notify["cellular-state"].connect(this.on_cellular_state_changed);

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
        this.builder = Utils.Gui.new_builder(datadir+"ui/mobile_box.ui");
        this.get_widgets();
        this.connect_signals();
        /// Packing
        this.pack_start(this.vbox_connections, true, true);

        /// Population
        var services = barman.get_services();
        foreach (var service in services) {
			if (service.get_service_type() == Barman.ServiceType.CELLULAR) {
                /* we support only one ethernet service for now */
				this.service = service;
                service.notify["state"].connect(this.service_state_changed);
                break;
            }
        }

        this.update_widget_states(this.barman.get_cellular_state());
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
        string status_text = "", connect = "";

        switch (state) {
        case Barman.TechnologyState.ENABLED:
            device_editable = true;
            settings_editable = true;
            toggleswitch_state = true;
            status_text = ("Mobile Broadband is enabled.");
            break;
        case Barman.TechnologyState.CONNECTED:
            device_editable = true;
            settings_editable = true;
            toggleswitch_state = true;
            status_text = ("Connected to the Internet.");
            break;
        case Barman.TechnologyState.AVAILABLE:
            device_editable = true;
            settings_editable = false;
            toggleswitch_state = false;
            status_text = ("The Mobile Broadband device is powered off.");
            break;
        case Barman.TechnologyState.OFFLINE:
            device_editable = true;
            settings_editable = false;
            toggleswitch_state = false;
            status_text = ("Mobile Broadband is offline.");
            break;
        }

        this.vbox_connections.set_sensitive(settings_editable);
        this.toggleswitch.set_sensitive(device_editable);
        this.toggleswitch.set_active(toggleswitch_state);
        this.label_status.set_text(status_text);

        if (this.service == null) {
            this.button_connect.set_sensitive(false);
            return;
        }

        switch (this.service.get_state()) {
        case Barman.ServiceState.ASSOCIATION:
        case Barman.ServiceState.CONFIGURATION:
        case Barman.ServiceState.READY:
        case Barman.ServiceState.ONLINE:
            connect = "Disconnect";
            break;
        default:
            connect = "Connect";
            break;
        }

        this.button_connect.set_label(connect);
    }

    private void on_cellular_state_changed(ParamSpec p) {
        this.update_widget_states(this.barman.get_cellular_state());
    }

    private void service_state_changed() {
        this.update_widget_states(this.barman.get_cellular_state());
    }

    private void on_toggleswitch_toggled() {
        if (this.toggleswitch.get_active()) {
            this.barman.enable_technology(Barman.TechnologyType.CELLULAR,
                                           null);
        } else {
            this.barman.disable_technology(Barman.TechnologyType.CELLULAR,
                                            null);
        }
    }

    private void on_button_connect_clicked() {
        if (this.service == null)
            return;

        switch (this.service.get_state()) {
        case Barman.ServiceState.ASSOCIATION:
        case Barman.ServiceState.CONFIGURATION:
        case Barman.ServiceState.READY:
        case Barman.ServiceState.ONLINE:
            this.service.disconnect();
            break;
        default:
            this.service.connect();
            break;
        }
    }

    private void on_button_edit_clicked() {
        if (this.service == null)
            return;

        var dialog = new EditConnectionDialog(this.service, this.datadir);
        //FIXME: make this work
        // //dialog.response.connect(() => { delete dialog; });
        dialog.run();
    }
}
