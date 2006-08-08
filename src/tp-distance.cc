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

#include "tp.hh"
#include "tp-io.hh"

#include <stdlib.h>

class DistanceTurnPointReader : public TurnPointReader {
private:
    TurnPointReader *reader;
    Position center;
    Distance max_distance;
public:
    DistanceTurnPointReader(TurnPointReader *_reader,
                            const Position &_center,
                            const Distance &_max_distance)
        :reader(_reader), center(_center),
         max_distance(_max_distance) {}
    virtual ~DistanceTurnPointReader();
public:
    virtual const TurnPoint *read();
};

const Distance parseDistance(const char *p) {
    char *q;
    double value;
    Distance::unit_t unit;

    value = strtod(p, &q);
    if (q == p)
        throw TurnPointReaderException("Failed to parse distance value");

    if (*q == 0)
        throw TurnPointReaderException("No distance unit was provided");

    if (strcmp(q, "km") == 0) {
        value *= 1000.;
        unit = Distance::UNIT_METERS;
    } else if (strcmp(q, "m") == 0) {
        unit = Distance::UNIT_METERS;
    } else if (strcmp(q, "ft") == 0) {
        unit = Distance::UNIT_FEET;
    } else if (strcmp(q, "NM") == 0) {
        unit = Distance::UNIT_NAUTICAL_MILES;
    } else {
        throw TurnPointReaderException("Unknown distance unit");
    }

    return Distance(unit, value);
}

TurnPointReader *
DistanceTurnPointFilter::createFilter(TurnPointReader *reader,
                                      const char *args) const {
    if (args == NULL || *args == 0)
        throw TurnPointReaderException("No maximum distance provided");

    return new DistanceTurnPointReader(reader,
                                       Position(Latitude(1, 46, 28, 49),
                                                Longitude(1, 8, 15, 48),
                                                Altitude()),
                                       parseDistance(args));
}

DistanceTurnPointReader::~DistanceTurnPointReader() {
    if (reader != NULL)
        delete reader;
}

const TurnPoint *DistanceTurnPointReader::read() {
    const TurnPoint *tp;

    if (reader == NULL)
        return NULL;

    while ((tp = reader->read()) != NULL) {
        if (tp->getPosition() - center <= max_distance)
            return tp;
        else
            delete tp;
    }

    delete reader;
    reader = NULL;
    return NULL;
}
