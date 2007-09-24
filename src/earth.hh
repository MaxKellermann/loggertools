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

public:
    const Distance toUnit(unit_t new_unit) const {
        if (new_unit == unit || unit == UNIT_UNKNOWN)
            return *this;
        if (unit == UNIT_METERS && new_unit == UNIT_NAUTICAL_MILES)
            return Distance(new_unit, value / 1852.);
        // XXX
        return Distance(UNIT_UNKNOWN, 0);
    }
};

/** vertical altitude of an object */
class Altitude {
public:
    typedef long value_t;
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
    value_t value;
    unit_t unit;
    ref_t ref;
public:
    Altitude();
    Altitude(value_t _value, unit_t _unit, ref_t _ref);
public:
    bool defined() const {
        return unit != UNIT_UNKNOWN && ref != REF_UNKNOWN;
    }
    value_t getValue() const {
        return value;
    }
    unit_t getUnit() const {
        return unit;
    }
    ref_t getRef() const {
        return ref;
    }
public:
    const Altitude toUnit(unit_t new_unit) const {
        if (new_unit == unit || unit == UNIT_UNKNOWN)
            return *this;
        if (unit == UNIT_METERS && new_unit == UNIT_FEET)
            return Altitude((long)(value * 3.28), new_unit, ref);
        if (unit == UNIT_FEET && new_unit == UNIT_METERS)
            return Altitude((long)(value / 3.28), new_unit, ref);
        return Altitude();
    }
};

/** abstract class: angle which describes the position of an object on
    the earth */
class Angle {
public:
    typedef int value_t;
private:
    static const int undef = 0x80000000;
    value_t value;
protected:
    Angle():value(undef) {}
    Angle(value_t _value):value(_value) {}
    Angle(value_t _value, int factor);
    Angle(int sign, unsigned degrees, unsigned minutes, unsigned seconds);
    Angle(double _value);
public:
    bool defined() const {
        return value != undef;
    }
    value_t getValue() const {
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
    Latitude(double value):Angle(value) {}
};

/** the longitude value */
class Longitude : public Angle {
public:
    Longitude() {}
    Longitude(int value):Angle(value) {}
    Longitude(int value, int factor):Angle(value, factor) {}
    Longitude(int sign, unsigned degrees, unsigned minutes, unsigned seconds)
        :Angle(sign, degrees, minutes, seconds) {}
    Longitude(double value):Angle(value) {}
};

/** the 2D position of an object on the earth */
class SurfacePosition {
private:
    Latitude latitude;
    Longitude longitude;
public:
    SurfacePosition() {}
    SurfacePosition(const Latitude &_lat, const Longitude &_lon)
        :latitude(_lat), longitude(_lon) {}
    SurfacePosition(const SurfacePosition &position)
        :latitude(position.getLatitude()),
         longitude(position.getLongitude()) {}
    void operator =(const SurfacePosition &position) {
        latitude = position.getLatitude();
        longitude = position.getLongitude();
    }
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

/** the 3D position of an object on the earth */
class Position : public SurfacePosition {
private:
    Altitude altitude;

public:
    Position() {}
    Position(const Latitude &_lat, const Longitude &_lon,
             const Altitude &_alt)
        :SurfacePosition(_lat, _lon),
         altitude(_alt) {}
    Position(const Position &position)
        :SurfacePosition(position),
         altitude(position.getAltitude()) {}
    void operator =(const SurfacePosition &pos) {
        SurfacePosition::operator =(pos);
        altitude = Altitude();
    }
    void operator =(const Position &pos) {
        SurfacePosition::operator =(pos);
        altitude = pos.getAltitude();
    }

public:
    /* no defined() implementation here since we regard this object as
       defined even if there is no altitude */
    const Altitude &getAltitude() const {
        return altitude;
    }
};

/** calculate the great circle distance */
const Distance operator -(const SurfacePosition& a, const SurfacePosition &b);

#endif
