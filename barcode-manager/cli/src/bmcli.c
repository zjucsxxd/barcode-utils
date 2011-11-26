/* bmcli - command-line tool to control BarcodeManager
 *
 * Jakob Flierl <jakob flierl@gmail.com>
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
 * (C) Copyright 2011 Jakob Flierl
 */

/* Generated configuration file */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <bm-client.h>
#include <bm-setting-connection.h>
#include <bm-remote-settings.h>
#include <bm-remote-settings-system.h>
#include <bm-settings-interface.h>
#include <bm-settings-connection-interface.h>

#include "bmcli.h"
#include "utils.h"
#include "connections.h"
#include "devices.h"
#include "network-manager.h"

#if defined(BM_DIST_VERSION)
# define BMCLI_VERSION BM_DIST_VERSION
#else
# define BMCLI_VERSION VERSION
#endif


typedef struct {
        BmCli *bmc;
	int argc;
	char **argv;
} ArgsInfo;

/* --- Global variables --- */
GMainLoop *loop = NULL;


static void
usage (const char *prog_name)
{
	fprintf (stderr,
	         _("Usage: %s [OPTIONS] OBJECT { COMMAND | help }\n\n"
	         "OPTIONS\n"
	         "  -t[erse]                                   terse output\n"
	         "  -p[retty]                                  pretty output\n"
	         "  -m[ode] tabular|multiline                  output mode\n"
	         "  -f[ields] <field1,field2,...>|all|common   specify fields to output\n"
	         "  -e[scape] yes|no                           escape columns separators in values\n"
	         "  -v[ersion]                                 show program version\n"
	         "  -h[elp]                                    print this help\n\n"
	         "OBJECT\n"
	         "  bm          BarcodeManager status\n"
	         "  con         BarcodeManager connections\n"
	         "  dev         devices managed by BarcodeManager\n\n"),
	          prog_name);
}

static BMCResultCode 
do_help (BmCli *bmc, int argc, char **argv)
{
	usage ("bmcli");
	return BMC_RESULT_SUCCESS;
}

static const struct cmd {
	const char *cmd;
	BMCResultCode (*func) (BmCli *bmmc, int argc, char **argv);
} bmcli_cmds[] = {
	{ "bm",         do_barcode_manager },
	{ "con",        do_connections },
	{ "dev",        do_devices },
	{ "help",       do_help },
	{ 0 }
};

static BMCResultCode
do_cmd (BmCli *bmc, const char *argv0, int argc, char **argv)
{
	const struct cmd *c;

	for (c = bmcli_cmds; c->cmd; ++c) {
		if (matches (argv0, c->cmd) == 0)
			return c->func (bmc, argc-1, argv+1);
	}

	g_string_printf (bmc->return_text, _("Error: Object '%s' is unknown, try 'bmcli help'."), argv0);
	bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
	return bmc->return_value;
}

static BMCResultCode
parse_command_line (BmCli *bmc, int argc, char **argv)
{
	char *base;

	base = strrchr (argv[0], '/');
	if (base == NULL)
		base = argv[0];
	else
		base++;

	/* parse options */
	while (argc > 1) {
		char *opt = argv[1];
		/* '--' ends options */
		if (strcmp (opt, "--") == 0) {
			argc--; argv++;
			break;
		}
		if (opt[0] != '-')
			break;
		if (opt[1] == '-')
			opt++;
		if (matches (opt, "-terse") == 0) {
			if (mc->print_output == BMC_PRINT_TERSE) {
				g_string_printf (bmc->return_text, _("Error: Option '--terse' is specified the second time."));
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			else if (bmc->print_output == BMC_PRINT_PRETTY) {
				g_string_printf (bmc->return_text, _("Error: Option '--terse' is mutually exclusive with '--pretty'."));
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			else
				bmc->print_output = BMC_PRINT_TERSE;
		} else if (matches (opt, "-pretty") == 0) {
			if (bmc->print_output == BMC_PRINT_PRETTY) {
				g_string_printf (bmc->return_text, _("Error: Option '--pretty' is specified the second time."));
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			else if (bmc->print_output == BMC_PRINT_TERSE) {
				g_string_printf (bmc->return_text, _("Error: Option '--pretty' is mutually exclusive with '--terse'."));
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			else
				bmc->print_output = BMC_PRINT_PRETTY;
		} else if (matches (opt, "-mode") == 0) {
			bmc->mode_specified = TRUE;
			next_arg (&argc, &argv);
			if (argc <= 1) {
		 		g_string_printf (bmc->return_text, _("Error: missing argument for '%s' option."), opt);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			if (!strcmp (argv[1], "tabular"))
				bmc->multiline_output = FALSE;
			else if (!strcmp (argv[1], "multiline"))
				bmc->multiline_output = TRUE;
			else {
		 		g_string_printf (bmc->return_text, _("Error: '%s' is not valid argument for '%s' option."), argv[1], opt);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
		} else if (matches (opt, "-escape") == 0) {
			next_arg (&argc, &argv);
			if (argc <= 1) {
		 		g_string_printf (bmc->return_text, _("Error: missing argument for '%s' option."), opt);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			if (!strcmp (argv[1], "yes"))
				bmc->escape_values = TRUE;
			else if (!strcmp (argv[1], "no"))
				bmc->escape_values = FALSE;
			else {
		 		g_string_printf (bmc->return_text, _("Error: '%s' is not valid argument for '%s' option."), argv[1], opt);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
		} else if (matches (opt, "-fields") == 0) {
			next_arg (&argc, &argv);
			if (argc <= 1) {
		 		g_string_printf (bmc->return_text, _("Error: fields for '%s' options are missing."), opt);
				bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
				return bmc->return_value;
			}
			bmc->required_fields = g_strdup (argv[1]);
		} else if (matches (opt, "-version") == 0) {
			printf (_("bmcli tool, version %s\n"), BMCLI_VERSION);
			return BMC_RESULT_SUCCESS;
		} else if (matches (opt, "-help") == 0) {
			usage (base);
			return BMC_RESULT_SUCCESS;
		} else {
			g_string_printf (bmc->return_text, _("Error: Option '%s' is unknown, try 'bmcli -help'."), opt);
			bmc->return_value = BMC_RESULT_ERROR_USER_INPUT;
			return bmc->return_value;
		}
		argc--;
		argv++;
	}

	if (argc > 1)
		return do_cmd (bmc, argv[1], argc-1, argv+1);

	usage (base);
	return bmc->return_value;
}

static void
signal_handler (int signo)
{
	if (signo == SIGINT || signo == SIGTERM) {
		g_message (_("Caught signal %d, shutting down..."), signo);
		g_main_loop_quit (loop);
	}
}

static void
setup_signals (void)
{
	struct sigaction action;
	sigset_t mask;

	sigemptyset (&mask);
	action.sa_handler = signal_handler;
	action.sa_mask = mask;
	action.sa_flags = 0;
	sigaction (SIGTERM,  &action, NULL);
	sigaction (SIGINT,  &action, NULL);
}

static BMClient *
bmc_get_client (BmCli *bmc)
{
	if (!bmc->client) {
		bmc->client = bm_client_new ();
		if (!bmc->client) {
			g_critical (_("Error: Could not create BMClient object."));
			exit (BMC_RESULT_ERROR_UNKNOWN);
		}
	}

	return bmc->client;
}

/* Initialize BmCli structure - set default values */
static void
bmc_init (BmCli *bmc)
{
	bmc->client = NULL;
	bmc->get_client = &bmc_get_client;

	bmc->return_value = BMC_RESULT_SUCCESS;
	bmc->return_text = g_string_new (_("Success"));

	bmc->timeout = 10;

	bmc->system_settings = NULL;
	bmc->user_settings = NULL;

	bmc->system_settings_running = FALSE;
	bmc->user_settings_running = FALSE;

	bmc->system_connections = NULL;
	bmc->user_connections = NULL;

	bmc->should_wait = FALSE;
	bmc->nowait_flag = TRUE;
	bmc->print_output = BMC_PRINT_NORMAL;
	bmc->multiline_output = FALSE;
	bmc->mode_specified = FALSE;
	bmc->escape_values = TRUE;
	bmc->required_fields = NULL;
	bmc->allowed_fields = NULL;
	memset (&bmc->print_fields, '\0', sizeof (BmcPrintFields));
}

static void
bmc_cleanup (BmCli *bmc)
{
	if (bmc->client) g_object_unref (bmc->client);

	g_string_free (bmc->return_text, TRUE);

	if (bmc->system_settings) g_object_unref (bmc->system_settings);
	if (bmc->user_settings) g_object_unref (bmc->user_settings);

	g_slist_free (bmc->system_connections);
	g_slist_free (bmc->user_connections);

	g_free (bmc->required_fields);
	if (bmc->print_fields.indices)
		g_array_free (bmc->print_fields.indices, TRUE);
}

static gboolean
start (gpointer data)
{
	ArgsInfo *info = (ArgsInfo *) data;
	info->bmc->return_value = parse_command_line (info->bmc, info->argc, info->argv);

	if (!info->bmc->should_wait)
		g_main_loop_quit (loop);

	return FALSE;
}


int
main (int argc, char *argv[])
{
	bmCli bmc;
	ArgsInfo args_info = { &bmc, argc, argv };

	/* Set locale to use environment variables */
	setlocale (LC_ALL, "");

#ifdef GETTEXT_PACKAGE
	/* Set i18n stuff */
	bindtextdomain (GETTEXT_PACKAGE, BMCLI_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	g_type_init ();

	bmc_init (&bmc);
	g_idle_add (start, &args_info);

	loop = g_main_loop_new (NULL, FALSE);  /* create main loop */
	setup_signals ();                      /* setup UNIX signals */
	g_main_loop_run (loop);                /* run main loop */

	/* Print result descripting text */
	if (bmc.return_value != BMC_RESULT_SUCCESS) {
		fprintf (stderr, "%s\n", bmc.return_text->str);
	}

	g_main_loop_unref (loop);
	bmc_cleanup (&bmc);

	return bmc.return_value;
}
