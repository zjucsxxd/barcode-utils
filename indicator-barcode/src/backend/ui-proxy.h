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

#ifndef _UI_PROXY_H_
#define _UI_PROXY_H_

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define TYPE_UI_PROXY ui_proxy_get_type()

#define UI_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_UI_PROXY, \
			      UIProxy))

#define UI_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_UI_PROXY, \
			   UIProxyClass))

#define IS_UI_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_UI_PROXY))

#define IS_UI_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_UI_PROXY))

#define UI_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_UI_PROXY, \
			     UIProxyClass))

typedef struct {
  GObject parent;
} UIProxy;

typedef struct {
  GObjectClass parent_class;
} UIProxyClass;

GType ui_proxy_get_type(void);

void ui_proxy_start(UIProxy *self);
void ui_proxy_stop(UIProxy *self);
void ui_proxy_show_connect_error(UIProxy *self, const gchar *error_id);
void ui_proxy_show_wireless_connect(UIProxy *self);
void ui_proxy_set_debug(UIProxy *self, guint level);
void ui_proxy_ask_pin(UIProxy *self, const gchar *type,
		      GCancellable *cancellable, GAsyncReadyCallback callback,
		      gpointer user_data);
gchar *ui_proxy_ask_pin_finish(UIProxy *self, GAsyncResult *res,
			       GError **error);
gboolean ui_proxy_is_connected(UIProxy *self);
UIProxy *ui_proxy_new(void);

#endif
