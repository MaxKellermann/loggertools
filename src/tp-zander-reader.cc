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

#include "tp.hh"
#include "tp-io.hh"

#include <istream>

class ZanderTurnPointReader : public TurnPointReader {
private:
    std::istream *stream;
    bool is_eof;
public:
    ZanderTurnPointReader(std::istream *stream);
public:
    virtual const TurnPoint *read();
};

ZanderTurnPointReader::ZanderTurnPointReader(std::istream *_stream)
    :stream(_stream), is_eof(false) {}

template<class T, char minusLetter, char plusLetter>
static const T parseAngle(const char *p) {
    long n;
    int sign, degrees, minutes, seconds;
    char *endptr;

    if (p == NULL || *p == 0)
        return T();

    n = (long)strtoul(p, &endptr, 10);
    if (endptr == NULL)
        return T();

    if (*endptr == minusLetter)
        sign = -1;
    else if (*endptr == plusLetter)
        sign = 1;
    else
        return T();

    seconds = (int)(n % 100);
    n /= 100;
    minutes = (int)(n % 100);
    n /= 100;
    degrees = (int)n;

    if (degrees > 180 || minutes >= 60 || seconds >= 60)
        return T();

    return T(sign * ((degrees * 60) + minutes) * 60 + seconds, 60);
}

static const Frequency parseFrequency(const char *p) {
    char *endptr;
    unsigned long n1, n2;

    if (p == NULL || *p == 0)
        return Frequency();

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || *endptr != '.' )
        return Frequency();

    p = endptr + 1;

    if (*p)
        n2 = strtoul(endptr + 1, &endptr, 10);

    return Frequency(n1, n2);
}

static bool is_whitespace(char ch) {
    return ch > 0 && ch <= 0x20;
}

static const char *get_next_column(char **pp, size_t width) {
    char *p = *pp, *end;

    while (is_whitespace(*p)) {
        ++p;
        --width;
    }

    end = (char*)memchr(p, 0, width);
    if (end == NULL)
        end = p + width;

    *pp = end;

    while (end > p && is_whitespace(end[-1]))
        --end;

    if (end == p)
        return NULL;

    *end = 0;

    return p;
}

const TurnPoint *ZanderTurnPointReader::read() {
    char line[256], *p = line;
    const char *q;
    TurnPoint tp;
    Latitude latitude;
    Longitude longitude;
    Altitude altitude;
    Runway::type_t rwy_type = Runway::TYPE_UNKNOWN;

    if (is_eof || stream->eof())
        return NULL;

    try {
        stream->getline(line, sizeof(line));
    } catch (const std::ios_base::failure &e) {
        if (stream->eof())
            return NULL;
        else
            throw;
    }

    if (line[0] == '\x1a') {
        is_eof = true;
        return NULL;
    }

    tp.setFullName(get_next_column(&p, 13));

    latitude = parseAngle<Latitude,'S','N'>(get_next_column(&p, 8));
    longitude = parseAngle<Longitude,'W','E'>(get_next_column(&p, 9));
    q = get_next_column(&p, 5);
    if (q != NULL)
        altitude = Altitude(strtol(q, NULL, 10),
                            Altitude::UNIT_METERS,
                            Altitude::REF_MSL);
    tp.setPosition(Position(latitude,
                            longitude,
                            altitude));

    tp.setFrequency(parseFrequency(get_next_column(&p, 8)));

    q = get_next_column(&p, 2);
    if (q != NULL) {
        if (*q == 'G') {
            rwy_type = Runway::TYPE_GRASS;
            tp.setType(TurnPoint::TYPE_AIRFIELD);
        } else if (*q == 'A' || *q == 'C') {
            rwy_type = Runway::TYPE_ASPHALT;
            tp.setType(TurnPoint::TYPE_AIRFIELD);
        } else if (*q == 'V') {
            tp.setType(TurnPoint::TYPE_AIRFIELD);
        } else if (*q == 'S') {
            tp.setType(TurnPoint::TYPE_OUTLANDING);
        }
    }
    tp.setRunway(Runway(rwy_type, Runway::DIRECTION_UNDEFINED,
                        Runway::LENGTH_UNDEFINED));

    tp.setCountry(get_next_column(&p, 2));

    return new TurnPoint(tp);
}

TurnPointReader *
ZanderTurnPointFormat::createReader(std::istream *stream) const {
    return new ZanderTurnPointReader(stream);
}
