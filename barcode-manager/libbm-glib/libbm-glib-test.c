/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access network status & information from glib applications
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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2008 Red Hat, Inc.
 */

#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bm-client.h"
#include "bm-device.h"
#include "bm-utils.h"
#include "bm-active-connection.h"

static gboolean
test_get_state (BMClient *client)
{
	guint state;

	state = bm_client_get_state (client);
	g_print ("Current state: %d\n", state);

	return TRUE;
}

static void
dump_device (BMDevice *device)
{
	const char *str;
	BMDeviceState state;

	str = bm_device_get_iface (device);
	g_print ("Interface: %s\n", str);

	str = bm_device_get_udi (device);
	g_print ("Udi: %s\n", str);

	str = bm_device_get_driver (device);
	g_print ("Driver: %s\n", str);

	str = bm_device_get_vendor (device);
	g_print ("Vendor: %s\n", str);

	str = bm_device_get_product (device);
	g_print ("Product: %s\n", str);

	state = bm_device_get_state (device);
	g_print ("State: %d\n", state);
}

static gboolean
test_devices (BMClient *client)
{
	const GPtrArray *devices;
	int i;

	devices = bm_client_get_devices (client);
	g_print ("Got devices:\n");
	if (!devices) {
		g_print ("  NONE\n");
		return TRUE;
	}

	for (i = 0; i < devices->len; i++) {
		BMDevice *device = g_ptr_array_index (devices, i);
		dump_device (device);
		g_print ("\n");
	}

	return TRUE;
}

static void
active_connections_changed (BMClient *client, GParamSpec *pspec, gpointer user_data)
{
	const GPtrArray *connections;
	int i, j;

	g_print ("Active connections changed:\n");
	connections = bm_client_get_active_connections (client);
	for (i = 0; connections && (i < connections->len); i++) {
		BMActiveConnection *connection;
		const GPtrArray *devices;

		connection = g_ptr_array_index (connections, i);
		g_print ("    %s\n", bm_object_get_path (BM_OBJECT (connection)));
		devices = bm_active_connection_get_devices (connection);
		for (j = 0; devices && j < devices->len; j++)
			g_print ("           %s\n", bm_device_get_udi (g_ptr_array_index (devices, j)));
	}
}

static void
show_active_connection_device (gpointer data, gpointer user_data)
{
	BMDevice *device = BM_DEVICE (data);

	g_print ("           %s\n", bm_device_get_udi (device));
}

static void
test_get_active_connections (BMClient *client)
{
	const GPtrArray *connections;
	int i;

	g_print ("Active connections:\n");
	connections = bm_client_get_active_connections (client);
	for (i = 0; connections && (i < connections->len); i++) {
		const GPtrArray *devices;

		g_print ("    %s\n", bm_object_get_path (g_ptr_array_index (connections, i)));
		devices = bm_active_connection_get_devices (g_ptr_array_index (connections, i));
		if (devices)
			g_ptr_array_foreach ((GPtrArray *) devices, show_active_connection_device, NULL);
	}
}

static void
device_state_changed (BMDevice *device, GParamSpec *pspec, gpointer user_data)
{
	g_print ("Device state changed: %s %d\n",
	         bm_device_get_iface (device),
	         bm_device_get_state (device));
}

static void
device_added_cb (BMClient *client, BMDevice *device, gpointer user_data)
{
	g_print ("New device added\n");
	dump_device (device);
	g_signal_connect (G_OBJECT (device), "notify::state",
	                  (GCallback) device_state_changed, NULL);
}

static void
device_removed_cb (BMClient *client, BMDevice *device, gpointer user_data)
{
	g_print ("Device removed\n");
	dump_device (device);
}

static void
manager_running (BMClient *client, GParamSpec *pspec, gpointer user_data)
{
	if (bm_client_get_manager_running (client)) {
		g_print ("BM appeared\n");
		test_get_state (client);
		test_get_active_connections (client);
		test_devices (client);
	} else
		g_print ("BM disappeared\n");
}

static GMainLoop *loop = NULL;

static void
signal_handler (int signo)
{
	if (signo == SIGINT || signo == SIGTERM) {
		g_message ("Caught signal %d, shutting down...", signo);
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

int
main (int argc, char *argv[])
{
	BMClient *client;

	g_type_init ();

	client = bm_client_new ();
	if (!client) {
		exit (1);
	}

	g_signal_connect (client, "notify::" BM_CLIENT_MANAGER_RUNNING,
	                  G_CALLBACK (manager_running), NULL);
	g_signal_connect (client, "notify::" BM_CLIENT_ACTIVE_CONNECTIONS,
	                  G_CALLBACK (active_connections_changed), NULL);
	manager_running (client, NULL, NULL);

	g_signal_connect (client, "device-added",
					  G_CALLBACK (device_added_cb), NULL);
	g_signal_connect (client, "device-removed",
					  G_CALLBACK (device_removed_cb), NULL);

	loop = g_main_loop_new (NULL, FALSE);
	setup_signals ();
	g_main_loop_run (loop);

	g_object_unref (client);

	return 0;
}
