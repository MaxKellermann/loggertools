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

#ifndef __FLARM_H
#define __FLARM_H

#include <stdint.h>
#include <stddef.h>

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

/* flarm-recv.c */

int flarm_recv_frame(flarm_t flarm,
                     uint8_t *version_r, uint8_t *type_r,
                     uint16_t *seq_no_r,
                     const void **payload_r, size_t *length_r);

#ifdef __cplusplus
}
#endif


#endif
