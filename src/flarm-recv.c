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
#include <unistd.h>
#include <string.h>
#include <errno.h>

static flarm_result_t
flarm_fill_in(flarm_t flarm)
{
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
    return FLARM_RESULT_SUCCESS;
}

static flarm_result_t
flarm_read(flarm_t flarm, const uint8_t **p_r, size_t *length_r)
{
    flarm_result_t ret;

    *p_r = (const uint8_t*)fifo_buffer_read(flarm->in, length_r);
    if (*p_r != NULL)
        return FLARM_RESULT_SUCCESS;

    ret = flarm_fill_in(flarm);
    if (ret != FLARM_RESULT_SUCCESS)
        return ret;

    *p_r = (const uint8_t*)fifo_buffer_read(flarm->in, length_r);
    if (*p_r == NULL)
        return EAGAIN;

    return FLARM_RESULT_SUCCESS;
}

static flarm_result_t
flarm_wait_startframe(flarm_t flarm) {
    flarm_result_t ret;
    const uint8_t *src, *startframe;
    size_t length;

    for (;;) {
        ret = flarm_read(flarm, &src, &length);
        if (ret != FLARM_RESULT_SUCCESS)
            return ret;

        startframe = memchr(src, FLARM_STARTFRAME, length);
        if (startframe == NULL) {
            fifo_buffer_consume(flarm->in, length);
        } else {
            fifo_buffer_consume(flarm->in, startframe - src + 1);
            return FLARM_RESULT_SUCCESS;
        }
    }
}

static flarm_result_t
flarm_recv_unescape(flarm_t flarm) {
    const uint8_t *src;
    uint8_t *dest;
    flarm_result_t ret;
    size_t in_length, max_length, src_pos, dest_pos;

    ret = flarm_read(flarm, &src, &in_length);
    if (ret != FLARM_RESULT_SUCCESS)
        return ret;

    dest = (uint8_t*)fifo_buffer_write(flarm->frame, &max_length);
    if (dest == NULL)
        return ENOSPC;

    do {
        ret = flarm_unescape(dest, src,
                             in_length > max_length ? max_length : in_length,
                             &dest_pos, &src_pos);

        fifo_buffer_consume(flarm->in, src_pos);
        src += src_pos;
        in_length -= src_pos;

        fifo_buffer_append(flarm->frame, dest_pos);
        dest += dest_pos;
        max_length -= dest_pos;
    } while (in_length > 0 && ret == FLARM_RESULT_SUCCESS);

    return ret;
}

flarm_result_t
flarm_recv_frame(flarm_t flarm,
                 uint8_t *version_r, uint8_t *type_r,
                 uint16_t *seq_no_r,
                 const void **payload_r, size_t *length_r) {
    flarm_result_t ret;
    const struct flarm_frame_header *header;
    size_t length;

    assert(flarm != NULL);
    assert(flarm->fd >= 0);
    assert(type_r != NULL);
    assert(payload_r != NULL);
    assert(length_r != NULL);

    if (!flarm->binary_mode)
        return FLARM_RESULT_NOT_BINARY;

    if (!flarm->frame_started) {
        fifo_buffer_clear(flarm->frame);

        ret = flarm_wait_startframe(flarm);
        if (ret != FLARM_RESULT_SUCCESS)
            return ret;

        flarm->frame_started = 1;
    }

    ret = flarm_recv_unescape(flarm);
    if (ret == ENOSPC)
        flarm->frame_started = 0;
    if (ret != FLARM_RESULT_SUCCESS && ret != ECONNRESET)
        return ret;

    header = (const struct flarm_frame_header*)fifo_buffer_read(flarm->frame, &length);
    if (header == NULL || length < sizeof(*header) ||
        length < sizeof(*header) + header->length) {
        if (ret == ECONNRESET)
            flarm->frame_started = 0;
        return EAGAIN;
    }

    if (version_r != NULL)
        *version_r = header->version;

    *type_r = header->type;

    if (seq_no_r != NULL)
        *seq_no_r = header->seq_no;

    *payload_r = header + 1;
    *length_r = header->length;

    return FLARM_RESULT_SUCCESS;
}
