/* BarcodeManager -- Barcode scanner manager
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

#ifndef BM_GLIB_COMPAT_H
#define BM_GLIB_COMPAT_H

#include <glib.h>

#if !GLIB_CHECK_VERSION(2,14,0)

#define g_timeout_add_seconds(i, f, d) \
	g_timeout_add (i * G_USEC_PER_SEC, f, d)

#define g_timeout_add_seconds_full(p, i, f, d, n) \
	g_timeout_add_full (p, i * G_USEC_PER_SEC, f, d, n)

#endif /* !GLIB_CHECK_VERSION(2,14,0) */

#endif  /* BM_GLIB_COMPAT_H */
