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

class CenfisTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    TurnPoint *tp;
public:
    CenfisTurnPointReader(FILE *_file);
    virtual ~CenfisTurnPointReader();
protected:
    TurnPoint *handleLine(char *line);
public:
    virtual const TurnPoint *read();
};

CenfisTurnPointReader::CenfisTurnPointReader(FILE *_file)
    :file(_file), tp(NULL) {
}

CenfisTurnPointReader::~CenfisTurnPointReader() {
    if (tp != NULL)
        delete tp;
}

static Angle *parseAngle(char **pp, const char *letters) {
    const char *p = *pp;
    unsigned long n1, n2, n3;
    int sign;
    char *endptr;

    if (p == NULL || p[0] == 0 || p[1] != ' ' || p[2] == 0)
        return NULL;

    if (*p == letters[0])
        sign = -1;
    else if (*p == letters[1])
        sign = 1;
    else
        return NULL;

    p += 2;

    n1 = strtoul(p, &endptr, 10);
    if (n1 > 180 || endptr == NULL ||
        endptr[0] != ' ' || endptr[1] == 0)
        return NULL;

    n2 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 60 || endptr == NULL ||
        endptr[0] != ' ' || endptr[1] == 0)
        return NULL;

    n3 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 1000 || endptr == NULL ||
        endptr[0] != ' ' || endptr[1] == 0)
        return NULL;

    *pp = endptr + 1;

    return new Angle(sign * (((n1 * 60) + n2) * 1000 + n3));
}

static Angle *parseAngle60(char **pp, const char *letters) {
    const char *p = *pp;
    unsigned long n1, n2, n3;
    int sign;
    char *endptr;

    if (p == NULL || p[0] == 0 || p[1] != ' ' || p[2] == 0)
        return NULL;

    if (*p == letters[0])
        sign = -1;
    else if (*p == letters[1])
        sign = 1;
    else
        return NULL;

    p += 2;

    n1 = strtoul(p, &endptr, 10);
    if (n1 > 180 || endptr == NULL ||
        endptr[0] != ' ' || endptr[1] == 0)
        return NULL;

    n2 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 60 || endptr == NULL ||
        endptr[0] != ' ' || endptr[1] == 0)
        return NULL;

    n3 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 60 || endptr == NULL ||
        endptr[0] != ' ' || endptr[1] == 0)
        return NULL;

    *pp = endptr + 1;

    return new Angle(sign * ((n1 * 60) + n2) * 60 + n3, 60);
}

static Altitude *parseAltitude(const char *p) {
    Altitude::unit_t unit;
    long value;
    char *endptr;

    if (p == NULL || p[0] == 0 || p[1] != ' ' || p[2] == 0)
        return NULL;

    switch (p[0]) {
    case 'M':
        unit = Altitude::UNIT_METERS;
        break;
    case 'F':
        unit = Altitude::UNIT_FEET;
        break;
    case 'U':
        unit = Altitude::UNIT_UNKNOWN;
        break;
    default:
        return NULL;
    }

    value = strtol(p + 2, &endptr, 10);
    if (endptr != NULL && *endptr)
        return NULL;

    return new Altitude((long)value, unit, Altitude::REF_MSL);
}

static unsigned parseFrequency(const char *p) {
    unsigned long n1, n2;
    char *endptr;

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || (*endptr != ' ' && *endptr != '.'))
        return (unsigned)n1 * 1000000;

    n2 = strtoul(endptr + 1, NULL, 10);

    return (unsigned)(n1 * 1000 + n2) * 1000;
}

static char *nextWord(char **p) {
    char *ret;

    if (**p == 0)
        return NULL;

    while (**p == ' ')
        (*p)++;

    if (**p == 0)
        return NULL;

    ret = *p;

    while (**p != ' ') {
        (*p)++;
        if (**p == 0)
            return ret;
    }

    **p = 0;
    (*p)++;

    return ret;
}

static Runway *parseRunway(char *p) {
    unsigned long u;
    Runway::type_t type = Runway::TYPE_UNKNOWN;
    unsigned direction = UINT_MAX, length = 0;
    char *word, *endptr;

    while ((word = nextWord(&p)) != NULL) {
        u = strtoul(word, &endptr, 10);
        if (endptr == word) {
            if (strncasecmp(word, "GR", 2) == 0)
                type = Runway::TYPE_GRASS;
            else if (strncasecmp(word, "AS" ,2) == 0 ||
                     strncasecmp(word, "SO", 2) == 0)
                type = Runway::TYPE_ASPHALT;
        } else {
            if (u < 100) {
                if (direction == UINT_MAX)
                    direction = (unsigned)u * 10;
            } else {
                length = (unsigned)u;
            }
        }
    }

    return new Runway(type, direction, length);
}

TurnPoint *CenfisTurnPointReader::handleLine(char *line) {
    TurnPoint *ret;
    char *p;
    size_t length;

    /* remove comments after semicolon */
    p = strchr(line, ';');
    if (p != NULL)
        *p = 0;

    /* rtrim */
    length = strlen(line);
    while (length > 0 &&
           line[length - 1] > 0 &&
           line[length - 1] < ' ') {
        /* no, don't delete the whitespace please */
        length--;
        line[length] = 0;
    }

    /* check code */
    if (strncmp(line, "11 ", 3) == 0) {
        ret = tp;
        tp = new TurnPoint();
    } else if (strncmp(line, "   ", 3) == 0) {
        ret = NULL;
    } else if (*line == 0 || *line == ' ') {
        return NULL;
    } else {
        ret = tp;
        tp = NULL;
        return ret;
    }

    line += 3;

    if (line[0] == 0 || line[1] != ' ')
        return ret;

    /* check field */
    switch (*line) {
    case 'N': /* name */
        line += 2;

        if (*line != 0)
            tp->setCode(line);
        break;

    case 'T': /* type and description */
        line += 2;

        if (strncmp(line, " # ", 3) == 0)
            tp->setType(TurnPoint::TYPE_AIRFIELD);
        else if (strncmp(line, " #M", 3) == 0)
            tp->setType(TurnPoint::TYPE_MILITARY_AIRFIELD);
        else if (strncmp(line, " #S", 3) == 0)
            tp->setType(TurnPoint::TYPE_GLIDER_SITE);
        else if (strncmp(line, "LW ", 3) == 0)
            tp->setType(TurnPoint::TYPE_OUTLANDING);
        else if (strncmp(line, "TQ ", 3) == 0)
            tp->setType(TurnPoint::TYPE_THERMIK);
        else
            tp->setType(TurnPoint::TYPE_UNKNOWN);

        line += 4;

        if (*line != 0)
            tp->setTitle(line);

        break;

    case 'C': /* position */
        {
            Angle *longitude, *latitude;
            Altitude *altitude;

            line += 2;

            latitude = parseAngle60(&line, "SN");
            if (latitude == NULL)
                break;

            longitude = parseAngle60(&line, "WE");
            if (longitude == NULL) {
                delete latitude;
                break;
            }

            altitude = parseAltitude(line);
            if (altitude == NULL)
                altitude = new Altitude();

            tp->setPosition(Position(*latitude, *longitude, *altitude));
            delete latitude;
            delete longitude;
            delete altitude;
        }
        break;

    case 'K': /* position */
        {
            Angle *longitude, *latitude;
            Altitude *altitude;

            line += 2;

            latitude = parseAngle(&line, "SN");
            if (latitude == NULL)
                break;

            longitude = parseAngle(&line, "WE");
            if (longitude == NULL) {
                delete latitude;
                break;
            }

            altitude = parseAltitude(line);
            if (altitude == NULL)
                altitude = new Altitude();

            tp->setPosition(Position(*latitude, *longitude, *altitude));
            delete latitude;
            delete longitude;
            delete altitude;
        }
        break;

    case 'F': /* frequency */
        tp->setFrequency(parseFrequency(line + 2));
        break;

    case 'R': /* runway */
        {
            Runway *rwy;

            rwy = parseRunway(line + 2);
            if (rwy != NULL) {
                tp->setRunway(*rwy);
                delete rwy;
            }
        }
        break;
    }

    return ret;
}

const TurnPoint *CenfisTurnPointReader::read() {
    char line[1024];
    TurnPoint *ret = NULL;

    while ((fgets(line, sizeof(line), file)) != NULL) {
        ret = handleLine(line);
        if (ret != NULL)
            return ret;
    }

    if (tp != NULL) {
        ret = tp;
        tp = NULL;
    }

    return ret;
}

TurnPointReader *CenfisTurnPointFormat::createReader(FILE *file) const {
    return new CenfisTurnPointReader(file);
}
