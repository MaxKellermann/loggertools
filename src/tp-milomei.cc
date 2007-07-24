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

static int
check_exact(const char *, size_t length)
{
    return length == 0;
}

static int
check_highway_exit(const char *p, size_t length)
{
    return length > 0 && p[0] >= '0' && p[0] <= '9';
}

static int
check_highway_intersection(const char *p, size_t length)
{
    if (length < 4 || p[0] < '0' || p[0] > '9')
        return 0;

    const char *x = (const char*)memchr(p + 1, 'X', length);
    if (x == NULL)
        x = (const char*)memchr(p + 1, 'Y', length);
    if (x == NULL || x > p + length - 3 ||
        (x[1] != 'A' && x[1] != 'B') || x[2] < '0' || x[2] > '9')
        return 0;

    return 1;
}

static int
word_match(const std::string &s, const char *begin,
           int (*callback)(const char *p, size_t length))
{
    std::string::size_type pos = 0, space = 0;
    size_t begin_length = begin == NULL ? 0 : strlen(begin);

    while (pos < s.length()) {
        space = s.find(' ', pos);
        if (space == (std::string::size_type)-1)
            space = s.length();

        if (space > pos &&
            (begin == NULL || (space - pos >= begin_length &&
                               memcmp(s.data() + pos, begin, begin_length) == 0))) {
            int ret = callback(s.data() + pos + begin_length, space - pos - begin_length);
            if (ret)
                return ret;
        }

        pos = space + 1;
    }

    return 0;
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
    else if (line[23] == '*')
        tp.setType(TurnPoint::TYPE_OUTLANDING);

    tp.setShortName(shortName);

    if (line[23] == '#' || line[23] == '*')
        tp.setFullName(stripped_substring(line + 7, 16));
    else
        tp.setFullName(stripped_substring(line + 7, 34));

    if (line[23] == '#' && line[24] != ' ')
        tp.setCode(stripped_substring(line + 24, 4));

    Altitude altitude = parse_altitude(std::string(line + 41, 4));
    Latitude latitude = parseAngle<Latitude,'S','N'>(std::string(line + 45, 7));
    Longitude longitude = parseAngle<Longitude,'W','E'>(std::string(line + 52, 8));

    tp.setPosition(Position(latitude, longitude, altitude));

    if (line[23] == '#')
        tp.setFrequency(parse_frequency(std::string(line + 36, 5)));

    if (tp.getType() == TurnPoint::TYPE_UNKNOWN) {
        if (word_match(tp.getFullName(), "TV", check_exact) ||
            word_match(tp.getFullName(), "SENDER", check_exact))
            tp.setType(TurnPoint::TYPE_SENDER);
        else if (word_match(tp.getFullName(), "BR", check_exact))
            tp.setType(TurnPoint::TYPE_BRIDGE);
        else if (word_match(tp.getFullName(), "EX", check_exact) ||
                 word_match(tp.getFullName(), "EY", check_exact))
            tp.setType(TurnPoint::TYPE_RAILWAY_INTERSECTION);
        else if (word_match(tp.getFullName(), "BF", check_exact) ||
                 word_match(tp.getFullName(), "RS", check_exact) ||
                 word_match(tp.getFullName(), "GARE", check_exact))
            tp.setType(TurnPoint::TYPE_RAILWAY_STATION);
        else if (word_match(tp.getFullName(), "KIRCHE", check_exact) ||
                 word_match(tp.getFullName(), "EGLISE", check_exact) ||
                 word_match(tp.getFullName(), "STIFTSKIRCHE", check_exact) ||
                 word_match(tp.getFullName(), "KAPELLE", check_exact) ||
                 word_match(tp.getFullName(), "KLOSTER", check_exact) ||
                 word_match(tp.getFullName(), "KLOSTERKIRCHE", check_exact))
            tp.setType(TurnPoint::TYPE_CHURCH);
        else if (word_match(tp.getFullName(), "SCHLOSS", check_exact) ||
                 word_match(tp.getFullName(), "WASSERSCHLOSS", check_exact) ||
                 word_match(tp.getFullName(), "FESTUNG", check_exact))
            tp.setType(TurnPoint::TYPE_CASTLE);
        else if (word_match(tp.getFullName(), "RUINE", check_exact))
            tp.setType(TurnPoint::TYPE_RUIN);
        else if (word_match(tp.getFullName(), "RESTAURANT", check_exact))
            tp.setType(TurnPoint::TYPE_BUILDING);
        else if (word_match(tp.getFullName(), "GIPFEL", check_exact) ||
                 word_match(tp.getFullName(), "GIPFELKREUZ", check_exact))
            tp.setType(TurnPoint::TYPE_MOUNTAIN_TOP);
        else if (word_match(tp.getFullName(), "SEILBAHN", check_exact) ||
                 word_match(tp.getFullName(), "SKILIFT", check_exact))
            tp.setType(TurnPoint::TYPE_ROPEWAY);
        else if (word_match(tp.getFullName(), "PASS", check_exact) ||
                 word_match(tp.getFullName(), "PASSHOEHE", check_exact))
            tp.setType(TurnPoint::TYPE_MOUNTAIN_PASS);
        else if (word_match(tp.getFullName(), "TUNNEL", check_exact))
            tp.setType(TurnPoint::TYPE_TUNNEL);
        else if (word_match(tp.getFullName(), "STAUSEE", check_exact))
            tp.setType(TurnPoint::TYPE_LAKE);
        else if (word_match(tp.getFullName(), "STAUMAUER", check_exact) ||
                 word_match(tp.getFullName(), "STAUDAMM", check_exact))
            tp.setType(TurnPoint::TYPE_DAM);
        else if (word_match(tp.getFullName(), "SCHORNSTEIN", check_exact) ||
                 word_match(tp.getFullName(), "SCHORNST", check_exact))
            tp.setType(TurnPoint::TYPE_CHIMNEY);
        else if (word_match(tp.getFullName(), "KUEHLTURM", check_exact))
            tp.setType(TurnPoint::TYPE_COOL_TOWER);
        else if (word_match(tp.getFullName(), "SX", check_exact) ||
                 word_match(tp.getFullName(), "SY", check_exact) ||
                 word_match(tp.getFullName(), "B", check_highway_intersection))
            tp.setType(TurnPoint::TYPE_HIGHWAY_INTERSECTION);
        else if (word_match(tp.getFullName(), "TR", check_exact))
            tp.setType(TurnPoint::TYPE_GARAGE);
        else if (word_match(tp.getFullName(), "BAB", check_highway_exit)) {
            if (word_match(tp.getFullName(), "A", check_highway_intersection))
                tp.setType(TurnPoint::TYPE_HIGHWAY_INTERSECTION);
            else
                tp.setType(TurnPoint::TYPE_HIGHWAY_EXIT);
        } else if (word_match(tp.getFullName(), "FOEHNWELLE", check_exact))
            tp.setType(TurnPoint::TYPE_MOUNTAIN_WAVE);
    }

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
