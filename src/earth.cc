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

#include "earth.hh"

#include <assert.h>
#include <math.h>

Altitude::Altitude()
    :value(0), unit(UNIT_UNKNOWN), ref(REF_UNKNOWN) {
}

Altitude::Altitude(long _value, unit_t _unit, ref_t _ref)
    :value(_value), unit(_unit), ref(_ref) {
}

static int refactor(int v, int old_factor, int new_factor) {
    int sign = v < 0 ? -1 : 1;
    int a = v * sign;

    return sign * ((a * new_factor + (old_factor + 1) / 2 - 1) / old_factor);
}

Angle::Angle(int _value, int factor)
    :value(::refactor(_value, factor, 1000)) {}

Angle::Angle(int sign, unsigned degrees, unsigned minutes, unsigned seconds)
    :value(::refactor(sign * ((degrees * 60) + minutes) * 60 +
                      seconds, 60, 1000)) {
    assert(sign == -1 || sign == 1);
    assert(degrees <= 180);
    assert(minutes < 60);
    assert(seconds < 60);
}

int Angle::refactor(int factor) const {
    if (!defined())
        return 0;

    return ::refactor(value, 1000, factor);
}

Position::Position(const Latitude &_lat, const Longitude &_lon,
                   const Altitude &_alt)
    :latitude(_lat),
     longitude(_lon),
     altitude(_alt) {
}

Position::Position(const Position &position)
    :latitude(position.getLatitude()),
     longitude(position.getLongitude()),
     altitude(position.getAltitude()) {
}

void Position::operator =(const Position &pos) {
    latitude = pos.getLatitude();
    longitude = pos.getLongitude();
    altitude = pos.getAltitude();
}

const Distance operator -(const Position& a, const Position &b) {
    double lat1 = a.getLatitude();
    double lon1 = a.getLongitude();
    double lat2 = b.getLatitude();
    double lon2 = b.getLongitude();

    // formula from http://en.wikipedia.org/wiki/Great-circle_distance
    return Distance(Distance::UNIT_METERS,
                    atan2(hypot(cos(lat2) * sin(lon2 - lon1),
                                cos(lat1) * sin(lat2) -
                                sin(lat1) * cos(lat2) * cos(lon2 - lon1)),
                          (sin(lat1) * sin(lat2) +
                           cos(lat1) * cos(lat2) * cos(lon2 - lon1))) *
                    6372795.);
}
