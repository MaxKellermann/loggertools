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

#include "zander-internal.h"

#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int zander_read_serial(zander_t zander,
                       struct zander_serial *serial) {
    static const unsigned char cmd = ZANDER_CMD_READ_SERIAL;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_read(zander, serial, sizeof(*serial));
}

int zander_write_memory(zander_t zander, unsigned address,
                        const void *data, size_t length) {
    struct zander_write_data header;
    int ret;

    assert(data != NULL);

    /* zander has only 64kB writable memory */
    if (length == 0)
        return EINVAL;

    if (length > 1000 || address + length > 0x10000)
        return EFBIG;

    memset(&header, 0, sizeof(header));
    header.address = htons(address);

    ret = zander_write(zander, &header, sizeof(header));
    if (ret != 0)
        return ret;

    ret = zander_write(zander, data, length);
    if (ret != 0)
        return ret;

    return 0;
}
