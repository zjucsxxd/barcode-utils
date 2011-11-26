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

#ifndef __CONNECT_ERROR_DIALOG_H__
#define __CONNECT_ERROR_DIALOG_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define CONNECT_ERROR_DIALOG_TYPE (connect_error_dialog_get_type())
#define CONNECT_ERROR_DIALOG(o)						\
	(G_TYPE_CHECK_INSTANCE_CAST ((o),				\
				     CONNECT_ERROR_DIALOG_TYPE,		\
				     ConnectErrorDialog))
#define CONNECT_ERROR_DIALOG_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), CONNECT_ERROR_DIALOG_TYPE,	\
				 ConnectErrorDialogClass))
#define IS_CONNECT_ERROR_DIALOG(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), CONNECT_ERROR_DIALOG_TYPE))
#define IS_CONNECT_ERROR_DIALOG_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), CONNECT_ERROR_DIALOG_TYPE))
#define CONNECT_ERROR_DIALOG_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), CONNECT_ERROR_DIALOG_TYPE,	\
				   ConnectErrorDialogClass))

typedef struct _ConnectErrorDialog ConnectErrorDialog;
typedef struct _ConnectErrorDialogClass ConnectErrorDialogClass;

struct _ConnectErrorDialog
{
  GtkDialog parent;
};

struct _ConnectErrorDialogClass
{
  GtkDialogClass parent_class;
};

GType connect_error_dialog_get_type(void) G_GNUC_CONST;

ConnectErrorDialog *connect_error_dialog_new(void);
void connect_error_dialog_show_error(ConnectErrorDialog *self,
				     const gchar *error);

#endif
