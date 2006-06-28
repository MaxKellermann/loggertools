/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
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

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "filser.h"

int filser_write_cmd(int fd, unsigned char cmd) {
    static const unsigned char prefix = FILSER_PREFIX;
    ssize_t nbytes;

    nbytes = write(fd, &prefix, sizeof(prefix));
    if (nbytes <= 0)
        return (int)nbytes;

    nbytes = write(fd, &cmd, sizeof(cmd));
    if (nbytes <= 0)
        return (int)nbytes;

    return 1;
}

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

int filser_write_packet(int fd, unsigned char cmd,
                        const void *packet, size_t length) {
    int ret;

    assert(fd >= 0);
    assert(packet != NULL && length > 0);

    ret = filser_write_cmd(fd, cmd);
    if (ret <= 0)
        return ret;

    ret = filser_write_crc(fd, packet, length);
    if (ret <= 0)
        return ret;

    return 1;
}

int filser_read_crc(int fd, void *p0, size_t length,
                    time_t timeout) {
    unsigned char *p = p0;
    ssize_t nbytes;
    size_t pos = 0;
    unsigned char crc;
    time_t end_time;

    if (timeout > 0)
        end_time = time(NULL) + timeout;

    for (;;) {
        nbytes = read(fd, p + pos, length - pos);
        if (nbytes < 0)
            return -1;

        if (nbytes > 0)
            end_time = time(NULL) + timeout;

        pos += (size_t)nbytes;
        if (pos >= length)
            break;

        if (timeout > 0 && time(NULL) > end_time)
            return 0;
    }

    nbytes = read(fd, &crc, sizeof(crc));
    if (nbytes < 0)
        return -1;
    if ((size_t)nbytes < sizeof(crc))
        return 0;

    if (crc != filser_calc_crc(p0, length))
        return -2;

    return 1;
}
