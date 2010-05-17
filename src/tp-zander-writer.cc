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
#include "tp.hh"
#include "tp-io.hh"

#include <ostream>
#include <iomanip>

#include <stdlib.h>
#include <stdio.h>

class ZanderTurnPointWriter : public TurnPointWriter {
private:
    std::ostream *stream;
public:
    ZanderTurnPointWriter(std::ostream *stream);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

static void write_column(std::ostream *stream, const std::string &value,
                         size_t width) {
    size_t length;

    length = value.length();
    if (length > width)
        length = width;

    stream->write(value.data(), length);
    for (; length < width; ++length)
        *stream << ' ';
}

static const std::string format(const Latitude &angle) {
    char buffer[16];
    int value = angle.refactor(60);
    int a = abs(value);

    if (!angle.defined())
        return std::string();

    snprintf(buffer, sizeof(buffer), "%02u%02u%02u%c",
             a / 3600,
             (a / 60) % 60,
             a % 60,
             value < 0 ? 'S' : 'N');

    return std::string(buffer);
}

static const std::string format(const Longitude &angle) {
    char buffer[16];
    int value = angle.refactor(60);
    int a = abs(value);

    if (!angle.defined())
        return std::string();

    snprintf(buffer, sizeof(buffer), "%03u%02u%02u%c",
             a / 3600,
             (a / 60) % 60,
             a % 60,
             value < 0 ? 'W' : 'E');

    return std::string(buffer);
}

static std::ostream &operator <<(std::ostream &os, const Altitude &altitude) {
    return os << std::setfill('0') << std::setw(4)
              << altitude.getValue();
}

static std::ostream &operator <<(std::ostream &os,
                                 const Frequency &frequency) {
    if (frequency.defined())
        return os << std::setfill(' ') << std::setw(3)
                  << frequency.getMegaHertz()
                  << std::setfill('0') << std::setw(3)
                  << frequency.getKiloHertzPart();
    else
        return os << "1      ";
}

ZanderTurnPointWriter::ZanderTurnPointWriter(std::ostream *_stream)
    :stream(_stream) {}

unsigned makeZanderStyle(const TurnPoint &tp) {
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
    case TurnPoint::TYPE_HIGHWAY_INTERSECTION:
    case TurnPoint::TYPE_RAILWAY_INTERSECTION:
        return 17;
    default:
        return 1;
    }
}

static char formatType(const TurnPoint &tp) {
    switch (tp.getType()) {
    case TurnPoint::TYPE_AIRFIELD:
    case TurnPoint::TYPE_MILITARY_AIRFIELD:
    case TurnPoint::TYPE_GLIDER_SITE:
        switch (tp.getRunway().getType()) {
        case Runway::TYPE_GRASS:
            return 'G';
        case Runway::TYPE_ASPHALT:
            return 'A';
        default:
            return 'V';
        }
        break;
    case TurnPoint::TYPE_OUTLANDING:
        return 'S';
    default:
        return ' ';
    }
}

void ZanderTurnPointWriter::write(const TurnPoint &tp) {
    if (stream == NULL)
        throw already_flushed();

    write_column(stream, tp.getAbbreviatedName(12), 12);
    *stream << ' ';
    write_column(stream, format(tp.getPosition().getLatitude()), 7);
    *stream << ' ';
    write_column(stream, format(tp.getPosition().getLongitude()), 8);
    *stream << ' '
            << tp.getPosition().getAltitude() << ' '
            << tp.getFrequency() << ' ';
    *stream << formatType(tp);
    *stream << ' ';
    write_column(stream, tp.getCountry(), 2);
    *stream << "\r\n";
}

void ZanderTurnPointWriter::flush() {
    if (stream == NULL)
        throw already_flushed();

    *stream << '\x1a';
    stream = NULL;
}

TurnPointWriter *
ZanderTurnPointFormat::createWriter(std::ostream *stream) const {
    return new ZanderTurnPointWriter(stream);
}
