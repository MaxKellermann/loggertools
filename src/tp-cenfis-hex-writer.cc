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

#include "tp.hh"
#include "tp-io.hh"
#include "hexfile-writer.hh"

#include <stdexcept>

class CenfisHexWriter : public TurnPointWriter {
private:
    HexfileOutputFilter out;
    TurnPointWriter *tpw;
public:
    CenfisHexWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

CenfisHexWriter::CenfisHexWriter(std::ostream *stream)
    :out(*stream) {
    const TurnPointFormat *format = getTurnPointFormat("dab");
    if (format == NULL)
        throw std::runtime_error("no such format: dab");

    tpw = format->createWriter(&out);
    if (tpw == NULL)
        throw std::runtime_error("failed to create dab writer");
}

void CenfisHexWriter::write(const TurnPoint &tp) {
    tpw->write(tp);
}

void CenfisHexWriter::flush() {
    tpw->flush();
    out.flush();
}

TurnPointWriter *
CenfisHexTurnPointFormat::createWriter(std::ostream *stream) const {
    return new CenfisHexWriter(stream);
}
