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

#ifndef __PASSPHRASE_DIALOG_H__
#define __PASSPHRASE_DIALOG_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define PASSPHRASE_DIALOG_TYPE (passphrase_dialog_get_type())
#define PASSPHRASE_DIALOG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o),			\
						   PASSPHRASE_DIALOG_TYPE,	\
						   PassphraseDialog))
#define PASSPHRASE_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k),		\
						     PASSPHRASE_DIALOG_TYPE,	\
						     PassphraseDialogClass))
#define IS_PASSPHRASE_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE((o),		\
						     PASSPHRASE_DIALOG_TYPE))
#define IS_PASSPHRASE_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
							PASSPHRASE_DIALOG_TYPE))
#define PASSPHRASE_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o),		\
							   PASSPHRASE_DIALOG_TYPE, \
							   PassphraseDialogClass))

typedef struct _PassphraseDialog PassphraseDialog;
typedef struct _PassphraseDialogClass PassphraseDialogClass;

struct _PassphraseDialog
{
  /* GObject parent; */
  GtkDialog parent;
};

struct _PassphraseDialogClass
{
  /* GObjectClass parent_class; */
  GtkDialogClass parent_class;
};

GType passphrase_dialog_get_type(void) G_GNUC_CONST;

PassphraseDialog *passphrase_dialog_new(void);

#endif
