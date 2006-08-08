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

#ifndef __LOGGERTOOLS_TP_HH
#define __LOGGERTOOLS_TP_HH

#include "earth.hh"

#include <string>

class Runway {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_GRASS,
        TYPE_ASPHALT
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

class Frequency {
private:
    unsigned hertz;
public:
    Frequency():hertz(0) {}
    Frequency(unsigned _hertz):hertz(_hertz) {}
    Frequency(unsigned mhz, unsigned khz):hertz((mhz * 1000 + khz) * 1000) {}
public:
    bool defined() const {
        return hertz > 0;
    }
    unsigned getHertz() const {
        return hertz;
    }
    unsigned getMegaHertz() const {
        return hertz / 1000000;
    }
    unsigned getKiloHertz() const {
        return hertz / 1000;
    }
    unsigned getKiloHertzPart() const {
        return getKiloHertz() % 1000;
    }
};

class TurnPoint {
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
    std::string title, code, country;
    Position position;
    type_t type;
    Runway runway;
    Frequency frequency;
    std::string description;
public:
    TurnPoint();
    TurnPoint(const std::string &_title,
              const std::string &_code,
              const std::string &_country,
              const Position &_position,
              type_t _type,
              const Runway &_runway,
              const Frequency &_frequency,
              const std::string &_description);
public:
    const std::string &getTitle() const {
        return title;
    }
    void setTitle(const std::string &_title);
    const std::string &getCode() const {
        return code;
    }
    void setCode(const std::string &_code);
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
