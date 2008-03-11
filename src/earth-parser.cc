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
#include "earth-parser.hh"

#include <stdlib.h>
#include <string.h>

static inline void
skipWhitespace(const char *&p)
{
    while (*p > 0 && *p <= 0x20)
        ++p;
}

const Distance
parseDistance(const char *p)
{
    char *q;
    double value;
    Distance::unit_t unit;

    value = strtod(p, &q);
    if (q == p)
        throw malformed_input("failed to parse distance value");

    if (*q == 0)
        throw malformed_input("no distance unit was provided");

    if (strcmp(q, "km") == 0) {
        value *= 1000.;
        unit = Distance::UNIT_METERS;
    } else if (strcmp(q, "m") == 0) {
        unit = Distance::UNIT_METERS;
    } else if (strcmp(q, "ft") == 0) {
        unit = Distance::UNIT_FEET;
    } else if (strcmp(q, "NM") == 0) {
        unit = Distance::UNIT_NAUTICAL_MILES;
    } else {
        throw malformed_input("unknown distance unit");
    }

    return Distance(unit, value);
}

template<char minusLetter, char plusLetter>
static inline void
parseSignLetter(const char *&p, int &sign)
{
    switch (*p) {
    case minusLetter:
        sign = -1;
        ++p;
        break;

    case plusLetter:
        sign = 1;
        ++p;
        break;
    }
}

template<class T, char minusLetter, char plusLetter, unsigned long maxValue>
static const T
parseAngle(const char **pp)
{
    const char *p = *pp;
    char *endptr;
    unsigned long n1, n2, n3;
    int sign = 0;

    /* sign letter */

    parseSignLetter<minusLetter, plusLetter>(p, sign);
    skipWhitespace(p);

    /* degrees */

    n1 = strtoul(p, &endptr, 10);
    if (n1 > maxValue)
        throw malformed_input("degrees out of range");

    if (endptr == p)
        throw malformed_input("numeric degree value expected");

    if (*endptr != ' ' && *endptr != '.')
        throw malformed_input("separator expected after degree value");
    p = endptr + 1;

    skipWhitespace(p);

    /* minutes */

    n2 = strtoul(p, &endptr, 10);
    if (n2 > 60)
        throw malformed_input("minutes out of range");

    if (endptr == p)
        throw malformed_input("numeric minute value expected");

    if (*endptr != ' ' && *endptr != '.' && *endptr != '\'')
        throw malformed_input("separator expected after minute value");
    p = endptr + 1;

    skipWhitespace(p);

    /* seconds */

    n3 = strtoul(p, &endptr, 10);
    if (n3 > 60)
        throw malformed_input("seconds out of range");

    if (endptr == p)
        throw malformed_input("numeric seconds value expected");

    if (*endptr == '"')
        ++endptr;
    p = endptr;

    skipWhitespace(p);

    /* trailing sign letter */

    if (sign == 0) {
        parseSignLetter<minusLetter, plusLetter>(p, sign);
        skipWhitespace(p);
    }

    /* calculate */

    if (sign == 0)
        throw malformed_input("sign letter is missing");

    *pp = p;

    return T(sign, (unsigned)n1, (unsigned)n2, (unsigned)n3);
}

const Position
parsePosition(const char *&p)
{
    Latitude latitude;
    Longitude longitude;

    latitude = parseAngle<Latitude,'S','N',90>(&p);
    longitude = parseAngle<Longitude,'W','E',180>(&p);

    return Position(latitude, longitude, Altitude());
}
