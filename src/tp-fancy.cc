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
 *
 * $Id$
 */

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"

#include <ostream>
#include <iomanip>

class FancyTurnPointWriter : public TurnPointWriter {
private:
    std::ostream *stream;
public:
    FancyTurnPointWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

FancyTurnPointWriter::FancyTurnPointWriter(std::ostream *_stream)
    :stream(_stream) {}

static char *formatAngle(char *buffer, size_t buffer_max_len,
                         const Angle &angle, const char *letters) {
    int value = angle.refactor(60);
    int a = abs(value);

    snprintf(buffer, buffer_max_len, "%02u.%02u.%02u%c",
             a / 60 / 60, (a / 60) % 60,
             a % 60,
             value < 0 ? letters[0] : letters[1]);

    return buffer;
}

static std::ostream &operator <<(std::ostream &os,
                                 Altitude::unit_t unit) {
    switch (unit) {
    case Altitude::UNIT_UNKNOWN:
        return os << "<unknown>";
    case Altitude::UNIT_METERS:
        return os << "m";
    case Altitude::UNIT_FEET:
        return os << "ft";
    }

    return os;
}

static std::ostream &operator <<(std::ostream &os,
                                 Altitude::ref_t ref) {
    switch (ref) {
    case Altitude::REF_UNKNOWN:
        return os << "<unknown>";
    case Altitude::REF_MSL:
        return os << "MSL";
    case Altitude::REF_GND:
        return os << "GND";
    case Altitude::REF_1013:
        return os << "Std";
    case Altitude::REF_AIRFIELD:
        return os << "above airfield";
    }

    return os;
}

static std::ostream &operator <<(std::ostream &os,
                                 const Altitude &altitude) {
    return os << altitude.getValue()
              << " " << altitude.getUnit()
              << " " << altitude.getRef();
}

static std::ostream &operator <<(std::ostream &os,
                                 const Position &position) {
    char buffer[16];

    formatAngle(buffer, sizeof(buffer),
                position.getLatitude(), "SN");
    os << buffer;

    formatAngle(buffer, sizeof(buffer),
                position.getLongitude(), "WE");
    os << " " << buffer;

    if (position.getAltitude().defined())
        os << " " << position.getAltitude();

    return os;
}

static std::ostream &operator <<(std::ostream &os,
                                 const Runway &runway) {
    os << std::setfill('0') << std::setw(2) << runway.getDirection();
    if (runway.getLength() > 0)
        os << " " << runway.getLength() << "m";
    switch (runway.getType()) {
    case Runway::TYPE_UNKNOWN:
        break;
    case Runway::TYPE_GRASS:
        os << " grass";
        break;
    case Runway::TYPE_ASPHALT:
        os << " asphalt";
        break;
    }
    return os;
}

static std::ostream &operator <<(std::ostream &os,
                                 const Frequency &frequency) {
    os << frequency.getMegaHertz()
       << "."
       << std::setfill('0') << std::setw(3)
       << frequency.getKiloHertzPart()
       << " MHz";
    return os;
}

void FancyTurnPointWriter::write(const TurnPoint &tp) {
    if (stream == NULL)
        throw already_flushed();

    if (tp.getFullName().length() > 0) {
        *stream << "\"" << tp.getFullName() << "\"";
        if (tp.getCode().length() > 0)
            *stream << " (" << tp.getCode() << ")";
        *stream << "\n";
    } else if (tp.getCode().length() > 0) {
        *stream << tp.getCode() << "\n";
    } else {
        *stream << "<no name>" << "\n";
    }

    if (tp.getCountry().length() > 0)
        *stream << "Country: " << tp.getCountry() << "\n";

    if (tp.getPosition().defined())
        *stream << "Position: " << tp.getPosition() << "\n";

    if (tp.getRunway().defined())
        *stream << "Runway: " << tp.getRunway() << "\n";

    if (tp.getFrequency().defined())
        *stream << "Frequency: " << tp.getFrequency() << "\n";

    *stream << std::endl;
}

void FancyTurnPointWriter::flush() {
    if (stream == NULL)
        throw already_flushed();
    stream = NULL;
}

TurnPointReader *
FancyTurnPointFormat::createReader(std::istream *) const {
    return NULL;
}

TurnPointWriter *
FancyTurnPointFormat::createWriter(std::ostream *stream) const {
    return new FancyTurnPointWriter(stream);
}
