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

#include "airspace.hh"

Vertex::Vertex(const Angle &_lat, const Angle &_lon)
    :latitude(_lat),
     longitude(_lon) {
}

Vertex::Vertex(const Vertex &position)
    :latitude(position.getLatitude()),
     longitude(position.getLongitude()) {
}

void Vertex::operator =(const Vertex &pos) {
    latitude = pos.getLatitude();
    longitude = pos.getLongitude();
}

Airspace::Airspace(const std::string &_name, type_t _type,
                   const Altitude &_bottom, const Altitude &_top,
                   const std::vector<Vertex> _vertices)
    :name(_name), type(_type),
     bottom(_bottom), top(_top),
     vertices(_vertices) {
}
