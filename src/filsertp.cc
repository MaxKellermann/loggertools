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

#include <netinet/in.h>

#include <vector>

#include "tp.hh"

#include "filser.h"

class FilserTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    unsigned long counts[sizeof(struct filser_turn_point)];
    unsigned c;
public:
    FilserTurnPointReader(FILE *_file);
public:
    virtual const TurnPoint *read();
};

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

FilserTurnPointReader::FilserTurnPointReader(FILE *_file)
    :file(_file) {
    memset(counts, 0, sizeof(counts));
    c = 0;
}

const TurnPoint *FilserTurnPointReader::read() {
    struct filser_turn_point data;
    size_t nmemb;
    TurnPoint *tp;
    char code[sizeof(data.code) + 1];
    size_t length;
    const unsigned char *p = (const unsigned char*)(const void*)&data;

    do {
        nmemb = fread(&data, sizeof(data), 1, file);
        if (nmemb != 1)
            return NULL;
    } while (data.valid != 1);

    fprintf(stderr, "avg:");
    c++;
    for (length = 0; length < sizeof(data); length++) {
        counts[length] += p[length];
        fprintf(stderr, " %lx", counts[length] / c);
    }
    fprintf(stderr, "\n");

    /* create object */
    tp = new TurnPoint();

    /* extract code */
    length = sizeof(data.code);
    memcpy(code, data.code, length);
    while (length > 0 && code[length - 1] >= 0 &&
           code[length - 1] <= ' ')
        length--;
    code[length] = 0;

    if (code[0] != 0) {
        tp->setTitle(code);
        tp->setCode(code);
    }

    return tp;
}

FilserTurnPointWriter::FilserTurnPointWriter(FILE *_file)
    :file(_file), count(0) {
}

void FilserTurnPointWriter::write(const TurnPoint &tp) {
    struct filser_turn_point data;
    size_t length, nmemb;

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    memset(&data, 0, sizeof(data));

    length = strlen(tp.getCode());
    if (length > sizeof(data.code))
        length = sizeof(data.code);

    memcpy(data.code, tp.getCode(), length);
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
    for (; count < 6200; count++) {
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

TurnPointReader *FilserTurnPointFormat::createReader(FILE *file) {
    return new FilserTurnPointReader(file);
}

TurnPointWriter *FilserTurnPointFormat::createWriter(FILE *file) {
    return new FilserTurnPointWriter(file);
}
