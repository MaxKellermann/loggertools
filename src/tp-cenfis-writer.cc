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

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "tp.hh"
#include "tp-io.hh"

#include <ostream>
#include <iomanip>

class CenfisTurnPointWriter : public TurnPointWriter {
private:
    std::ostream *stream;
public:
    CenfisTurnPointWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

CenfisTurnPointWriter::CenfisTurnPointWriter(std::ostream *_stream)
    :stream(_stream) {
    *stream << "0 created by loggertools\n";
}

static const char *formatType(TurnPoint::type_t type) {
    switch (type) {
    case TurnPoint::TYPE_AIRFIELD:
        return " # ";
    case TurnPoint::TYPE_MILITARY_AIRFIELD:
        return " #M";
    case TurnPoint::TYPE_GLIDER_SITE:
        return " #S";
    case TurnPoint::TYPE_OUTLANDING:
        return "LW ";
    default:
        return "   ";
    }
}

static char *formatAngle(char *buffer, size_t buffer_max_len,
                         const Angle &angle, const char *letters) {
    int value = angle.getValue();
    int a = abs(value);

    snprintf(buffer, buffer_max_len, "%c %02u %02u %03u",
             value < 0 ? letters[0] : letters[1],
             a / 60 / 1000, (a / 1000) % 60,
             a % 1000);

    return buffer;
}

void CenfisTurnPointWriter::write(const TurnPoint &tp) {
    std::string p;

    if (stream == NULL)
        throw already_flushed();

    p = tp.getCode();
    if (p.length() == 0)
        p = tp.getTitle();
    if (p.length() == 0)
        p = "unknown";
    *stream << "11 N " << p << "\n";

    *stream << "   T "
            << std::setw(3) << std::setfill(' ')
            << formatType(tp.getType());

    if (tp.getTitle().length() > 0)
        *stream << " " << tp.getTitle();

    *stream << "\n";

    if (tp.getPosition().defined()) {
        char latitude[16], longitude[16];

        formatAngle(latitude, sizeof(latitude),
                    tp.getPosition().getLatitude(), "SN");
        formatAngle(longitude, sizeof(longitude),
                    tp.getPosition().getLongitude(), "EW");

        *stream << "   K " << latitude << " " << longitude;

        if (tp.getPosition().getAltitude().defined()) {
            char letter;
            switch (tp.getPosition().getAltitude().getUnit()) {
            case Altitude::UNIT_METERS:
                letter = 'M';
                break;
            case Altitude::UNIT_FEET:
                letter = 'F';
                break;
            default:
                letter = 'U';
            }
            *stream << " " << letter << tp.getPosition().getAltitude().getValue();
        } else {
            *stream << " U     0";
        }

        *stream << "\n";
    }

    if (tp.getFrequency().defined())
        *stream << "  F " << tp.getFrequency().getMegaHertz()
                << std::setfill('0') << std::setw(3)
                << tp.getFrequency().getKiloHertzPart()
                << "\n";

    if (tp.getRunway().defined()) {
        *stream << "   R "
                << std::setfill('0') << std::setw(2)
                << (tp.getRunway().getDirection() / 10);

        if (tp.getRunway().getLength() > 0)
            *stream << " "
                    << std::setfill('0') << std::setw(4)
                    << tp.getRunway().getLength();

        switch (tp.getRunway().getType()) {
        case Runway::TYPE_UNKNOWN:
            break;
        case Runway::TYPE_GRASS:
            *stream << " GR";
            break;
        case Runway::TYPE_ASPHALT:
            *stream << " AS";
            break;
        }

        *stream << "\n";
    }
}

void CenfisTurnPointWriter::flush() {
    if (stream == NULL)
        throw already_flushed();

    *stream << "0 End of File, created by loggertools\n";
    stream = NULL;
}

TurnPointWriter *CenfisTurnPointFormat::createWriter(std::ostream *stream) const {
    return new CenfisTurnPointWriter(stream);
}
