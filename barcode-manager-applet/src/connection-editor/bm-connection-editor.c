/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager Connection editor -- Connection editor for BarcodeManager
 *
 * Jakob Flierl <jakob.flierl@gmail.com>
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

#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>

#include <bm-setting-connection.h>
#include <bm-utils.h>

#include "bm-connection-editor.h"
#include "gconf-helpers.h"
#include "bma-marshal.h"

#include "ce-page.h"
#include "ce-polkit-button.h"

G_DEFINE_TYPE (BMConnectionEditor, bm_connection_editor, G_TYPE_OBJECT)

enum {
	EDITOR_DONE,
	EDITOR_LAST_SIGNAL
};

static guint editor_signals[EDITOR_LAST_SIGNAL] = { 0 };

static gboolean bm_connection_editor_set_connection (BMConnectionEditor *editor,
                                                     BMConnection *connection,
                                                     GError **error);

struct GetSecretsInfo {
	BMConnectionEditor *self;
	CEPage *page;
	char *setting_name;
	gboolean canceled;
};

static void
bm_connection_editor_update_title (BMConnectionEditor *editor)
{
	BMSettingConnection *s_con;
	const char *id;

	g_return_if_fail (editor != NULL);

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (editor->connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	id = bm_setting_connection_get_id (s_con);
	if (id && strlen (id)) {
		char *title = g_strdup_printf (_("Editing %s"), id);
		gtk_window_set_title (GTK_WINDOW (editor->window), title);
		g_free (title);
	} else
		gtk_window_set_title (GTK_WINDOW (editor->window), _("Editing un-named connection"));
}

static gboolean
ui_to_setting (BMConnectionEditor *editor)
{
	BMSettingConnection *s_con;
	GtkWidget *widget;
	const char *name;
	gboolean autoconnect = FALSE;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (editor->connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_name"));
	name = gtk_entry_get_text (GTK_ENTRY (widget));

	g_object_set (G_OBJECT (s_con), BM_SETTING_CONNECTION_ID, name, NULL);
	bm_connection_editor_update_title (editor);

	if (!name || !strlen (name))
		return FALSE;

	widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_autoconnect"));
	autoconnect = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	g_object_set (G_OBJECT (s_con), BM_SETTING_CONNECTION_AUTOCONNECT, autoconnect, NULL);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->system_checkbutton)))
		bm_connection_set_scope (editor->connection, BM_CONNECTION_SCOPE_SYSTEM);
	else
		bm_connection_set_scope (editor->connection, BM_CONNECTION_SCOPE_USER);

	return TRUE;
}

static gboolean
editor_is_initialized (BMConnectionEditor *editor)
{
	return (g_slist_length (editor->initializing_pages) == 0);
}

static void
update_sensitivity (BMConnectionEditor *editor)
{
	BMSettingConnection *s_con;
	gboolean actionable = FALSE, authorized = FALSE, sensitive = FALSE;
	GtkWidget *widget;
	GSList *iter;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (editor->connection, BM_TYPE_SETTING_CONNECTION));

	/* Can't modify read-only connections; can't modify anything before the
	 * editor is initialized either.
	 */
	if (   !bm_setting_connection_get_read_only (s_con)
	    && editor_is_initialized (editor)) {
		if (editor->system_settings_can_modify) {
			actionable = ce_polkit_button_get_actionable (CE_POLKIT_BUTTON (editor->ok_button));
			authorized = ce_polkit_button_get_authorized (CE_POLKIT_BUTTON (editor->ok_button));
		}

		if (editor->orig_scope == BM_CONNECTION_SCOPE_SYSTEM) {
			/* If the user cannot ever be authorized to change system connections, and
			 * the connection is a system connection, we desensitize the entire dialog.
			 */
			sensitive = authorized;
		} else {
			/* Otherwise, if the connection is originally a user-connection,
			 * then everything is sensitive except possible the system checkbutton,
			 * which will be insensitive if the user has no possibility of
			 * authorizing.
			 */
			sensitive = TRUE;
		}
	}

	gtk_widget_set_sensitive (GTK_WIDGET (editor->system_checkbutton), actionable && authorized);

	/* Cancel button is always sensitive */
	gtk_widget_set_sensitive (GTK_WIDGET (editor->cancel_button), TRUE);

	widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_name_label"));
	gtk_widget_set_sensitive (widget, sensitive);

	widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_name"));
	gtk_widget_set_sensitive (widget, sensitive);

	widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_autoconnect"));
	gtk_widget_set_sensitive (widget, sensitive);

	widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_name"));
	gtk_widget_set_sensitive (widget, sensitive);

	for (iter = editor->pages; iter; iter = g_slist_next (iter)) {
		widget = ce_page_get_page (CE_PAGE (iter->data));
		gtk_widget_set_sensitive (widget, sensitive);
	}
}

static void
connection_editor_validate (BMConnectionEditor *editor)
{
	BMSettingConnection *s_con;
	gboolean valid = FALSE;
	GSList *iter;

	if (!editor_is_initialized (editor))
		goto done;

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (editor->connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);
	if (bm_setting_connection_get_read_only (s_con))
		goto done;

	if (!ui_to_setting (editor))
		goto done;

	for (iter = editor->pages; iter; iter = g_slist_next (iter)) {
		GError *error = NULL;

		if (!ce_page_validate (CE_PAGE (iter->data), editor->connection, &error)) {
			/* FIXME: use the error to indicate which UI widgets are invalid */
			if (error) {
				g_warning ("Invalid setting %s: %s", CE_PAGE (iter->data)->title, error->message);
				g_error_free (error);
			} else
				g_warning ("Invalid setting %s", CE_PAGE (iter->data)->title);

			goto done;
		}
	}
	valid = TRUE;

done:
	ce_polkit_button_set_master_sensitive (CE_POLKIT_BUTTON (editor->ok_button), valid);
	update_sensitivity (editor);
}

static void
ok_button_actionable_cb (GtkWidget *button,
                         gboolean actionable,
                         BMConnectionEditor *editor)
{
	connection_editor_validate (editor);
}

static void
can_modify_changed_cb (BMRemoteSettingsSystem *settings,
                       GParamSpec *pspec,
                       BMConnectionEditor *editor)
{
	connection_editor_validate (editor);
}

static void
system_checkbutton_toggled_cb (GtkWidget *widget, BMConnectionEditor *editor)
{
	gboolean use_polkit = TRUE;

	/* The only time the Save button does not need to use polkit is when the
	 * original connection scope was USER and the "system" checkbutton is
	 * unchecked.
	 */
	if (   !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))
	    && (editor->orig_scope == BM_CONNECTION_SCOPE_USER))
		use_polkit = FALSE;

	ce_polkit_button_set_use_polkit (CE_POLKIT_BUTTON (editor->ok_button), use_polkit);
	connection_editor_validate (editor);
}

#define NMCE_UI UIDIR "/bm-connection-editor.ui"

static void
bm_connection_editor_init (BMConnectionEditor *editor)
{
	GtkWidget *dialog;
	GError *error = NULL;

	editor->builder = gtk_builder_new ();

	if (!gtk_builder_add_from_file (editor->builder, NMCE_UI, &error)) {
		g_warning ("Couldn't load builder file " NMCE_UI ": %s", error->message);
		g_error_free (error);

		dialog = gtk_message_dialog_new (NULL, 0,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_OK,
		                                 "%s",
		                                 _("The connection editor could not find some required resources (the .ui file was not found)."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		gtk_main_quit ();
		return;
	}

	editor->window = GTK_WIDGET (gtk_builder_get_object (editor->builder, "bm-connection-editor"));
	editor->cancel_button = GTK_WIDGET (gtk_builder_get_object (editor->builder, "cancel_button"));
	editor->system_checkbutton = GTK_WIDGET (gtk_builder_get_object (editor->builder, "system_checkbutton"));
}

static void
get_secrets_info_free (GetSecretsInfo *info)
{
	g_free (info->setting_name);
	g_free (info);
}

static void
dispose (GObject *object)
{
	BMConnectionEditor *editor = BM_CONNECTION_EDITOR (object);
	GSList *iter;

	editor->disposed = TRUE;

	g_slist_foreach (editor->initializing_pages, (GFunc) g_object_unref, NULL);
	g_slist_free (editor->initializing_pages);
	editor->initializing_pages = NULL;

	g_slist_foreach (editor->pages, (GFunc) g_object_unref, NULL);
	g_slist_free (editor->pages);
	editor->pages = NULL;

	/* Mark any in-progress secrets call as canceled; it will clean up after itself. */
	if (editor->secrets_call)
		editor->secrets_call->canceled = TRUE;

	/* Kill any pending secrets calls */
	for (iter = editor->pending_secrets_calls; iter; iter = g_slist_next (iter)) {
		get_secrets_info_free ((GetSecretsInfo *) iter->data);
	}
	g_slist_free (editor->pending_secrets_calls);
	editor->pending_secrets_calls = NULL;

	if (editor->connection) {
		g_object_unref (editor->connection);
		editor->connection = NULL;
	}
	if (editor->orig_connection) {
		g_object_unref (editor->orig_connection);
		editor->orig_connection = NULL;
	}
	if (editor->window) {
		gtk_widget_destroy (editor->window);
		editor->window = NULL;
	}
	if (editor->builder) {
		g_object_unref (editor->builder);
		editor->builder = NULL;
	}

	G_OBJECT_CLASS (bm_connection_editor_parent_class)->dispose (object);
}

static void
bm_connection_editor_class_init (BMConnectionEditorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	/* virtual methods */
	object_class->dispose = dispose;

	/* Signals */
	editor_signals[EDITOR_DONE] =
		g_signal_new ("done",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (BMConnectionEditorClass, done),
		              NULL, NULL,
		              bma_marshal_VOID__INT_POINTER,
		              G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_POINTER);
}

BMConnectionEditor *
bm_connection_editor_new (BMConnection *connection,
                          BMRemoteSettingsSystem *settings,
                          GError **error)
{
	BMConnectionEditor *editor;
	GtkWidget *hbox;
	gboolean sensitive = TRUE, use_polkit = FALSE;

	g_return_val_if_fail (BM_IS_CONNECTION (connection), NULL);

	editor = g_object_new (BM_TYPE_CONNECTION_EDITOR, NULL);
	if (!editor) {
		g_set_error (error, 0, 0, "%s", _("Error creating connection editor dialog."));
		return NULL;
	}

	g_object_get (settings,
	              BM_SETTINGS_SYSTEM_INTERFACE_CAN_MODIFY,
	              &editor->system_settings_can_modify,
	              NULL);
	g_signal_connect (settings,
	                  "notify::" BM_SETTINGS_SYSTEM_INTERFACE_CAN_MODIFY,
	                  G_CALLBACK (can_modify_changed_cb),
	                  editor);

	/* If this is a system connection that we can't ever modify,
	 * set the editor's Save button always insensitive.
	 */
	if (bm_connection_get_scope (connection) == BM_CONNECTION_SCOPE_SYSTEM) {
		sensitive = editor->system_settings_can_modify;
		use_polkit = TRUE;
	}

	editor->ok_button = ce_polkit_button_new (_("_Save"),
	                                          _("Save any changes made to this connection."),
	                                          _("_Save..."),
	                                          _("Authenticate to save this connection for all users of this machine."),
	                                          GTK_STOCK_APPLY,
	                                          settings,
	                                          BM_SETTINGS_SYSTEM_PERMISSION_CONNECTION_MODIFY);
	gtk_button_set_use_underline (GTK_BUTTON (editor->ok_button), TRUE);
	ce_polkit_button_set_use_polkit (CE_POLKIT_BUTTON (editor->ok_button), use_polkit);

	g_signal_connect (editor->ok_button, "actionable",
	                  G_CALLBACK (ok_button_actionable_cb), editor);
	g_signal_connect (editor->ok_button, "authorized",
	                  G_CALLBACK (ok_button_actionable_cb), editor);
	hbox = GTK_WIDGET (gtk_builder_get_object (editor->builder, "action_area_hbox"));
	gtk_box_pack_end (GTK_BOX (hbox), editor->ok_button, TRUE, TRUE, 0);
	gtk_widget_show_all (editor->ok_button);

	if (!bm_connection_editor_set_connection (editor, connection, error)) {
		g_object_unref (editor);
		return NULL;
	}

	return editor;
}

BMConnection *
bm_connection_editor_get_connection (BMConnectionEditor *editor)
{
	g_return_val_if_fail (BM_IS_CONNECTION_EDITOR (editor), NULL);

	return editor->orig_connection;
}

gboolean
bm_connection_editor_update_connection (BMConnectionEditor *editor, GError **error)
{
	GHashTable *settings;

	g_return_val_if_fail (BM_IS_CONNECTION_EDITOR (editor), FALSE);

	if (!bm_connection_verify (editor->connection, error))
		return FALSE;

	/* Copy the modified connection to the original connection */
	settings = bm_connection_to_hash (editor->connection);
	bm_connection_replace_settings (editor->orig_connection, settings, NULL);
	g_hash_table_destroy (settings);

	bm_connection_set_scope (editor->orig_connection,
	                         bm_connection_get_scope (editor->connection));
	return TRUE;
}

static void
populate_connection_ui (BMConnectionEditor *editor)
{
	BMSettingConnection *s_con;
	GtkWidget *name;
	GtkWidget *autoconnect;

	name = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_name"));
	autoconnect = GTK_WIDGET (gtk_builder_get_object (editor->builder, "connection_autoconnect"));

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (editor->connection, BM_TYPE_SETTING_CONNECTION));
	if (s_con) {
		const char *id = bm_setting_connection_get_id (s_con);

		gtk_entry_set_text (GTK_ENTRY (name), id);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (autoconnect),
		                              bm_setting_connection_get_autoconnect (s_con));
	} else {
		gtk_entry_set_text (GTK_ENTRY (name), NULL);
	}

	g_signal_connect_swapped (name, "changed", G_CALLBACK (connection_editor_validate), editor);
	g_signal_connect_swapped (autoconnect, "toggled", G_CALLBACK (connection_editor_validate), editor);

	g_signal_connect (editor->system_checkbutton, "toggled", G_CALLBACK (system_checkbutton_toggled_cb), editor);

	if (editor->orig_scope == BM_CONNECTION_SCOPE_SYSTEM)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->system_checkbutton), TRUE);

	connection_editor_validate (editor);
}

/*
static void
page_changed (CEPage *page, gpointer user_data)
{
	BMConnectionEditor *editor = BM_CONNECTION_EDITOR (user_data);

	connection_editor_validate (editor);
	}*/

static gboolean
idle_validate (gpointer user_data)
{
	connection_editor_validate (BM_CONNECTION_EDITOR (user_data));
	return FALSE;
}

static void
recheck_initialization (BMConnectionEditor *editor)
{
	if (!editor_is_initialized (editor) || editor->init_run)
		return;

	editor->init_run = TRUE;

	populate_connection_ui (editor);

	/* When everything is initialized, re-present the window to ensure it's on top */
	bm_connection_editor_present (editor);

	/* Validate the connection from an idle handler to ensure that stuff like
	 * GtkFileChoosers have had a chance to asynchronously find their files.
	 */
	g_idle_add (idle_validate, editor);
}

/*
static void
page_initialized (CEPage *page, GError *error, gpointer user_data)
{
	BMConnectionEditor *editor = BM_CONNECTION_EDITOR (user_data);
	GtkWidget *widget;
	GtkNotebook *notebook;
	GtkWidget *label;

	if (error) {
		gtk_widget_hide (editor->window);
		g_signal_emit (editor, editor_signals[EDITOR_DONE], 0, GTK_RESPONSE_NONE, error);
		return;
	}

	// Add the page to the UI
	notebook = GTK_NOTEBOOK (gtk_builder_get_object (editor->builder, "notebook"));
	label = gtk_label_new (ce_page_get_title (page));
	widget = ce_page_get_page (page);
	gtk_widget_unparent (widget);
	gtk_notebook_append_page (notebook, widget, label);

	// Move the page from the initializing list to the main page list
	editor->initializing_pages = g_slist_remove (editor->initializing_pages, page);
	editor->pages = g_slist_append (editor->pages, page);

	recheck_initialization (editor);
}*/

static void request_secrets (GetSecretsInfo *info);

static void
get_secrets_cb (BMSettingsConnectionInterface *connection,
                GHashTable *secrets,
                GError *error,
                gpointer user_data)
{
	GetSecretsInfo *info = user_data;
	BMConnectionEditor *self;

	if (info->canceled) {
		get_secrets_info_free (info);
		return;
	}

	self = info->self;

	/* Complete this secrets request; completion can actually dispose of the
	 * dialog if there was an error.
	 */
	self->secrets_call = NULL;
	ce_page_complete_init (info->page, info->setting_name, secrets, error);
	get_secrets_info_free (info);

	/* Kick off the next secrets request if there is one queued; if the dialog
	 * was disposed of by the completion above we don't need to do anything.
	 */
	if (!self->disposed && self->pending_secrets_calls) {
		self->secrets_call = g_slist_nth_data (self->pending_secrets_calls, 0);
		self->pending_secrets_calls = g_slist_remove (self->pending_secrets_calls, self->secrets_call);

		request_secrets (self->secrets_call);
	}
}

static void
request_secrets (GetSecretsInfo *info)
{
	BMSettingsConnectionInterface *connection;
	gboolean success;
	GError *error;

	g_return_if_fail (info != NULL);

	connection = BM_SETTINGS_CONNECTION_INTERFACE (info->self->orig_connection);
	success = bm_settings_connection_interface_get_secrets (connection,
	                                                        info->setting_name,
	                                                        NULL,
	                                                        FALSE,
	                                                        get_secrets_cb,
	                                                        info);
	if (!success) {
		error = g_error_new_literal (0, 0, _("Failed to update connection secrets due to an unknown error."));
		get_secrets_cb (connection, NULL, error, info);
		g_error_free (error);
	}
}

/*
static gboolean
add_page (BMConnectionEditor *editor,
          CEPageNewFunc func,
          BMConnection *connection,
          GError **error)
{
	CEPage *page;
	const char *secrets_setting_name = NULL;

	g_return_val_if_fail (editor != NULL, FALSE);
	g_return_val_if_fail (func != NULL, FALSE);
	g_return_val_if_fail (connection != NULL, FALSE);

	page = (*func) (connection, GTK_WINDOW (editor->window), &secrets_setting_name, error);
	if (!page)
		return FALSE;

	editor->initializing_pages = g_slist_prepend (editor->initializing_pages, page);
	g_signal_connect (page, "changed", G_CALLBACK (page_changed), editor);
	g_signal_connect (page, "initialized", G_CALLBACK (page_initialized), editor);

	// Request any secrets the page might require; or if it doesn't want any,
	// let the page initialize.
	get_secrets_for_page (editor, page, secrets_setting_name);

	return TRUE;
}

*/

static gboolean
bm_connection_editor_set_connection (BMConnectionEditor *editor,
                                     BMConnection *orig_connection,
                                     GError **error)
{
	BMSettingConnection *s_con;
	const char *connection_type;
	gboolean success = FALSE;

	g_return_val_if_fail (BM_IS_CONNECTION_EDITOR (editor), FALSE);
	g_return_val_if_fail (BM_IS_CONNECTION (orig_connection), FALSE);

	/* clean previous connection */
	if (editor->connection)
		g_object_unref (editor->connection);

	editor->connection = bm_connection_duplicate (orig_connection);

	editor->orig_connection = g_object_ref (orig_connection);
	editor->orig_scope = bm_connection_get_scope (editor->connection);
	bm_connection_editor_update_title (editor);

	s_con = BM_SETTING_CONNECTION (bm_connection_get_setting (editor->connection, BM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	connection_type = bm_setting_connection_get_connection_type (s_con);
	g_warning ("Unhandled setting type '%s'", connection_type);

	/* set the UI */
	recheck_initialization (editor);
	success = TRUE;

	//out:
	return success;
}

void
bm_connection_editor_present (BMConnectionEditor *editor)
{
	g_return_if_fail (BM_IS_CONNECTION_EDITOR (editor));

	gtk_window_present (GTK_WINDOW (editor->window));
}

static void
cancel_button_clicked_cb (GtkWidget *widget, gpointer user_data)
{
	BMConnectionEditor *self = BM_CONNECTION_EDITOR (user_data);

	g_signal_emit (self, editor_signals[EDITOR_DONE], 0, GTK_RESPONSE_CANCEL, NULL);
}

static void
editor_closed_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	cancel_button_clicked_cb (widget, user_data);
}

static void
ok_button_clicked_cb (GtkWidget *widget, gpointer user_data)
{
	BMConnectionEditor *self = BM_CONNECTION_EDITOR (user_data);

	g_signal_emit (self, editor_signals[EDITOR_DONE], 0, GTK_RESPONSE_OK, NULL);
}

void
bm_connection_editor_run (BMConnectionEditor *self)
{
	g_return_if_fail (BM_IS_CONNECTION_EDITOR (self));

	g_signal_connect (G_OBJECT (self->window), "delete-event",
	                  G_CALLBACK (editor_closed_cb), self);

	g_signal_connect (G_OBJECT (self->ok_button), "clicked",
	                  G_CALLBACK (ok_button_clicked_cb), self);
	g_signal_connect (G_OBJECT (self->cancel_button), "clicked",
	                  G_CALLBACK (cancel_button_clicked_cb), self);

	bm_connection_editor_present (self);
}

GtkWindow *
bm_connection_editor_get_window (BMConnectionEditor *editor)
{
	g_return_val_if_fail (BM_IS_CONNECTION_EDITOR (editor), NULL);

	return GTK_WINDOW (editor->window);
}

gboolean
bm_connection_editor_get_busy (BMConnectionEditor *editor)
{
	g_return_val_if_fail (BM_IS_CONNECTION_EDITOR (editor), FALSE);

	return editor->busy;
}

void
bm_connection_editor_set_busy (BMConnectionEditor *editor, gboolean busy)
{
	g_return_if_fail (BM_IS_CONNECTION_EDITOR (editor));

	if (busy != editor->busy) {
		editor->busy = busy;
		gtk_widget_set_sensitive (editor->window, !busy);
	}
}

