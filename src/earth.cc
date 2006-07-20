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

Altitude::Altitude()
    :value(0), unit(UNIT_UNKNOWN), ref(REF_UNKNOWN) {
}

Altitude::Altitude(long _value, unit_t _unit, ref_t _ref)
    :value(_value), unit(_unit), ref(_ref) {
}

Position::Position(const Angle &_lat, const Angle &_lon,
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
