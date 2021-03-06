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

#ifndef __LOGGERTOOLS_TP_HH
#define __LOGGERTOOLS_TP_HH

#include "earth.hh"
#include "aviation.hh"

#include <string>

/** description of an airfield's runway */
class Runway {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_GRASS,
        TYPE_ASPHALT
    };
    enum {
        DIRECTION_UNDEFINED = 0,
        LENGTH_UNDEFINED = 0
    };
private:
    type_t type;
    unsigned direction, length;
public:
    Runway();
    Runway(type_t _type, unsigned _direction, unsigned _length);
public:
    bool defined() const;
    type_t getType() const {
        return type;
    }
    unsigned getDirection() const {
        return direction;
    }
    unsigned getLength() const {
        return length;
    }
};

/** a turn point used for navigation */
class TurnPoint {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_AIRFIELD,
        TYPE_MILITARY_AIRFIELD,
        TYPE_GLIDER_SITE,
        TYPE_ULTRALIGHT_FIELD,
        TYPE_OUTLANDING,
        TYPE_MOUNTAIN_PASS,
        TYPE_MOUNTAIN_TOP,
        TYPE_ROPEWAY,
        TYPE_SENDER,
        TYPE_VOR,
        TYPE_NDB,
        TYPE_COOL_TOWER,
        TYPE_CHIMNEY,
        TYPE_LAKE,
        TYPE_DAM,
        TYPE_TUNNEL,
        TYPE_BRIDGE,
        TYPE_POWER_PLANT,
        TYPE_CASTLE,
        TYPE_CHURCH,
        TYPE_RUIN,
        TYPE_BUILDING,
        TYPE_HIGHWAY_INTERSECTION,
        TYPE_HIGHWAY_EXIT,
        TYPE_GARAGE,
        TYPE_RAILWAY_INTERSECTION,
        TYPE_RAILWAY_STATION,
        TYPE_MOUNTAIN_WAVE,
        TYPE_THERMALS
    };
private:
    std::string fullName, shortName, code, country;
    Position position;
    type_t type;
    Runway runway;
    Frequency frequency;
    std::string description;
public:
    TurnPoint();
    TurnPoint(const std::string &_fullName,
              const std::string &_code,
              const std::string &_country,
              const Position &_position,
              type_t _type,
              const Runway &_runway,
              const Frequency &_frequency,
              const std::string &_description);
public:
    const std::string &getFullName() const {
        return fullName;
    }
    void setFullName(const std::string &_fullName);
    const std::string &getShortName() const {
        return shortName;
    }
    void setShortName(const std::string &_shortName);
    const std::string &getCode() const {
        return code;
    }
    void setCode(const std::string &_code);
    const std::string &getAnyName() const;
    const std::string getAbbreviatedName(std::string::size_type max_length) const;
    const std::string &getCountry() const {
        return country;
    }
    void setCountry(const std::string &_country);
    const Position &getPosition() const {
        return position;
    }
    void setPosition(const Position &_position);
    type_t getType() const {
        return type;
    }
    void setType(type_t _type);
    const Runway &getRunway() const {
        return runway;
    }
    void setRunway(const Runway &runway);
    const Frequency &getFrequency() const {
        return frequency;
    }
    void setFrequency(const Frequency &_freq);
    const std::string &getDescription() const {
        return description;
    }
    void setDescription(const std::string &_description);
};

#endif
