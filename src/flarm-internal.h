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

#ifndef __FLARM_INTERNAL_H
#define __FLARM_INTERNAL_H

#include "flarm.h"
#include "fifo-buffer.h"

/* for ssize_t */
#include <sys/types.h>

#define FLARM_STARTFRAME 0x73
#define FLARM_ESCAPE 0x78
#define FLARM_ESC_ESC 0x55
#define FLARM_ESC_START 0x31

struct flarm {
    int fd;
    int binary_mode;
    unsigned last_seq_no;
    uint8_t *buffer;
    size_t buffer_size;
    int frame_started;
    fifo_buffer_t in, frame;
};

struct flarm_frame_header {
    uint16_t length;
    uint8_t version;
    uint16_t seq_no;
    uint8_t type;
    uint16_t crc;
};

/* flarm-buffer.c */

flarm_result_t
flarm_need_buffer(flarm_t flarm, size_t min_size);

/* flarm-crc.c */

uint16_t flarm_crc_update(uint16_t crc, uint8_t data);

uint16_t flarm_crc_update_block(uint16_t crc, const void *src,
                                size_t length);

/* flarm-escape.c */

size_t flarm_escape(uint8_t *dest, const void *src, size_t length);

flarm_result_t
flarm_unescape(void *dest0,
               const uint8_t *src, size_t src_length,
               size_t *dest_pos_r, size_t *src_pos_r);

#endif
