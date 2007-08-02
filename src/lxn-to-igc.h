/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __LXN_TO_IGC_H
#define __LXN_TO_IGC_H

#include <stdio.h>

typedef struct lxn_to_igc *lxn_to_igc_t;

int lxn_to_igc_open(FILE *igc, lxn_to_igc_t *fti_r);

int lxn_to_igc_close(lxn_to_igc_t *fti_r);

int lxn_to_igc_process(lxn_to_igc_t fti,
                       const unsigned char *fil,
                       size_t fil_length, size_t *fil_consumed_r);

const char *lxn_to_igc_error(lxn_to_igc_t fti);

#endif
