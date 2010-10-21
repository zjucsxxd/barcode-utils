#include <stdio.h>
#include <dbus/dbus.h>

#include <dbus/dbus-glib.h>
#include <glib.h>
 
static DBusHandlerResult dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data) {
  char *code;
 
  if ( dbus_message_is_signal (message, "me.koppi.BarcodeReader","read" ) ) {
    printf ("read '");

    DBusError error;

    dbus_error_init (&error);

    if (!dbus_message_get_args (message, &error, DBUS_TYPE_STRING, &code, DBUS_TYPE_INVALID)) {
      fprintf (stderr, "failed to get StatusChanged arguments: %s (%s)", error.message, error.name);
      dbus_error_free (&error);
    }

    printf ("%s'\n", code);

    return DBUS_HANDLER_RESULT_HANDLED;
  }
 
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main() {

  DBusConnection *connection;
  DBusError error;
  DBusMessage *message;
 
  const char *service_name = "me.koppi.BarcodeReader";

  dbus_uint32_t flag;
  dbus_bool_t result;

  GMainLoop *loop;
  loop = g_main_loop_new(NULL,FALSE);

  dbus_error_init (&error);
 
  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
 
  if ( dbus_error_is_set (&error) ) {
    printf ("Error getting dbus connection: %s\n", error.message);
    dbus_error_free (&error);
    dbus_connection_unref(connection);

    return 0;
  }
 
  message = dbus_message_new_method_call ("org.freedesktop.DBus",
					  "/org/freedesktop/DBus",
					  "org.freedesktop.DBus",
					  "StartServiceByName");

  if (!message) {
    printf ("Error creating DBus message\n");
    dbus_connection_unref (connection);

    return 0;
  }

  /* We don't want to receive a reply */
  dbus_message_set_no_reply (message, TRUE);
 
  /* Append the argument to the message, must ends with DBUS_TYPE_INVALID*/
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING,
			    &service_name,
			    DBUS_TYPE_UINT32,
			    &flag,
			    DBUS_TYPE_INVALID);
  result = dbus_connection_send (connection, message, NULL);
 
  if (result) {
    printf ("Connected to the %s service\n", service_name);
  } else {
    printf ("Failed to activate the %s service\n", service_name);
  }

  dbus_bus_add_match (connection, "type=\'signal\',interface=\'me.koppi.BarcodeReader\'", NULL);
  dbus_connection_add_filter (connection, dbus_filter, loop, NULL);
 
  dbus_connection_setup_with_g_main (connection, NULL);
 
  g_main_loop_run (loop);

  dbus_message_unref(message);
  dbus_connection_unref(connection);

  return 0;
}
