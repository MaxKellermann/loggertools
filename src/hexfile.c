/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
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
#include <stdio.h>

void write_record(FILE *file, size_t length, unsigned address,
                  unsigned type, const unsigned char *data) {
    unsigned z;
    unsigned char checksum = 0;

    /* write header */
    fprintf(file, ":%02X%04X%02X",
            length & 0xff, address & 0xffff,
            type & 0xff);

    checksum -= length + (address >> 8) + address + type;

    /* write data */
    for (z = 0; z < length; z++) {
        fprintf(file, "%02X", data[z] & 0xff);
        checksum -= data[z];
    }

    /* write checksum */
    fprintf(file, "%02X\n", checksum & 0xff);
}

int main(int argc, char **argv) {
    const int fd = 0;
    FILE *const file = stdout;
    unsigned address = 0;
    unsigned char buffer[0x10];
    ssize_t nbytes;

    (void)argc;
    (void)argv;

    /* write data records, 16 bytes each */
    while ((nbytes = read(fd, buffer, sizeof(buffer))) > 0) {
        write_record(file, (size_t)nbytes, address,
                     0, buffer);

        address += (size_t)nbytes;
    }

    /* write end-of-file record */
    write_record(file, 0, 0, 1, NULL);

    return 0;
}
