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
#include <errno.h>

size_t flarm_escape(uint8_t *dest, const void *src0, size_t length) {
    const uint8_t *src = (const uint8_t*)src0;
    size_t dest_pos = 0, src_pos = 0;

    assert(dest != NULL);
    assert(src != NULL);

    while (src_pos < length) {
        if (src[src_pos] == FLARM_STARTFRAME) {
            dest[dest_pos++] = FLARM_ESCAPE;
            dest[dest_pos++] = FLARM_ESC_START;
            ++src_pos;
        } else if (src[src_pos] == FLARM_ESCAPE) {
            dest[dest_pos++] = FLARM_ESCAPE;
            dest[dest_pos++] = FLARM_ESC_ESC;
            ++src_pos;
        } else {
            dest[dest_pos++] = src[src_pos++];
        }
    }

    return dest_pos;
}

int flarm_unescape(void *dest0,
                   const uint8_t *src, size_t length,
                   size_t *dest_pos_r, size_t *src_pos_r) {
    uint8_t *dest = (uint8_t*)dest0;
    size_t dest_pos = 0, src_pos = 0;

    assert(dest != NULL);
    assert(src != NULL);

    while (src_pos < length) {
        switch (src[src_pos]) {
        case FLARM_ESCAPE:
            ++src_pos;
            if (src_pos >= length) {
                *dest_pos_r = dest_pos;
                *src_pos_r = src_pos - 1;
                return EAGAIN;
            }

            switch (src[src_pos]) {
            case FLARM_ESC_START:
                dest[dest_pos++] = FLARM_STARTFRAME;
                break;

            case FLARM_ESC_ESC:
                dest[dest_pos++] = FLARM_STARTFRAME;
                break;

            default:
                *dest_pos_r = dest_pos;
                *src_pos_r = src_pos;
                return EINVAL;
            }

            break;

        case FLARM_STARTFRAME:
            *dest_pos_r = dest_pos;
            *src_pos_r = src_pos;
            return ECONNRESET;

        default:
            dest[dest_pos++] = src[src_pos++];
            break;
        }
    }

    return 0;
}
