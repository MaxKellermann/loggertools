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

#include <istream>

class MilomeiTurnPointReader : public TurnPointReader {
private:
    std::istream *stream;
public:
    MilomeiTurnPointReader(std::istream *stream);
public:
    virtual const TurnPoint *read();
};

MilomeiTurnPointReader::MilomeiTurnPointReader(std::istream *_stream)
    :stream(_stream) {}

static bool
is_whitespace(char ch)
{
    return ch > 0 && ch <= 0x20;
}

static const std::string
stripped_substring(const char *p, size_t length)
{
    while (is_whitespace(p[length - 1]))
        --length;

    return std::string(p, length);
}

static Altitude
parse_altitude(const std::string &s)
{
    char *endptr;
    Altitude::value_t value = (Altitude::value_t)strtoul(s.c_str(), &endptr, 10);
    if (endptr == NULL || *endptr != 0)
        return Altitude();

    return Altitude(value, Altitude::UNIT_METERS,
                    Altitude::REF_MSL);
}

template<class T, char minusLetter, char plusLetter>
static const T
parseAngle(const std::string &s)
{
    unsigned long value;
    int sign;
    unsigned degrees, minutes, seconds;
    char *endptr;

    if (s[0] == minusLetter)
        sign = -1;
    else if (s[0] == plusLetter)
        sign = 1;
    else
        return T();

    value = strtoul(s.c_str() + 1, &endptr, 10);
    if (endptr == NULL || *endptr != 0 || value >= 1800000)
        return T();

    degrees = value / 10000;
    minutes = (value / 100) % 100;
    seconds = value % 100;

    return T(sign, degrees, minutes, seconds);
}

static const Frequency
parse_frequency(const std::string &s)
{
    unsigned long value;
    char *endptr;

    value = strtoul(s.c_str(), &endptr, 10);
    if (endptr == NULL || *endptr != 0 || value < 9000 || value > 15000)
        return Frequency();

    value *= 10;

    if (value % 100 == 20 ||
        value % 100 == 70)
        value += 5;

    return Frequency(value * 1000);
}

const TurnPoint *
MilomeiTurnPointReader::read()
{
    char line[1024];
    TurnPoint tp;

    do {
        try {
            stream->getline(line, sizeof(line));
        } catch (const std::ios_base::failure &e) {
            if (stream->eof())
                return NULL;
            else
                throw;
        }
    } while (line[0] == '$' || strlen(line) < 64);

    std::string shortName = stripped_substring(line, 6);

    if (line[5] == '2')
        tp.setType(TurnPoint::TYPE_OUTLANDING);
    else if (memcmp(line + 23, "# S", 3) == 0 ||
             memcmp(line + 20, "GLD#", 4) == 0)
        tp.setType(TurnPoint::TYPE_GLIDER_SITE);
    else if (line[23] == '#')
        tp.setType(TurnPoint::TYPE_AIRFIELD);

    tp.setShortName(shortName);
    tp.setFullName(stripped_substring(line + 7, 34));

    Altitude altitude = parse_altitude(std::string(line + 41, 4));
    Latitude latitude = parseAngle<Latitude,'S','N'>(std::string(line + 45, 7));
    Longitude longitude = parseAngle<Longitude,'W','E'>(std::string(line + 52, 8));

    tp.setPosition(Position(latitude, longitude, altitude));

    if (line[23] == '#')
        tp.setFrequency(parse_frequency(std::string(line + 36, 5)));

    return new TurnPoint(tp);
}

TurnPointReader *
MilomeiTurnPointFormat::createReader(std::istream *stream) const
{
    return new MilomeiTurnPointReader(stream);
}

TurnPointWriter *
MilomeiTurnPointFormat::createWriter(std::ostream *) const
{
    return NULL;
}
