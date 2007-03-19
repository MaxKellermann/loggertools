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

#include "flarm.h"
#include "flarm-internal.h"

/* This code was taken from FLARM_BINCOMM.pdf Revision 0.91: "For crc
 * computation, the XMODEM algorithm is to be applied.  Sample code is
 * given below.  The init value for crc is 0x00."
 */
uint16_t flarm_crc_update(uint16_t crc, uint8_t data) {
    int i;

    crc = crc ^ ((uint16_t)data << 8);
    for (i = 0; i < 8; ++i) {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }

    return crc;
}

uint16_t flarm_crc_update_block(uint16_t crc, const void *src0,
                                size_t length) {
    const uint8_t *src = (const uint8_t*)src0;
    size_t position;

    for (position = 0; position < length; ++position)
        crc = flarm_crc_update(crc, src[position]);

    return crc;
}
