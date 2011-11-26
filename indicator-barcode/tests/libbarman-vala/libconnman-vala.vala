/*
 * indicator-barcode - user interface for barman
 * Copyright 2011 Canonical Ltd.
 *
 * Authors:
 * Kalle Valo <kalle.valo@canonical.com>
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

using Barman;

static Barman.Manager manager;
static MainLoop loop;

/*
 * Because readline blocks and we cannot block in the main thread due
 * to gmainloop, we need to have a separate thread for handinling
 * input from the user. Commands from that thread are passed through
 * cmd_queue to the main thread.
 *
 * Main thread handles printing the output to stdout normally.
 */

static AsyncQueue<string> cmd_queue;

static void quit() {
	loop.quit();
}

static void list() {
	Barman.Service[] services = manager.get_services();

    foreach (var service in services) {
		stdout.printf("%s\t%s\n", service.get_name(), service.get_path());
	}

	stdout.printf("%d services\n", services.length);
}

static void default_service() {
	var service = manager.get_default_service();
	stdout.printf(service.get_name());
}

static void status() {
	stdout.printf("%s\n",
				  manager.connected ? "connected" : "disconnected");
}

static void show(string[] args) {

	if (args.length != 1) {
		stdout.printf("Invalid number of arguments: %d\n", args.length);
		return;
	}

	var path = args[0];

	var service = manager.get_service(path);

	if (service == null) {
		stdout.printf("service %s not found", path);
		return;
	}

	/* service.type will call service_get_type() which is not what we want */
	var type = service.get_service_type();

	stdout.printf("[ %s ]\n", service.path);
	stdout.printf("  Name = %s\n", service.name);
	stdout.printf("  State = %u\n", service.state);

	if (type == ServiceType.ETHERNET) {
		stdout.printf("  Type = Ethernet\n");
	} else if (type == ServiceType.WIFI) {
		stdout.printf("  Type = Wifi\n");
		stdout.printf("  Security = %u\n", service.security);
		stdout.printf("  Strength = %u\n", service.strength);
		stdout.printf("  Passphrase = %s\n", service.passphrase);
		stdout.printf("  PassphraseRequired = %s\n",
					  service.passphrase_required ? "true" : "false");
	} else if (type == ServiceType.CELLULAR) {
		stdout.printf("  Type = Cellular\n");
		stdout.printf("  SetupRequired = %s\n",
					  service.setup_required ? "true" : "false");
		stdout.printf("  APN = %s\n", service.apn);
		stdout.printf("  MCC = %s\n", service.mcc);
		stdout.printf("  MNC = %s\n", service.mnc);
	}

	var ipv4 = service.ipv4;
	stdout.printf("  IPv4 = { Method=%s Address=%s Netmask=%s Gateway=%s\n }\n",
				  ipv4.get_method_as_string(), ipv4.address, ipv4.netmask,
				  ipv4.gateway);

	ipv4 = service.ipv4_configuration;
	stdout.printf("  IPv4.Configuration = " +
				  "{ Method=%s Address=%s Netmask=%s Gateway=%s\n }\n",
				  ipv4.get_method_as_string(), ipv4.address, ipv4.netmask,
				  ipv4.gateway);

	stdout.printf("  Nameservers = %s\n",
				  string.joinv(" ", service.get_nameservers()));
	stdout.printf("  Nameservers.Configuration = %s\n",
				  string.joinv(" ", service.get_nameservers_configuration()));
	stdout.printf("  Domains = %s\n",
				  string.joinv(" ", service.get_domains()));
	stdout.printf("  Domains.Configuration = %s\n",
				  string.joinv(" ", service.get_domains_configuration()));

	stdout.printf("  Error = %s\n", service.error);
	stdout.printf("  AutoConnect = %s\n",
				  service.autoconnect ? "true" : "false");
	stdout.printf("  Favorite = %s\n",
				  service.favorite ? "true" : "false");
	stdout.printf("  Immutable = %s\n",
				  service.immutable ? "true" : "false");
}

static void edit_passphrase(Service service, string[] args) {

	if (args.length != 1) {
		stdout.printf("Invalid number of arguments: %d\n", args.length);
		return;
	}

	var passphrase = args[0];
	service.passphrase = passphrase;
}

static void edit_nameservers(Service service, string[] args) {

	string[] nameservers;

	if (args.length < 1) {
		stdout.printf("Invalid number of arguments: %d\n", args.length);
		return;
	}

	if (args[0] != "auto")
		nameservers = args;
	else
		nameservers = {};

	service.set_nameservers_configuration(nameservers);
}

static void edit_domains(Service service, string[] args) {

	string[] domains;

	if (args.length < 1) {
		stdout.printf("Invalid number of arguments: %d\n", args.length);
		return;
	}

	if (args[0] != "auto")
		domains = args;
	else
		domains = {};

	service.set_domains_configuration(domains);
}

static void edit_ipv4(Service service, string[] args) {
	string method, address, netmask, gateway;

	if (args.length < 1) {
		stdout.printf("Invalid number of arguments: %d\n", args.length);
		return;
	}

	method = args[0];

	switch (method) {
	case "dhcp":
	case "off":
		address = "";
	    netmask = "";
	    gateway = "";
		break;
	case "manual":
		if (args.length < 4) {
			stdout.printf("Invalid number of ipv4 arguments: %d\n", args.length);			return;
		}
		address = args[1];
		netmask = args[2];
		gateway = args[3];
		break;
	default:
		stdout.printf("Unknown ipv4 method: %s\n", method);
		return;
	}
	
	try {
		var ipv4 = new IPv4.with_strings(method, address, netmask, gateway);
		service.set_ipv4_configuration(ipv4);
	} catch(GLib.Error e) {
		stdout.printf("Failed to set ipv4 address: %s", e.message);
		return;
	}
}

static void edit(string[] args) {

	if (args.length < 2) {
		stdout.printf("Invalid number of arguments: %d\n", args.length);
		return;
	}

	var path = args[0];
	var property = args[1];
	string[] edit_args = {};

	if (args.length == 3) {
		edit_args += args[2];
	}
	else if (args.length > 3) {
		edit_args = args[2: args.length];
	}

	var service = manager.get_service(path);

	if (service == null) {
		stdout.printf("service %s not found", path);
		return;
	}

	switch(property) {
	case "passphrase":
		edit_passphrase(service, edit_args);
		break;
	case "nameservers":
		edit_nameservers(service, edit_args);
		break;
	case "domains":
		edit_domains(service, edit_args);
		break;
	case "ipv4":
		edit_ipv4(service, edit_args);
		break;
	}
}

static void tech(string[] args) {
    stdout.printf("  wifi %d\n", manager.get_wifi_state());
    stdout.printf("  ethernet %d\n", manager.get_ethernet_state());
    stdout.printf("  cellular %d\n", manager.get_cellular_state());
    stdout.printf("  bluetooth %d\n", manager.get_bluetooth_state());
}

static Barman.TechnologyType str2tech(string s)
{
    switch (s) {
    case "wifi":
        return Barman.TechnologyType.WIFI;
    case "ethernet":
        return Barman.TechnologyType.ETHERNET;
    case "cellular":
        return Barman.TechnologyType.CELLULAR;
    case "bluetooth":
        return Barman.TechnologyType.BLUETOOTH;
    default:
        stdout.printf("Unknown tech: %s\n", s);
        return Barman.TechnologyType.UNKNOWN;
    }
}

static void enable(string[] args) {

	if (args.length != 1) {
		stdout.printf("Invalid number of arguments for enable: %d\n",
                      args.length);
		return;
	}

	var tech = str2tech(args[0]);

    manager.enable_technology(tech, null);
}

static void disable(string[] args) {

	if (args.length != 1) {
		stdout.printf("Invalid number of arguments for enable: %d\n",
                      args.length);
		return;
	}

	var tech = str2tech(args[0]);

    manager.disable_technology(tech, null);
}

static void offline(string[] args) {

	if (args.length != 1) {
		stdout.printf("Invalid number of arguments for offline: %d\n",
                      args.length);
		return;
	}

    var mode = args[0];
    bool value;

    switch (mode) {
    case "on":
        value = true;
        break;
    case "off":
        value = false;
        break;
    default:
        stdout.printf("Unknown mode for offline: %s\n", mode);
        return;
    }

    manager.offline_mode = value;
}

static bool cmd_handler() {
	var line = cmd_queue.pop();
	var tokens = line.split(" ");
	var cmd = tokens[0];
	string[] args = {};

	if (tokens.length == 2) {
		args += tokens[1];
	}
	if (tokens.length > 2) {
		args = tokens[1:tokens.length];
	}

	switch (cmd) {
	case "quit":
	case "q":
		quit();
		break;
	case "list":
		list();
		break;
	case "default-service":
		default_service();
		break;
	case "status":
		status();
		break;
	case "show":
		show(args);
		break;
	case "edit":
		edit(args);
		break;
	case "tech":
		tech(args);
		break;
	case "enable":
		enable(args);
		break;
	case "disable":
		disable(args);
		break;
	case "offline":
		offline(args);
		break;
	default:
		stdout.printf("Unknown command: %s\n", cmd);
		break;
	}

	return false;
}

static void *input_handler() {
	var quit = false;

    while (!quit) {
        var line = Readline.readline("> ");

        if (line == "") {
			    continue;
			  } else if (line == null) {
			    stdout.printf ("Stream closed\n");
			    line = "quit";
			  }

		Readline.History.add(line);
		cmd_queue.push(line);
		GLib.Idle.add(cmd_handler);
    }

	return null;
}

int main () {
	manager = new Barman.Manager();
	loop = new MainLoop();
	cmd_queue = new AsyncQueue<string>();

	if (!Thread.supported()) {
        stderr.printf("Cannot run without threads.\n");
        return 1;
    }

    try {
        Thread.create<void*>(input_handler, false);
    } catch (ThreadError e) {
        stderr.printf("Failed to create thread.\n");
		return 1;
    }

	manager.notify["connected"].connect((s, p) => {
			stdout.printf("%s\n", manager.get_connected() ? "connected" : "disconnected");
    });

	manager.notify["wifi-state"].connect((s, p) => {
			stdout.printf("wifi-state %d\n", manager.get_wifi_state());
    });

	manager.notify["ethernet-state"].connect((s, p) => {
			stdout.printf("ethernet-state %d\n", manager.get_ethernet_state());
    });

	manager.notify["cellular-state"].connect((s, p) => {
			stdout.printf("cellular-state %d\n", manager.get_cellular_state());
    });

	manager.notify["bluetooth-state"].connect((s, p) => {
			stdout.printf("bluetooth-state %d\n",
                          manager.get_bluetooth_state());
    });

	manager.notify["offline-mode"].connect((s, p) => {
			stdout.printf("offline-mode %s\n",
                          manager.offline_mode ? "on" : "off");
    });

	loop.run();

	return 0;
}
