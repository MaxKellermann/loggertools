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

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"
#include "cenfis-db.h"

#include <istream>

class CenfisDatabaseReader : public TurnPointReader {
private:
    std::istream *stream;
    struct header header;
    unsigned current, overall_count;
public:
    CenfisDatabaseReader(std::istream *stream);
    virtual ~CenfisDatabaseReader();
public:
    virtual const TurnPoint *read();
};

CenfisDatabaseReader::CenfisDatabaseReader(std::istream *_stream)
    :stream(_stream), current(0), overall_count(0) {
    stream->read((char*)&header, sizeof(header));

    if (ntohs(header.magic1) != 0x4610 &&
        ntohs(header.magic2) != 0x4131)
        throw malformed_input("wrong magic");

    if (ntohl(header.header_size) != sizeof(header))
        throw malformed_input("wrong header size");

    overall_count = ntohs(header.overall_count);

    if (ntohl(header.after_tp_offset) != sizeof(header) + sizeof(struct turn_point) * overall_count)
        throw malformed_input("wrong header size");
}

CenfisDatabaseReader::~CenfisDatabaseReader() {
}

template<class T>
const T cenfisToAngle(int value) {
    return T(value, 600);
}

const TurnPoint *CenfisDatabaseReader::read() {
    struct turn_point data;
    TurnPoint *tp;
    char title[sizeof(data.title) + 1];
    char description[sizeof(data.description) + 1];
    size_t length;

    if (current >= overall_count)
        return NULL;

    /* read this record */
    stream->read((char*)&data, sizeof(data));

    ++current;

    /* create object */
    tp = new TurnPoint();

    /* position */
    tp->setPosition(Position(cenfisToAngle<Latitude>(ntohl(data.latitude)),
                             cenfisToAngle<Longitude>(-ntohl(data.longitude)),
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
    default:
        tp->setType(TurnPoint::TYPE_UNKNOWN);
    }

    /* frequency */
    tp->setFrequency(Frequency(((data.freq[0] << 16) +
                                (data.freq[1] << 8) +
                                data.freq[2]) * 1000));

    /* extract title */
    length = sizeof(data.title);
    memcpy(title, data.title, length);
    while (length > 0 && title[length - 1] >= 0 &&
           title[length - 1] <= ' ')
        length--;
    title[length] = 0;

    if (title[0] != 0)
        tp->setFullName(title);

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

    if (data.rwy1 > 0) {
        unsigned direction = data.rwy1;
        if (direction < 1 || direction > 36)
            direction = Runway::DIRECTION_UNDEFINED;
        else if (direction > 18)
            direction -= 18;

        tp->setRunway(Runway(Runway::TYPE_UNKNOWN, direction,
                             Runway::LENGTH_UNDEFINED));
    }

    return tp;
}

TurnPointReader *
CenfisDatabaseFormat::createReader(std::istream *stream) const {
    return new CenfisDatabaseReader(stream);
}
