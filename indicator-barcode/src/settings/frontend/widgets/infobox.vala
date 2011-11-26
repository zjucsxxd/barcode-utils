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

class InfoBox : Gtk.HBox {
    
    /* A gtk.HBox that is filled with the mid-colour and stroked with
    the dark colour from a user's gtk theme so that it stands out.

    This is used instead of a gtk.InfoBar as this has a more subtle
    colour, and doesn't need an icon or button. */

    private const int YPAD = 1;
    private const int XPAD = 1;
    
    public InfoBox(bool homogeneous=false, int spacing=0) {
        this.set_homogeneous(homogeneous);
        this.set_spacing(spacing);
        this.expose_event.connect(this.on_expose_event);
    }

    public bool on_expose_event(Gdk.EventExpose event) {
        var cr = Gdk.cairo_create(window);

        // Get geometry
        var x = (int) (this.allocation.x + this.XPAD);
        var y = (int) (this.allocation.y + this.YPAD);
        var width = (int) (this.allocation.width - (this.XPAD * 2));
        var height = (int) (this.allocation.height - (this.YPAD * 2));

        // Fill with the mid color from the current gtk theme
        var fill = this.style.mid[this.state];
        // Fill with the dark color from the current gtk theme
        var stroke = this.style.dark[this.state];

        // Layout the rectangle
        cr.rectangle(x, y, width, height);
        // Stroke the outline
        cr.set_source_rgb((double) stroke.red / 65535.0,
                        (double) stroke.green / 65535.0,
                        (double) stroke.blue / 65535.0);
        cr.stroke_preserve();
        // Fill it in
        cr.set_source_rgb((double) fill.red / 65535.0,
                        (double) fill.green / 65535.0,
                        (double) fill.blue / 65535.0);  
        cr.fill();
        return false;
    }
}
