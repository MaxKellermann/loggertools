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

struct header {
    __uint16_t magic1;
    char reserved1[0x06];
    __uint16_t magic2;
    char reserved2[0x36];
    /*
      table 0: other
      table 1: airfield
      table 2: glider site
      table 3: outlanding
     */
    struct {
        __uint32_t offset;
        __uint16_t three, count;
    } tables[4];
    char reserved3[0xe0];
    __uint32_t header_size;
    __uint16_t thirty1, overall_count;
    __uint16_t seven1, zero1;
    __uint32_t zero2;
    __uint32_t after_tp_offset;
    __uint16_t twentyone1, a_1;
    char reserved4[0xa8];
};

struct turn_point {
    char foo1[15];
    char code[28];
    char foo2[5];
};

struct foo {
    char reserved[0x150];
};

struct table_entry {
    unsigned char index0, index1, index2;
};

class CenfisDatabaseWriter : public TurnPointWriter {
private:
    struct header header;
    FILE *file;
    std::vector<long> offsets[4];
public:
    CenfisDatabaseWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

CenfisDatabaseWriter::CenfisDatabaseWriter(FILE *_file)
    :file(_file) {
    long t;
    size_t nmemb;
    unsigned z;

    rewind(file);
    t = ftell(file);
    if (t != 0)
        throw new TurnPointWriterException("cannot seek this stream");

    memset(&header, 0xff, sizeof(header));
    header.magic1 = 0x1046;
    header.magic2 = 0x3141;
    for (z = 0; z < 4; z++) {
        header.tables[z].offset = 0xdeadbeef;
        header.tables[z].three = 3;
        header.tables[z].count = 0xbeef;
    }
    header.header_size = htonl(sizeof(header));
    header.thirty1 = htons(0x30);
    header.overall_count = 0xbeef;
    header.seven1 = htons(0x07);
    header.zero1 = 0;
    header.zero2 = 0;
    header.after_tp_offset = 0xdeadbeef;
    header.twentyone1 = htons(0x21);
    header.a_1 = htons(0x0a);

    nmemb = fwrite(&header, sizeof(header), 1, file);
    if (nmemb < 1)
        throw new TurnPointWriterException("failed to write header");
}

void CenfisDatabaseWriter::write(const TurnPoint &tp) {
    unsigned table;
    struct turn_point data;
    size_t length, nmemb;

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    /* append index to table */
    switch (tp.getType()) {
    case TurnPoint::TYPE_AIRFIELD:
        table = 1;
        break;
    case TurnPoint::TYPE_GLIDER_SITE:
        table = 2;
        break;
    case TurnPoint::TYPE_OUTLANDING:
        table = 3;
        break;
    default:
        table = 0;
    }

    offsets[table].push_back(ftell(file));

    /* fill out entry */
    memset(&data, 0, sizeof(data));

    length = strlen(tp.getCode());
    if (length > sizeof(data.code))
        length = sizeof(data.code);

    memcpy(data.code, tp.getCode(), length);
    memset(data.code + length, ' ', sizeof(data.code) - length);

    nmemb = fwrite(&data, sizeof(data), 1, file);
    if (nmemb != 1)
        throw new TurnPointWriterException("failed to write record");
}

void CenfisDatabaseWriter::flush() {
    long offset;
    struct table_entry entry;
    struct foo foo;
    int ret;
    size_t nmemb;

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    /* write foo */
    memset(&foo, 0xff, sizeof(foo));
    nmemb = fwrite(&foo, sizeof(foo), 1, file);
    if (nmemb != 1)
        throw new TurnPointWriterException("failed to write");

    /* write tables */
    for (unsigned z = 0; z < 4; z++) {
        unsigned size = offsets[z].size();

        header.tables[z].offset = htonl(ftell(file));
        header.tables[z].count = htons(size);

        for (unsigned i = 0; i < size; i++) {
            offset = offsets[z][i];
            entry.index0 = (offset >> 16) & 0xff;
            entry.index1 = (offset >> 8) & 0xff;
            entry.index2 = offset & 0xff;

            nmemb = fwrite(&entry, sizeof(entry), 1, file);
            if (nmemb != 1)
                throw new TurnPointWriterException("failed to write table entry");
        }
    }

    /* re-write header */
    ret = fseek(file, 0, SEEK_SET);
    if (ret < 0)
        throw new TurnPointWriterException("failed to seek");

    nmemb = fwrite(&header, sizeof(header), 1, file);
    if (nmemb < 1)
        throw new TurnPointWriterException("failed to write header");

    fclose(file);
    file = NULL;
}

TurnPointReader *CenfisDatabaseFormat::createReader(FILE *file) {
    (void)file;
    return NULL;
}

TurnPointWriter *CenfisDatabaseFormat::createWriter(FILE *file) {
    return new CenfisDatabaseWriter(file);
}
