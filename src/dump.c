/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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

#include "dump.h"

#include <assert.h>
#include <stdio.h>

void hexdump_line(size_t offset,
                  const unsigned char *line,
                  size_t length) {
    size_t i;

    assert(length > 0);
    assert(length <= 0x10);

    if (length == 0)
        return;

    printf("%04x ", (unsigned)offset);

    for (i = 0; i < 0x10; i++) {
        if (i % 8 == 0)
            printf(" ");

        if (i >= length)
            printf("   ");
        else
            printf(" %02x", line[i]);
    }

    printf(" | ");
    for (i = 0; i < length; i++)
        putchar(line[i] >= 0x20 && line[i] < 0x80 ? line[i] : '.');

    printf("\n");
}

void hexdump_buffer(size_t offset,
                    const void *q, size_t length) {
    const unsigned char *p = q;

    while (length >= 0x10) {
        hexdump_line(offset, p, 0x10);

        p += 0x10;
        length -= 0x10;
        offset += 0x10;
    }

    if (length > 0)
        hexdump_line(offset, p, length);
}
