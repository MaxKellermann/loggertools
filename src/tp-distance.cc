/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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
#include "io-compare.hh"
#include "io-match.hh"
#include "earth-parser.hh"

class TurnPointFindByName {
    std::string name;

public:
    TurnPointFindByName(const std::string &_name)
        :name(_name) {}

public:
    bool operator ()(const TurnPoint &tp) {
        return tp.getCode() == name || tp.getShortName() == name ||
            tp.getFullName() == name;
    }
};

class TurnPointCompareDistance {
    Distance distance;

public:
    TurnPointCompareDistance(const Distance &_distance)
        :distance(_distance) {}

public:
    bool operator ()(const TurnPoint &reference, const TurnPoint &tp) {
        return tp.getPosition() - reference.getPosition() <= distance;
    }
};

typedef FindCompareReader<TurnPoint, TurnPointFindByName,
                          TurnPointCompareDistance>
NameDistanceTurnPointReader;

class TurnPointMatchDistance {
    const SurfacePosition center;
    const Distance distance;

public:
    TurnPointMatchDistance(const SurfacePosition &_center,
                           const Distance &_distance)
        :center(_center), distance(_distance) {}

public:
    bool operator ()(const TurnPoint &tp) {
        return tp.getPosition() - center <= distance;
    }
};

typedef MatchReader<TurnPoint, TurnPointMatchDistance>
DistanceTurnPointReader;

TurnPointReader *
DistanceTurnPointFilter::createFilter(TurnPointReader *reader,
                                      const char *args) const {
    if (args == NULL || *args == 0)
        throw malformed_input("No maximum distance provided");

    Position center;
    try {
         center = parsePosition(args);
    } catch (const malformed_input &e) {
        const char *colon = strchr(args, ':');
        if (colon == NULL)
            throw malformed_input("Radius is missing");

        std::string name(args, colon - args);
        Distance radius = parseDistance(colon + 1);
        return new NameDistanceTurnPointReader(reader,
                                               TurnPointFindByName(name),
                                               TurnPointCompareDistance(radius));
    }

    Distance radius = parseDistance(args);

    /*
    if (*p != 0)
        throw malformed_input("malformed trailing input");
    */

    return new DistanceTurnPointReader(reader,
                                       TurnPointMatchDistance(center, radius));
}
