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

#include "fifo-buffer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct fifo_buffer {
    size_t size, start, end;
    unsigned char buffer[1];
};

int fifo_buffer_new(size_t size, fifo_buffer_t *buffer_r) {
    fifo_buffer_t buffer;

    assert(size > 0);
    assert(buffer_r != NULL);

    buffer = (fifo_buffer_t)malloc(sizeof(*buffer) - sizeof(buffer->buffer) + size);
    if (buffer == NULL)
        return errno;

    buffer->size = size;
    buffer->start = 0;
    buffer->end = 0;

    *buffer_r = buffer;
    return 0;
}

void fifo_buffer_delete(fifo_buffer_t *buffer_r) {
    fifo_buffer_t buffer;

    assert(buffer_r != NULL);
    assert(*buffer_r != NULL);

    buffer = *buffer_r;
    *buffer_r = NULL;

    free(buffer);
}

void fifo_buffer_clear(fifo_buffer_t buffer) {
    assert(buffer != NULL);
    buffer->start = 0;
    buffer->end = 0;
}

const void *fifo_buffer_read(const fifo_buffer_t buffer, size_t *length_r) {
    assert(buffer != NULL);
    assert(buffer->end >= buffer->start);
    assert(length_r != NULL);

    if (buffer->start == buffer->end)
        return NULL;

    *length_r = buffer->end - buffer->start;
    return buffer->buffer + buffer->start;
}

void fifo_buffer_consume(fifo_buffer_t buffer, size_t length) {
    assert(buffer != NULL);
    assert(buffer->end >= buffer->start);
    assert(buffer->start + length <= buffer->end);

    buffer->start += length;
}

static void fifo_buffer_move(fifo_buffer_t buffer) {
    if (buffer->start == 0)
        return;

    if (buffer->end > buffer->start)
        memmove(buffer->buffer,
                buffer->buffer + buffer->start,
                buffer->end - buffer->start);
    buffer->end -= buffer->start;
    buffer->start = 0;
}

void *fifo_buffer_write(fifo_buffer_t buffer, size_t *max_length_r) {
    assert(buffer != NULL);
    assert(buffer->end <= buffer->size);
    assert(max_length_r != NULL);

    if (buffer->end == buffer->size) {
        fifo_buffer_move(buffer);
        if (buffer->end == buffer->size)
            return NULL;
    }

    *max_length_r = buffer->size - buffer->end;
    return buffer->buffer + buffer->end;
}

void fifo_buffer_append(fifo_buffer_t buffer, size_t length) {
    assert(buffer != NULL);
    assert(buffer->end >= buffer->start);
    assert(buffer->end + length <= buffer->size);

    buffer->end += length;
}
