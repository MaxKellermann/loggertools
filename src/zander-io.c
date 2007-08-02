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

#include "zander-internal.h"

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

int zander_write(zander_t zander, const void *data, size_t length) {
    ssize_t nbytes;

    assert(zander != NULL);
    assert(zander->fd >= 0);

    nbytes = write(zander->fd, data, length);
    if (nbytes < 0)
        return errno;
    if ((size_t)nbytes < length)
        return ENOSPC;

    return 0;
}

int zander_read(zander_t zander, void *p0, size_t length) {
    static const time_t timeout = 2;
    unsigned char *p = p0;
    ssize_t nbytes;
    size_t pos = 0;
    time_t end_time = 0;

    if (timeout > 0)
        end_time = time(NULL) + timeout;

    for (;;) {
        nbytes = read(zander->fd, p + pos, length - pos);
        if (nbytes < 0)
            return errno;

        if (nbytes > 0)
            end_time = time(NULL) + timeout;

        pos += (size_t)nbytes;
        if (pos >= length)
            return 0;

        if (timeout > 0 && time(NULL) > end_time)
            return EINTR;
    }
}
