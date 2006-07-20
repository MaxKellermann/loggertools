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
    Angle latitude, longitude;
public:
    Vertex() {}
    Vertex(const Angle &_lat, const Angle &_lon);
    Vertex(const Vertex &position);
    void operator =(const Vertex &pos);
public:
    bool defined() const {
        return latitude.defined() &&
            longitude.defined();
    }
    const Angle &getLatitude() const {
        return latitude;
    }
    const Angle &getLongitude() const {
        return longitude;
    }
};

class Airspace {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_AIRFIELD,
        TYPE_MILITARY_AIRFIELD,
        TYPE_GLIDER_SITE,
        TYPE_OUTLANDING,
        TYPE_MOUNTAIN_PASS,
        TYPE_MOUNTAIN_TOP,
        TYPE_SENDER,
        TYPE_VOR,
        TYPE_NDB,
        TYPE_COOL_TOWER,
        TYPE_DAM,
        TYPE_TUNNEL,
        TYPE_BRIDGE,
        TYPE_POWER_PLANT,
        TYPE_CASTLE,
        TYPE_CHURCH,
        TYPE_INTERSECTION,
        TYPE_THERMIK
    };
private:
    std::string name;
    type_t type;
    Altitude bottom, top;
    std::vector<Vertex> vertices;
public:
    Airspace(const char *name, type_t type,
             const Altitude &bottom, const Altitude &top,
             const std::vector<Vertex> vertices);
public:
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

class AirspaceReader {
public:
    virtual ~AirspaceReader();
public:
    virtual const Airspace *read() = 0;
};

class AirspaceWriter {
public:
    virtual ~AirspaceWriter();
public:
    virtual void write(const Airspace &tp) = 0;
    virtual void flush() = 0;
};

class AirspaceFormat {
public:
    virtual ~AirspaceFormat();
public:
    virtual AirspaceReader *createReader(FILE *file) = 0;
    virtual AirspaceWriter *createWriter(FILE *file) = 0;
};

#endif
