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

#ifndef __LOGGERTOOLS_SERIALIO_H
#define __LOGGERTOOLS_SERIALIO_H

#define SERIALIO_FLUSH_INPUT 0x1
#define SERIALIO_FLUSH_OUTPUT 0x2

#define SERIALIO_SELECT_AVAILABLE 0x1
#define SERIALIO_SELECT_READY 0x2

struct serialio;

int serialio_open(const char *device,
                  struct serialio **serialiop);

void serialio_close(struct serialio *serio);

void serialio_flush(struct serialio *serio, unsigned options);

int serialio_select(struct serialio *serio, int options,
                    struct timeval *timeout);

int serialio_read(struct serialio *serio, void *data, size_t *nbytes);

int serialio_write(struct serialio *serio, const void *data, size_t *nbytes);

#endif
