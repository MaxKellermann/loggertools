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
    __uint32_t latitude, longitude;
    __uint16_t altitude;
    char type;
    char foo1[1];
    __uint8_t freq[3];
    char code[14];
    char title[14];
    __uint8_t rwy1, rwy2;
    char foo2[3];
};

struct foo {
    char reserved[0x150];
};

struct table_entry {
    unsigned char index0, index1, index2;
};

class CenfisDatabaseReader : public TurnPointReader {
private:
    FILE *file;
    struct header header;
    unsigned table_index, table_position, table_size;
    struct table_entry *table;
public:
    CenfisDatabaseReader(FILE *_file);
    virtual ~CenfisDatabaseReader();
protected:
    void nextTable();
public:
    virtual const TurnPoint *read();
};

class CenfisDatabaseWriter : public TurnPointWriter {
private:
    FILE *file;
    struct header header;
    std::vector<long> offsets[4];
public:
    CenfisDatabaseWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

CenfisDatabaseReader::CenfisDatabaseReader(FILE *_file)
    :file(_file),
     table_index(UINT_MAX), table(NULL) {
    long t;
    size_t nmemb;

    rewind(file);
    t = ftell(file);
    if (t != 0)
        throw new TurnPointReaderException("cannot seek this stream");

    nmemb = fread(&header, sizeof(header), 1, file);
    if (nmemb != 1)
        throw new TurnPointReaderException("failed to read header");

    if (ntohs(header.magic1) != 0x4610 &&
        ntohs(header.magic2) != 0x4131)
        throw new TurnPointReaderException("wrong magic");

    nextTable();
}

CenfisDatabaseReader::~CenfisDatabaseReader() {
    if (table != NULL)
        free(table);
}

void CenfisDatabaseReader::nextTable() {
    long offset;
    int ret;
    size_t nmemb;

    if (table != NULL) {
        free(table);
        table = NULL;
    }

    if (table_index == UINT_MAX)
        table_index = 0;
    else
        table_index++;

    if (table_index >= 4) {
        table_position = 0;
        table_size = 0;
        return;
    }

    table_position = 0;
    table_size = ntohs(header.tables[table_index].count);
    if (table_size == 0)
        return;

    offset = ntohl(header.tables[table_index].offset);
    ret = fseek(file, offset, SEEK_SET);
    if (ret < 0)
        throw new TurnPointReaderException("failed to seek");

    table = (struct table_entry*)malloc(sizeof(*table) * table_size);
    if (table == NULL)
        throw new TurnPointReaderException("out of memory");

    nmemb = fread(table, sizeof(*table), table_size, file);
    if (nmemb != table_size)
        throw new TurnPointReaderException("failed to read table");
}

const TurnPoint *CenfisDatabaseReader::read() {
    long offset;
    int ret;
    struct turn_point data;
    size_t nmemb;
    TurnPoint *tp;
    char code[sizeof(data.code) + 1], title[sizeof(data.title) + 1];
    size_t length;

    /* find table entry */
    while (table_index < 4 && table_position >= table_size)
        nextTable();

    if (table_index >= 4)
        return NULL;

    offset = ((long)table[table_position].index0 << 16)
        + ((long)table[table_position].index1 << 8)
        + (long)table[table_position].index2;

    table_position++;

    /* read this record */
    ret = fseek(file, offset, SEEK_SET);
    if (ret < 0)
        throw new TurnPointReaderException("failed to seek");

    nmemb = fread(&data, sizeof(data), 1, file);
    if (nmemb != 1)
        throw new TurnPointReaderException("failed to read data");

    /* create object */
    tp = new TurnPoint();

    /* position */
    tp->setPosition(Position(Angle((ntohl(data.latitude)*10)/6),
                             Angle(-((ntohl(data.longitude)*10)/6)),
                             Altitude(ntohs(data.altitude),
                                      Altitude::UNIT_METERS,
                                      Altitude::REF_MSL)));

    /* type */
    switch (data.type) {
    case 1:
        tp->setType(TurnPoint::TYPE_AIRFIELD);
        break;
    case 2:
        tp->setType(TurnPoint::TYPE_GLIDER_SITE);
        break;
    case 3:
        tp->setType(TurnPoint::TYPE_MILITARY_AIRFIELD);
        break;
    case 4:
        tp->setType(TurnPoint::TYPE_OUTLANDING);
        break;
    case 5:
        tp->setType(TurnPoint::TYPE_THERMIK);
        break;
    default:
        tp->setType(TurnPoint::TYPE_UNKNOWN);
    }

    /* frequency */
    tp->setFrequency(((data.freq[0] << 16) +
                      (data.freq[1] << 8) +
                      data.freq[2]) * 1000);

    /* extract code */
    length = sizeof(data.code);
    memcpy(code, data.code, length);
    while (length > 0 && code[length - 1] >= 0 &&
           code[length - 1] <= ' ')
        length--;
    code[length] = 0;

    if (code[0] != 0)
        tp->setCode(code);

    /* extract title */
    length = sizeof(data.title);
    memcpy(title, data.title, length);
    while (length > 0 && title[length - 1] >= 0 &&
           title[length - 1] <= ' ')
        length--;
    title[length] = 0;

    if (title[0] != 0)
        tp->setTitle(title);

    /* runway */
    if (data.rwy1 > 0)
        tp->setRunway(Runway(Runway::TYPE_UNKNOWN, data.rwy1 * 10, 0));

    return tp;
}

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

    memset(&data, 0, sizeof(data));

    /* append index to table */
    switch (tp.getType()) {
    case TurnPoint::TYPE_AIRFIELD:
        table = 1;
        data.type = 1;
        break;
    case TurnPoint::TYPE_MILITARY_AIRFIELD:
        table = 1;
        data.type = 3;
        break;
    case TurnPoint::TYPE_GLIDER_SITE:
        table = 2;
        data.type = 2;
        break;
    case TurnPoint::TYPE_OUTLANDING:
        table = 3;
        data.type = 4;
        break;
    case TurnPoint::TYPE_THERMIK:
        table = 0;
        data.type = 5;
        break;
    default:
        table = 0;
        data.type = 0;
    }

    offsets[table].push_back(ftell(file));

    /* fill out entry */
    if (tp.getPosition().defined()) {
        data.latitude = htonl((tp.getPosition().getLatitude().getValue() * 600) / 1000);
        data.longitude = htonl((tp.getPosition().getLongitude().getValue() * 600) / 1000);
        data.altitude = htons(tp.getPosition().getAltitude().getValue());
    }

    if (tp.getFrequency() > 0) {
        unsigned freq = tp.getFrequency() / 1000;
        data.freq[0] = freq >> 16;
        data.freq[1] = freq >> 8;
        data.freq[2] = freq;
    }

    length = strlen(tp.getCode());
    if (length > sizeof(data.code))
        length = sizeof(data.code);

    memcpy(data.code, tp.getCode(), length);
    memset(data.code + length, ' ', sizeof(data.code) - length);

    length = strlen(tp.getTitle());
    if (length > sizeof(data.title))
        length = sizeof(data.title);

    memcpy(data.title, tp.getTitle(), length);
    memset(data.title + length, ' ', sizeof(data.title) - length);

    if (tp.getRunway().defined())
        data.rwy1 = tp.getRunway().getDirection() / 10;

    /* write entry */
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
    return new CenfisDatabaseReader(file);
}

TurnPointWriter *CenfisDatabaseFormat::createWriter(FILE *file) {
    return new CenfisDatabaseWriter(file);
}
