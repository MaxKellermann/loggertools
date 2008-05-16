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

#include "exception.hh"
#include "airspace.hh"
#include "airspace-io.hh"

#include <istream>

#include <stdlib.h>
#include <string.h>

class CenfisTextAirspaceReader : public AirspaceReader {
public:
    std::istream *stream;
public:
    CenfisTextAirspaceReader(std::istream *stream);
public:
    virtual const Airspace *read();
};

CenfisTextAirspaceReader::CenfisTextAirspaceReader(std::istream *_stream)
    :stream(_stream) {}

static void chomp(char *p) {
    size_t length = strlen(p);

    while (length > 0 && p[length - 1] > 0 && p[length - 1] <= 0x20)
        --length;

    p[length] = 0;
}

/** only used for AC and AN*, don't strip spaces, hack for the test suite */
static void chomp2(char *p) {
    size_t length = strlen(p);

    while (length > 0 && (p[length - 1] == '\n' || p[length - 1] == '\r'))
        --length;

    p[length] = 0;
}

static Airspace::type_t parse_type(const char *p) {
    if (strcmp(p, "A") == 0)
        return Airspace::TYPE_ALPHA;
    if (strcmp(p, "B") == 0)
        return Airspace::TYPE_BRAVO;
    if (strcmp(p, "C") == 0)
        return Airspace::TYPE_CHARLY;
    if (strcmp(p, "D") == 0)
        return Airspace::TYPE_DELTA;
    if (strcmp(p, "E") == 0)
        return Airspace::TYPE_ECHO_LOW;
    if (strcmp(p, "W") == 0)
        return Airspace::TYPE_ECHO_HIGH;
    if (strcmp(p, "F") == 0)
        return Airspace::TYPE_FOX;
    if (strcmp(p, "CTR") == 0)
        return Airspace::TYPE_CTR;
    if (strcmp(p, "TMZ") == 0)
        return Airspace::TYPE_TMZ;
    if (strcmp(p, "R") == 0 ||
        strcmp(p, "TRA") == 0 ||
        strcmp(p, "GP") == 0)
        return Airspace::TYPE_RESTRICTED;
    if (strcmp(p, "Q") == 0)
        return Airspace::TYPE_DANGER;
    if (strcmp(p, "GSEC") == 0)
        return Airspace::TYPE_GLIDER;
    return Airspace::TYPE_UNKNOWN;
}

static const Altitude
parse_altitude(const char *p)
{
    long value;
    Altitude::ref_t ref;
    char *endptr;

    if (p[0] == 'F' && p[1] == 'L') {
        ref = Altitude::REF_1013;
        value = strtol(p + 2, &endptr, 10) * 100;
        if (value == 0 && *endptr != 0)
            ref = Altitude::REF_UNKNOWN;
    } else {
        value = strtol(p, &endptr, 10);
        while (*endptr == ' ')
            ++endptr;
        if (strcmp(endptr, "GND") == 0)
            ref = Altitude::REF_GND;
        else if (strcmp(endptr, "MSL") == 0)
            ref = Altitude::REF_MSL;
        else if (strcmp(endptr, "STD") == 0)
            ref = Altitude::REF_1013;
        else
            ref = Altitude::REF_UNKNOWN;
    }

    return Altitude(value, Altitude::UNIT_FEET, ref);
}

static int
char_is_delimiter(char ch)
{
    return ch == ' ' || ch == ',' || ch == ':';
}

static const char *
next_word(char *&p)
{
    char *ret = p;

    while (!char_is_delimiter(*p)) {
        if (*p == 0)
            return ret;
        ++p;
    }

    *p++ = 0;
    return ret;
}

template<class angle_class>
static const angle_class
parse_angle(char *&src)
{
    int degrees, minutes, seconds;
    int sign = 1;

    degrees = atoi(next_word(src));
    if (degrees < 0) {
        sign = -1;
        degrees = -degrees;
    }

    minutes = atoi(next_word(src));
    seconds = atoi(next_word(src));

    return angle_class(1, degrees, minutes, seconds);
}

static const SurfacePosition
parse_surface_position(char *&src)
{
    Latitude latitude = parse_angle<Latitude>(src);
    Longitude longitude = parse_angle<Longitude>(src);
    return SurfacePosition(latitude, longitude);
}

static const Edge
parse_circle(char *&src)
{
    SurfacePosition center = parse_surface_position(src);
    int miles, deci_miles;

    miles = atoi(next_word(src));
    deci_miles = atoi(next_word(src));
    Distance radius = Distance(Distance::UNIT_NAUTICAL_MILES,
                               (double)miles + (deci_miles / 10.));

    return Edge(center, radius);
}

static const Edge
parse_arc(char *&src)
{
    int sign = next_word(src)[0] == '+' ? 1 : -1;
    SurfacePosition end = parse_surface_position(src);
    SurfacePosition center = parse_surface_position(src);

    return Edge(sign, end, center);
}

const Airspace *CenfisTextAirspaceReader::read() {
    char line[512], *p;
    Airspace::type_t type = Airspace::TYPE_UNKNOWN;
    std::string cmd, name, name2, name3, name4, type_string;
    Altitude bottom(0, Altitude::UNIT_METERS, Altitude::REF_GND), top, top2;
    Airspace::EdgeList edges;
    Frequency frequency;
    unsigned voice = 0;
    bool has_start = false;

    while (!stream->eof()) {
        try {
            stream->getline(line, sizeof(line));
        } catch (const std::ios_base::failure &e) {
            if (stream->eof())
                break;
            else
                throw;
        }

        if (line[0] == '*') /* comment */
            continue;

        if (line[0] == 'A' && (line[1] == 'C' || line[1] == 'N'))
            chomp2(line);
        else
            chomp(line);
        if (line[0] == 0) {
            if (edges.empty())
                continue;
            else
                break;
        }

        p = line;
        cmd = next_word(p);
        if (cmd.length() == 0)
            throw malformed_input("command expected");

        if (cmd == "AC") {
            type_string = p;
            type = parse_type(p);
        } else if (cmd == "AN") {
            name = p;
        } else if (cmd == "AN2") {
            name2 = p;
            if (name4.length() > 0)
                /* reproduce bug: AN4 before AN2, memorize that with
                   the "dash" marker */
                name2.insert(name2.begin(), '-');
        } else if (cmd == "AN3") {
            name3 = p;
        } else if (cmd == "AN4") {
            name4 = p;
        } else if (cmd == "AL") {
            bottom = parse_altitude(p);
        } else if (cmd == "AH") {
            top = parse_altitude(p);
        } else if (cmd == "AH2") {
            top2 = top;
            top = parse_altitude(p);
        } else if (cmd == "S") {
            edges.push_back(parse_surface_position(p));
            has_start = true;
        } else if (cmd == "L") {
            edges.push_back(parse_surface_position(p));
            if (!has_start && (type_string.length() == 0 ||
                               type_string[0] != '_'))
                /* reproduce bug: there is no "S" line, memorize that
                   with the "underscore" marker */
                type_string.insert(type_string.begin(), '_');
        } else if (cmd == "C") {
            edges.push_back(parse_circle(p));
        } else if (cmd == "A") {
            edges.push_back(parse_arc(p));
        } else if (cmd == "V") {
            voice = (unsigned)atoi(next_word(p));
            const char *type = next_word(p);
            if (*type == 'R')
                voice |= 0x8000;
        } else if (cmd == "FIS") {
            unsigned mhz = (unsigned)atoi(next_word(p));
            unsigned khz = (unsigned)atoi(next_word(p));

            frequency = Frequency(mhz, khz);
        } else if (cmd == "UPD") {
            // ignore UPD lines
        } else {
            throw malformed_input("invalid command: " + cmd);
        }
    }

    if (edges.size() == 0)
        return NULL;

    if (name2.length() > 0 || name3.length() > 0 || name4.length() > 0 || type_string.length() > 0) {
        name += '|';
        name += name2;
    }

    if (name3.length() > 0 || name4.length() > 0 || type_string.length() > 0) {
        name += '|';
        name += name3;
    }

    if (name4.length() > 0 || type_string.length() > 0) {
        name += '|';
        name += name4;
    }

    if (type_string.length() > 0) {
        name += '|';
        name += type_string;
    }

    return new Airspace(name, type,
                        bottom, top, top2,
                        edges,
                        frequency,
                        voice);
}

AirspaceReader *
CenfisTextAirspaceFormat::createReader(std::istream *stream) const {
    return new CenfisTextAirspaceReader(stream);
}

AirspaceWriter *
CenfisTextAirspaceFormat::createWriter(std::ostream *stream) const {
    (void)stream;
    return NULL;
}
