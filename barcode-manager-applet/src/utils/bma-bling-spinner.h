/* @Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */


#ifndef _BMA_BLING_SPINNER_H_
#define _BMA_BLING_SPINNER_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BMA_TYPE_BLING_SPINNER           (bma_bling_spinner_get_type ())
#define BMA_BLING_SPINNER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BMA_TYPE_BLING_SPINNER, BmaBlingSpinner))
#define BMA_BLING_SPINNER_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), BMA_BLING_SPINNER,  BmaBlingSpinnerClass))
#define BMA_IS_BLING_SPINNER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BMA_TYPE_BLING_SPINNER))
#define BMA_IS_BLING_SPINNER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), BMA_TYPE_BLING_SPINNER))
#define BMA_BLING_SPINNER_GET_CLASS      (G_TYPE_INSTANCE_GET_CLASS ((obj), BMA_TYPE_BLING_SPINNER, BmaBlingSpinnerClass))

typedef struct _BmaBlingSpinner      BmaBlingSpinner;
typedef struct _BmaBlingSpinnerClass BmaBlingSpinnerClass;
typedef struct _BmaBlingSpinnerPrivate  BmaBlingSpinnerPrivate;

struct _BmaBlingSpinner
{
	GtkDrawingArea parent;
};

struct _BmaBlingSpinnerClass
{
	GtkDrawingAreaClass parent_class;
	BmaBlingSpinnerPrivate *priv;
};

GType bma_bling_spinner_get_type (void);

GtkWidget * bma_bling_spinner_new (void);

void bma_bling_spinner_start (BmaBlingSpinner *spinner);
void bma_bling_spinner_stop  (BmaBlingSpinner *spinner);

G_END_DECLS

#endif
