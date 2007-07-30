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
#include <errno.h>

flarm_result_t
flarm_send(flarm_t flarm,
           const void *src, size_t length)
{
    ssize_t nbytes;

    assert(flarm != NULL);
    assert(flarm->fd >= 0);
    assert(src != NULL || length == 0);

    if (length == 0)
        return FLARM_RESULT_SUCCESS;

    nbytes = write(flarm->fd, src, length);
    if (nbytes < 0)
        return errno;

    if ((size_t)nbytes != length)
        return ENOSPC;

    return FLARM_RESULT_SUCCESS;
}

flarm_result_t
flarm_send_frame(flarm_t flarm, uint8_t type,
                 const void *src, size_t length)
{
    flarm_result_t ret;
    struct flarm_frame_header header;
    size_t buffer_length;

    assert(flarm != NULL);
    assert(flarm->fd >= 0);
    assert(src != NULL || length == 0);

    if (!flarm->binary_mode)
        return FLARM_RESULT_NOT_BINARY;

    if (length >= 32768)
        /* make sure the length fits into 16 bit header.length */
        return EFBIG;

    ret = flarm_need_buffer(flarm, sizeof(header) + length * 2);
    if (ret != FLARM_RESULT_SUCCESS)
        return ret;

    header.length = (uint16_t)sizeof(header) + length;
    header.version = 1; /* XXX */
    header.seq_no = ++flarm->last_seq_no;
    header.type = type;
    header.crc = flarm_crc_update_block(0, &header, sizeof(header) - sizeof(header.crc));
    header.crc = flarm_crc_update_block(header.crc, src, length);

    flarm->buffer[0] = FLARM_STARTFRAME;

    buffer_length = 1 + flarm_escape(flarm->buffer + 1, &header, sizeof(header));
    if (length > 0)
        buffer_length += flarm_escape(flarm->buffer + buffer_length, src, length);

    return flarm_send(flarm, flarm->buffer, buffer_length);
}

unsigned
flarm_last_seq_no(flarm_t flarm)
{
    return flarm->last_seq_no;
}

