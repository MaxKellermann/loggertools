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

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"

#include <ostream>
#include <iomanip>

class SeeYouTurnPointWriter : public TurnPointWriter {
private:
    std::ostream *stream;
public:
    SeeYouTurnPointWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

static void write_column(std::ostream *stream, const std::string &value) {
    if (value.length() == 0)
        return;
    *stream << '"' << value << '"';
}

static char *formatAngle(char *buffer, size_t buffer_max_len,
                         const Angle &angle, const char *letters) {
    int value = angle.getValue();
    int a = abs(value);

    snprintf(buffer, buffer_max_len, "%02u%02u.%03u%c",
             a / 60 / 1000, (a / 1000) % 60,
             a % 1000,
             value < 0 ? letters[0] : letters[1]);

    return buffer;
}

SeeYouTurnPointWriter::SeeYouTurnPointWriter(std::ostream *_stream)
    :stream(_stream) {
    *stream << "Title,Code,Country,Latitude,Longitude,Elevation,Style,Direction,Length,Frequency,Description\r\n";
}

static unsigned makeSeeYouStyle(const TurnPoint &tp) {
    switch (tp.getType()) {
    case TurnPoint::TYPE_AIRFIELD:
    case TurnPoint::TYPE_MILITARY_AIRFIELD:
        return tp.getRunway().getType() == Runway::TYPE_ASPHALT
            ? 5 : 2;
    case TurnPoint::TYPE_GLIDER_SITE:
        return 4;
    case TurnPoint::TYPE_OUTLANDING:
        return 3;
    case TurnPoint::TYPE_MOUNTAIN_PASS:
        return 6;
    case TurnPoint::TYPE_MOUNTAIN_TOP:
        return 7;
    case TurnPoint::TYPE_SENDER:
        return 8;
    case TurnPoint::TYPE_VOR:
        return 9;
    case TurnPoint::TYPE_NDB:
        return 10;
    case TurnPoint::TYPE_COOL_TOWER:
        return 11;
    case TurnPoint::TYPE_DAM:
        return 12;
    case TurnPoint::TYPE_TUNNEL:
        return 13;
    case TurnPoint::TYPE_BRIDGE:
        return 14;
    case TurnPoint::TYPE_POWER_PLANT:
        return 15;
    case TurnPoint::TYPE_CASTLE:
        return 16;
    case TurnPoint::TYPE_INTERSECTION:
        return 17;
    default:
        return 1;
    }
}

static std::ostream &operator <<(std::ostream &os,
                                 const Frequency &frequency) {
    if (frequency.defined())
        return os << frequency.getMegaHertz()
                  << std::setfill('0') << std::setw(3)
                  << frequency.getKiloHertzPart();
    else
        return os;
}

void SeeYouTurnPointWriter::write(const TurnPoint &tp) {
    char latitude[16], longitude[16];

    if (stream == NULL)
        throw already_flushed();

    if (tp.getPosition().defined()) {
        formatAngle(latitude, sizeof(latitude),
                    tp.getPosition().getLatitude(), "SN");
        formatAngle(longitude, sizeof(longitude),
                    tp.getPosition().getLongitude(), "WE");
    } else {
        latitude[0] = 0;
        longitude[0] = 0;
    }

    write_column(stream, tp.getTitle());
    *stream << ',';
    write_column(stream, tp.getCode());
    *stream << ',';
    write_column(stream, tp.getCountry());
    *stream << ',' << latitude << ',' << longitude << ',';
    Altitude altitude = tp.getPosition().getAltitude().toUnit(Altitude::UNIT_METERS);
    if (altitude.defined() && altitude.getRef() == Altitude::REF_MSL)
        *stream << altitude.getValue() << 'M';
    *stream << ',' << makeSeeYouStyle(tp) << ',';
    if (tp.getRunway().defined())
        *stream << tp.getRunway().getDirection();
    *stream << ',';
    if (tp.getRunway().getLength() > 0)
        *stream << tp.getRunway().getLength();
    *stream << ',';
    *stream << tp.getFrequency();
    *stream << ',';
    write_column(stream, tp.getDescription());
    *stream << "\r\n";
}

void SeeYouTurnPointWriter::flush() {
    if (stream == NULL)
        throw already_flushed();

    *stream << "-----Related Tasks-----\r\n";
    stream = NULL;
}

TurnPointWriter *
SeeYouTurnPointFormat::createWriter(std::ostream *stream) const {
    return new SeeYouTurnPointWriter(stream);
}
