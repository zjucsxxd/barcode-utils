/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* BarcodeManager -- barcode scanner manager
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
 * Copyright (C) 2011 Jakob Flierl
 */

#ifndef BARCODE_MANAGER_POLICY_H
#define BARCODE_MANAGER_POLICY_H

#include "BarcodeManager.h"
#include "bm-manager.h"
#include "bm-device.h"
#include "bm-activation-request.h"

typedef struct BMPolicy BMPolicy;

BMPolicy *bm_policy_new (BMManager *manager);
void bm_policy_destroy (BMPolicy *policy);

#endif /* BARCODE_MANAGER_POLICY_H */
