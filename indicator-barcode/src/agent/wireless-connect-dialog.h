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

#ifndef __WIRELESS_CONNECT_DIALOG_H__
#define __WIRELESS_CONNECT_DIALOG_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define WIRELESS_CONNECT_DIALOG_TYPE (wireless_connect_dialog_get_type())
#define WIRELESS_CONNECT_DIALOG(o)					\
  (G_TYPE_CHECK_INSTANCE_CAST ((o),					\
			       WIRELESS_CONNECT_DIALOG_TYPE,		\
			       WirelessConnectDialog))
#define WIRELESS_CONNECT_DIALOG_CLASS(k)			\
  (G_TYPE_CHECK_CLASS_CAST((k),					\
			   WIRELESS_CONNECT_DIALOG_TYPE,	\
			   WirelessConnectDialogClass))
#define IS_WIRELESS_CONNECT_DIALOG(o)					\
  (G_TYPE_CHECK_INSTANCE_TYPE((o),					\
			      WIRELESS_CONNECT_DIALOG_TYPE))
#define IS_WIRELESS_CONNECT_DIALOG_CLASS(k)				\
  (G_TYPE_CHECK_CLASS_TYPE((k),						\
			   WIRELESS_CONNECT_DIALOG_TYPE))
#define WIRELESS_CONNECT_DIALOG_GET_CLASS(o)				\
  (G_TYPE_INSTANCE_GET_CLASS((o),					\
			     WIRELESS_CONNECT_DIALOG_TYPE,		\
			     WirelessConnectDialogClass))

typedef struct _WirelessConnectDialog WirelessConnectDialog;
typedef struct _WirelessConnectDialogClass WirelessConnectDialogClass;

struct _WirelessConnectDialog
{
  GtkDialog parent;
};

struct _WirelessConnectDialogClass
{
  GtkDialogClass parent_class;
};

GType wireless_connect_dialog_get_type(void) G_GNUC_CONST;

WirelessConnectDialog *wireless_connect_dialog_new(void);
void wireless_connect_dialog_show(WirelessConnectDialog *self);

#endif
