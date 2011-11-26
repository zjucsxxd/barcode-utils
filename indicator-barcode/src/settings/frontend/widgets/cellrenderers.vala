/*
 * indicator-barcode - user interface for barman
 * Copyright Jakob Flierl <jakob.flierl@gmail.com>
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

public const double PI = 3.1415926535897931;
public const double PI_OVER_180 = 0.017453292519943295;

class PinCellRenderer : Gtk.CellRenderer {

    public string color { get; set; }

    private const string RED_FILL = "#ef2929";
    private const string RED_STROKE = "#a40000";
    private const string GREEN_FILL = "#8ae234";
    private const string GREEN_STROKE = "#4e9a06";
    private const string YELLOW_FILL = "#fce94f";
    private const string YELLOW_STROKE = "#c4a000";
    private const string GREY_FILL = "#d3d7cf";
    private const string GREY_STROKE = "#555753";

    // Minimum size + (minimum padding times 2)
    private const int MIN_CELL_WIDTH = 8 + (4 * 2);
    private const int MIN_CELL_HEIGHT = 8 + (4 * 2);

    // Ratio of radius of pin to width/height of cell
    private const double PIN_RADIUS = 0.5;

    public PinCellRenderer() {
        GLib.Object ();
    }
    
    public override void get_size(Gtk.Widget widget, Gdk.Rectangle? cell_area,
                                   out int x_offset, out int y_offset,
                                   out int width, out int height) {
        if (&x_offset != null) x_offset = 0;
        if (&y_offset != null) y_offset = 0;
        if (&width != null) width = this.MIN_CELL_WIDTH;
        if (&height != null) height = this.MIN_CELL_HEIGHT;
    }

    public override void render(Gdk.Window window, Gtk.Widget widget,
                                    Gdk.Rectangle background_area,
                                    Gdk.Rectangle cell_area,
                                    Gdk.Rectangle expose_area,
                                    Gtk.CellRendererState flags) {
                                        
        var cr = Gdk.cairo_create(window);
        
        var x = cell_area.x;
        var y = cell_area.y;
        var width = cell_area.width;
        var height = cell_area.height;

        var lower_dimension = this.get_lower_dimension(width, height);
        var pin_radius = (int) lower_dimension * this.PIN_RADIUS;

        var x_offset = (width - lower_dimension) / 2;
        var y_offset = (height - lower_dimension) / 2;

        cr.arc(x+x_offset+pin_radius, y+y_offset+pin_radius, pin_radius/2, 0, 2 * Math.PI);

        var stroke_string = "";
        var fill_string = "";

        switch (this.color) {
            case "red":
                stroke_string = this.RED_STROKE;
                fill_string = this.RED_FILL;
                break;
            case "green":
                stroke_string = this.GREEN_STROKE;
                fill_string = this.GREEN_FILL;
                break;
            case "yellow":
                stroke_string = this.YELLOW_STROKE;
                fill_string = this.YELLOW_FILL;
                break;
            case "grey":
                stroke_string = this.GREY_STROKE;
                fill_string = this.GREY_FILL;
                break;
        }
        
        var stroke = Gdk.Color();
        stroke.parse(stroke_string, out stroke);
        var fill = Gdk.Color();
        fill.parse(fill_string, out fill);

        cr.set_source_rgb((double) stroke.red / 65535.0,
                        (double) stroke.green / 65535.0,
                        (double) stroke.blue / 65535.0);
        cr.stroke_preserve();

        cr.set_source_rgb((double) fill.red / 65535.0,
                        (double) fill.green / 65535.0,
                        (double) fill.blue / 65535.0);  
        cr.fill();
    }

    private float get_lower_dimension(float n1, float n2) {
        if (n1 > n2) {
            return n2;
        }
        return n1;
    }
}


class SignalStrengthCellRenderer : Gtk.CellRenderer {
    
    public double strength { get; set; }

    private Gtk.Widget style_widget;
    private Gtk.Widget state_widget;
    
    // Minimum size + (minimum padding times 2)
    private const int MIN_CELL_WIDTH = 20 + (5 * 2);
    private const int MIN_CELL_HEIGHT = 8 + (6 * 2);

    // Ratio of width/height of bar to width/height of cell
    private const double BAR_WIDTH = 0.8;
    private const double BAR_HEIGHT = 0.45;

    private const int CORNER_RADIUS = 3;

    public SignalStrengthCellRenderer(Gtk.Widget style_widget, Gtk.Widget state_widget) {
        GLib.Object();
        this.style_widget = style_widget;
        this.state_widget = state_widget;
    }

    public override void get_size(Gtk.Widget widget, Gdk.Rectangle? cell_area,
                                   out int x_offset, out int y_offset,
                                   out int width, out int height) {
        if (&x_offset != null) x_offset = 0;
        if (&y_offset != null) y_offset = 0;
        if (&width != null) width = this.MIN_CELL_WIDTH;
        if (&height != null) height = this.MIN_CELL_HEIGHT;
    }

    public override void render(Gdk.Window window, Gtk.Widget widget,
                                    Gdk.Rectangle background_area,
                                    Gdk.Rectangle cell_area,
                                    Gdk.Rectangle expose_area,
                                    Gtk.CellRendererState flags) {

        var cr = Gdk.cairo_create(window);
        
        var x = (int) cell_area.x;
        var y = (int) cell_area.y;
        var width = (int) cell_area.width;
        var height = (int) cell_area.height;
        
        var bar_width = (int) (width * this.BAR_WIDTH);
        var bar_height = (int) (height * this.BAR_HEIGHT);

        var x_offset = (int) ((width - bar_width) / 2);
        var y_offset = (int) ((height - bar_height) / 2);

        var state = this.state_widget.state;

        var bg_fill = this.style_widget.style.bg[state];
        var bg_stroke = this.style_widget.style.dark[state];
        
        var fg_fill = this.style_widget.style.mid[state];
        var fg_stroke = this.style_widget.style.dark[state];

        this.draw_rounded_rectangle(cr, x+x_offset, y+y_offset,
            x+x_offset+bar_width, y+y_offset+bar_height,
            bg_fill, bg_stroke);

        if (this.strength == 0) {
            stdout.printf("strength = 0\n\n");
        }
        
            
        this.draw_rounded_rectangle(cr, x+x_offset, y+y_offset,
            x+x_offset+((int) (bar_width*this.strength)), y+y_offset+bar_height,
            fg_fill, fg_stroke);
    }

    private void draw_rounded_rectangle(Cairo.Context cr, int x, int y,
                        int w, int h, Gdk.Color fill, Gdk.Color stroke) {
        this.layout_rounded_rectangle(cr, x, y, w, h);

        cr.set_source_rgb((double) stroke.red / 65535.0,
                        (double) stroke.green / 65535.0,
                        (double) stroke.blue / 65535.0);
        cr.stroke_preserve();
        
        cr.set_source_rgb((double) fill.red / 65535.0,
                        (double) fill.green / 65535.0,
                        (double) fill.blue / 65535.0);  
        cr.fill();
    }
        
    private void layout_rounded_rectangle(Cairo.Context cr, int x, int y,
                                            int w, int h) {
        var r = this.CORNER_RADIUS;

        cr.new_sub_path();
        cr.arc(r+x, r+y, r, PI, 270*PI_OVER_180);
        cr.arc(w-r, r+y, r, 270*PI_OVER_180, 0);
        cr.arc(w-r, h-r, r, 0, 90*PI_OVER_180);
        cr.arc(r+x, h-r, r, 90*PI_OVER_180, PI);
        cr.close_path();
    }
}
