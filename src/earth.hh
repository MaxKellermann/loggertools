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

/** the great circle distance between two points on earth's surface */
class Distance {
public:
    enum unit_t {
        UNIT_UNKNOWN = 0,
        UNIT_METERS = 1,
        UNIT_FEET = 2,
        UNIT_NAUTICAL_MILES = 3
    };
private:
    unit_t unit;
    double value;
public:
    Distance(unit_t _unit, double _value)
        :unit(_unit), value(_value) {}
public:
    bool defined() const {
        return unit != UNIT_UNKNOWN;
    }
    double getValue() const {
        return value;
    }
    unit_t getUnit() const {
        return unit;
    }
    double getMeters() const {
        switch (unit) {
        case UNIT_UNKNOWN:
            return 0.0;
        case UNIT_METERS:
            return value;
        case UNIT_FEET:
            return value / 3.2808399;
        case UNIT_NAUTICAL_MILES:
            return value / 1.852;
        }

        return 0.0;
    }
    bool operator <(const Distance &b) const {
        return getMeters() < b.getMeters();
    }
    bool operator >(const Distance &b) const {
        return getMeters() > b.getMeters();
    }
    bool operator <=(const Distance &b) const {
        return !(*this > b);
    }
    bool operator >=(const Distance &b) const {
        return !(*this < b);
    }
};

/** vertical altitude of an object */
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

/** abstract class: angle which describes the position of an object on
    the earth */
class Angle {
private:
    static const int undef = 0x80000000;
    int value;
protected:
    Angle():value(undef) {}
    Angle(int _value):value(_value) {}
    Angle(int _value, int factor);
    Angle(int sign, unsigned degrees, unsigned minutes, unsigned seconds);
public:
    bool defined() const {
        return value != undef;
    }
    int getValue() const {
        return value;
    }
    int refactor(int factor) const;
    operator double() const {
        return ((double)value) * 3.14159265 / (180. * 60. * 1000.);
    }
};

/** the latitude value */
class Latitude : public Angle {
public:
    Latitude() {}
    Latitude(int value):Angle(value) {}
    Latitude(int value, int factor):Angle(value, factor) {}
    Latitude(int sign, unsigned degrees, unsigned minutes, unsigned seconds)
        :Angle(sign, degrees, minutes, seconds) {}
};

/** the longitude value */
class Longitude : public Angle {
public:
    Longitude() {}
    Longitude(int value):Angle(value) {}
    Longitude(int value, int factor):Angle(value, factor) {}
    Longitude(int sign, unsigned degrees, unsigned minutes, unsigned seconds)
        :Angle(sign, degrees, minutes, seconds) {}
};

/** the 3D position of an object on the earth */
class Position {
private:
    Latitude latitude;
    Longitude longitude;
    Altitude altitude;
public:
    Position() {}
    Position(const Latitude &_lat, const Longitude &_lon,
             const Altitude &_alt);
    Position(const Position &position);
    void operator =(const Position &pos);
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
    const Altitude &getAltitude() const {
        return altitude;
    }
};

/** calculate the great circle distance */
const Distance operator -(const Position& a, const Position &b);

#endif
