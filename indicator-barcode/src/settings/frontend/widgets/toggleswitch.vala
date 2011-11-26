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

public class ToggleSwitch : Gtk.EventBox {

    /* A switch containing the values ON and OFF (or user-inputed) that
       hides the inactive value. It can be clicked or dragged to change
       state, or through an enter/return keypress.
       
       This is being used until the move to GTK+3, at which point we will
       switch (excuse the pun) to Gtk.Switch as it is better. */
    
    public signal void clicked();
    public signal void toggled();

    // (In relation to the height of the slider)
    private double LINE_HEIGHT_RATIO = 9.0/25.0;
    private double LINE_SPACING_RATIO = 5.0/38.0;
    private double CORNER_RADIUS = 3.0;

    // (In relation to the width/height of the text)
    private double TEXT_XPAD = 0.4;
    private double TEXT_YPAD = 0.35;

    private string[] values={"ON", "OFF"};
    private Pango.Layout? layout = null;
    private Atk.Object? atk = null;

    public bool active = false;
    
   public ToggleSwitch() {
        this.set_visible_window(false);
        this.app_paintable = true;
        this.double_buffered = false;

        // Set events for the widget to receive
        this.set_can_focus(true);
        this.set_events(Gdk.EventMask.KEY_PRESS_MASK|
                        Gdk.EventMask.ENTER_NOTIFY_MASK|
                        Gdk.EventMask.LEAVE_NOTIFY_MASK|
                        Gdk.EventMask.BUTTON_PRESS_MASK|
                        Gdk.EventMask.BUTTON_RELEASE_MASK);

        // Connect signls and callbacks
        this.style_set.connect(this.on_style_set);
        this.expose_event.connect(this.on_expose_event);
        this.button_press_event.connect(this.on_press);
        this.button_release_event.connect(this.on_release);
        this.key_release_event.connect(this.on_key_release);
        this.enter_notify_event.connect(this.on_enter);
        this.leave_notify_event.connect(this.on_leave);
        this.motion_notify_event.connect(this.on_motion_notify_event);

        // Calculate the best size
        int calc_width;
        int calc_height;
        this.calculate_size(out calc_width, out calc_height);
        this.set_size_request(calc_width, calc_height);

        this.update_atk();
    }

    private void calculate_size(out int width, out int height) {
        // Calculate the best size for the toggleswitch based on the
        // dimensions of the user's default font.
        this.layout = this.create_pango_layout(null);
        
        this.layout.set_text(this.values[0], -1);

        Pango.Rectangle extents;
        
        this.layout.get_pixel_extents(null, out extents);
        var text_width = extents.width;
        var text_height = extents.height;

        width = (int) ((text_width+(text_width*this.TEXT_XPAD*2))*2);
        height = (int) (text_height+(text_height*this.TEXT_YPAD*2));
    }

    
    private void draw_widget() {
        // Do the drawing of the actual widget.
        var cr = Gdk.cairo_create(this.window);

        // Draw the main background
        this.draw_background(cr);
        
        // Draw the text
        this.draw_text();

        // Draw the slider
        this.draw_slider(cr);
    }

    private void draw_background(Cairo.Context cr) {       
        int x = this.allocation.x;
        int y = this.allocation.y;
        int width = this.allocation.width;
        int height = this.allocation.height;

        var state = this.state;
        // Don't change colour of main background on hover or press
        if ((this.state == Gtk.StateType.PRELIGHT) | (this.state == Gtk.StateType.ACTIVE)) {
            state = Gtk.StateType.NORMAL;
        }

        // Get colours from theme
        var bg_fill = this.style.base[state];
        var bg_stroke = this.style.dark[state];
        var left_fill = this.style.light[Gtk.StateType.SELECTED];
        var left_stroke = this.style.mid[Gtk.StateType.SELECTED];
        var right_fill = this.style.base[state];
        var right_stroke = this.style.dark[state];

        // Draw the background of the whole widget
        this.draw_flat_rounded_rectangle(cr, x, y, width, height, bg_stroke, bg_fill);
        
        // Draw the left (highlighted) part
        this.draw_flat_rounded_rectangle(cr, x, y, width/2, height, left_stroke, left_fill);
        
        // Draw the right (normal) part
        this.draw_flat_rounded_rectangle(cr, x+width/2, y, width/2, height, right_stroke, right_fill);
    }
        
    private void draw_slider(Cairo.Context cr) {
        int x = this.allocation.x;
        int y = this.allocation.y;
        int width = this.allocation.width;
        int height = this.allocation.height;
        int x_offset = 0;

        if (this.active) {
            x_offset = width/2;
        }

        var stroke = this.style.dark[this.state];
        Gdk.Color fill1_rectangle;
        Gdk.Color fill2_rectangle;

        // Don't draw a gradient if insensitive
        if (this.state == Gtk.StateType.INSENSITIVE) {
            fill1_rectangle = this.style.bg[this.state];
            fill2_rectangle = this.style.bg[this.state];
        } else {
            fill1_rectangle = this.style.light[this.state];
            fill2_rectangle = this.style.mid[this.state];
        }

        // Draw the slider
        this.draw_rounded_rectangle(cr, x+x_offset, y, width/2, height, stroke, fill1_rectangle, fill2_rectangle);

        // Draw the vertical lines
        this.draw_vertical_lines(cr, x+x_offset, y, width, height);

    }

    private void draw_vertical_lines(Cairo.Context cr, int x, int y, int width, int height) {
        var fill_line_dark = this.style.dark[this.state];
        var fill_line_light = this.style.light[this.state];
        var line_height = (int) height*this.LINE_HEIGHT_RATIO;
        var line_spacing = (int) (this.LINE_SPACING_RATIO * (width / 2));
        
        var x_offset = ((width / 2) - (6+(line_spacing*2)))/2 + 2;
        var y_offset = (height  - line_height)/2;
                         
        double c_spacing = 0;
        for (int i=0; i<3; i++) {
            cr.rectangle(x+x_offset+c_spacing+1, y+y_offset, 1, line_height);
            cr.set_source_rgb((double) fill_line_dark.red / 65535.0,
                                (double) fill_line_dark.green / 65535.0,
                                (double) fill_line_dark.blue / 65535.0);
            cr.fill();
            cr.rectangle(x+x_offset+c_spacing, y+y_offset, 1, line_height);
            cr.set_source_rgb((double) fill_line_light.red / 65535.0,
                                (double) fill_line_light.green / 65535.0,
                                (double) fill_line_light.blue / 65535.0);
            cr.fill();
            c_spacing += line_spacing;
        }
    }

    private void draw_text() {
        this.draw_centered_text(this.values[0], "left");
        this.draw_centered_text(this.values[1], "right");
    }

    private void draw_centered_text(string text, string gravity) {
        if (this.layout == null) {
            this.layout = this.create_pango_layout(null);
        }

        int x = this.allocation.x;
        int y = this.allocation.y;
        int width = this.allocation.width;
        int height = this.allocation.height;

        int x_offset;
        int y_offset;
        Pango.Rectangle extents;

        this.layout.set_text(text, -1);
        this.layout.get_pixel_extents(null, out extents);
        var text_width = extents.width;
        var text_height = extents.height;

        x_offset = (width/2 - text_width) / 2;
        
        if (gravity == "right") {
            x_offset += width/2;
        }
        
        y_offset = (height - text_height) / 2;
        
        this.style.draw_layout(this.window, this.get_state(), true,
                                {x, y, width, height},
                                this, "", x+x_offset, y+y_offset,
                                this.layout);
    }
        

    private void draw_rounded_rectangle(Cairo.Context cr, double x,
            double y, double w, double h, Gdk.Color stroke, Gdk.Color fill1, Gdk.Color fill2) {
                
        var r = this.CORNER_RADIUS;
        cr.new_sub_path();
        cr.arc(r+x, r+y, r, PI, 270*PI_OVER_180);
        cr.arc(x+w-r, r+y, r, 270*PI_OVER_180, 0);
        cr.arc(x+w-r, y+h-r, r, 0, 90*PI_OVER_180);
        cr.arc(r+x, y+h-r, r, 90*PI_OVER_180, PI);
        cr.close_path();
        
        cr.set_source_rgb((double) stroke.red / 65535.0,
                        (double) stroke.green / 65535.0,
                        (double) stroke.blue / 65535.0);  
        cr.stroke_preserve();

        var linear = new Cairo.Pattern.linear(x, y, x, y+h);
        linear.add_color_stop_rgba(0.00, (double) fill1.red / 65535.0,
                (double) fill1.green / 65535.0, (double) fill1.blue / 65535.0, 1);  
        linear.add_color_stop_rgba(0.8, (double) fill2.red / 65535.0,
                (double) fill2.green / 65535.0, (double) fill2.blue / 65535.0, 1);  
        cr.set_source(linear);
        cr.fill();
    }
    
    private void draw_flat_rounded_rectangle(Cairo.Context cr, double x,
            double y, double w, double h, Gdk.Color stroke, Gdk.Color fill) {
                
        var r = this.CORNER_RADIUS;
        cr.new_sub_path();
        cr.arc(r+x, r+y, r, PI, 270*PI_OVER_180);
        cr.arc(x+w-r, r+y, r, 270*PI_OVER_180, 0);
        cr.arc(x+w-r, y+h-r, r, 0, 90*PI_OVER_180);
        cr.arc(r+x, y+h-r, r, 90*PI_OVER_180, PI);
        cr.close_path();
        
        cr.set_source_rgb((double) stroke.red / 65535.0,
                        (double) stroke.green / 65535.0,
                        (double) stroke.blue / 65535.0);  
        cr.stroke_preserve();

        cr.set_source_rgb((double) fill.red / 65535.0,
                        (double) fill.green / 65535.0,
                        (double) fill.blue / 65535.0);  
        cr.fill();
    }

    private void update_atk() {
        // Accessibility info
        if (this.atk == null) {
            this.atk = this.get_accessible();
            this.atk.set_role(Atk.Role.CHECK_BOX);
        }
        
        // Update ATK description, based on state
        if (this.active) {
            this.atk.set_name(this.values[0]);
        } else {
            this.atk.set_name(this.values[1]);
        }
    }

    private void on_style_set(Gtk.Style? style) {
        int calc_width;
        int calc_height;
        
        this.calculate_size(out calc_width, out calc_height);
        this.set_size_request(calc_width, calc_height);
    }

    private bool on_expose_event(Gdk.EventExpose event) {
        this.draw_widget();
        return true;
    }

    private bool on_press(Gdk.EventButton event) {
        this.set_state(Gtk.StateType.ACTIVE);
        this.grab_focus();
        return false;
    }
    
    private bool on_release(Gdk.EventButton event) {
        this.set_state(Gtk.StateType.PRELIGHT);
        this.active = !this.active;
        this.toggled();
        return false;
    }
            
    private bool on_motion_notify_event(Gdk.EventMotion event) {
        this.queue_draw();
        return false;
    }
        
    private bool on_key_release(Gdk.EventKey event) {
        if (this.has_focus) {
            if ((Gdk.keyval_name(event.keyval) == "Return") | (Gdk.keyval_name(event.keyval) == "Enter")) {
                this.toggled();
            }
        }
        return false;
    }

    private bool on_enter(Gdk.EventCrossing event) {
        this.set_state(Gtk.StateType.PRELIGHT);
        return false;
    }

    private bool on_leave(Gdk.EventCrossing event) {
        this.set_state(Gtk.StateType.NORMAL);
		return false;
    }

    public bool get_active() {
        return this.active;
    }
        
    public void set_active(bool active) {
        this.active = active;
        this.queue_draw();
    }

}
