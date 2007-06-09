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

#include <istream>

class SeeYouTurnPointReader : public TurnPointReader {
private:
    std::istream *stream;
    bool is_eof;
    unsigned num_columns;
    char **columns;
public:
    SeeYouTurnPointReader(std::istream *stream);
    virtual ~SeeYouTurnPointReader();
public:
    virtual const TurnPoint *read();
};

static unsigned count_columns(const char *p) {
    unsigned count = 1;
    int in_string = 0;

    for (; *p; p++) {
        if (*p == '"')
            in_string = !in_string;
        else if (!in_string && *p == ',')
            count++;
    }

    return count;
}

static int read_column(const char **line, char *column, size_t column_max_len) {
    if (**line == 0)
        return 0;

    if (**line == '"') {
        (*line)++;

        for (; **line != 0 && **line != '"'; (*line)++) {
            if (column_max_len > 1) {
                *column++ = **line;
                column_max_len--;
            }
        }

        *column = 0;

        if (**line == '"') {
            (*line)++;

            while (**line > 0 && **line <= ' ')
                (*line)++;
        }
    } else {
        char *first_whitespace = NULL;

        for (; **line != 0 && **line != ','; (*line)++) {
            if (**line > 0 && **line <= ' ') {
                if (first_whitespace == NULL)
                    first_whitespace = column;
            } else {
                first_whitespace = NULL;
            }

            if (column_max_len > 1) {
                *column++ = **line;
                column_max_len--;
            }
        }

        if (first_whitespace == NULL)
            *column = 0;
        else
            *first_whitespace = 0;
    }

    if (**line == ',')
        (*line)++;

    return 1;
}

SeeYouTurnPointReader::SeeYouTurnPointReader(std::istream *_stream)
    :stream(_stream), is_eof(false),
     num_columns(0), columns(NULL) {
    char line[4096], column[1024];
    const char *p;
    unsigned z;
    int ret;

    stream->getline(line, sizeof(line));

    num_columns = count_columns(line);
    if (num_columns == 0)
        throw malformed_input("no columns in header");

    columns = (char**)malloc(num_columns * sizeof(*columns));
    if (columns == NULL)
        throw std::bad_alloc();

    for (p = line, z = 0; z < num_columns; z++) {
        ret = read_column(&p, column, sizeof(column));
        if (!ret)
            column[0] = 0;

        if (column[0] == 0)
            columns[z] = NULL;
        else
            columns[z] = strdup(column);
    }
}

SeeYouTurnPointReader::~SeeYouTurnPointReader() {
    for (unsigned i = 0; i < num_columns; ++i)
        if (columns[i] != NULL)
            free(columns[i]);
    free(columns);
}

template<class T, char minusLetter, char plusLetter>
static const T parseAngle(const char *p) {
    unsigned long n1, n2;
    int sign, degrees;
    char *endptr;

    if (p == NULL || *p == 0)
        return T();

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || *endptr != '.')
        return T();

    n2 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 1000 || endptr == NULL)
        return T();

    if (*endptr == minusLetter)
        sign = -1;
    else if (*endptr == plusLetter)
        sign = 1;
    else
        return T();

    degrees = (int)n1 / 100;
    n1 %= 100;

    if (degrees > 180 || n1 >= 60)
        return T();

    return T((int)(sign * (((degrees * 60) + n1) * 1000 + n2)));
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

const TurnPoint *SeeYouTurnPointReader::read() {
    char line[4096], column[1024];
    const char *p = line;
    unsigned z;
    int ret;
    TurnPoint tp;
    Latitude latitude;
    Longitude longitude;
    Altitude altitude;
    Runway::type_t rwy_type = Runway::TYPE_UNKNOWN;
    unsigned rwy_direction = Runway::DIRECTION_UNDEFINED;
    unsigned rwy_length = Runway::LENGTH_UNDEFINED;

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

    if (strncmp(p, "-----Related", 12) == 0) {
        is_eof = true;
        return NULL;
    }

    for (z = 0; z < num_columns; z++) {
        ret = read_column(&p, column, sizeof(column));
        if (!ret)
            column[0] = 0;

        if (columns[z] == NULL)
            continue;

        if (strcasecmp(columns[z], "title") == 0 ||
            strcasecmp(columns[z], "name") == 0) {
            tp.setTitle(column);
        } else if (strcasecmp(columns[z], "code") == 0) {
            tp.setCode(column);
        } else if (strcasecmp(columns[z], "country") == 0) {
            tp.setCountry(column);
        } else if (strcasecmp(columns[z], "latitude") == 0 ||
                   strcasecmp(columns[z], "lat") == 0) {
            latitude = parseAngle<Latitude,'S','N'>(column);
        } else if (strcasecmp(columns[z], "longitude") == 0 ||
                   strcasecmp(columns[z], "lon") == 0) {
            longitude = parseAngle<Longitude,'W','E'>(column);
        } else if (strcasecmp(columns[z], "elevation") == 0 ||
                   strcasecmp(columns[z], "elev") == 0) {
            if (*column == 0)
                altitude = Altitude();
            else
                altitude = Altitude(strtol(column, NULL, 10),
                                    Altitude::UNIT_METERS,
                                    Altitude::REF_MSL);
        } else if (strcasecmp(columns[z], "style") == 0) {
            TurnPoint::type_t type;

            switch (atoi(column)) {
            case 2:
                rwy_type = Runway::TYPE_GRASS;
                type = TurnPoint::TYPE_AIRFIELD;
                break;
            case 3:
                type = TurnPoint::TYPE_OUTLANDING;
                break;
            case 4:
                type = TurnPoint::TYPE_GLIDER_SITE;
                break;
            case 5:
                rwy_type = Runway::TYPE_ASPHALT;
                type = TurnPoint::TYPE_AIRFIELD;
                break;
            case 6:
                type = TurnPoint::TYPE_MOUNTAIN_PASS;
                break;
            case 7:
                type = TurnPoint::TYPE_MOUNTAIN_TOP;
                break;
            case 8:
                type = TurnPoint::TYPE_SENDER;
                break;
            case 9:
                type = TurnPoint::TYPE_VOR;
                break;
            case 10:
                type = TurnPoint::TYPE_NDB;
                break;
            case 11:
                type = TurnPoint::TYPE_COOL_TOWER;
                break;
            case 12:
                type = TurnPoint::TYPE_DAM;
                break;
            case 13:
                type = TurnPoint::TYPE_TUNNEL;
                break;
            case 14:
                type = TurnPoint::TYPE_BRIDGE;
                break;
            case 15:
                type = TurnPoint::TYPE_POWER_PLANT;
                break;
            case 16:
                type = TurnPoint::TYPE_CASTLE;
                break;
            case 17:
                type = TurnPoint::TYPE_INTERSECTION;
                break;
            default:
                type = TurnPoint::TYPE_UNKNOWN;
            }
            tp.setType(type);
        } else if (strcasecmp(columns[z], "direction") == 0 ||
                   strcasecmp(columns[z], "rwdir") == 0) {
            if (*column != 0)
                rwy_direction = (unsigned)atoi(column);
        } else if (strcasecmp(columns[z], "length") == 0 ||
                   strcasecmp(columns[z], "rwlen") == 0) {
            if (*column != 0)
                rwy_length = (unsigned)atoi(column);
        } else if (strcasecmp(columns[z], "frequency") == 0 ||
                   strcasecmp(columns[z], "freq") == 0) {
            tp.setFrequency(parseFrequency(column));
        } else if (strcasecmp(columns[z], "description") == 0 ||
                   strcasecmp(columns[z], "desc") == 0) {
            tp.setDescription(column);
        }
    }

    if (latitude.defined() && longitude.defined())
        tp.setPosition(Position(latitude,
                                longitude,
                                altitude));

    tp.setRunway(Runway(rwy_type, rwy_direction, rwy_length));

    return new TurnPoint(tp);
}

TurnPointReader *
SeeYouTurnPointFormat::createReader(std::istream *stream) const {
    return new SeeYouTurnPointReader(stream);
}
