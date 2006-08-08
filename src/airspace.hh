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

#ifndef __LOGGERTOOLS_AIRSPACE_HH
#define __LOGGERTOOLS_AIRSPACE_HH

#include <string>
#include <vector>

#include "earth.hh"

class Vertex {
private:
    Latitude latitude;
    Longitude longitude;
public:
    Vertex() {}
    Vertex(const Latitude &_lat, const Longitude &_lon);
    Vertex(const Vertex &position);
    void operator =(const Vertex &pos);
public:
    bool defined() const {
        return latitude.defined() &&
            longitude.defined();
    }
    const Latitude &getLatitude() const {
        return latitude;
    }
    const Longitude &getLongitude() const {
        return longitude;
    }
};

class Airspace {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_ALPHA,
        TYPE_BRAVO,
        TYPE_CHARLY,
        TYPE_DELTA,
        TYPE_ECHO_LOW,
        TYPE_ECHO_HIGH,
        TYPE_FOX,
        TYPE_CTR,
        TYPE_TMZ,
        TYPE_RESTRICTED,
        TYPE_DANGER,
        TYPE_GLIDER
    };
private:
    std::string name;
    type_t type;
    Altitude bottom, top;
    std::vector<Vertex> vertices;
public:
    Airspace(const std::string &name, type_t type,
             const Altitude &bottom, const Altitude &top,
             const std::vector<Vertex> vertices);
public:
    const std::string &getName() const {
        return name;
    }
    type_t getType() const {
        return type;
    }
    const Altitude &getBottom() const {
        return bottom;
    }
    const Altitude &getTop() const {
        return top;
    }
    const std::vector<Vertex> &getVertices() const {
        return vertices;
    }
};

#endif
