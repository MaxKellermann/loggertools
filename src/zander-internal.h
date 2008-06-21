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

#ifndef __ZANDER_INTERNAL_H
#define __ZANDER_INTERNAL_H

#include "zander.h"

struct zander {
    int fd;
};

enum zander_cmd {
    ZANDER_CMD_WRITE_MEMORY = 0x02,
    ZANDER_CMD_READ_9V_BATTERY = 0x0a,
    ZANDER_CMD_READ_LI_BATTERY = 0x0d,
    ZANDER_CMD_READ_SERIAL = 0x0f,
    ZANDER_CMD_WRITE_PERSONAL_DATA = 0x10,
    ZANDER_CMD_READ_PERSONAL_DATA = 0x11,
    ZANDER_CMD_WRITE_TASK = 0x12,
    ZANDER_CMD_READ_TASK = 0x13,
};

struct zander_address {
    unsigned char address[3];
} __attribute__((packed));

struct zander_write_data {
    struct zander_address address;
    /*unsigned char data[x];*/
} __attribute__((packed));


/* zander-io.c */

int zander_write(zander_t zander, const void *data, size_t length);

int zander_read(zander_t zander, void *p, size_t length);


#endif
