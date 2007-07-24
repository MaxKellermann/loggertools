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
#include "earth-parser.hh"

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

TurnPointReader *
DistanceTurnPointFilter::createFilter(TurnPointReader *reader,
                                      const char *args) const {
    if (args == NULL || *args == 0)
        throw malformed_input("No maximum distance provided");

    Position center = parsePosition(args);
    Distance radius = parseDistance(args);

    /*
    if (*p != 0)
        throw malformed_input("malformed trailing input");
    */

    return new DistanceTurnPointReader(reader, center, radius);
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
