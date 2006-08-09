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

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"
#include "cenfis-db.h"

#include <ostream>
#include <vector>
#include <algorithm>

int operator <(const struct turn_point &a,
               const struct turn_point &b) {
    return memcmp(a.title, b.title, sizeof(a.title)) < 0;
}

class CenfisDatabaseWriter : public TurnPointWriter {
private:
    std::ostream *stream;
    struct header header;
    std::vector<struct turn_point> tps;
    std::vector<unsigned> offsets[4];
public:
    CenfisDatabaseWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

CenfisDatabaseWriter::CenfisDatabaseWriter(std::ostream *_stream)
    :stream(_stream) {
    memset(&header, 0xff, sizeof(header));
    header.magic1 = 0x1046;
    header.magic2 = 0x3141;
    for (unsigned z = 0; z < 4; z++) {
        header.tables[z].offset = 0xdeadbeef;
        header.tables[z].three = htons(3);
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
}

static char toCenfisType(TurnPoint::type_t type) {
    switch (type) {
    case TurnPoint::TYPE_UNKNOWN:
        return 0;

    case TurnPoint::TYPE_AIRFIELD:
        return 1;

    case TurnPoint::TYPE_MILITARY_AIRFIELD:
        return 3;

    case TurnPoint::TYPE_GLIDER_SITE:
        return 2;

    case TurnPoint::TYPE_OUTLANDING:
        return 4;

    case TurnPoint::TYPE_THERMIK:
        return 5;

    default:
        return 0;
    }
}

static int angleToCenfis(const Angle &angle) {
    return angle.refactor(600);
}

static void copyString(char *dest, size_t dest_size,
                       std::string src) {
    size_t length = src.length();

    if (length > dest_size)
        length = dest_size;

    const char *data = src.data();

    for (size_t i = 0; i < length; ++i)
        dest[i] = toupper(data[i]);

    memset(dest + length, ' ', dest_size - length);
}

void CenfisDatabaseWriter::write(const TurnPoint &tp) {
    struct turn_point data;

    if (stream == NULL)
        throw already_flushed();

    if (tps.size() >= 0xffff)
        throw TurnPointWriterException("too many turn points");

    memset(&data, 0, sizeof(data));

    /* fill out entry */

    data.type = toCenfisType(tp.getType());

    if (tp.getPosition().defined()) {
        data.latitude = htonl(angleToCenfis(tp.getPosition().getLatitude()));
        data.longitude = htonl(angleToCenfis(tp.getPosition().getLongitude()));
        data.altitude = htons(tp.getPosition().getAltitude().getValue());
    }

    if (tp.getFrequency().defined()) {
        unsigned freq = tp.getFrequency().getKiloHertz();
        data.freq[0] = freq >> 16;
        data.freq[1] = freq >> 8;
        data.freq[2] = freq;
    }

    copyString(data.title, sizeof(data.title),
               tp.getTitle());
    copyString(data.description, sizeof(data.description),
               tp.getDescription());

    if (tp.getRunway().defined())
        data.rwy1 = tp.getRunway().getDirection() / 10;

    /* write entry */

    tps.push_back(data);
}

static int typeToTable(char type) {
    switch (type) {
    case 0:
        return 0;

    case 1:
    case 3:
        return 1;

    case 2:
        return 2;

    case 4:
        return 3;

    default:
        return -1;
    }
}

void CenfisDatabaseWriter::flush() {
    struct table_entry entry;
    struct foo foo;
    u_int32_t foo_offset, table_offset;

    if (stream == NULL)
        throw already_flushed();

    foo_offset = sizeof(header) + sizeof(struct turn_point) * tps.size();
    table_offset = foo_offset + sizeof(struct foo);

    /* sort TPs alphabetically */

    sort(tps.begin(), tps.end());

    /* build tables */

    unsigned n = 0;
    for (std::vector<struct turn_point>::const_iterator it = tps.begin();
         it != tps.end(); ++it, ++n) {
        int table = typeToTable((*it).type);
        if (table >= 0) {
            unsigned offset = sizeof(header)
                + sizeof(struct turn_point) * n;
            offsets[table].push_back(offset);

            /* glider site also goes into "airfield" table */
            if (table == 2)
                offsets[1].push_back(offset);
        }
    }

    /* update header */

    for (unsigned i = 0; i < 4; ++i) {
        unsigned size = offsets[i].size();

        header.tables[i].offset = htonl(table_offset);
        header.tables[i].count = htons(size);

        table_offset += size * sizeof(struct table_entry);
    }

    header.overall_count = htons(tps.size());
    header.after_tp_offset = htonl(foo_offset);

    /* write header */

    stream->write((char*)&header, sizeof(header));

    /* write all TPs */

    for (std::vector<struct turn_point>::iterator it = tps.begin();
         it != tps.end(); ++it) {
        struct turn_point *tp = &(*it);

        stream->write((char*)tp, sizeof(*tp));
    }

    /* write foo */
    memset(&foo, 0xff, sizeof(foo));
    stream->write((char*)&foo, sizeof(foo));

    /* write tables */
    for (unsigned z = 0; z < 4; z++) {
        unsigned size = offsets[z].size();

        for (unsigned i = 0; i < size; i++) {
            unsigned offset = offsets[z][i];
            entry.index0 = (offset >> 15) & 0xff;
            entry.index1 = (offset >> 8) & 0x7f;
            entry.index2 = offset & 0xff;

            stream->write((char*)&entry, sizeof(entry));
        }
    }

    stream = NULL;
}

TurnPointWriter *
CenfisDatabaseFormat::createWriter(std::ostream *stream) const {
    return new CenfisDatabaseWriter(stream);
}
