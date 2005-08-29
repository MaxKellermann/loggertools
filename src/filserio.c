/*
 * loggertools
 * Copyright (C) 2004-2005 Max Kellermann (max@duempel.org)
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

#include <sys/types.h>
#include <unistd.h>

#include "filser.h"

int filser_write_crc(int fd, const void *p0, size_t length) {
    const unsigned char *p = p0;
    ssize_t nbytes;
    unsigned char crc;
    size_t pos = 0, sublen;

    while (pos < length) {
        if (pos > 0) {
            struct timeval tv;

            /* we have to throttle here ... */
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
            select(0, NULL, NULL, NULL, &tv);
        }

        sublen = length - pos;
        if (sublen > 1024)
            sublen = 1024;

        nbytes = write(fd, p + pos, sublen);
        if (nbytes < 0)
            return -1;

        if ((size_t)nbytes != sublen)
            return 0;

        pos += sublen;
    }

    crc = filser_calc_crc(p, length);

    nbytes = write(fd, &crc, sizeof(crc));
    if (nbytes < 0)
        return -1;

    if ((size_t)nbytes != sizeof(crc))
        return 0;

    return 1;
}
