/*
 * indicator-barcodee - user interface for barman
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

namespace Utils.Gui {
    static Gtk.Builder new_builder(string path) {
        var builder = new Gtk.Builder();
        try {
            builder.add_from_file(path);
        } catch (Error e) {
            logger.debug(@"Can't find $path.");
        }
        builder.set_translation_domain("indicator-barcode-settings");
        return builder;
    }
}
