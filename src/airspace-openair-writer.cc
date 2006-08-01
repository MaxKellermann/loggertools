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

#include <sstream>

class OpenAirAirspaceWriter : public AirspaceWriter {
public:
    FILE *file;
public:
    OpenAirAirspaceWriter(FILE *file);
public:
    virtual void write(const Airspace &as);
    virtual void flush();
};

OpenAirAirspaceWriter::OpenAirAirspaceWriter(FILE *_file)
    :file(_file) {
    fprintf(file, "* Written by loggertools\n\n");
}

static const char *type_to_string(Airspace::type_t type) {
    switch (type) {
    case Airspace::TYPE_UNKNOWN:
        return "UNKNOWN";
    case Airspace::TYPE_ALPHA:
        return "A";
    case Airspace::TYPE_BRAVO:
        return "B";
    case Airspace::TYPE_CHARLY:
        return "C";
    case Airspace::TYPE_DELTA:
        return "D";
    case Airspace::TYPE_ECHO_LOW:
        return "E";
    case Airspace::TYPE_ECHO_HIGH:
        return "W";
    case Airspace::TYPE_FOX:
        return "F";
    case Airspace::TYPE_CTR:
        return "CTR";
    case Airspace::TYPE_TMZ:
        return "TMZ";
    case Airspace::TYPE_RESTRICTED:
        return "R";
    case Airspace::TYPE_DANGER:
        return "Q";
    case Airspace::TYPE_GLIDER:
        return "GSEC";
    }

    return "INVALID";
}

static const char *altitude_ref_to_string(Altitude::ref_t ref) {
    switch (ref) {
    case Altitude::REF_UNKNOWN:
        return "UNKNOWN";
    case Altitude::REF_MSL:
        return "MSL";
    case Altitude::REF_GND:
    case Altitude::REF_AIRFIELD:
        return "GND";
    case Altitude::REF_1013:
        return "FL";
    }

    return "INVALID";
}

static std::string altitude_to_string(const Altitude &alt) {
    long value;
    Altitude::ref_t ref;
    const char *ref_s;
    std::ostringstream buf;

    if (!alt.defined())
        return "UNKNOWN";

    value = alt.getValue();
    ref = alt.getRef();
    ref_s = altitude_ref_to_string(ref);

    if (value == 0 && ref == Altitude::REF_GND)
        return ref_s;

    switch (alt.getUnit()) {
    case Altitude::UNIT_METERS:
        value = (value * 10) / 3;
        break;

    case Altitude::UNIT_FEET:
        break;

    default:
        return "UNKNOWN";
    }

    if (ref == Altitude::REF_1013) {
        buf << ref_s << ((value + 499) / 1000);
    } else {
        char value_s[16];
        snprintf(value_s, sizeof(value_s), "%04ld", value);
        buf << value_s << ref_s;
    }

    return buf.str();
}

void OpenAirAirspaceWriter::write(const Airspace &as) {
    fprintf(file, "AC %s\n", type_to_string(as.getType()));
    fprintf(file, "AN %s\n", as.getName().c_str());
    fprintf(file, "AL %s\n", altitude_to_string(as.getBottom()).c_str());
    fprintf(file, "AH %s\n", altitude_to_string(as.getTop()).c_str());

    const std::vector<Vertex> &vertices = as.getVertices();
    for (std::vector<Vertex>::const_iterator it = vertices.begin();
         it != vertices.end(); ++it) {
        int latitude = (*it).getLatitude().refactor(60);
        int absLatitude = abs(latitude);
        int longitude = (*it).getLongitude().refactor(60);
        int absLongitude = abs(longitude);

        fprintf(file, "DP %02d:%02d:%02d %c %03d:%02d:%02d %c\n",
                absLatitude / 3600, (absLatitude / 60) % 60,
                absLatitude % 60,
                latitude < 0 ? 'S' : 'N',
                absLongitude / 3600, (absLongitude / 60) % 60,
                absLongitude % 60,
                longitude < 0 ? 'W' : 'E');
    }

    fprintf(file, "\n");
}

void OpenAirAirspaceWriter::flush() {
    if (file == NULL)
        throw new AirspaceWriterException("already flushed");

    fclose(file);
    file = NULL;
}

AirspaceWriter *OpenAirAirspaceFormat::createWriter(FILE *file) {
    return new OpenAirAirspaceWriter(file);
}
