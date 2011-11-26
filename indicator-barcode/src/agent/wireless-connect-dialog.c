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

#include "wireless-connect-dialog.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <string.h>

#include "barman-manager.h"
#include "barman-service.h"

G_DEFINE_TYPE(WirelessConnectDialog, wireless_connect_dialog, GTK_TYPE_DIALOG);

typedef struct _WirelessConnectDialogPriv WirelessConnectDialogPriv;

struct _WirelessConnectDialogPriv
{
  BarmanManager *manager;

  GtkWidget *network_combo;
  GtkWidget *count_label;
  GtkWidget *security_combo;

  guint service_count;
  GtkListStore *store;

  gboolean service_selected;
  gboolean connecting;
};

#define WIRELESS_CONNECT_DIALOG_GET_PRIVATE(o)				\
  (G_TYPE_INSTANCE_GET_PRIVATE((o),					\
			       WIRELESS_CONNECT_DIALOG_TYPE,		\
			       WirelessConnectDialogPriv))

enum {
  COLUMN_SERVICE,
  COLUMN_NAME,
  N_COLUMNS,
};

static void wireless_connect_dialog_init(WirelessConnectDialog *self)
{
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);

  priv->manager = barman_manager_new();

  priv->network_combo = NULL;
  priv->count_label = NULL;
  priv->security_combo = NULL;

  priv->service_count = 0;
  priv->store = NULL;

  priv->service_selected = FALSE;
  priv->connecting = FALSE;

  return;
}

static void wireless_connect_dialog_dispose(GObject *object)
{
  WirelessConnectDialog *self = WIRELESS_CONNECT_DIALOG(object);
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);

  if (priv->manager != NULL) {
    g_object_unref(priv->manager);
    priv->manager = NULL;
  }

  /* all the gtkwidgets are destroyed at the same time as this dialog */

  /* FIXME: what about priv->store? */

  G_OBJECT_CLASS(wireless_connect_dialog_parent_class)->dispose(object);
}

static void wireless_connect_dialog_finalize(GObject *object)
{
  G_OBJECT_CLASS(wireless_connect_dialog_parent_class)->finalize(object);
}

static void wireless_connect_dialog_class_init(WirelessConnectDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private(object_class, sizeof(WirelessConnectDialogPriv));

  object_class->dispose = wireless_connect_dialog_dispose;
  object_class->finalize = wireless_connect_dialog_finalize;

  g_assert(klass != NULL);
}

static void connect_service_cb(GObject *object, GAsyncResult *res,
			       gpointer user_data)
{
  BarmanManager *manager = BARMAN_MANAGER(object);
  GError *error = NULL;

  barman_manager_connect_service_finish(manager, res, &error);

  if (error != NULL) {
    g_print("connect service failed: %s\n", error->message);
    g_error_free(error);
    return;
  }
}

static void connect_cb(GObject *object, GAsyncResult *res,
		       gpointer user_data)
{
  BarmanService *service = BARMAN_SERVICE(object);
  GError *error = NULL;

  barman_service_connect_finish(service, res, &error);

  if (error != NULL) {
    g_print("connect failed: %s\n", error->message);
    g_error_free(error);
    return;
  }
}

static void responded(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  WirelessConnectDialog *self = WIRELESS_CONNECT_DIALOG(user_data);
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);
  BarmanServiceSecurity security;
  BarmanService *service;
  gchar *name;
  GtkTreeIter iter;
  gboolean result;

  g_return_if_fail(priv != NULL);

  gtk_widget_hide_all(GTK_WIDGET(self));

  if (response_id != GTK_RESPONSE_ACCEPT)
    return;

  /* connect to the service */

  if (priv->service_selected) {
    result = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->network_combo),
					   &iter);
    if (!result)
      return;

    gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
		       COLUMN_SERVICE, &service, -1);

    g_return_if_fail(BARMAN_IS_SERVICE(service));

    barman_service_connect(service, connect_cb, dialog);
  } else {
    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(priv->network_combo));
    security = gtk_combo_box_get_active(GTK_COMBO_BOX(priv->security_combo));
    barman_manager_connect_service(priv->manager,
				    BARMAN_SERVICE_TYPE_WIFI,
				    BARMAN_SERVICE_MODE_MANAGED,
				    security,
				    (guint8 *) name,
				    strlen(name),
				    NULL,
				    connect_service_cb,
				    self);

    g_free(name);
    name = NULL;
  }
}

static void network_changed(GtkComboBox *combo, gpointer user_data)
{
  WirelessConnectDialog *self = WIRELESS_CONNECT_DIALOG(user_data);
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);
  BarmanServiceSecurity security;
  BarmanService *service;
  GtkTreeIter iter;
  gboolean result;

  result = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->network_combo),
					 &iter);
  priv->service_selected = result;

  if (!result)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
		     COLUMN_SERVICE, &service, -1);

  security = barman_service_get_security(service);

  gtk_combo_box_set_active(GTK_COMBO_BOX(priv->security_combo), security);
}

static void security_changed(GtkComboBox *combo, gpointer user_data)
{
  WirelessConnectDialog *self = WIRELESS_CONNECT_DIALOG(user_data);
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);
  BarmanServiceSecurity active, security;
  BarmanService *service;
  GtkTreeIter iter;
  gboolean result;

  result = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->network_combo),
					 &iter);
  if (!result) {
    priv->service_selected = FALSE;
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
		     COLUMN_SERVICE, &service, -1);

  g_return_if_fail(BARMAN_IS_SERVICE(service));

  active = gtk_combo_box_get_active(GTK_COMBO_BOX(priv->security_combo));
  security = barman_service_get_security(service);

  if (active != security)
    priv->service_selected = FALSE;
  else
    priv->service_selected = TRUE;
}

static void update_count_label(WirelessConnectDialog *self)
{
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);
  gchar buf[200];

  /* FIXME: use g_strdup_printf()? */
  g_snprintf(buf, sizeof(buf), _("%d networks detected"), priv->service_count);

  gtk_label_set_text(GTK_LABEL(priv->count_label), buf);
}

void wireless_connect_dialog_show(WirelessConnectDialog *self)
{
  WirelessConnectDialogPriv *priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);
  BarmanService **services, *service;
  GtkTreeIter iter;
  GtkWidget *entry;

  g_return_if_fail(IS_WIRELESS_CONNECT_DIALOG(self));
  g_return_if_fail(priv != NULL);

  if (gtk_widget_get_visible(GTK_WIDGET(self))) {
    g_warning("Already showing wireless connect dialog, canceling request");
    return;
  }

  if (!barman_manager_get_connected(priv->manager)) {
    g_warning("Not connected to barman, not showing wireless connect dialog");
    return;
  }

  priv->service_selected = FALSE;

  gtk_combo_box_set_active(GTK_COMBO_BOX(priv->network_combo), -1);
  gtk_combo_box_set_active(GTK_COMBO_BOX(priv->security_combo), 0);

  /* clear the entry inside GtkComboBoxEditor */
  entry = gtk_bin_get_child(GTK_BIN(priv->network_combo));

  g_return_if_fail(entry != NULL);
  g_return_if_fail(GTK_IS_ENTRY(entry));

  gtk_entry_set_text(GTK_ENTRY(entry), "");

  gtk_list_store_clear(priv->store);


  services = barman_manager_get_services(priv->manager);
  priv->service_count = 0;

  if (services == NULL)
    goto out;

  for (;*services != NULL; services++) {
    service = *services;
    g_object_ref(service);

    gtk_list_store_append(priv->store, &iter);
    gtk_list_store_set(priv->store, &iter,
		       COLUMN_SERVICE, service,
		       COLUMN_NAME, barman_service_get_name(service),
		       -1);
    priv->service_count++;
  }

 out:
  update_count_label(self);

  gtk_widget_show_all(GTK_WIDGET(self));
}

WirelessConnectDialog *wireless_connect_dialog_new(void)
{
  GtkWidget *vbox, *label, *table;
  WirelessConnectDialogPriv *priv;
  WirelessConnectDialog *self;

  self = g_object_new(WIRELESS_CONNECT_DIALOG_TYPE, NULL);
  priv = WIRELESS_CONNECT_DIALOG_GET_PRIVATE(self);

  gtk_window_set_title(GTK_WINDOW(self), _("Connect to Wireless Network"));
  gtk_window_set_position(GTK_WINDOW(self), GTK_WIN_POS_CENTER);
  gtk_window_set_deletable(GTK_WINDOW(self), FALSE);
  
  gtk_container_set_border_width(GTK_CONTAINER(self), 12);

  gtk_dialog_add_buttons(GTK_DIALOG(self),
			 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			 GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT,
			 NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT);

  vbox = gtk_dialog_get_content_area(GTK_DIALOG(self));
  gtk_box_set_spacing(GTK_BOX(vbox), 6);

  table = gtk_table_new(3, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 6);
  gtk_table_set_row_spacing(GTK_TABLE(table), 0, 3);
  gtk_table_set_row_spacing(GTK_TABLE(table), 1, 6);
  
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), TRUE, TRUE, 5);

  label = gtk_label_new(_("Network name:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

  priv->store = gtk_list_store_new(N_COLUMNS, BARMAN_TYPE_SERVICE,
				   G_TYPE_STRING);
  priv->network_combo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(priv->store),
							   COLUMN_NAME);
  g_signal_connect(G_OBJECT(priv->network_combo), "changed",
		   G_CALLBACK(network_changed), self);
  gtk_table_attach(GTK_TABLE(table), priv->network_combo, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);

  priv->count_label = gtk_label_new(_("0 networks detected"));
  gtk_misc_set_alignment(GTK_MISC(priv->count_label), 0, 0.5);
  update_count_label(self);
  gtk_table_attach(GTK_TABLE(table), priv->count_label, 1, 2, 1, 2,
		    GTK_FILL, 0, 0, 0);

  label = gtk_label_new(_("Wireless security:"));
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
		    GTK_FILL, 0, 0, 0);


  priv->security_combo = gtk_combo_box_new_text();
  gtk_combo_box_insert_text(GTK_COMBO_BOX(priv->security_combo),
			    BARMAN_SERVICE_SECURITY_NONE,
			    _("None"));
  gtk_combo_box_insert_text(GTK_COMBO_BOX(priv->security_combo),
			    BARMAN_SERVICE_SECURITY_WEP,
			    _("WEP"));
  gtk_combo_box_insert_text(GTK_COMBO_BOX(priv->security_combo),
			    BARMAN_SERVICE_SECURITY_PSK,
			    _("WPA-PSK"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(priv->security_combo),
			   BARMAN_SERVICE_SECURITY_NONE);
  g_signal_connect(G_OBJECT(priv->security_combo), "changed",
		   G_CALLBACK(security_changed), self);
  gtk_table_attach(GTK_TABLE(table), priv->security_combo, 1, 2, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);

  g_signal_connect(G_OBJECT(self), "response", G_CALLBACK(responded),
		   self);

  return self;
}
