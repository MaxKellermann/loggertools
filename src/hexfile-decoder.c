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

#include "hexfile-decoder.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum hexfile_decoder_state {
    HEXFILE_DECODER_NONE = 0,
    HEXFILE_DECODER_RECORD,
    HEXFILE_DECODER_NIBBLE
};

struct hexfile_decoder {
    hexfile_decoder_callback_t callback;
    void *ctx;
    enum hexfile_decoder_state state;
    int nibble;
    unsigned char record[0x100];
    unsigned record_position, record_length;
};

int hexfile_decoder_new(hexfile_decoder_callback_t callback, void *ctx,
                        struct hexfile_decoder **hfd_r) {
    struct hexfile_decoder *hfd;

    assert(callback != NULL);
    assert(hfd_r != NULL);
    assert(*hfd_r == NULL);

    hfd = (struct hexfile_decoder*)calloc(1, sizeof(*hfd));
    if (hfd == NULL)
        return -1;

    hfd->callback = callback;
    hfd->ctx = ctx;

    *hfd_r = hfd;

    return 0;
}

int hexfile_decoder_close(struct hexfile_decoder **hfd_r) {
    struct hexfile_decoder *hfd = *hfd_r;
    int ret = 0;

    assert(hfd_r != NULL && *hfd_r != NULL);

    if (hfd->state != HEXFILE_DECODER_NONE) {
        errno = EINVAL;
        ret = -1;
    }

    free(hfd);

    *hfd_r = NULL;

    return ret;
}

static int decode_hex_digit(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';

    if (ch >= 'a' && ch <= 'z')
        return 10 + ch - 'a';

    if (ch >= 'A' && ch <= 'Z')
        return 10 + ch - 'A';

    return -1;
}

static unsigned char calc_checksum(unsigned char *p, size_t length) {
    size_t i;
    unsigned char checksum = 0;

    for (i = 0; i < length; ++i)
        checksum += p[i];

    return checksum;
}

static int handle_raw_record(struct hexfile_decoder *hfd) {
    assert(hfd->record_length >= 5);
    assert(hfd->record_length < sizeof(hfd->record));
    assert(hfd->record_position == hfd->record_length);
    assert(hfd->state == HEXFILE_DECODER_RECORD);

    if (calc_checksum(hfd->record, hfd->record_length) != 0) {
        errno = EIO;
        return -1;
    }

    hfd->state = HEXFILE_DECODER_NONE;
    hfd->record_position = 0;
    hfd->record_length = 0;

    return hfd->callback(hfd->ctx, hfd->record[3],
                         hfd->record[1] * 0x100 | hfd->record[2],
                         &hfd->record[4], hfd->record[0]);
}

static int append_digit(struct hexfile_decoder *hfd, char ch) {
    int digit = decode_hex_digit(ch);
    if (digit < 0) {
        errno = EINVAL;
        return -1;
    }

    if (hfd->record_position >= sizeof(hfd->record)) {
        errno = E2BIG;
        return -1;
    }

    assert(hfd->state == HEXFILE_DECODER_RECORD ||
           hfd->state == HEXFILE_DECODER_NIBBLE);

    if (hfd->state == HEXFILE_DECODER_RECORD) {
        hfd->nibble = digit;
        hfd->state = HEXFILE_DECODER_NIBBLE;

        return 0;
    } else if (hfd->state == HEXFILE_DECODER_NIBBLE) {
        hfd->record[hfd->record_position++] = hfd->nibble * 0x10 + digit;
        hfd->state = HEXFILE_DECODER_RECORD;

        if (hfd->record_position == 1) {
            assert(hfd->record_length == 0);
            hfd->record_length = 1 + 2 + 1 + hfd->record[0] + 1;
        }

        if (hfd->record_position == hfd->record_length)
            return handle_raw_record(hfd);

        return 0;
    } else {
        errno = ENOSYS;
        return -1;
    }
}

int hexfile_decoder_feed(struct hexfile_decoder *hfd,
                         char *buffer, size_t length) {
    size_t i;
    int ret;

    while (length > 0) {
        if (hfd->state == HEXFILE_DECODER_NONE) {
            char *colon = (char*)memchr(buffer, ':', length);
            if (colon == NULL)
                return 0;

            length -= colon - buffer + 1;
            buffer = colon + 1;

            hfd->state = HEXFILE_DECODER_RECORD;
        }

        for (i = 0; hfd->state != HEXFILE_DECODER_NONE && i < length; ++i) {
            ret = append_digit(hfd, buffer[i]);
            if (ret < 0)
                return -1;
        }

        buffer += i;
        length -= i;
    }

    return 0;
}
