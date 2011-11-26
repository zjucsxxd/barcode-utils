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

#ifndef _AGENT_H_
#define _AGENT_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define NETWORK_AGENT_TYPE (network_agent_get_type())
#define NETWORK_AGENT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o),		\
						      NETWORK_AGENT_TYPE, \
						      BarcodeAgent))
#define NETWORK_AGENT_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), \
							 NETWORK_AGENT_TYPE, \
							 BarcodeAgentClass))
#define IS_NETWORK_AGENT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
							 NETWORK_AGENT_TYPE))
#define IS_NETWORK_AGENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), \
							    NETWORK_AGENT_TYPE))
#define NETWORK_AGENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
							       NETWORK_AGENT_TYPE, \
							       BarcodeAgentClass))


typedef struct _BarcodeAgent BarcodeAgent;
typedef struct _BarcodeAgentClass BarcodeAgentClass;

struct _BarcodeAgent
{
  GObject parent;
};

struct _BarcodeAgentClass
{
  GObjectClass parent_class;
};

GType network_agent_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif
