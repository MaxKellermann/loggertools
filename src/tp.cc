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

#include "tp.hh"

#include <assert.h>

Runway::Runway()
    :type(TYPE_UNKNOWN), direction(DIRECTION_UNDEFINED),
     length(LENGTH_UNDEFINED) {
}

Runway::Runway(type_t _type, unsigned _direction, unsigned _length)
    :type(_type), direction(_direction), length(_length) {
    assert(direction <= 18);
}

bool Runway::defined() const {
    return direction >= 1 && direction <= 18;
}

TurnPoint::TurnPoint(void)
    :type(TYPE_UNKNOWN) {
}

TurnPoint::TurnPoint(const std::string &_title,
                     const std::string &_code,
                     const std::string &_country,
                     const Position &_position,
                     type_t _type,
                     const Runway &_runway,
                     const Frequency &_frequency,
                     const std::string &_description)
    :title(_title), code(_code), country(_country),
     position(_position),
     type(_type),
     runway(_runway),
     frequency(_frequency),
     description(_description) {
}

void TurnPoint::setTitle(const std::string &_title) {
    title = _title;
}

void TurnPoint::setCode(const std::string &_code) {
    code = _code;
}

void TurnPoint::setCountry(const std::string &_country) {
    country = _country;
}

void TurnPoint::setPosition(const Position &_position) {
    position = _position;
}

void TurnPoint::setType(type_t _type) {
    type = _type;
}

void TurnPoint::setRunway(const Runway &_runway) {
    runway = _runway;
}

void TurnPoint::setFrequency(const Frequency &_frequency) {
    frequency = _frequency;
}

void TurnPoint::setDescription(const std::string &_description) {
    description = _description;
}
