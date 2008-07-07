/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
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

#include <netinet/in.h>

#include "tp.hh"
#include "tp-io.hh"
#include "filser.h"

#include <istream>

class FilserTurnPointReader : public TurnPointReader {
private:
    std::istream *stream;
    unsigned count;
public:
    FilserTurnPointReader(std::istream *stream);
public:
    virtual const TurnPoint *read();
};

FilserTurnPointReader::FilserTurnPointReader(std::istream *_stream)
    :stream(_stream), count(0) {
}

static float le32_to_float(uint32_t input) {
    union {
        uint32_t input;
        float output;
    } u;

    /* XXX what to do on big-endian machines? */

    u.input = input;
    return u.output;
}

template<class T>
static const T convertAngle(uint32_t input) {
    return T((int)(le32_to_float(input) * 60 * 1000));
}

static const Frequency convertFrequency(uint32_t input) {
    return Frequency((unsigned)(le32_to_float(input) * 1000.) * 1000);
}

static Runway::type_t convertRunwayType(char ch) {
    if (ch == 'G')
        return Runway::TYPE_GRASS;
    else if (ch == 'C')
        return Runway::TYPE_ASPHALT;
    else
        return Runway::TYPE_UNKNOWN;
}

const TurnPoint *FilserTurnPointReader::read() {
    struct filser_turn_point data;
    TurnPoint *tp;
    size_t length;

    if (count >= 600 || stream->eof())
        return NULL;

    do {
        stream->read((char*)&data, sizeof(data));
        count++;
    } while (data.valid == 0);

    /* create object */
    tp = new TurnPoint();

    /* extract code */
    length = sizeof(data.code);
    while (length > 0 && data.code[length - 1] >= 0 &&
           data.code[length - 1] <= ' ')
        length--;

    if (length > 0)
        tp->setShortName(std::string(data.code, 0, length));

    tp->setPosition(Position(convertAngle<Latitude>(data.latitude),
                             convertAngle<Longitude>(data.longitude),
                             Altitude(ntohs(data.altitude_ft), Altitude::UNIT_FEET, Altitude::REF_MSL)));

    tp->setFrequency(convertFrequency(data.frequency));

    tp->setRunway(Runway(convertRunwayType(data.runway_type),
                         data.runway_direction >= 1 && data.runway_direction <= 36 ? data.runway_direction : (unsigned)Runway::DIRECTION_UNDEFINED,
                         (unsigned)(ntohs(data.runway_length_ft) / 3.28)));

    return tp;
}

TurnPointReader *
FilserTurnPointFormat::createReader(std::istream *stream) const {
    return new FilserTurnPointReader(stream);
}
