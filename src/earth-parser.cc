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

const Distance parseDistance(const char *p) {
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
