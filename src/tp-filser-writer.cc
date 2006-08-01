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
#include "filser.h"

#include <ostream>

class FilserTurnPointWriter : public TurnPointWriter {
private:
    std::ostream *stream;
    unsigned count;
public:
    FilserTurnPointWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

FilserTurnPointWriter::FilserTurnPointWriter(std::ostream *_stream)
    :stream(_stream), count(0) {
}

void FilserTurnPointWriter::write(const TurnPoint &tp) {
    struct filser_turn_point data;
    size_t length;

    if (stream == NULL)
        throw new TurnPointWriterException("already flushed");

    if (count >= 600)
        throw new TurnPointWriterException("Filser databases cannot hold more than 600 turn points");

    memset(&data, 0, sizeof(data));

    if (tp.getCode().length() > 0) {
        length = tp.getCode().length();
        if (length > sizeof(data.code))
            length = sizeof(data.code);

        memcpy(data.code, tp.getCode().c_str(), length);
    } else {
        length = 0;
    }

    memset(data.code + length, ' ', sizeof(data.code) - length);

    /* write entry */
    stream->write((char*)&data, sizeof(data));
    if (stream->bad())
        throw new TurnPointWriterException("failed to write record");

    count++;
}

void FilserTurnPointWriter::flush() {
    struct filser_turn_point data;
    char zero[6900];

    if (stream == NULL)
        throw new TurnPointWriterException("already flushed");

    memset(&data, 0, sizeof(data));
    for (; count < 600; count++) {
        stream->write((char*)&data, sizeof(data));
        if (stream->bad())
            throw new TurnPointWriterException("failed to write record");
    }

    memset(zero, 0, sizeof(zero));
    stream->write(zero, sizeof(zero));
    if (stream->bad())
        throw new TurnPointWriterException("failed to write tail");

    stream = NULL;
}

TurnPointWriter *
FilserTurnPointFormat::createWriter(std::ostream *stream) const {
    return new FilserTurnPointWriter(stream);
}
