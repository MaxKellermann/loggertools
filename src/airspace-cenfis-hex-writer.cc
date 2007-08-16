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

#include "airspace.hh"
#include "airspace-io.hh"
#include "hexfile-writer.hh"

#include <stdexcept>

class CenfisHexAirspaceWriter : public AirspaceWriter {
private:
    HexfileOutputFilter out;
    AirspaceWriter *asw;
public:
    CenfisHexAirspaceWriter(std::ostream *stream);
public:
    virtual void write(const Airspace &airspace);
    virtual void flush();
};

CenfisHexAirspaceWriter::CenfisHexAirspaceWriter(std::ostream *stream)
    :out(*stream, 0xc)
{
    const AirspaceFormat *format = getAirspaceFormat("cenfis");
    if (format == NULL)
        throw std::runtime_error("no such format: cenfis");

    asw = format->createWriter(&out);
    if (asw == NULL)
        throw std::runtime_error("failed to create cenfis writer");
}

void CenfisHexAirspaceWriter::write(const Airspace &airspace)
{
    asw->write(airspace);
}

void CenfisHexAirspaceWriter::flush()
{
    asw->flush();
    out.flush();
}

AirspaceReader *
CenfisHexAirspaceFormat::createReader(std::istream *stream) const
{
    (void)stream;
    return NULL;
}

AirspaceWriter *
CenfisHexAirspaceFormat::createWriter(std::ostream *stream) const
{
    return new CenfisHexAirspaceWriter(stream);
}
