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

public class FlightModeBox : DeviceBox {

    /* A Gtk.VBox which holds all of the widgets to control flightmode.
      It contains an InfoBox and ToggleSwitch to control and
      monitor the state of flightmode, a checkbutton. */

    private string datadir;
    private Device device;
    private Barman.Manager barman;
    private InfoBox infobox;
    private ToggleSwitch toggleswitch;
    private Gtk.Label label_status;
    private Gtk.Builder builder;
    
    private Gtk.VBox vbox_information;
    private Gtk.Button button_more_information;
    private Gtk.CheckButton checkbutton_show;

    public FlightModeBox(Device device, Barman.Manager barman,
                         string datadir) {
        this.set_spacing(12);
        this.datadir = datadir;
        this.device = device;
        this.barman = barman;
        this.barman.notify["offline-mode"].connect(this.offline_mode_changed);

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
        this.builder = Utils.Gui.new_builder(datadir+"ui/flightmode_box.ui");
        this.get_widgets();
        this.connect_signals();
        /// Packing
        this.pack_start(this.vbox_information, true, true);

        this.update_widget_states(this.barman.offline_mode);
    }

    private void get_widgets() {
        this.vbox_information = this.builder.get_object("vbox_information") as Gtk.VBox;
        this.button_more_information = this.builder.get_object("button_more_information") as Gtk.Button;
        this.checkbutton_show = this.builder.get_object("checkbutton_show") as Gtk.CheckButton;
    }

    private void connect_signals() {
    }

    private void update_widget_states(bool offline) {
        bool toggleswitch_state = false;
        string status_text = "";

        if (offline) {
            toggleswitch_state = true;
            status_text = ("Flight Mode is on.");
        } else {
            toggleswitch_state = false;
            status_text = ("Flight Mode is off.");
        }

        this.toggleswitch.set_active(toggleswitch_state);
        this.label_status.set_text(status_text);
    }

    private void offline_mode_changed(ParamSpec p) {
        this.update_widget_states(this.barman.offline_mode);
    }

    private void on_toggleswitch_toggled() {
        if (this.toggleswitch.get_active()) {
            this.barman.offline_mode = true;
        } else {
            this.barman.offline_mode = false;
        }
    }
}
