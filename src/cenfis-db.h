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

#ifndef __CENFIS_DB_H
#define __CENFIS_DB_H

struct header {
    __uint16_t magic1;
    char reserved1[0x06];
    __uint16_t magic2;
    char reserved2[0x36];
    /*
      table 0: other
      table 1: airfield
      table 2: glider site
      table 3: outlanding
     */
    struct {
        __uint32_t offset;
        __uint16_t three, count;
    } tables[4] __attribute__((packed));
    char reserved3[0xe0];
    __uint32_t header_size;
    __uint16_t thirty1, overall_count;
    __uint16_t seven1, zero1;
    __uint32_t zero2;
    __uint32_t after_tp_offset;
    __uint16_t twentyone1, a_1;
    char reserved4[0xa8];
} __attribute__((packed));

struct turn_point {
    __uint32_t latitude, longitude;
    __uint16_t altitude;
    char type;
    char foo1[1];
    __uint8_t freq[3];
    char code[14];
    char title[14];
    __uint8_t rwy1, rwy2;
    char foo2[3];
} __attribute__((packed));

struct foo {
    char reserved[0x150];
} __attribute__((packed));

struct table_entry {
    unsigned char index0, index1, index2;
} __attribute__((packed));

#endif
