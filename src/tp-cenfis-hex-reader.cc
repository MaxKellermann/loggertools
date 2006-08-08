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

#include <netinet/in.h>

#include "tp.hh"
#include "tp-io.hh"
#include "cenfis-db.h"
#include "hexfile-decoder.h"

#include <assert.h>
#include <errno.h>
#include <istream>
#include <sstream>

const unsigned BANK_SIZE = 0x8000;

struct decoded_hexfile {
    size_t start, length, max_length;
    unsigned char *data;
    size_t base;
    int eof;
};

static int expand(struct decoded_hexfile *dh,
                  size_t new_length) {
    unsigned char *new_data;

    if (new_length <= dh->max_length)
        return 0;

    if (new_length > 1024 * 1024 * 16) { // 16 MB limit
        errno = ENOMEM;
        return -1;
    }

    new_length |= 0x1fff; // round up to 8k

    new_data = (unsigned char*)realloc(dh->data, new_length);
    if (new_data == NULL)
        return -1;

    dh->data = new_data;
    dh->max_length = new_length;

    return 0;
}

static int write_data(struct decoded_hexfile *dh,
                      size_t offset,
                      unsigned char *p, size_t length) {
    int ret;

    if (offset + length < dh->start)
        return 0;

    if (offset < dh->start) {
        length -= dh->start - offset;
        p += dh->start - offset;
        offset = dh->start;
    }

    offset -= dh->start;

    ret = expand(dh, offset + length);
    if (ret < 0)
        return -1;

    memcpy(dh->data + offset, p, length);
    if (offset + length > dh->length)
        dh->length = offset + length;

    return 0;
}

static int handle_hexfile_record(void *ctx,
                                 unsigned char type, unsigned offset,
                                 unsigned char *data, size_t length) {
    struct decoded_hexfile *dh = (struct decoded_hexfile*)ctx;

    if (dh->eof) {
        errno = EINVAL;
        return -1;
    }

    if (type == 0x00) {
        return write_data(dh, dh->base + offset, data, length);
    } else if (type == 0x01) {
        dh->eof = 1;
        return 0;
    } else if (type >= 0x10) {
        dh->base = (type - 0x10) * BANK_SIZE;
        return 0;
    } else {
        errno = ENOSYS;
        return -1;
    }
}

static int decode_hexfile(std::istream *stream,
                          size_t start,
                          struct decoded_hexfile **dh_r) {
    struct decoded_hexfile dh;
    int ret;
    struct hexfile_decoder *hfd = NULL;

    assert(dh_r != NULL);
    assert(*dh_r == NULL);

    memset(&dh, 0, sizeof(dh));
    dh.start = start;

    ret = hexfile_decoder_new(handle_hexfile_record, &dh, &hfd);
    if (ret < 0)
        return -1;

    while (1) {
        char buffer[4096];
        std::streamsize nbytes = stream->readsome(buffer, sizeof(buffer));
        if (nbytes > 0) {
            ret = hexfile_decoder_feed(hfd, buffer, nbytes);
            if (ret < 0) {
                if (dh.data != NULL)
                    free(dh.data);
                return -1;
            }
        } else {
            break;
        }
    }

    ret = hexfile_decoder_close(&hfd);
    if (ret < 0)
        return -1;

    if (stream->bad()) {
        if (dh.data != NULL)
            free(dh.data);
        errno = EIO;
        return -1;
    }

    if (!dh.eof) {
        if (dh.data != NULL)
            free(dh.data);
        errno = EINVAL;
        return -1;
    }

    *dh_r = new decoded_hexfile;
    if (*dh_r == NULL) {
        if (dh.data != NULL)
            free(dh.data);
        return -1;
    }

    memcpy(*dh_r, &dh, sizeof(dh));
    return 0;
}

static void free_hexfile(struct decoded_hexfile **dh_r) {
    assert(dh_r != NULL);
    assert(*dh_r != NULL);

    struct decoded_hexfile *dh = *dh_r;
    if (dh->data != NULL)
        free(dh->data);

    if (dh != NULL)
        delete dh;
}


class CenfisHexReader : public TurnPointReader {
private:
    struct decoded_hexfile *dh;
    std::istream *stream;
    TurnPointReader *tpr;
public:
    CenfisHexReader(std::istream *stream);
    virtual ~CenfisHexReader();
public:
    virtual const TurnPoint *read();
};

CenfisHexReader::CenfisHexReader(std::istream *_stream)
    :dh(NULL), stream(NULL), tpr(NULL) {
    int ret = decode_hexfile(_stream, 0, &dh);
    if (ret < 0)
        throw TurnPointReaderException("failed to read hexfile");

    stream = new std::istringstream(std::string((const char*)dh->data, dh->length));

    const TurnPointFormat *format = getTurnPointFormat("dab");
    if (format == NULL)
        throw TurnPointWriterException("no such format: dab");

    tpr = format->createReader(stream);
    if (tpr == NULL)
        throw TurnPointWriterException("failed to create dab reader");
}

CenfisHexReader::~CenfisHexReader() {
    if (tpr != NULL)
        delete tpr;

    if (stream != NULL)
        delete stream;

    if (dh != NULL)
        free_hexfile(&dh);
}

const TurnPoint *CenfisHexReader::read() {
    if (tpr != NULL)
        return tpr->read();

    return NULL;
}

TurnPointReader *
CenfisHexTurnPointFormat::createReader(std::istream *stream) const {
    return new CenfisHexReader(stream);
}
