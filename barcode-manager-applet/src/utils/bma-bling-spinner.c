/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * @file libbling/bling-spinner.c A apple-esque spinner widger
 *
 * @Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>

#include "bma-bling-spinner.h"

#define BMA_BLING_SPINNER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BMA_TYPE_BLING_SPINNER, BmaBlingSpinnerPrivate))

G_DEFINE_TYPE (BmaBlingSpinner, bma_bling_spinner, GTK_TYPE_DRAWING_AREA);

enum
{
	PROP_0,
	PROP_NUM_LINES
};

/* STRUCTS & ENUMS */
struct _BmaBlingSpinnerPrivate
{
	/* state */
	guint current;
	guint timeout;

	/* appearance */
	guint lines;
};

/* FORWARDS */
static void bma_bling_spinner_class_init(BmaBlingSpinnerClass *klass);
static void bma_bling_spinner_init(BmaBlingSpinner *spinner);
static void bma_bling_spinner_finalize (GObject *gobject);
static void bma_bling_spinner_set_property(GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static gboolean bma_bling_spinner_expose(GtkWidget *widget, GdkEventExpose *event);
static void bma_bling_spinner_screen_changed (GtkWidget* widget, GdkScreen* old_screen);

static GtkDrawingAreaClass *parent_class;

/* DRAWING FUNCTIONS */
static void
draw (GtkWidget *widget, cairo_t *cr)
{
	BmaBlingSpinnerPrivate *priv;
	GtkAllocation allocation;
	double x, y;
	double radius;
	double half;
	int i;

	priv = BMA_BLING_SPINNER_GET_PRIVATE (widget);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

#if GTK_CHECK_VERSION(2,18,0)
	gtk_widget_get_allocation (widget, &allocation);
#else
	allocation = widget->allocation;
#endif
	if ((allocation.width < 12) || (allocation.height < 12))
		gtk_widget_set_size_request(widget, 12, 12);

	//x = allocation.x + allocation.width / 2;
	//y = allocation.y + allocation.height / 2;
	x = allocation.width / 2;
	y = allocation.height / 2;
	radius = MIN (allocation.width / 2, allocation.height / 2);
	half = priv->lines / 2;

	/*FIXME: render in B&W for non transparency */

	for (i = 0; i < priv->lines; i++) {
		int inset = 0.7 * radius;
		/* transparency is a function of time and intial value */
		double t = (double) ((i + priv->lines - priv->current)
				   % priv->lines) / priv->lines;

		cairo_save (cr);

		cairo_set_source_rgba (cr, 0, 0, 0, t);
		//cairo_set_line_width (cr, 2 * cairo_get_line_width (cr));
		cairo_set_line_width (cr, 2.0);
		cairo_move_to (cr,
					   x + (radius - inset) * cos (i * M_PI / half),
					   y + (radius - inset) * sin (i * M_PI / half));
		cairo_line_to (cr,
					   x + radius * cos (i * M_PI / half),
					   y + radius * sin (i * M_PI / half));
		cairo_stroke (cr);

		cairo_restore (cr);
	}
}


/*	GOBJECT INIT CODE */
static void
bma_bling_spinner_class_init(BmaBlingSpinnerClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent(klass);

	gobject_class = G_OBJECT_CLASS(klass);
	g_type_class_add_private (gobject_class, sizeof (BmaBlingSpinnerPrivate));
	gobject_class->set_property = bma_bling_spinner_set_property;
	gobject_class->finalize = bma_bling_spinner_finalize;

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->expose_event = bma_bling_spinner_expose;
	widget_class->screen_changed = bma_bling_spinner_screen_changed;

	g_object_class_install_property(gobject_class, PROP_NUM_LINES,
		g_param_spec_uint("lines", "Num Lines",
							"The number of lines to animate",
							0,20,12,
							G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

}

static void
bma_bling_spinner_init (BmaBlingSpinner *spinner)
{
	BmaBlingSpinnerPrivate *priv;

	priv = BMA_BLING_SPINNER_GET_PRIVATE (spinner);
	priv->current = 0;
	priv->timeout = 0;

#if GTK_CHECK_VERSION(2,18,0)
	gtk_widget_set_has_window (GTK_WIDGET (spinner), FALSE);
#else
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (spinner), GTK_NO_WINDOW);
#endif
}

static gboolean
bma_bling_spinner_expose (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;

	/* get cairo context */
	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	/* set a clip region for the expose event */
	cairo_rectangle (cr,
					 event->area.x, event->area.y,
					 event->area.width, event->area.height);
	cairo_clip (cr);

	cairo_translate (cr, event->area.x, event->area.y);

	/* draw clip region */
	draw (widget, cr);

	/* free memory */
	cairo_destroy (cr);

	return FALSE;
}

static void
bma_bling_spinner_screen_changed (GtkWidget* widget, GdkScreen* old_screen)
{
	BmaBlingSpinner *spinner;
	GdkScreen* new_screen;
	GdkColormap* colormap;

	spinner = BMA_BLING_SPINNER(widget);

	new_screen = gtk_widget_get_screen (widget);
	colormap = gdk_screen_get_rgba_colormap (new_screen);

	if (!colormap)
		colormap = gdk_screen_get_rgb_colormap (new_screen);

	gtk_widget_set_colormap (widget, colormap);
}

static gboolean
bma_bling_spinner_timeout (gpointer data)
{
	BmaBlingSpinner *spinner;
	BmaBlingSpinnerPrivate *priv;

	spinner = BMA_BLING_SPINNER (data);
	priv = BMA_BLING_SPINNER_GET_PRIVATE (spinner);

	if (priv->current + 1 >= priv->lines) {
		priv->current = 0;
	} else {
		priv->current++;
	}

	gtk_widget_queue_draw (GTK_WIDGET (data));

	return TRUE;
}

static void
bma_bling_spinner_set_property(GObject *gobject, guint prop_id,
					const GValue *value, GParamSpec *pspec)
{
	BmaBlingSpinner *spinner;
	BmaBlingSpinnerPrivate *priv;

	spinner = BMA_BLING_SPINNER(gobject);
	priv = BMA_BLING_SPINNER_GET_PRIVATE (spinner);

	switch (prop_id)
	{
		case PROP_NUM_LINES:
			priv->lines = g_value_get_uint(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
			break;
	}
}

static void
bma_bling_spinner_finalize (GObject *gobject)
{
	BmaBlingSpinner *spinner;
	BmaBlingSpinnerPrivate *priv;

	spinner = BMA_BLING_SPINNER(gobject);
	priv = BMA_BLING_SPINNER_GET_PRIVATE (spinner);

	if (priv->timeout != 0) {
		g_source_remove (priv->timeout);
		priv->timeout = 0;
	}
}

/**
 * bma_bling_spinner_new
 *
 * Returns a default spinner. Not yet started.
 *
 * Returns: a new #BmaBlingSpinner
 */
GtkWidget *
bma_bling_spinner_new (void)
{
	return g_object_new (BMA_TYPE_BLING_SPINNER, NULL);
}

/**
 * bma_bling_spinner_start
 *
 * Starts the animation
 */
void
bma_bling_spinner_start (BmaBlingSpinner *spinner)
{
	BmaBlingSpinnerPrivate *priv;

	g_return_if_fail (BMA_IS_BLING_SPINNER (spinner));

	priv = BMA_BLING_SPINNER_GET_PRIVATE (spinner);
	if (priv->timeout != 0)
		return;
	priv->timeout = g_timeout_add (80, bma_bling_spinner_timeout, spinner);
}

/**
 * bma_bling_spinner_stop
 *
 * Stops the animation
 */
void
bma_bling_spinner_stop (BmaBlingSpinner *spinner)
{
	BmaBlingSpinnerPrivate *priv;

	g_return_if_fail (BMA_IS_BLING_SPINNER (spinner));

	priv = BMA_BLING_SPINNER_GET_PRIVATE (spinner);
	if (priv->timeout == 0)
		return;
	g_source_remove (priv->timeout);
	priv->timeout = 0;
}
