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

#ifndef __LOGGERTOOLS_AIRSPACE_HH
#define __LOGGERTOOLS_AIRSPACE_HH

#include "earth.hh"
#include "aviation.hh"

#include <string>
#include <list>

class Edge {
public:
    enum type_t {
        TYPE_VERTEX,
        TYPE_CIRCLE,
        TYPE_ARC
    };

private:
    type_t type;
    int sign;
    SurfacePosition end, center;
    Distance radius;

public:
    Edge(const SurfacePosition &_end)
        :type(TYPE_VERTEX), end(_end),
         radius(Distance::UNIT_UNKNOWN, 0) {}
    Edge(const SurfacePosition &_center, const Distance &_radius)
        :type(TYPE_CIRCLE), center(_center), radius(_radius) {}
    Edge(int _sign, const SurfacePosition &_end, const SurfacePosition &_center)
        :type(TYPE_ARC), sign(_sign), end(_end), center(_center),
         radius(Distance::UNIT_UNKNOWN, 0) {}

public:
    type_t getType() const {
        return type;
    }
    int getSign() const {
        return sign;
    }
    const SurfacePosition &getEnd() const {
        return end;
    }
    const SurfacePosition &getCenter() const {
        return center;
    }
    const Distance &getRadius() const {
        return radius;
    }
};

static inline bool operator ==(const Edge &a, const Edge &b)
{
    if (a.getType() != b.getType())
        return false;

    switch (a.getType()) {
    case Edge::TYPE_VERTEX:
        return a.getEnd() == b.getEnd();

    case Edge::TYPE_CIRCLE:
        return a.getCenter() == b.getCenter() &&
            a.getRadius() == b.getRadius();

    case Edge::TYPE_ARC:
        return a.getEnd() == b.getEnd() &&
            a.getCenter() == b.getCenter() &&
            a.getSign() == b.getSign();
            a.getRadius() == b.getRadius();
    }

    return false;
}

static inline bool operator !=(const Edge &a, const Edge &b)
{
    return !(a == b);
}


/** an airspace: polygon with a lower and an upper bound */
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
    typedef std::list<Edge> EdgeList;
private:
    std::string name;
    type_t type;
    Altitude bottom, top;

    /** only used for the Cenfis test suite */
    Altitude top2;

    EdgeList edges;

    Frequency frequency;

    /** cenfis specific */
    unsigned voice;

public:
    Airspace(const std::string &name, type_t type,
             const Altitude &bottom, const Altitude &top,
             const EdgeList &edges);
    Airspace(const std::string &name, type_t type,
             const Altitude &bottom, const Altitude &top, const Altitude &top2,
             const EdgeList &edges,
             const Frequency &_frequency,
             unsigned voice);

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

    /** only used for the Cenfis test suite */
    const Altitude &getTop2() const {
        return top2;
    }

    const EdgeList &getEdges() const {
        return edges;
    }

    const Frequency &getFrequency() const {
        return frequency;
    }

    /** cenfis specific */
    unsigned getVoice() const {
        return voice;
    }
};

#endif
