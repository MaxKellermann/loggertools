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

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"
#include "filser.h"

#include <ostream>

#include <netinet/in.h>
#include <string.h>

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

static uint32_t float_to_le32(float input) {
    union {
        float input;
        uint32_t output;
    } u;

    /* XXX what to do on big-endian machines? */

    u.input = input;
    return u.output;
}

static uint32_t convertAngle(const Angle &input) {
    return float_to_le32(input.getValue() / 60. / 1000.);
}

template<class T>
static uint32_t convertFrequency(const Frequency &input) {
    return float_to_le32(input.getHertz() / 1.e6);
}

static void copyString(char *dest, size_t dest_size,
                       const std::string &src) {
    size_t length = src.length();

    if (length > dest_size)
        length = dest_size;

    memcpy(dest, src.data(), length);
    memset(dest + length, ' ', dest_size - length);
}

void FilserTurnPointWriter::write(const TurnPoint &tp) {
    struct filser_turn_point data;
    std::string name;

    if (stream == NULL)
        throw already_flushed();

    if (count >= 600)
        throw container_full("Filser databases cannot hold more than 600 turn points");

    memset(&data, 0, sizeof(data));

    copyString(data.code, sizeof(data.code),
               tp.getAbbreviatedName(sizeof(data.code)));

    if (tp.getPosition().getLatitude().defined())
        data.latitude = convertAngle(tp.getPosition().getLatitude());

    if (tp.getPosition().getLongitude().defined())
        data.longitude = convertAngle(tp.getPosition().getLongitude());

    Altitude altitude = tp.getPosition().getAltitude().toUnit(Altitude::UNIT_FEET);
    if (altitude.defined() && altitude.getRef() == Altitude::REF_MSL)
        data.altitude_ft = htons(altitude.getValue());

    if (tp.getRunway().defined()) {
        data.runway_direction = tp.getRunway().getDirection();

        switch (tp.getRunway().getType()) {
        case Runway::TYPE_GRASS:
            data.runway_type = 'G';
            break;
        case Runway::TYPE_ASPHALT:
            data.runway_type = 'C';
            break;
        default:
            data.runway_type = ' ';
        }

        data.runway_length_ft = htons((int)(tp.getRunway().getLength() * 3.28));
    } else {
        data.runway_type = ' ';
    }

    /* write entry */
    stream->write((char*)&data, sizeof(data));

    count++;
}

void FilserTurnPointWriter::flush() {
    struct filser_turn_point data;
    char zero[6900];

    if (stream == NULL)
        throw already_flushed();

    memset(&data, 0, sizeof(data));
    for (; count < 600; count++)
        stream->write((char*)&data, sizeof(data));

    memset(zero, 0, sizeof(zero));
    stream->write(zero, sizeof(zero));

    stream = NULL;
}

TurnPointWriter *
FilserTurnPointFormat::createWriter(std::ostream *stream) const {
    return new FilserTurnPointWriter(stream);
}
