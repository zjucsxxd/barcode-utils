/*
 * service_menuitem - a menuitem subclass that has the ability to do lots
 *                   of different things depending on it's settings.
 * Note: this is a clone of dbusmenu/libdbusmenu-gtk/genericmenuitem.c
 *
 * Copyright 2010 Canonical Ltd.
 *
 * Authors:
 * Ted Gould <ted@canonical.com>
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

#include "service-menuitem.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libdbusmenu-gtk/client.h>

#include "dbus-shared-names.h"

/**
	ServiceMenuitemPrivate:
	@check_type: What type of check we have, or none at all.
	@state: What the state of our check is.
*/
struct _ServiceMenuitemPrivate {
	ServiceMenuitemCheckType   check_type;
	ServiceMenuitemState       state;
	DbusmenuMenuitem           *dbusmenuitem;
	GtkWidget                  *hbox;
	GtkWidget                  *label;
	GtkWidget                  *image;
	GtkWidget                  *protection_image;
};

/* Private macro */
#define SERVICE_MENUITEM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), SERVICE_MENUITEM_TYPE, ServiceMenuitemPrivate))

#define PROTECTED_ICON "locked"


/* Prototypes */
static void service_menuitem_dispose    (GObject *object);
static void service_menuitem_finalize   (GObject *object);
static void draw_indicator (GtkCheckMenuItem *check_menu_item, GdkRectangle *area);
static void set_label (GtkMenuItem * menu_item, const gchar * label);
static const gchar * get_label (GtkMenuItem * menu_item);
static void activate (GtkMenuItem * menu_item);
static gint get_hpadding (GtkWidget * widget);

/* GObject stuff */
G_DEFINE_TYPE (ServiceMenuitem, service_menuitem, GTK_TYPE_CHECK_MENU_ITEM);

/* Globals */
static void (*parent_draw_indicator) (GtkCheckMenuItem *check_menu_item, GdkRectangle *area) = NULL;

/* Initializing all of the classes.  Most notably we're
   disabling the drawing of the check early. */
static void
service_menuitem_class_init (ServiceMenuitemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCheckMenuItemClass *check_class;
	GtkMenuItemClass *menuitem_class;

	g_type_class_add_private (klass, sizeof (ServiceMenuitemPrivate));

	object_class->dispose = service_menuitem_dispose;
	object_class->finalize = service_menuitem_finalize;

	check_class = GTK_CHECK_MENU_ITEM_CLASS (klass);
	parent_draw_indicator = check_class->draw_indicator;
	check_class->draw_indicator = draw_indicator;

	menuitem_class = GTK_MENU_ITEM_CLASS (klass);
	menuitem_class->set_label = set_label;
	menuitem_class->get_label = get_label;
	menuitem_class->activate = activate;

	return;
}

/* Sets default values for all the class variables.  Mostly,
   this puts us in a default state. */
static void
service_menuitem_init (ServiceMenuitem *self)
{
	gint width, height;
	GtkWidget *hbox;

	self->priv = SERVICE_MENUITEM_GET_PRIVATE(self);

	self->priv->check_type = SERVICE_MENUITEM_CHECK_TYPE_NONE;
	self->priv->state = SERVICE_MENUITEM_STATE_UNCHECKED;
	self->priv->dbusmenuitem = NULL;
	self->priv->image = NULL;

	hbox = gtk_hbox_new(FALSE, 0);

	self->priv->image = gtk_image_new();
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
	gtk_widget_set_size_request(GTK_WIDGET(self->priv->image),
				    width, height);
	gtk_misc_set_alignment(GTK_MISC(self->priv->image), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), self->priv->image,
			   FALSE, FALSE, get_hpadding(GTK_WIDGET(self)));

	self->priv->label = gtk_label_new(NULL);
	gtk_label_set_use_underline(GTK_LABEL(self->priv->label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(self->priv->label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), self->priv->label, TRUE, TRUE,
			   get_hpadding(GTK_WIDGET(self)));
	gtk_widget_show(self->priv->label);


	self->priv->protection_image = gtk_image_new();
	gtk_image_set_from_icon_name(GTK_IMAGE(self->priv->protection_image),
				     PROTECTED_ICON, GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(hbox), self->priv->protection_image,
			   FALSE, FALSE, get_hpadding(GTK_WIDGET(self)));

	gtk_container_add(GTK_CONTAINER(self), hbox);
	gtk_widget_show(hbox);

	g_object_ref(hbox);
	self->priv->hbox = hbox;

	return;
}

/* Clean everything up.  Whew, that can be work. */
static void
service_menuitem_dispose (GObject *object)
{
	ServiceMenuitem *self = SERVICE_MENUITEM(object);
	ServiceMenuitemPrivate *priv = SERVICE_MENUITEM_GET_PRIVATE(self);

	if (priv->hbox != NULL) {
		g_object_unref(priv->hbox);
		priv->hbox = NULL;

		/* these widgets are destroyed at the same time as hbox */
		priv->label = NULL;
		priv->image = NULL;
	}

	G_OBJECT_CLASS (service_menuitem_parent_class)->dispose (object);
	return;
}

/* Now free memory, we no longer need it. */
static void
service_menuitem_finalize (GObject *object)
{

	G_OBJECT_CLASS (service_menuitem_parent_class)->finalize (object);
	return;
}

/* Checks to see if we should be drawing a little box at
   all.  If we should be, let's do that, otherwise we're
   going suppress the box drawing. */
static void
draw_indicator (GtkCheckMenuItem *check_menu_item, GdkRectangle *area)
{
	ServiceMenuitem * self = SERVICE_MENUITEM(check_menu_item);
	if (self->priv->check_type != SERVICE_MENUITEM_CHECK_TYPE_NONE) {
		parent_draw_indicator(check_menu_item, area);
	}
	return;
}

/* A quick little function to grab the padding from the
   style.  It should be considered for caching when
   optimizing. */
static gint
get_hpadding (GtkWidget * widget)
{
	gint padding = 0;
	gtk_widget_style_get(widget, "horizontal-padding", &padding, NULL);
	return padding;
}

/* Set the label on the item */
static void
set_label (GtkMenuItem * menu_item, const gchar * text)
{
	ServiceMenuitem *self = SERVICE_MENUITEM(menu_item);
	ServiceMenuitemPrivate *priv = SERVICE_MENUITEM_GET_PRIVATE(self);

	g_return_if_fail(priv->label != NULL);

	if (text == NULL)
		return;

	if (g_strcmp0(text, gtk_label_get_label(GTK_LABEL(priv->label))) == 0)
		return;

	gtk_label_set_label(GTK_LABEL(priv->label), text);
	g_object_notify(G_OBJECT(menu_item), "label");
}

/* Get the text of the label for the item */
static const gchar *
get_label (GtkMenuItem * menu_item)
{
	ServiceMenuitem *self = SERVICE_MENUITEM(menu_item);
	ServiceMenuitemPrivate *priv = SERVICE_MENUITEM_GET_PRIVATE(self);

	if (priv->label == NULL)
		return NULL;

	return gtk_label_get_label(GTK_LABEL(priv->label));
}

/* Make sure we don't toggle when there is an
   activate like a normal check menu item. */
static void
activate (GtkMenuItem * menu_item)
{
	return;
}

/**
	service_menuitem_set_check_type:
	@item: #ServiceMenuitem to set the type on
	@check_type: Which type of check should be displayed

	This function changes the type of the checkmark that
	appears in the left hand gutter for the menuitem.
*/
void
service_menuitem_set_check_type (ServiceMenuitem * item, ServiceMenuitemCheckType check_type)
{
	GValue value = {0};

	if (item->priv->check_type == check_type) {
		return;
	}

	item->priv->check_type = check_type;

	switch (item->priv->check_type) {
	case SERVICE_MENUITEM_CHECK_TYPE_NONE:
		/* We don't need to do anything here as we're queuing the
		   draw and then when it draws it'll avoid drawing the
		   check on the item. */
		break;
	case SERVICE_MENUITEM_CHECK_TYPE_CHECKBOX:
		g_value_init(&value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&value, FALSE);
		g_object_set_property(G_OBJECT(item), "draw-as-radio", &value);
		break;
	case SERVICE_MENUITEM_CHECK_TYPE_RADIO:
		g_value_init(&value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&value, TRUE);
		g_object_set_property(G_OBJECT(item), "draw-as-radio", &value);
		break;
	default:
		g_warning("Service Menuitem invalid check type: %d", check_type);
		return;
	}

	gtk_widget_queue_draw(GTK_WIDGET(item));

	return;
}

/**
	service_menuitem_set_state:
	@item: #ServiceMenuitem to set the type on
	@check_type: What is the state of the check 

	Sets the state of the check in the menu item.  It does
	not require, but isn't really useful if the type of
	check that the menuitem is set to #SERVICE_MENUITEM_CHECK_TYPE_NONE.
*/
void
service_menuitem_set_state (ServiceMenuitem * item, ServiceMenuitemState state)
{
	GtkCheckMenuItem * check = GTK_CHECK_MENU_ITEM(item);
	gboolean old_active = check->active;
	gboolean old_inconsist = check->inconsistent;

	if (item->priv->state == state) {
		return;
	}

	item->priv->state = state;


	switch (item->priv->state) {
	case SERVICE_MENUITEM_STATE_UNCHECKED:
		check->active = FALSE;
		check->inconsistent = FALSE;
		break;
	case SERVICE_MENUITEM_STATE_CHECKED:
		check->active = TRUE;
		check->inconsistent = FALSE;
		break;
	case SERVICE_MENUITEM_STATE_INDETERMINATE:
		check->active = TRUE;
		check->inconsistent = TRUE;
		break;
	default:
		g_warning("Service Menuitem invalid check state: %d", state);
		return;
	}

	if (old_active != check->active) {
		g_object_notify(G_OBJECT(item), "active");
	}

	if (old_inconsist != check->inconsistent) {
		g_object_notify(G_OBJECT(item), "inconsistent");
	}

	gtk_widget_queue_draw(GTK_WIDGET(item));

	return;
}

static void
service_menuitem_set_protection (ServiceMenuitem * menu_item, gint protection)
{
	ServiceMenuitem *self = SERVICE_MENUITEM(menu_item);
	ServiceMenuitemPrivate *priv = SERVICE_MENUITEM_GET_PRIVATE(self);

	g_return_if_fail(priv->protection_image != NULL);

	switch (protection) {
	case SERVICE_MENUITEM_PROP_PROTECTED:
		gtk_widget_show(priv->protection_image);
		break;
	case SERVICE_MENUITEM_PROP_OPEN:
	default:
		gtk_widget_hide(priv->protection_image);
		break;
	}

	return;
}

static void
process_visible (DbusmenuMenuitem * mi, GtkMenuItem * gmi,
		 const GVariant *value)
{
	gboolean val = TRUE;

	if (value != NULL) {
		val = dbusmenu_menuitem_property_get_bool(mi, DBUSMENU_MENUITEM_PROP_VISIBLE);
	}

	if (val) {
		gtk_widget_show(GTK_WIDGET(gmi));
	} else {
		gtk_widget_hide(GTK_WIDGET(gmi));
	}
	return;
}

static void
process_sensitive (DbusmenuMenuitem * mi, GtkMenuItem * gmi,
		   const GVariant *value)
{
	gboolean val = TRUE;
	if (value != NULL) {
		val = dbusmenu_menuitem_property_get_bool(mi, DBUSMENU_MENUITEM_PROP_ENABLED);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gmi), val);
	return;
}

/* Process the sensitive property */
static void
process_toggle_state (DbusmenuMenuitem * mi, GtkMenuItem * gmi,
		      GVariant * value)
{
	ServiceMenuitemState state = SERVICE_MENUITEM_STATE_UNCHECKED;
	int val;

	if (!IS_SERVICE_MENUITEM(gmi)) return;

	if (value == NULL)
		goto out;

	val = g_variant_get_int32(value);

	if (val == DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED) {
		state = SERVICE_MENUITEM_STATE_CHECKED;
	} else if (val == DBUSMENU_MENUITEM_TOGGLE_STATE_UNKNOWN) {
		state = SERVICE_MENUITEM_STATE_INDETERMINATE;
	}

out:
	service_menuitem_set_state(SERVICE_MENUITEM(gmi), state);
	return;
}

static void
process_protection(DbusmenuMenuitem *mi, GtkMenuItem *gmi, GVariant *value)
{
	int protection;

	if (value == NULL)
		return;

	protection = g_variant_get_int32(value);
	service_menuitem_set_protection(SERVICE_MENUITEM(gmi), protection);
}

/* This handler looks at property changes for items that are
   image menu items. */
static void
image_property_handle (DbusmenuMenuitem * item, gpointer user_data)
{
	ServiceMenuitem *self = SERVICE_MENUITEM(user_data);
	ServiceMenuitemPrivate *priv = SERVICE_MENUITEM_GET_PRIVATE(self);
	const gchar *name;

	name = dbusmenu_menuitem_property_get(item,
					      DBUSMENU_MENUITEM_PROP_ICON_NAME);

	if (name == NULL)
		/* If there is no name, by golly we want no
		   icon either. */
		return;

	if (g_strcmp0(name, DBUSMENU_MENUITEM_ICON_NAME_BLANK) == 0) {
		gtk_image_clear(GTK_IMAGE(priv->image));
		gtk_widget_hide(priv->image);
		return;
	}

	gtk_image_set_from_icon_name(GTK_IMAGE(priv->image), name,
				     GTK_ICON_SIZE_MENU);
	gtk_widget_show(priv->image);

	return;
}

static void menu_prop_changed(DbusmenuMenuitem *mi,
			      gchar *prop, GVariant *value, gpointer user_data)
{
	GtkMenuItem *gmi = user_data;

	if (g_strcmp0(prop, DBUSMENU_MENUITEM_PROP_LABEL) == 0) {
		gtk_menu_item_set_label(gmi, g_variant_get_string(value, NULL));
	} else if (g_strcmp0(prop, DBUSMENU_MENUITEM_PROP_VISIBLE) == 0) {
		process_visible(mi, gmi, value);
	} else if (g_strcmp0(prop, DBUSMENU_MENUITEM_PROP_ENABLED) == 0) {
		process_sensitive(mi, gmi, value);
	} else if (g_strcmp0(prop, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE) == 0) {
		process_toggle_state(mi, gmi, value);
	} else if (g_strcmp0(prop, SERVICE_MENUITEM_PROP_PROTECTION_MODE) == 0) {
		process_protection(mi, gmi, value);
	}
}

void
service_menuitem_set_dbusmenu (ServiceMenuitem *self, DbusmenuMenuitem *item)
{
	ServiceMenuitemPrivate *priv = SERVICE_MENUITEM_GET_PRIVATE(self);
	GtkMenuItem * gmi;
	const gchar *s;
	gint state, protection;

	if (priv->dbusmenuitem != NULL)
		g_object_unref(priv->dbusmenuitem);

	priv->dbusmenuitem = item;
	g_object_ref(priv->dbusmenuitem);

	g_signal_connect(G_OBJECT(priv->dbusmenuitem),
			 DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED,
			 G_CALLBACK(menu_prop_changed), self);

	gmi = GTK_MENU_ITEM(self);

	s = dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_LABEL);
	gtk_menu_item_set_label(gmi, s);

	service_menuitem_set_check_type(self,
					SERVICE_MENUITEM_CHECK_TYPE_RADIO);

	state = dbusmenu_menuitem_property_get_int(item,
						   DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);
	service_menuitem_set_state(self, state);

	image_property_handle(item, self);

	protection = dbusmenu_menuitem_property_get_int(item,
							SERVICE_MENUITEM_PROP_PROTECTION_MODE);
	service_menuitem_set_protection(self, protection);
}

GtkWidget *
service_menuitem_new ()
{
	return g_object_new(SERVICE_MENUITEM_TYPE, NULL);
}
