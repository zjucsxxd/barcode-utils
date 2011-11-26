/*
 * indicator-barcode - user interface for barman
 * Jakob Flierl <jakob.flierl@gmail.com>
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

public enum LogLevel {
	NONE,
    INFO,
    DEBUG
}

public class Logger : Object {
	
    public LogLevel level {get; set;}
	
    public void debug(string text) {
        if (this.level >= LogLevel.DEBUG) {
            var msg = @"DEBUG: $text\n";
            stdout.printf(msg);
        }
    }

    public void info(string text) {
        if (this.level >= LogLevel.INFO) {
            var msg = @"INFO: $text\n";
            stdout.printf(msg);
        }
    }
}
