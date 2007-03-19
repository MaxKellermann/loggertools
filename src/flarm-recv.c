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

static int flarm_recv_unescape(flarm_t flarm, void *dest0, size_t length) {
    uint8_t *dest = (uint8_t*)dest0;
    size_t dest_pos = 0;
    ssize_t nbytes;

    while (dest_pos < length) {
        nbytes = read(flarm->fd, dest + dest_pos, length - dest_pos);
        if (nbytes < 0)
            return errno;

        if (nbytes == 0)
            return ENOSPC;

        nbytes = flarm_unescape(dest + dest_pos, dest + dest_pos, (size_t)nbytes);
        if (nbytes < 0)
            return EAGAIN; /* XXX */

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
