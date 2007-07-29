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

#ifndef __FLARM_H
#define __FLARM_H

#include <stdint.h>
#include <stddef.h>

enum {
    FLARM_MESSAGE_ACK = 0xa0,
    FLARM_MESSAGE_NACK = 0xb7,
    FLARM_MESSAGE_PING = 0x01,
    FLARM_MESSAGE_SETBAUDRATE = 0x02,
    FLARM_MESSAGE_FLASHUPLOAD = 0x10,
    FLARM_MESSAGE_EXIT = 0x12,
    FLARM_MESSAGE_SELECTRECORD = 0x20,
    FLARM_MESSAGE_GETRECORDINFO = 0x21,
    FLARM_MESSAGE_GETIGCDATA = 0x22,
};

enum {
    FLARM_BAUD_4800 = 0,
    FLARM_BAUD_9600 = 1,
    FLARM_BAUD_19200 = 2,
    FLARM_BAUD_38400 = 3,
    FLARM_BAUD_57600 = 4,
};

typedef struct flarm *flarm_t;


#ifdef __cplusplus
extern "C" {
#endif

/* flarm-open.c */

int flarm_fdopen(int fd, flarm_t *flarm_r);

int flarm_open(const char *device_path, flarm_t *flarm_r);

int flarm_fileno(flarm_t flarm);

void flarm_close(flarm_t *flarm_r);

/* flarm-send.c */

int flarm_send_frame(flarm_t flarm, uint8_t type,
                     const void *src, size_t length);

unsigned
flarm_last_seq_no(flarm_t flarm);


/* flarm-message.c */

int
flarm_send_ping(flarm_t flarm);

int
flarm_send_set_baud_rate(flarm_t flarm, int speed);

int
flarm_send_exit(flarm_t flarm);

int
flarm_send_select_record(flarm_t flarm, unsigned record_no);

int
flarm_send_get_record_info(flarm_t flarm);

int
flarm_send_get_igc_data(flarm_t flarm);


/* flarm-recv.c */

int flarm_recv_frame(flarm_t flarm,
                     uint8_t *version_r, uint8_t *type_r,
                     uint16_t *seq_no_r,
                     const void **payload_r, size_t *length_r);

#ifdef __cplusplus
}
#endif


#endif
