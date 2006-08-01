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

#ifndef __LOGGERTOOLS_EARTH_HH
#define __LOGGERTOOLS_EARTH_HH

class Altitude {
public:
    enum unit_t {
        UNIT_UNKNOWN = 0,
        UNIT_METERS = 1,
        UNIT_FEET = 2
    };
    enum ref_t {
        REF_UNKNOWN = 0,
        REF_MSL = 1,
        REF_GND = 2,
        REF_1013 = 3,
        REF_AIRFIELD = 4
    };
private:
    long value;
    unit_t unit;
    ref_t ref;
public:
    Altitude();
    Altitude(long _value, unit_t _unit, ref_t _ref);
public:
    bool defined() const {
        return unit != UNIT_UNKNOWN && ref != REF_UNKNOWN;
    }
    long getValue() const {
        return value;
    }
    unit_t getUnit() const {
        return unit;
    }
    ref_t getRef() const {
        return ref;
    }
};

class Angle {
private:
    int value;
public:
    Angle():value(0) {}
    Angle(int _value):value(_value) {}
    Angle(int _value, int factor);
public:
    bool defined() const {
        /* XXX */
        return value != 0;
    }
    int getValue() const {
        return value;
    }
    int refactor(int factor) const;
};

class Position {
private:
    Angle latitude, longitude;
    Altitude altitude;
public:
    Position() {}
    Position(const Angle &_lat, const Angle &_lon,
             const Altitude &_alt);
    Position(const Position &position);
    void operator =(const Position &pos);
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
    const Altitude &getAltitude() const {
        return altitude;
    }
};

#endif
