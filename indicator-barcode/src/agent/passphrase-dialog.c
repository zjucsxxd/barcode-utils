/*
 * indicator-barcode - user interface for barman
 * Copyright 2010 Canonical Ltd.
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

#include "passphrase-dialog.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "barman-agent-xml.h"
#include "barman.h"
#include "dbus-shared-names.h"

G_DEFINE_TYPE(PassphraseDialog, passphrase_dialog, GTK_TYPE_DIALOG);

typedef struct _PassphraseDialogPrivate PassphraseDialogPrivate;

struct _PassphraseDialogPrivate
{
  GtkWidget *entry;
  gchar *security_type;

  GDBusNodeInfo *node_info;
  guint owner_id;
  guint registration_id;
  GDBusConnection *connection;

  GDBusProxy *barman_proxy;

  GDBusMethodInvocation *invocation;
};

#define PASSPHRASE_DIALOG_GET_PRIVATE(o)				\
  (G_TYPE_INSTANCE_GET_PRIVATE((o), PASSPHRASE_DIALOG_TYPE,		\
			       PassphraseDialogPrivate))

#define NETWORK_AGENT_OBJECT "/indicator/agent"

#define WEP_40BIT_HEXKEY_LEN 10
#define WEP_104BIT_HEXKEY_LEN 26
#define WEP_40BIT_ASCIIKEY_LEN 5
#define WEP_104BIT_ASCIIKEY_LEN 13
#define WPA_MIN_LEN 8
#define WPA_MAX_LEN 64

static gboolean is_security_wpa(PassphraseDialog *self)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  if (g_strcmp0(priv->security_type, "wpa") == 0)
    return TRUE;
  else if (g_strcmp0(priv->security_type, "psk") == 0)
    return TRUE;
  else if (g_strcmp0(priv->security_type, "rsn") == 0)
    return TRUE;

  return FALSE;
}

static gboolean is_security_wep(PassphraseDialog *self)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  return g_strcmp0(priv->security_type, "wep") == 0;
}

static void register_agent_cb(GObject *source_object, GAsyncResult *res,
			      gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  GError *error = NULL;
  GVariant *result;

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->barman_proxy != NULL);

  result = g_dbus_proxy_call_finish(priv->barman_proxy, res, &error);

  if (error != NULL) {
    g_warning("Failed to register agent: %s", error->message);
    g_error_free(error);
    return;
  }

  if (result == NULL)
    return;

  g_variant_unref(result);
}


static void register_agent(PassphraseDialog *self)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  GVariant *parameters;

  g_return_if_fail(priv != NULL);
  g_return_if_fail(priv->barman_proxy != NULL);

  parameters = g_variant_new("(o)", NETWORK_AGENT_OBJECT);

  g_dbus_proxy_call(priv->barman_proxy, "RegisterAgent", parameters,
  		    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
  		    register_agent_cb, self);
}

static void create_barman_proxy_cb(GObject *source_object, GAsyncResult *res,
				    gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  GError *error = NULL;

  priv->barman_proxy = g_dbus_proxy_new_finish(res, &error);

  if (error != NULL) {
    g_warning("Failed to get barman proxy: %s", error->message);
    g_error_free(error);
    return;
  }

  if (priv->barman_proxy == NULL) {
    g_warning("Failed to get barman proxy, but no errors");
    return;
  }

  register_agent(self);
}

static void create_barman_proxy(PassphraseDialog *self)
{
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
			   NULL, BARMAN_SERVICE_NAME, "/",
			   BARMAN_MANAGER_INTERFACE, NULL,
			   create_barman_proxy_cb, self);
}

static void ask_passphrase(PassphraseDialog *self)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  g_debug("%s()", __func__);

  gtk_entry_set_text(GTK_ENTRY(priv->entry), "");

  gtk_widget_show_all(GTK_WIDGET(self));
}

static void parse_passphrase_dict(PassphraseDialog *self, GVariant *dict)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  GVariantIter iter;
  GVariant *value;
  gchar *key;

  g_variant_iter_init(&iter, dict);

  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    g_debug("key %s", key);

    if (g_strcmp0(key, "Type") == 0 &&
	g_variant_type_equal(G_VARIANT_TYPE_STRING,
			     g_variant_get_type(value))) {
      /* priv->security_type should be empty now */
      g_variant_get(value, "s", &priv->security_type);
    }

    g_variant_unref(value);
    g_free(key);
  }
}

static void method_request_input(PassphraseDialog *self,
				 GVariant *parameters,
				 GDBusMethodInvocation *invocation)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  GVariantIter iter, dict_iter;
  GVariant *dict, *value;
  gchar *path, *key;

  g_debug("%s()", __func__);

  g_return_if_fail(priv != NULL);

  if (priv->invocation != NULL) {
    g_warning("Ongoing request input event, droppping the new one.");

    /* FIXME: what should we really return now? An error? */
    g_dbus_method_invocation_return_value(invocation, NULL);
    return;
  }

  priv->invocation = invocation;

  g_variant_iter_init(&iter, parameters);
  g_variant_iter_next(&iter, "o", &path);
  dict = g_variant_iter_next_value(&iter);

  g_variant_iter_init(&dict_iter, dict);

  /* find security type */
  while (g_variant_iter_next(&dict_iter, "{sv}", &key, &value)) {
    g_debug("key %s", key);

    if (g_strcmp0(key, "Passphrase") == 0)
      parse_passphrase_dict(self, value);

    g_variant_unref(value);
    g_free(key);
  }

  g_debug("%s() %s %s", __func__, path, priv->security_type);

  ask_passphrase(self);

  g_free(path);
  g_variant_unref(dict);
}

static void method_release(PassphraseDialog *self, GVariant *parameters,
			   GDBusMethodInvocation *invocation)
{
  g_debug("%s()", __func__);

  /* barman has released us, most probably due because it's shutting down */

  /* FIXME: wait for barman to reappear and then re-register */
}

static void handle_method_call(GDBusConnection *connection,
			       const gchar *sender,
			       const gchar *object_path,
			       const gchar *interface_name,
			       const gchar *method_name,
			       GVariant *parameters,
			       GDBusMethodInvocation *invocation,
			       gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);

  g_return_if_fail(self != NULL);

  if (g_strcmp0 (method_name, "RequestInput") == 0)
    method_request_input(self, parameters, invocation);
  else if (g_strcmp0 (method_name, "Release") == 0)
    method_release(self, parameters, invocation);
  else
    /* unsupported method call */
    g_return_if_reached();
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL,
};

static void on_bus_acquired(GDBusConnection *connection,
			    const gchar *name,
			    gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  guint id;

  g_debug("%s()", __func__);

  g_return_if_fail(priv->node_info != NULL);

  if (priv->connection == NULL) {
    priv->connection = connection;
    g_object_ref(connection);
  }

  id = g_dbus_connection_register_object(connection,
					 NETWORK_AGENT_OBJECT,
					 priv->node_info->interfaces[0],
					 &interface_vtable,
					 self,
					 NULL,  /* user_data_free_func */
					 NULL); /* GError** */

  g_return_if_fail(id != 0);

  /* FIXME: check if priv->registration_id != 0 and act accordingly */

  priv->registration_id = id;

  create_barman_proxy(self);
}

static void on_name_acquired (GDBusConnection *connection, const gchar *name,
			      gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
			 gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  g_return_if_fail(priv != NULL);

  if (connection == NULL && priv->connection != NULL) {
    g_object_unref(priv->connection);
    priv->connection = NULL;
  }
}

static void passphrase_dialog_init(PassphraseDialog *self)
{
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  memset(priv, 0, sizeof(*priv));

  priv->node_info = g_dbus_node_info_new_for_xml(barman_agent_xml, NULL);

  g_return_if_fail(priv->node_info != NULL);

  priv->owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
				  INDICATOR_BARCODE_AGENT_NAME,
				  G_BUS_NAME_OWNER_FLAGS_NONE,
				  on_bus_acquired,
				  on_name_acquired,
				  on_name_lost,
				  self,
				  NULL);

  priv->registration_id = 0;

  priv->barman_proxy = NULL;

  priv->invocation = NULL;

  return;
}

static void passphrase_dialog_dispose(GObject *object)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(object);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  if (priv->entry != NULL) {
    gtk_widget_destroy(priv->entry);
    priv->entry = NULL;
  }

  if (priv->security_type != NULL) {
    g_free(priv->security_type);
    priv->security_type = NULL;
  }

  if (priv->registration_id != 0) {
    g_dbus_connection_unregister_object(priv->connection,
					priv->registration_id);
    priv->registration_id = 0;
  }

  if (priv->connection != NULL) {
    g_object_unref(priv->connection);
    priv->connection = NULL;
  }

  if (priv->owner_id != 0) {
    g_bus_unown_name(priv->owner_id);
    priv->owner_id = 0;
  }

  G_OBJECT_CLASS(passphrase_dialog_parent_class)->dispose(object);
}

static void passphrase_dialog_finalize(GObject *object)
{
  G_OBJECT_CLASS(passphrase_dialog_parent_class)->finalize(object);
}

static void passphrase_dialog_class_init(PassphraseDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private(object_class, sizeof(PassphraseDialogPrivate));

  object_class->dispose = passphrase_dialog_dispose;
  object_class->finalize = passphrase_dialog_finalize;

  g_assert(klass != NULL);
}

static void responded(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  const gchar *passphrase;
  GVariant *result;
  GVariantBuilder *builder;

  g_return_if_fail(priv->invocation != NULL);

  g_debug("%s(): response_id %d", __func__, response_id);

  gtk_widget_hide_all(GTK_WIDGET(self));

  g_free(priv->security_type);
  priv->security_type = NULL;

  builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

  if (response_id == GTK_RESPONSE_ACCEPT) {
    passphrase = gtk_entry_get_text(GTK_ENTRY(priv->entry));
    g_variant_builder_add(builder, "{sv}", BARMAN_PROPERTY_PASSPHRASE,
			  g_variant_new_string(passphrase));
  }

  result = g_variant_new("(a{sv})", builder);

  /* FIXME: if user cancels should we send an error to barman? */

  g_dbus_method_invocation_return_value(priv->invocation, result);
  priv->invocation = NULL;
}

static void validate(GtkWidget *widget, gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);
  const gchar *text, *security;
  guint len;

  text = gtk_entry_get_text(GTK_ENTRY(priv->entry));
  len = strlen(text);

  security = priv->security_type;
  if (security == NULL) {
    g_warning("passphrase-dialog: security type not defined");
    gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
				      TRUE);
    return;
  }

  if (is_security_wep(self)) {
    switch (len) {
    case WEP_40BIT_HEXKEY_LEN:
    case WEP_104BIT_HEXKEY_LEN:
    case WEP_40BIT_ASCIIKEY_LEN:
    case WEP_104BIT_ASCIIKEY_LEN:
      gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
					TRUE);
      break;
    default:
      gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
					FALSE);
      break;
    }
  } else if (is_security_wpa(self)) {
    if ((len >= WPA_MIN_LEN) && (len <= WPA_MAX_LEN)) {
      gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
					TRUE);
    } else {
      gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
					FALSE);
    }
  } else {
    g_warning("passphrase-dialog: unknown security mode: %s",
	      priv->security_type);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
				      TRUE);
  }
}

static void show_password_toggled(GtkToggleButton *button, gpointer user_data)
{
  PassphraseDialog *self = PASSPHRASE_DIALOG(user_data);
  PassphraseDialogPrivate *priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  if (gtk_toggle_button_get_active(button))
    gtk_entry_set_visibility(GTK_ENTRY(priv->entry), TRUE);
  else
    gtk_entry_set_visibility(GTK_ENTRY(priv->entry), FALSE);
}

PassphraseDialog *passphrase_dialog_new(void)
{
  GtkWidget *vbox, *widget;
  PassphraseDialogPrivate *priv;
  PassphraseDialog *self;

  self = g_object_new(PASSPHRASE_DIALOG_TYPE, NULL);
  priv = PASSPHRASE_DIALOG_GET_PRIVATE(self);

  gtk_window_set_title(GTK_WINDOW(self), _("Network passphrase"));

  gtk_dialog_add_buttons(GTK_DIALOG(self),
			 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			 GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT,
			 NULL);
  gtk_dialog_set_response_sensitive(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT,
				    FALSE);

  priv->entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(priv->entry), FALSE);
  g_signal_connect(G_OBJECT(priv->entry), "changed", (GCallback) validate,
		   self);
  vbox = gtk_dialog_get_content_area(GTK_DIALOG(self));
  gtk_box_pack_start(GTK_BOX(vbox), priv->entry, TRUE, TRUE, 5);

  widget = gtk_check_button_new_with_label(_("Show password"));
  g_signal_connect(G_OBJECT(widget), "toggled",
		   G_CALLBACK(show_password_toggled), self);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 5);

  g_signal_connect(G_OBJECT(self), "response", G_CALLBACK(responded),
		   self);

  return self;
}
