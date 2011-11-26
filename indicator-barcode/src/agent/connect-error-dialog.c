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

#include "connect-error-dialog.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "marshal.h"

G_DEFINE_TYPE(ConnectErrorDialog, connect_error_dialog, GTK_TYPE_DIALOG);

typedef struct _ConnectErrorDialogPrivate ConnectErrorDialogPrivate;

struct _ConnectErrorDialogPrivate
{
  GtkWidget *label;
};

#define CONNECT_ERROR_DIALOG_GET_PRIVATE(o)				\
	(G_TYPE_INSTANCE_GET_PRIVATE((o), CONNECT_ERROR_DIALOG_TYPE,	\
				     ConnectErrorDialogPrivate))

static void connect_error_dialog_init(ConnectErrorDialog *self)
{
  ConnectErrorDialogPrivate *priv = CONNECT_ERROR_DIALOG_GET_PRIVATE(self);

  priv->label = NULL;

  return;
}

static void connect_error_dialog_dispose(GObject *object)
{
  G_OBJECT_CLASS(connect_error_dialog_parent_class)->dispose(object);
}

static void connect_error_dialog_finalize(GObject *object)
{
  G_OBJECT_CLASS(connect_error_dialog_parent_class)->finalize(object);
}

static void connect_error_dialog_class_init(ConnectErrorDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private(object_class, sizeof(ConnectErrorDialogPrivate));

  object_class->dispose = connect_error_dialog_dispose;
  object_class->finalize = connect_error_dialog_finalize;

  g_assert(klass != NULL);
}

static void responded(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  ConnectErrorDialog *self = user_data;

  g_debug("%s(): response_id %d", __func__, response_id);

  g_return_if_fail(IS_CONNECT_ERROR_DIALOG(self));

  gtk_widget_hide_all(GTK_WIDGET(self));
}

void connect_error_dialog_show_error(ConnectErrorDialog *self,
				     const gchar *error)
{
  ConnectErrorDialogPrivate *priv = CONNECT_ERROR_DIALOG_GET_PRIVATE(self);
  const gchar *msg;

  g_debug("%s(%s)", __func__, error);

  g_return_if_fail(IS_CONNECT_ERROR_DIALOG(self));

  if (g_strcmp0(error, "wifi-open-connect-error") == 0 ) {
    msg = _("Was not able to connect to a Wi-Fi network.");
  }
  else if (g_strcmp0(error, "wifi-wep-connect-error") == 0 ) {
    msg = _("Was not able to connect a Wi-Fi network. Check your network "
	    "password.");
  }
  else if (g_strcmp0(error, "wifi-wep-dhcp-error") == 0 ) {
    msg = _("Was not able to contact a DHCP server in a Wi-Fi network. "
	    "Check your password.");
  }
  else if (g_strcmp0(error, "wifi-wpa-connect-error") == 0 ) {
    msg = _("Was not able to connect a Wi-Fi network. Check your network "
	    "password.");
  }
  else if (g_strcmp0(error, "wifi-wpa-dhcp-error") == 0 ) {
    msg = _("Was not able to contact a DHCP server in a Wi-Fi network.");
  }
  else {
    /* unknown error code, let's not show anything */
    g_warning("%s(): unknown error code: %s", __func__, error);
    return;
  }

  gtk_label_set_text(GTK_LABEL(priv->label), msg);

  gtk_widget_show_all(GTK_WIDGET(self));
}

ConnectErrorDialog *connect_error_dialog_new(void)
{
  ConnectErrorDialogPrivate *priv;
  ConnectErrorDialog *self;
  GtkWidget *vbox;

  self = g_object_new(CONNECT_ERROR_DIALOG_TYPE, NULL);
  priv = CONNECT_ERROR_DIALOG_GET_PRIVATE(self);

  gtk_window_set_title(GTK_WINDOW(self), _("Connect failed"));

  gtk_dialog_add_buttons(GTK_DIALOG(self),
			 GTK_STOCK_OK, GTK_RESPONSE_OK,
			 NULL);

  priv->label = gtk_label_new(NULL);
  vbox = gtk_dialog_get_content_area(GTK_DIALOG(self));
  gtk_box_pack_start(GTK_BOX(vbox), priv->label, TRUE, TRUE, 5);

  g_signal_connect(G_OBJECT(self), "response", G_CALLBACK(responded),
		   self);

  return self;
}
