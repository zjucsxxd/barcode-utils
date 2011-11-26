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

// Setup command-line parsing and logging
static Logger logger;
bool debug = false;
const OptionEntry[] options = {
    { "debug", 'd', 0, OptionArg.NONE,
	  ref debug, ("Print extra information, useful for debugging"), null },
    { null }
};

/* avoid "You must define GETTEXT_PACKAGE before including gi18n-lib.h" */
private const string dummy = Config.PACKAGE_VERSION;

public static int main (string[] args) {
    
    Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
    Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain(Config.GETTEXT_PACKAGE);

    try {
        Gtk.init_with_args(ref args, "", options,
						   "indicator-barcode-settings");
    } catch (Error e) {
        stdout.printf("%s\n", e.message);
        stdout.printf(("Run '%s --help' to see a full list of available command line options.\n"), args[0]);
        return 1;
    }

    logger = new Logger();
    if (debug) {
        logger.set("level", LogLevel.DEBUG);
    } else {
        logger.set("level", LogLevel.INFO);
    }

    var datadir = Config.PKGDATADIR + "/";

    if (File.new_for_path("./ui/main.ui").query_exists()) {
        logger.info("Using data files from current directory");
        datadir = "./";
    }

    var app = new BarcodeSettings(datadir);
    assert(app is BarcodeSettings);

    Gtk.main();
    return 0;
}
