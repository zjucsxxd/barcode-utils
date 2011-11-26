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

private enum NotebookPage {
    PAGE_MAIN = 0,
    PAGE_WIRELESS = 1,
    PAGE_CELLULAR = 2,
    PAGE_IPV4 = 3,
    PAGE_IPV6 = 4,
    PAGE_DNS = 5,
}

public class EditConnectionDialog : GLib.Object {

    public signal void response(int type);

    private Barman.Service connection;
    private Gtk.Builder builder;

    private Gtk.Window dialog;
    private Gtk.Table table_main;
    private Gtk.Button button_save;
    private Gtk.Button button_cancel;
    private Gtk.Label label_name;
    private Gtk.CheckButton checkbutton_autoconnect;

    /* wifi */
    private Gtk.Label label_mode;
    private Gtk.Label label_security;
    private Gtk.Entry entry_passphrase;

    /* ipv4 */
    private Gtk.ComboBox combobox_ipv4_method;
    private Gtk.Entry entry_ipv4_address;
    private Gtk.Entry entry_ipv4_netmask;
    private Gtk.Entry entry_ipv4_gateway;
    private Gtk.TreeIter ipv4_off_iter;
    private Gtk.TreeIter ipv4_manual_iter;
    private Gtk.TreeIter ipv4_dhcp_iter;

    /* ipv6 */
    private Gtk.ComboBox combobox_ipv6_method;
    private Gtk.Entry entry_ipv6_address;
    private Gtk.Entry entry_ipv6_prefix_length;
    private Gtk.Entry entry_ipv6_gateway;
    private Gtk.TreeIter ipv6_off_iter;
    private Gtk.TreeIter ipv6_manual_iter;
    private Gtk.TreeIter ipv6_auto_iter;

    private Gtk.Entry entry_nameservers;
    private Gtk.Entry entry_domains;

    private Gtk.Notebook notebook_sections;
    // private NotebookPage pages[];
    private Gee.ArrayList<NotebookPage> pages;

    // button_cancel here is an _ugly_ workaround to get signals from
    // objects working, reason for this is complete unknown
    private string[] hint_labels = { "button_cancel" };

    public EditConnectionDialog(Barman.Service connection, string datadir) {
        this.connection = connection;

        this.select_pages();

        /// Creation
        this.builder = Utils.Gui.new_builder(datadir+"ui/edit_connection_dialog.ui");
        this.get_widgets();
        this.create_extra_widgets();

        this.connect_signals();
        this.update_widget_values();

        this.update_tabs();
    }

    private void select_pages() {
        var pages = new Gee.ArrayList<NotebookPage>();

        switch (this.connection.get_service_type()) {
        case Barman.ServiceType.WIFI:
            pages.add(NotebookPage.PAGE_MAIN);
            pages.add(NotebookPage.PAGE_WIRELESS);
            pages.add(NotebookPage.PAGE_IPV4);
            pages.add(NotebookPage.PAGE_IPV6);
            pages.add(NotebookPage.PAGE_DNS);
            break;
        case Barman.ServiceType.ETHERNET:
            pages.add(NotebookPage.PAGE_MAIN);
            pages.add(NotebookPage.PAGE_IPV4);
            pages.add(NotebookPage.PAGE_IPV6);
            pages.add(NotebookPage.PAGE_DNS);
            break;
        case Barman.ServiceType.CELLULAR:
            pages.add(NotebookPage.PAGE_MAIN);
            pages.add(NotebookPage.PAGE_CELLULAR);
            pages.add(NotebookPage.PAGE_IPV4);
            pages.add(NotebookPage.PAGE_IPV6);
            pages.add(NotebookPage.PAGE_DNS);
            break;
        case Barman.ServiceType.BLUETOOTH:
            pages.add(NotebookPage.PAGE_MAIN);
            pages.add(NotebookPage.PAGE_IPV4);
            pages.add(NotebookPage.PAGE_IPV6);
            pages.add(NotebookPage.PAGE_DNS);
            break;
        }

        this.pages = pages;
    }

    private void update_tabs() {
        /*
         * As page numbers change as pages are removed it's important to
         * remove the pages from the end.
         */

        if (!(NotebookPage.PAGE_DNS in this.pages))
            this.notebook_sections.remove_page(NotebookPage.PAGE_DNS);

        if (!(NotebookPage.PAGE_IPV6 in this.pages))
            this.notebook_sections.remove_page(NotebookPage.PAGE_IPV6);

        if (!(NotebookPage.PAGE_IPV4 in this.pages))
            this.notebook_sections.remove_page(NotebookPage.PAGE_IPV4);

        if (!(NotebookPage.PAGE_CELLULAR in this.pages))
            this.notebook_sections.remove_page(NotebookPage.PAGE_CELLULAR);

        if (!(NotebookPage.PAGE_WIRELESS in this.pages))
            this.notebook_sections.remove_page(NotebookPage.PAGE_WIRELESS);

        if (!(NotebookPage.PAGE_MAIN in this.pages))
            this.notebook_sections.remove_page(NotebookPage.PAGE_MAIN);
    }

    private void create_extra_widgets() {
        // Make hint labels use the text_aa color (most often greyish)
        foreach (var hint_label in this.hint_labels) {
            var hint_label_widget = this.builder.get_object(hint_label) as Gtk.Widget;
            hint_label_widget.realize.connect(() => {
                                                    var state = hint_label_widget.get_state();
                                                    var color = hint_label_widget.style.text_aa[state];
                                                    hint_label_widget.modify_fg(state, color);
                                                    });
        }

        /* ipv4 combobox */
        var store = new Gtk.ListStore(2, typeof(int), typeof(string));
        Gtk.TreeIter iter;

        this.combobox_ipv4_method.model = store;

        var cell = new Gtk.CellRendererText();
        this.combobox_ipv4_method.pack_start(cell, false);
        this.combobox_ipv4_method.set_attributes(cell, "text", 1);

        store.append(out iter);
        store.set(iter, 0, Barman.IPv4Method.OFF, 1, "Off");
        this.ipv4_off_iter = iter;

        store.append(out iter);
        store.set(iter, 0, Barman.IPv4Method.MANUAL, 1, "Manual");
        this.ipv4_manual_iter = iter;

        store.append(out iter);
        store.set(iter, 0, Barman.IPv4Method.DHCP, 1, "DHCP");
        this.ipv4_dhcp_iter = iter;

        /* ipv6 combobox */
        store = new Gtk.ListStore(2, typeof(int), typeof(string));

        this.combobox_ipv6_method.model = store;

        cell = new Gtk.CellRendererText();
        this.combobox_ipv6_method.pack_start(cell, false);
        this.combobox_ipv6_method.set_attributes(cell, "text", 1);

        store.append(out iter);
        store.set(iter, 0, Barman.IPv6Method.OFF, 1, "Off");
        this.ipv6_off_iter = iter;

        store.append(out iter);
        store.set(iter, 0, Barman.IPv6Method.MANUAL, 1, "Manual");
        this.ipv6_manual_iter = iter;

        store.append(out iter);
        store.set(iter, 0, Barman.IPv6Method.AUTO, 1, "Auto");
        this.ipv6_auto_iter = iter;
    }

    private void get_widgets() {
        var b = this.builder;

        this.dialog = b.get_object("window_main") as Gtk.Window;
        this.table_main = b.get_object("table_main") as Gtk.Table;
        this.button_save = b.get_object("button_save") as Gtk.Button;
        this.button_cancel = b.get_object("button_cancel") as Gtk.Button;

        /* main */
        this.label_name = b.get_object("label_name") as Gtk.Label;
        this.checkbutton_autoconnect = b.get_object("checkbutton_autoconnect")
                                        as Gtk.CheckButton;

        /* wifi */
        this.label_mode = b.get_object("label_mode") as Gtk.Label;
        this.label_security = b.get_object("label_security") as Gtk.Label;
        this.entry_passphrase = b.get_object("entry_passphrase") as Gtk.Entry;

        /* ipv4 */
        this.combobox_ipv4_method = b.get_object("combobox_ipv4_method")
                        as Gtk.ComboBox;
        this.entry_ipv4_address = b.get_object("entry_ipv4_address")
                        as Gtk.Entry;
        this.entry_ipv4_netmask = b.get_object("entry_ipv4_netmask")
                        as Gtk.Entry;
        this.entry_ipv4_gateway = b.get_object("entry_ipv4_gateway")
                        as Gtk.Entry;

        /* ipv6 */
        this.combobox_ipv6_method = b.get_object("combobox_ipv6_method")
                        as Gtk.ComboBox;
        this.entry_ipv6_address = b.get_object("entry_ipv6_address")
                        as Gtk.Entry;
        this.entry_ipv6_prefix_length = b.get_object("entry_ipv6_prefix_length")
                        as Gtk.Entry;
        this.entry_ipv6_gateway = b.get_object("entry_ipv6_gateway")
                        as Gtk.Entry;

        /* dns */
        this.entry_nameservers = b.get_object("entry_nameservers") as Gtk.Entry;
        this.entry_domains = b.get_object("entry_domains") as Gtk.Entry;

        this.notebook_sections = b.get_object("notebook_sections") as Gtk.Notebook;
    }

    private void connect_signals() {
        this.button_save.clicked.connect(this.on_button_save_clicked);
        this.button_cancel.clicked.connect(this.on_button_cancel_clicked);
        this.combobox_ipv4_method.changed.connect(ipv4_method_changed);
        this.combobox_ipv6_method.changed.connect(ipv6_method_changed);
    }

    private void ipv4_method_changed(Gtk.ComboBox box) {
        Gtk.TreeIter iter;
        Gtk.ListStore store;
        Barman.IPv4Method method;

        if (this.combobox_ipv4_method.get_active_iter(out iter) == false)
            return;

        store = this.combobox_ipv4_method.model as Gtk.ListStore;
        store.get(iter, 0, out method);

        switch (method) {
        case Barman.IPv4Method.MANUAL:
            this.entry_ipv4_address.sensitive = true;
            this.entry_ipv4_netmask.sensitive = true;
            this.entry_ipv4_gateway.sensitive = true;
            break;
        case Barman.IPv4Method.OFF:
        case Barman.IPv4Method.DHCP:
        default:
            this.entry_ipv4_address.sensitive = false;
            this.entry_ipv4_netmask.sensitive = false;
            this.entry_ipv4_gateway.sensitive = false;
            break;
        }
    }

    private void ipv6_method_changed(Gtk.ComboBox box) {
        Gtk.TreeIter iter;
        Gtk.ListStore store;
        Barman.IPv6Method method;

        if (this.combobox_ipv6_method.get_active_iter(out iter) == false)
            return;

        store = this.combobox_ipv6_method.model as Gtk.ListStore;
        store.get(iter, 0, out method);

        switch (method) {
        case Barman.IPv6Method.MANUAL:
            this.entry_ipv6_address.sensitive = true;
            this.entry_ipv6_prefix_length.sensitive = true;
            this.entry_ipv6_gateway.sensitive = true;
            break;
        case Barman.IPv6Method.OFF:
        case Barman.IPv6Method.AUTO:
        default:
            this.entry_ipv6_address.sensitive = false;
            this.entry_ipv6_prefix_length.sensitive = false;
            this.entry_ipv6_gateway.sensitive = false;
            break;
        }
    }

    private void update_mode() {
        string mode = "";

        switch (this.connection.mode) {
        case Barman.ServiceMode.MANAGED:
            mode = "Infrastructure";
            break;
        case Barman.ServiceMode.ADHOC:
            mode = "Ad-Hoc";
            break;
        }

        this.label_mode.set_text(mode);
    }

    private void update_name() {
        this.label_name.set_text(this.connection.name);
    }

    private void update_autoconnect() {
        this.checkbutton_autoconnect.set_active(this.connection.autoconnect);
    }

    private void save_autoconnect() {
        this.connection.autoconnect = this.checkbutton_autoconnect.get_active();
    }

    private void update_security() {
        string security ="";

        /* security */
        switch (this.connection.security) {
        case Barman.ServiceSecurity.NONE:
            security = "None";
            break;
        case Barman.ServiceSecurity.WEP:
            security = "WEP";
            break;
        case Barman.ServiceSecurity.PSK:
            security = "WPA-PSK";
            break;
        case Barman.ServiceSecurity.IEEE8021X:
            security = "WPA-Enterprise";
            break;
        }

        this.label_security.set_text(security);
    }

    private void update_passphrase() {
        this.entry_passphrase.set_text(this.connection.passphrase);
    }

    private void save_passphrase() {
        this.connection.passphrase = this.entry_passphrase.get_text();
    }

    private void update_ipv4() {
        var ipv4 = this.connection.get_ipv4_configuration();
        Gtk.TreeIter iter;

        switch (ipv4.method) {
        case Barman.IPv4Method.OFF:
            iter = this.ipv4_off_iter;
            break;
        case Barman.IPv4Method.MANUAL:
            iter = this.ipv4_manual_iter;
            this.entry_ipv4_address.text = ipv4.address;
            this.entry_ipv4_netmask.text = ipv4.netmask;
            this.entry_ipv4_gateway.text = ipv4.gateway;
            break;
        case Barman.IPv4Method.DHCP:
        default:
            iter = this.ipv4_dhcp_iter;
            break;
        }

        this.combobox_ipv4_method.set_active_iter(iter);
    }

    private void save_ipv4() {
        Gtk.TreeIter iter;
        Barman.IPv4Method method;
        Barman.IPv4 ipv4;

        var store = this.combobox_ipv4_method.model as Gtk.ListStore;

        this.combobox_ipv4_method.get_active_iter(out iter);
        store.get(iter, 0, out method);

        var address = this.entry_ipv4_address.text;
        var netmask = this.entry_ipv4_netmask.text;
        var gateway = this.entry_ipv4_gateway.text;

        try {
            ipv4 = new Barman.IPv4(method, address, netmask, gateway);
            this.connection.set_ipv4_configuration(ipv4);
        } catch (GLib.Error e) {
            stderr.printf("Failed to create ipv4 object: %s", e.message);
        }
    }

    private void update_ipv6() {
        var ipv6 = this.connection.get_ipv6_configuration();
        Gtk.TreeIter iter;

        switch (ipv6.method) {
        case Barman.IPv6Method.OFF:
            iter = this.ipv6_off_iter;
            break;
        case Barman.IPv6Method.MANUAL:
            iter = this.ipv6_manual_iter;
            this.entry_ipv6_address.text = ipv6.address;
            this.entry_ipv6_prefix_length.text = ipv6.prefix_length.to_string();
            this.entry_ipv6_gateway.text = ipv6.gateway;
            break;
        case Barman.IPv6Method.AUTO:
        default:
            iter = this.ipv6_auto_iter;
            break;
        }

        this.combobox_ipv6_method.set_active_iter(iter);
    }

    private void save_ipv6() {
        Gtk.TreeIter iter;
        Barman.IPv6Method method;
        Barman.IPv6 ipv6;

        var store = this.combobox_ipv6_method.model as Gtk.ListStore;

        this.combobox_ipv6_method.get_active_iter(out iter);
        store.get(iter, 0, out method);

        var address = this.entry_ipv6_address.text;
        uint8 prefix_length = (uint8) uint64.parse(this.entry_ipv6_prefix_length.text);
        var gateway = this.entry_ipv6_gateway.text;

        try {
            ipv6 = new Barman.IPv6(method, address, prefix_length, gateway);
            this.connection.set_ipv6_configuration(ipv6);
        } catch (GLib.Error e) {
            stderr.printf("Failed to create ipv6 object: %s", e.message);
        }
    }

    private void update_nameservers() {
        string s = string.joinv(" ", this.connection.get_nameservers_configuration());
        this.entry_nameservers.set_text(s);
    }

    private void save_nameservers() {
        string[] array = this.entry_nameservers.get_text().split(" ");
        string[] servers = null;

        foreach (var server in array) {
            servers += server.strip();
        }

        this.connection.set_nameservers_configuration(servers);
    }

    private void update_domains() {
        string s = string.joinv(" ", this.connection.get_domains_configuration());
        this.entry_domains.set_text(s);
    }

    private void save_domains() {
        string[] array = this.entry_domains.get_text().split(" ");
        string[] domains = null;

        foreach (var server in array) {
            domains += server.strip();
        }

        this.connection.set_domains_configuration(domains);
    }

    private void update_widget_values() {
        switch (this.connection.get_service_type()) {
        case Barman.ServiceType.WIFI:
            update_name();
            update_autoconnect();
            update_mode();
            update_security();
            update_passphrase();
            update_ipv4();
            update_ipv6();
            update_nameservers();
            update_domains();
            break;
        case Barman.ServiceType.ETHERNET:
            update_name();
            update_autoconnect();
            update_ipv4();
            update_ipv6();
            update_nameservers();
            update_domains();
            break;
        case Barman.ServiceType.BLUETOOTH:
            update_name();
            update_autoconnect();
            update_ipv4();
            update_ipv6();
            update_nameservers();
            update_domains();
            break;
        }
    }

    private void save_widget_values() {
        switch (this.connection.get_service_type()) {
        case Barman.ServiceType.WIFI:
            save_autoconnect();
            save_passphrase();
            save_ipv4();
            save_ipv6();
            save_nameservers();
            save_domains();
            break;
        case Barman.ServiceType.ETHERNET:
            save_autoconnect();
            save_ipv4();
            save_ipv6();
            save_nameservers();
            save_domains();
            break;
        case Barman.ServiceType.BLUETOOTH:
            save_autoconnect();
            save_ipv4();
            save_ipv6();
            save_nameservers();
            save_domains();
            break;
        }
    }

    private void on_button_save_clicked() {
        this.save_widget_values();
        this.dialog.destroy();
        this.response(Gtk.ResponseType.APPLY);
    }

    private void on_button_cancel_clicked() {
        this.dialog.destroy();
        this.response(Gtk.ResponseType.CANCEL);
    }

    public int run() {
        this.dialog.show_all();
        return 0;
    }
}
