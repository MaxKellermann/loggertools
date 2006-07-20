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

#include "airspace.hh"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

class OpenAirAirspaceReader : public AirspaceReader {
public:
    FILE *file;
public:
    OpenAirAirspaceReader(FILE *file);
public:
    virtual const Airspace *read();
};

OpenAirAirspaceReader::OpenAirAirspaceReader(FILE *_file)
    :file(_file) {}

static void chomp(char *p) {
    size_t length = strlen(p);

    while (length > 0 && p[length - 1] > 0 && p[length - 1] <= 0x20)
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

static Altitude parse_altitude(const char *p) {
    long value;
    Altitude::ref_t ref;
    char *endptr;

    if (p[0] == 'F' && p[1] == 'L') {
        ref = Altitude::REF_1013;
        value = strtol(p + 2, &endptr, 10) * 1000;
        if (value == 0 && *endptr != 0)
            ref = Altitude::REF_UNKNOWN;
    } else {
        value = strtol(p, &endptr, 10);
        if (strcmp(endptr, "GND") == 0)
            ref = Altitude::REF_GND;
        else if (strcmp(endptr, "MSL") == 0)
            ref = Altitude::REF_MSL;
        else
            ref = Altitude::REF_UNKNOWN;
    }

    return Altitude(value, Altitude::UNIT_FEET, ref);
}

const Airspace *OpenAirAirspaceReader::read() {
    char line[512];
    Airspace::type_t type = Airspace::TYPE_UNKNOWN;
    std::string name;
    Altitude bottom, top;
    std::vector<Vertex> vertices;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] == '*') /* comment */
            continue;

        chomp(line);
        if (line[0] == 0) {
            if (vertices.size() == 0)
                continue;
            else
                break;
        }

        if (line[0] == 'A') {
            if (line[2] != ' ')
                throw AirspaceReaderException("malformed line");

            switch (line[1]) {
            case 'C':
                type = parse_type(line + 3);
                break;

            case 'N':
                name = line + 3;
                break;

            case 'L':
                bottom = parse_altitude(line + 3);
                break;

            case 'H':
                top = parse_altitude(line + 3);
                break;

            default:
                throw AirspaceReaderException("malformed line");
            }
        } else if (line[0] == 'D' && line[1] == 'P' && line[2] == ' ') {
            int lat1, lat2, lat3, lon1, lon2, lon3;
            char latSN[4], lonWE[4]; /* on AMD64, this must be 32 bits wide? wtf? */
            int ret;

            ret = sscanf(line + 3, "%2d:%2d:%2d %[SN] %3d:%2d:%2d %[WE]",
                         &lat1, &lat2, &lat3, latSN,
                         &lon1, &lon2, &lon3, lonWE);
            if (ret != 8)
                throw AirspaceReaderException("malformed line");

            int latitude = ((lat1 * 60) + lat2) * 1000 + (lat3 * 1000 + 499) / 60;
            int longitude = ((lon1 * 60) + lon2) * 1000 + (lon3 * 1000 + 499) / 60;

            if (*latSN == 'S')
                latitude = -latitude;
            else if (*latSN != 'N')
                throw AirspaceReaderException("malformed S/N");

            if (*lonWE == 'W')
                longitude = -longitude;
            else if (*lonWE != 'E')
                throw AirspaceReaderException("malformed W/E");

            vertices.push_back(Vertex(Angle(latitude), Angle(longitude)));
        } else {
            throw AirspaceReaderException("malformed line");
        }
    }

    if (vertices.size() > 0)
        return new Airspace(name.c_str(), type,
                            bottom, top,
                            vertices);

    return NULL;
}

AirspaceReader *OpenAirAirspaceFormat::createReader(FILE *file) {
    return new OpenAirAirspaceReader(file);
}