/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- barcode scanner manager
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2011 Jakob Flierl
 */

#include <config.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <string.h>

#include "BarcodeManager.h"
#include "BarcodeManagerUtils.h"
#include "bm-manager.h"
#include "bm-system.h"
#include "bm-dbus-manager.h"
#include "bm-logging.h"

#if !defined(BM_DIST_VERSION)
# define BM_DIST_VERSION VERSION
#endif

#define BM_DEFAULT_PID_FILE          LOCALSTATEDIR"/run/BarcodeManager.pid"
#define BM_DEFAULT_SYSTEM_CONF_FILE  SYSCONFDIR"/BarcodeManager/BarcodeManager.conf"
#define BM_OLD_SYSTEM_CONF_FILE      SYSCONFDIR"/BarcodeManager/bm-system-settings.conf"
#define BM_DEFAULT_SYSTEM_STATE_FILE LOCALSTATEDIR"/lib/BarcodeManager/BarcodeManager.state"

/*
 * Globals
 */
static BMManager *manager = NULL;
static GMainLoop *main_loop = NULL;

typedef struct {
	time_t time;
	GQuark domain;
	guint32 code;
	guint32 count;
} MonitorInfo;

static gboolean quit_early = FALSE;

static void
bm_signal_handler (int signo)
{
	static int in_fatal = 0;

	/* avoid loops */
	if (in_fatal > 0)
		return;
	++in_fatal;

	switch (signo)
	{
		case SIGSEGV:
		case SIGBUS:
		case SIGILL:
		case SIGABRT:
			bm_log_warn (LOGD_CORE, "caught signal %d. Generating backtrace...", signo);
			bm_logging_backtrace ();
			exit (1);
			break;

		case SIGFPE:
		case SIGPIPE:
			/* let the fatal signals interrupt us */
			--in_fatal;

			bm_log_warn (LOGD_CORE, "caught signal %d, shutting down abnormally. Generating backtrace...", signo);
			bm_logging_backtrace ();
			g_main_loop_quit (main_loop);
			break;

		case SIGINT:
		case SIGTERM:
			/* let the fatal signals interrupt us */
			--in_fatal;

			bm_log_info (LOGD_CORE, "caught signal %d, shutting down normally.", signo);
			quit_early = TRUE;
			g_main_loop_quit (main_loop);
			break;

		case SIGHUP:
			--in_fatal;
			/* FIXME:
			 * Reread config stuff like system config files, VPN service files, etc
			 */
			break;

		case SIGUSR1:
			--in_fatal;
			/* FIXME:
			 * Play with log levels or something
			 */
			break;

		default:
			signal (signo, bm_signal_handler);
			break;
	}
}

static void
setup_signals (void)
{
	struct sigaction action;
	sigset_t mask;

	sigemptyset (&mask);
	action.sa_handler = bm_signal_handler;
	action.sa_mask = mask;
	action.sa_flags = 0;
	sigaction (SIGTERM,  &action, NULL);
	sigaction (SIGINT,  &action, NULL);
	sigaction (SIGILL,  &action, NULL);
	sigaction (SIGBUS,  &action, NULL);
	sigaction (SIGFPE,  &action, NULL);
	sigaction (SIGHUP,  &action, NULL);
	sigaction (SIGSEGV, &action, NULL);
	sigaction (SIGABRT, &action, NULL);
	sigaction (SIGUSR1,  &action, NULL);
}

static gboolean
write_pidfile (const char *pidfile)
{
 	char pid[16];
	int fd;
	gboolean success = FALSE;
 
	if ((fd = open (pidfile, O_CREAT|O_WRONLY|O_TRUNC, 00644)) < 0) {
		fprintf (stderr, "Opening %s failed: %s\n", pidfile, strerror (errno));
		return FALSE;
	}

 	snprintf (pid, sizeof (pid), "%d", getpid ());
	if (write (fd, pid, strlen (pid)) < 0)
		fprintf (stderr, "Writing to %s failed: %s\n", pidfile, strerror (errno));
	else
		success = TRUE;

	if (close (fd))
		fprintf (stderr, "Closing %s failed: %s\n", pidfile, strerror (errno));

	return success;
}

/* Check whether the pidfile already exists and contains PID of a running BarcodeManager
 *  Returns:  FALSE - specified pidfile doesn't exist or doesn't contain PID of a running NM process
 *            TRUE  - specified pidfile already exists and contains PID of a running NM process
 */
static gboolean
check_pidfile (const char *pidfile)
{
	char *contents = NULL;
	gsize len = 0;
	glong pid;
	char *proc_cmdline = NULL;
	gboolean bm_running = FALSE;
	const char *process_name;

	if (!g_file_get_contents (pidfile, &contents, &len, NULL))
		return FALSE;

	if (len <= 0)
		goto done;

	errno = 0;
	pid = strtol (contents, NULL, 10);
	if (pid <= 0 || pid > 65536 || errno)
		goto done;

	g_free (contents);
	proc_cmdline = g_strdup_printf ("/proc/%ld/cmdline", pid);
	if (!g_file_get_contents (proc_cmdline, &contents, &len, NULL))
		goto done;

	process_name = strrchr (contents, '/');
	if (process_name)
		process_name++;
	else
		process_name = contents;
	if (strcmp (process_name, "BarcodeManager") == 0) {
		/* Check that the process exists */
		if (kill (pid, 0) == 0) {
			fprintf (stderr, "BarcodeManager is already running (pid %ld)\n", pid);
			bm_running = TRUE;
		}
	}

done:
	g_free (proc_cmdline);
	g_free (contents);
	return bm_running;
}

static gboolean
parse_config_file (const char *filename,
                   char **plugins,
                   char **dhcp_client,
                   char ***dns_plugins,
                   char **log_level,
                   char **log_domains,
                   GError **error)
{
	GKeyFile *config;

	config = g_key_file_new ();
	if (!config) {
		g_set_error (error, 0, 0,
		             "Not enough memory to load config file.");
		return FALSE;
	}

	g_key_file_set_list_separator (config, ',');
	if (!g_key_file_load_from_file (config, filename, G_KEY_FILE_NONE, error))
		return FALSE;

	*plugins = g_key_file_get_value (config, "main", "plugins", error);
	if (*error)
		return FALSE;

	*dhcp_client = g_key_file_get_value (config, "main", "dhcp", NULL);
	*dns_plugins = g_key_file_get_string_list (config, "main", "dns", NULL, NULL);

	*log_level = g_key_file_get_value (config, "logging", "level", NULL);
	*log_domains = g_key_file_get_value (config, "logging", "domains", NULL);

	g_key_file_free (config);
	return TRUE;
}

static gboolean
parse_state_file (const char *filename,
                  GError **error)
{
	GKeyFile *state_file;
	GError *tmp_error = NULL;

	state_file = g_key_file_new ();
	if (!state_file) {
		g_set_error (error, 0, 0,
		             "Not enough memory to load state file.");
		return FALSE;
	}

	g_key_file_set_list_separator (state_file, ',');
	if (!g_key_file_load_from_file (state_file, filename, G_KEY_FILE_KEEP_COMMENTS, &tmp_error)) {
		/* This is kinda ugly; create the file and directory if it doesn't
		 * exist yet.  We can't rely on distros necessarily creating the
		 * /var/lib/BarcodeManager for us since we have to ensure that
		 * users upgrading NM get this working too.
		 */
		if (   tmp_error->domain == G_FILE_ERROR
		    && tmp_error->code == G_FILE_ERROR_NOENT) {
			char *data, *dirname;
			gsize len = 0;
			gboolean ret = FALSE;

			/* try to create the directory if it doesn't exist */
			dirname = g_path_get_dirname (filename);
			errno = 0;
			if (mkdir (dirname, 0755) != 0) {
				if (errno != EEXIST) {
					g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES,
					             "Error creating state directory %s: %d", dirname, errno);
					g_free (dirname);
					return FALSE;
				}
			}
			g_free (dirname);

			data = g_key_file_to_data (state_file, &len, NULL);
			if (data)
				ret = g_file_set_contents (filename, data, len, error);
			g_free (data);

			return ret;
		} else {
			g_set_error_literal (error, tmp_error->domain, tmp_error->code, tmp_error->message);
			g_clear_error (&tmp_error);
		}

		/* Otherwise, file probably corrupt or inaccessible */
		return FALSE;
	}

	g_key_file_free (state_file);

	return TRUE;
}

/*
 * main
 *
 */
int
main (int argc, char *argv[])
{
	GOptionContext *opt_ctx = NULL;
	gboolean become_daemon = FALSE;
	gboolean g_fatal_warnings = FALSE;
	char *pidfile = NULL, *state_file = NULL, *dhcp = NULL;
	char *config = NULL, *plugins = NULL, *conf_plugins = NULL;
	char *log_level = NULL, *log_domains = NULL;
	char **dns = NULL;
	gboolean success;
	NMDBusManager *dbus_mgr = NULL;
	GError *error = NULL;
	gboolean wrote_pidfile = FALSE;
	char *cfg_log_level = NULL, *cfg_log_domains = NULL;

	GOptionEntry options[] = {
		{ "no-daemon", 0, 0, G_OPTION_ARG_NONE, &become_daemon, "Don't become a daemon", NULL },
		{ "g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, "Make all warnings fatal", NULL },
		{ "pid-file", 0, 0, G_OPTION_ARG_FILENAME, &pidfile, "Specify the location of a PID file", "filename" },
		{ "state-file", 0, 0, G_OPTION_ARG_FILENAME, &state_file, "State file location", "/path/to/state.file" },
		{ "config", 0, 0, G_OPTION_ARG_FILENAME, &config, "Config file location", "/path/to/config.file" },
		{ "plugins", 0, 0, G_OPTION_ARG_STRING, &plugins, "List of plugins separated by ','", "plugin1,plugin2" },
		{ "log-level", 0, 0, G_OPTION_ARG_STRING, &log_level, "Log level: one of [ERR, WARN, INFO, DEBUG]", "INFO" },
		{ "log-domains", 0, 0, G_OPTION_ARG_STRING, &log_domains,
		        "Log domains separated by ',': any combination of [NONE,HW,BT,USER_SET,SYS_SET,SUSPEND,CORE,DEVICE]",
		        "HW" },
		{NULL}
	};

	if (getuid () != 0) {
		fprintf (stderr, "You must be root to run BarcodeManager!\n");
		exit (1);
	}

	if (!g_module_supported ()) {
		fprintf (stderr, "GModules are not supported on your platform!\n");
		exit (1);
	}

	bindtextdomain (GETTEXT_PACKAGE, BMLOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Parse options */
	opt_ctx = g_option_context_new ("");
	g_option_context_set_translation_domain (opt_ctx, "UTF-8");
	g_option_context_set_ignore_unknown_options (opt_ctx, FALSE);
	g_option_context_set_help_enabled (opt_ctx, TRUE);
	g_option_context_add_main_entries (opt_ctx, options, NULL);

	g_option_context_set_summary (opt_ctx,
		"BarcodeManager monitors all barcode scanners automatically.");

	success = g_option_context_parse (opt_ctx, &argc, &argv, NULL);
	g_option_context_free (opt_ctx);

	if (!success) {
		fprintf (stderr, _("Invalid option.  Please use --help to see a list of valid options.\n"));
		exit (1);
	}

	/* Make GIO ignore the remote VFS service; otherwise it tries to use the
	 * session bus to contact the remote service, and NM shouldn't ever be
	 * talking on the session bus.  See rh #588745
	 */
	setenv ("GIO_USE_VFS", "local", 1);

	pidfile = pidfile ? pidfile : g_strdup (BM_DEFAULT_PID_FILE);
	state_file = state_file ? state_file : g_strdup (BM_DEFAULT_SYSTEM_STATE_FILE);

	/* check pid file */
	if (check_pidfile (pidfile))
		exit (1);

	/* Parse the config file */
	if (config) {
		if (!parse_config_file (config, &conf_plugins, &dhcp, &dns, &cfg_log_level, &cfg_log_domains, &error)) {
			fprintf (stderr, "Config file %s invalid: (%d) %s\n",
			         config,
			         error ? error->code : -1,
			         (error && error->message) ? error->message : "unknown");
			exit (1);
		}
	} else {
		gboolean parsed = FALSE;

		/* Even though we prefer BarcodeManager.conf, we need to check the
		 * old bm-system-settings.conf first to preserve compat with older
		 * setups.  In package managed systems dropping a BarcodeManager.conf
		 * onto the system would make NM use it instead of bm-system-settings.conf,
		 * changing behavior during an upgrade.  We don't want that.
		 */

		/* Try deprecated bm-system-settings.conf first */
		if (g_file_test (BM_OLD_SYSTEM_CONF_FILE, G_FILE_TEST_EXISTS)) {
			config = g_strdup (BM_OLD_SYSTEM_CONF_FILE);
			parsed = parse_config_file (config, &conf_plugins, &dhcp, &dns, &cfg_log_level, &cfg_log_domains, &error);
			if (!parsed) {
				fprintf (stderr, "Default config file %s invalid: (%d) %s\n",
				         config,
				         error ? error->code : -1,
				         (error && error->message) ? error->message : "unknown");
				g_free (config);
				config = NULL;
				g_clear_error (&error);
			}
		}

		/* Try the preferred BarcodeManager.conf last */
		if (!parsed && g_file_test (BM_DEFAULT_SYSTEM_CONF_FILE, G_FILE_TEST_EXISTS)) {
			config = g_strdup (BM_DEFAULT_SYSTEM_CONF_FILE);
			parsed = parse_config_file (config, &conf_plugins, &dhcp, &dns, &cfg_log_level, &cfg_log_domains, &error);
			if (!parsed) {
				fprintf (stderr, "Default config file %s invalid: (%d) %s\n",
				         config,
				         error ? error->code : -1,
				         (error && error->message) ? error->message : "unknown");
				g_free (config);
				config = NULL;
				g_clear_error (&error);
			}
		}
	}
	/* Logging setup */
	if (!bm_logging_setup (log_level ? log_level : cfg_log_level,
	                       log_domains ? log_domains : cfg_log_domains,
	                       &error)) {
		fprintf (stderr,
		         _("%s.  Please use --help to see a list of valid options.\n"),
		         error->message);
		exit (1);
	}

	/* Plugins specified with '--plugins' override those of config file */
	plugins = plugins ? plugins : g_strdup (conf_plugins);
	g_free (conf_plugins);

	/* Parse the state file */
	if (!parse_state_file (state_file, &error)) {
		fprintf (stderr, "State file %s parsing failed: (%d) %s\n",
		         state_file,
		         error ? error->code : -1,
		         (error && error->message) ? error->message : "unknown");
		/* Not a hard failure */
	}
	g_clear_error (&error);

	/* Tricky: become_daemon is FALSE by default, so unless it's TRUE because
	 * of a CLI option, it'll become TRUE after this
	 */
	become_daemon = !become_daemon;
	if (become_daemon) {
		if (daemon (0, 0) < 0) {
			int saved_errno;

			saved_errno = errno;
			fprintf (stderr, "Could not daemonize: %s [error %u]\n",
			         g_strerror (saved_errno),
			         saved_errno);
			exit (1);
		}
		if (write_pidfile (pidfile))
			wrote_pidfile = TRUE;
	}

	if (g_fatal_warnings) {
		GLogLevelFlags fatal_mask;

		fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
		fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
		g_log_set_always_fatal (fatal_mask);
	}

	/*
	 * Set the umask to 0022, which results in 0666 & ~0022 = 0644.
	 * Otherwise, if root (or an su'ing user) has a wacky umask, we could
	 * write out an unreadable resolv.conf.
	 */
	umask (022);

	g_type_init ();
	if (!g_thread_supported ())
		g_thread_init (NULL);
	dbus_g_thread_init ();

#ifndef HAVE_DBUS_GLIB_DISABLE_LEGACY_PROP_ACCESS
#error HAVE_DBUS_GLIB_DISABLE_LEGACY_PROP_ACCESS not defined
#endif

#if HAVE_DBUS_GLIB_DISABLE_LEGACY_PROP_ACCESS
	/* Ensure that non-exported properties don't leak out, and that the
	 * introspection 'access' permissions are respected.
	 */
	dbus_glib_global_set_disable_legacy_property_access ();
#endif

	setup_signals ();

	bm_logging_start (become_daemon);

	bm_log_info (LOGD_CORE, "BarcodeManager (version " BM_DIST_VERSION ") is starting...");
	success = FALSE;

	if (config)
		bm_log_info (LOGD_CORE, "Read config file %s", config);

	main_loop = g_main_loop_new (NULL, FALSE);

	/* Initialize our DBus service & connection */
	dbus_mgr = bm_dbus_manager_get ();

	manager = bm_manager_get (config, plugins, state_file, &error);
	if (manager == NULL) {
		bm_log_err (LOGD_CORE, "failed to initialize the barcode manager: %s",
		          error && error->message ? error->message : "(unknown)");
		goto done;
	}

	/* Start our DBus service */
	if (!bm_dbus_manager_start_service (dbus_mgr)) {
		bm_log_err (LOGD_CORE, "failed to start the dbus service.");
		goto done;
	}

	bm_manager_start (manager);

	success = TRUE;

	/* Told to quit before getting to the mainloop by the signal handler */
	if (quit_early == TRUE)
		goto done;

	g_main_loop_run (main_loop);

done:
	if (manager)
		g_object_unref (manager);

	bm_logging_shutdown ();

	if (pidfile && wrote_pidfile)
		unlink (pidfile);

	/* Free options */
	g_free (pidfile);
	g_free (state_file);
	g_free (config);
	g_free (plugins);
	g_free (dhcp);
	g_strfreev (dns);
	g_free (log_level);
	g_free (log_domains);
	g_free (cfg_log_level);
	g_free (cfg_log_domains);

	bm_log_info (LOGD_CORE, "exiting (%s)", success ? "success" : "error");
	exit (success ? 0 : 1);
}
