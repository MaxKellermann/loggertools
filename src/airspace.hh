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

#ifndef __LOGGERTOOLS_AIRSPACE_HH
#define __LOGGERTOOLS_AIRSPACE_HH

#include <string>
#include <vector>

#include "earth.hh"

class Vertex {
private:
    Angle latitude, longitude;
public:
    Vertex() {}
    Vertex(const Angle &_lat, const Angle &_lon);
    Vertex(const Vertex &position);
    void operator =(const Vertex &pos);
public:
    bool defined() const {
        return latitude.defined() &&
            longitude.defined();
    }
    const Angle &getLatitude() const {
        return latitude;
    }
    const Angle &getLongitude() const {
        return longitude;
    }
};

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
        TYPE_DANGER
    };
private:
    std::string name;
    type_t type;
    Altitude bottom, top;
    std::vector<Vertex> vertices;
public:
    Airspace(const char *name, type_t type,
             const Altitude &bottom, const Altitude &top,
             const std::vector<Vertex> vertices);
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
    const std::vector<Vertex> &getVertices() const {
        return vertices;
    }
};

class AirspaceReaderException {
private:
    char *msg;
public:
    AirspaceReaderException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    AirspaceReaderException(const AirspaceReaderException &ex);
    virtual ~AirspaceReaderException();
public:
    const char *getMessage() const {
        return msg;
    }
};

class AirspaceWriterException {
private:
    char *msg;
public:
    AirspaceWriterException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    AirspaceWriterException(const AirspaceWriterException &ex);
    virtual ~AirspaceWriterException();
public:
    const char *getMessage() const {
        return msg;
    }
};

class AirspaceReader {
public:
    virtual ~AirspaceReader();
public:
    virtual const Airspace *read() = 0;
};

class AirspaceWriter {
public:
    virtual ~AirspaceWriter();
public:
    virtual void write(const Airspace &as) = 0;
    virtual void flush() = 0;
};

class AirspaceFormat {
public:
    virtual ~AirspaceFormat();
public:
    virtual AirspaceReader *createReader(FILE *file) = 0;
    virtual AirspaceWriter *createWriter(FILE *file) = 0;
};

class OpenAirAirspaceFormat : public AirspaceFormat {
public:
    virtual AirspaceReader *createReader(FILE *file);
    virtual AirspaceWriter *createWriter(FILE *file);
};

AirspaceFormat *getAirspaceFormat(const char *ext);

#endif
