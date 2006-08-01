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

class FilserTurnPointWriter : public TurnPointWriter {
private:
    FILE *file;
    unsigned count;
public:
    FilserTurnPointWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

FilserTurnPointWriter::FilserTurnPointWriter(FILE *_file)
    :file(_file), count(0) {
}

void FilserTurnPointWriter::write(const TurnPoint &tp) {
    struct filser_turn_point data;
    size_t length, nmemb;

    if (file == NULL)
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
    nmemb = fwrite(&data, sizeof(data), 1, file);
    if (nmemb != 1)
        throw new TurnPointWriterException("failed to write record");

    count++;
}

void FilserTurnPointWriter::flush() {
    struct filser_turn_point data;
    size_t nmemb;
    char zero[6900];

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    memset(&data, 0, sizeof(data));
    for (; count < 600; count++) {
        nmemb = fwrite(&data, sizeof(data), 1, file);
        if (nmemb != 1)
            throw new TurnPointWriterException("failed to write record");
    }

    memset(zero, 0, sizeof(zero));
    nmemb = fwrite(zero, sizeof(zero), 1, file);
    if (nmemb != 1)
        throw new TurnPointWriterException("failed to write tail");

    fclose(file);
    file = NULL;
}

TurnPointWriter *FilserTurnPointFormat::createWriter(FILE *file) {
    return new FilserTurnPointWriter(file);
}
