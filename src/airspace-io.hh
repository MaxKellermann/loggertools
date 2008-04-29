/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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

#ifndef __LOGGERTOOLS_AIRSPACE_IO_HH
#define __LOGGERTOOLS_AIRSPACE_IO_HH

#include "io.hh"
#include "airspace.hh"

typedef Reader<Airspace> AirspaceReader;
typedef Writer<Airspace> AirspaceWriter;
typedef Format<Airspace> AirspaceFormat;

class OpenAirAirspaceFormat : public AirspaceFormat {
public:
    virtual AirspaceReader *createReader(std::istream *stream) const;
    virtual AirspaceWriter *createWriter(std::ostream *stream) const;
};

class CenfisAirspaceFormat : public AirspaceFormat {
public:
    virtual AirspaceReader *createReader(std::istream *stream) const;
    virtual AirspaceWriter *createWriter(std::ostream *stream) const;
};

class CenfisHexAirspaceFormat : public AirspaceFormat {
public:
    virtual AirspaceReader *createReader(std::istream *stream) const;
    virtual AirspaceWriter *createWriter(std::ostream *stream) const;
};

class CenfisTextAirspaceFormat : public AirspaceFormat {
public:
    virtual AirspaceReader *createReader(std::istream *stream) const;
    virtual AirspaceWriter *createWriter(std::ostream *stream) const;
};

const AirspaceFormat *getAirspaceFormat(const char *ext);

#endif
