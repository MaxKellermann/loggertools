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

#include <vector>

#include "tp.hh"
#include "cenfis-db.h"

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
    char title[sizeof(data.title) + 1];
    char description[sizeof(data.description) + 1];
    size_t length;

    /* find table entry */
    while (table_index < 4 && table_position >= table_size)
        nextTable();

    if (table_index >= 4)
        return NULL;

    offset = ((long)table[table_position].index0 << 15)
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

    /* extract title */
    length = sizeof(data.title);
    memcpy(title, data.title, length);
    while (length > 0 && title[length - 1] >= 0 &&
           title[length - 1] <= ' ')
        length--;
    title[length] = 0;

    if (title[0] != 0)
        tp->setTitle(title);

    /* extract description */
    length = sizeof(data.description);
    memcpy(description, data.description, length);
    while (length > 0 && description[length - 1] >= 0 &&
           description[length - 1] <= ' ')
        length--;
    description[length] = 0;

    if (description[0] != 0)
        tp->setDescription(description);

    /* runway */
    if (data.rwy1 > 0)
        tp->setRunway(Runway(Runway::TYPE_UNKNOWN, data.rwy1 * 10, 0));

    return tp;
}

TurnPointReader *CenfisDatabaseFormat::createReader(FILE *file) {
    return new CenfisDatabaseReader(file);
}
