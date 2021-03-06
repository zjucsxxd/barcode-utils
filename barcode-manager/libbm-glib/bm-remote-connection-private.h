/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * libbm_glib -- Access barcode scanner hardware & information from glib applications
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2011 Jakob Flierl
 */

#ifndef __BM_REMOTE_CONNECTION_PRIVATE_H__
#define __BM_REMOTE_CONNECTION_PRIVATE_H__

#define BM_REMOTE_CONNECTION_INIT_RESULT "init-result"

typedef enum {
	BM_REMOTE_CONNECTION_INIT_RESULT_UNKNOWN = 0,
	BM_REMOTE_CONNECTION_INIT_RESULT_SUCCESS,
	BM_REMOTE_CONNECTION_INIT_RESULT_ERROR
} BMRemoteConnectionInitResult;

#endif  /* __BM_REMOTE_CONNECTION_PRIVATE__ */

