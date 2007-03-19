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
 *
 * $Id$
 */

#include "flarm-internal.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>

static int flarm_fill_in(flarm_t flarm) {
    void *p;
    size_t max_length;
    ssize_t nbytes;

    p = fifo_buffer_write(flarm->in, &max_length);
    if (p == NULL)
        return ENOSPC;

    assert(max_length > 0);

    nbytes = read(flarm->fd, p, max_length);
    if (nbytes < 0)
        return errno;

    fifo_buffer_append(flarm->in, (size_t)nbytes);
    return 0;
}

static int flarm_recv_unescape(flarm_t flarm, void *dest0, size_t length) {
    uint8_t *dest = (uint8_t*)dest0;
    int ret;
    const uint8_t *src;
    size_t dest_pos = 0, in_length;
    ssize_t nbytes;

    while (dest_pos < length) {
        src = (const uint8_t*)fifo_buffer_read(flarm->in, &in_length);
        if (src == NULL) {
            ret = flarm_fill_in(flarm);
            if (ret != 0)
                return ret;

            src = (const uint8_t*)fifo_buffer_read(flarm->in, &in_length);
            if (src == NULL)
                return EAGAIN;
        }

        if (in_length > length - dest_pos)
            in_length = length - dest_pos;

        nbytes = flarm_unescape(dest + dest_pos, src, in_length);
        if (nbytes < 0) {
            fifo_buffer_consume(flarm->in, (size_t)-nbytes);
            return ECONNRESET; /* XXX */
        }

        fifo_buffer_consume(flarm->in, in_length);
        dest_pos += (size_t)nbytes;
    }

    return 0;
}

int flarm_recv_frame(flarm_t flarm,
                     uint8_t *version_r, uint8_t *type_r,
                     uint16_t *seq_no_r,
                     const void **payload_r, size_t *length_r) {
    int ret;
    struct flarm_frame_header header;

    assert(flarm != NULL);
    assert(flarm->fd >= 0);
    assert(type_r != NULL);
    assert(payload_r != NULL);
    assert(length_r != NULL);

    ret = flarm_recv_unescape(flarm, &header, sizeof(header));
    if (ret != 0)
        return ret;

    ret = flarm_need_buffer(flarm, header.length);
    if (ret != 0)
        return ret;

    ret = flarm_recv_unescape(flarm, flarm->buffer, header.length);
    if (ret != 0)
        return ret;

    if (version_r != NULL)
        *version_r = header.version;

    *type_r = header.type;

    if (seq_no_r != NULL)
        *seq_no_r = header.seq_no;

    *payload_r = flarm->buffer;
    *length_r = header.length;

    return 0;
}
