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

public class BarcodeSettings : Object {

    // Widgets from GTKBuilder
    private Gtk.Window window_main;
    private Gtk.Notebook notebook_main;
    private Gtk.Alignment alignment_connections;
    private Gtk.ScrolledWindow scrolledwindow_c_devices;
    private Gtk.Notebook notebook_c_right;

    private Gtk.IconTheme icons = Gtk.IconTheme.get_default();
    private ConnectionsPage[] pages;
    public Gtk.Builder builder;

	private Barman.Manager barman;
	private string datadir;

    public BarcodeSettings(string datadir) {
        this.builder = Utils.Gui.new_builder(datadir + "/ui/main.ui");
        this.icons.append_search_path(
			Path.build_path(
				Path.DIR_SEPARATOR_S, datadir, "icons"));

        this.get_widgets();
        this.connect_signals();

		this.datadir = datadir;

		this.barman = new Barman.Manager();
		this.barman.notify["connected"].connect(this.barman_connected);
    }

	private void barman_connected() {
		if (this.barman.get_connected() == false)
			return;

        var page_connection = new ConnectionsPage(
			datadir, this.icons,
			this.alignment_connections,
			this.scrolledwindow_c_devices,
			this.notebook_c_right,
			this.barman);
                            
        this.pages = {page_connection};
        this.on_notebook_main_page_switched(null, 0);

		this.window_main.show_all();
	}

    /* Private Functions */
    private void get_widgets() {
        bool is_gtk_builder = (this.builder is Gtk.Builder);
        assert(is_gtk_builder);
        this.window_main = this.builder.get_object("window_main") as Gtk.Window;
        this.notebook_main = this.builder.get_object("notebook_main") as Gtk.Notebook;
        this.alignment_connections = this.builder.get_object("alignment_connections") as Gtk.Alignment;
        this.scrolledwindow_c_devices = this.builder.get_object("scrolledwindow_c_devices") as Gtk.ScrolledWindow;
        this.notebook_c_right = this.builder.get_object("notebook_c_right") as Gtk.Notebook;
    }

    private void connect_signals() {
        this.window_main.delete_event.connect(this.on_window_main_delete_event);
        this.notebook_main.switch_page.connect(this.on_notebook_main_page_switched);
    }
    
    /* Callbacks */
    public bool on_window_main_delete_event(Gdk.Event event) {
        Gtk.main_quit();
        return true;
    }
    
    public void on_notebook_main_page_switched(Gtk.NotebookPage? notebook_page, uint page_num) {
        var page_alignment = this.notebook_main.get_nth_page((int) page_num);

        foreach (ConnectionsPage page in this.pages) {
            if (page.alignment == page_alignment && page.viewed == false) {
                page.on_first_expose();
            }
        }
    }
}
