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

class DistanceTurnPointReader : public TurnPointReader {
private:
    TurnPointReader *reader;
    Position center;
    unsigned max_distance;
public:
    DistanceTurnPointReader(TurnPointReader *_reader,
                            const Position &_center,
                            unsigned _max_distance)
        :reader(_reader), center(_center),
         max_distance(_max_distance) {}
    virtual ~DistanceTurnPointReader();
public:
    virtual const TurnPoint *read();
};

TurnPointReader *
DistanceTurnPointFilter::createFilter(TurnPointReader *reader,
                                      const char *args) const {
    (void)args; // XXX

    return new DistanceTurnPointReader(reader,
                                       Position(Angle(1, 46, 28, 49),
                                                Angle(1, 8, 15, 48),
                                                Altitude()),
                                       300 * 1000);
}

DistanceTurnPointReader::~DistanceTurnPointReader() {
    if (reader != NULL)
        delete reader;
}

static unsigned distance(const Position &a,
                         const Position &b) {
    // XXX correct implementation
    return abs(a.getLatitude().getValue() - b.getLatitude().getValue())
        + abs(a.getLongitude().getValue() - b.getLongitude().getValue());
}

const TurnPoint *DistanceTurnPointReader::read() {
    const TurnPoint *tp;

    if (reader == NULL)
        return NULL;

    while ((tp = reader->read()) != NULL) {
        if (distance(tp->getPosition(), center) <= max_distance)
            return tp;
        else
            delete tp;
    }

    delete reader;
    reader = NULL;
    return NULL;
}
