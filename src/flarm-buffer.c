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

#include "flarm-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

int flarm_need_buffer(flarm_t flarm, size_t min_size) {
    if (flarm->buffer_size >= min_size) {
        assert(flarm->buffer != NULL);
        return 0;
    }

    if (flarm->buffer != NULL) {
        assert(flarm->buffer_size > 0);
        free(flarm->buffer);
    }

    min_size = ((min_size - 1) | 0xff) + 1;
    flarm->buffer = (uint8_t*)malloc(min_size);
    if (flarm->buffer == NULL) {
        flarm->buffer_size = 0;
        return errno;
    }

    flarm->buffer_size = min_size;
    return 0;
}
