/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <string>

#include "tp.hh"

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

class CenfisTurnPointWriter : public TurnPointWriter {
private:
    FILE *file;
public:
    CenfisTurnPointWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
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

    return new Angle(sign * (((n1 * 60) + n2) * 1000 + ((n3 * 1000) / 60)));
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
            tp->setStyle(TurnPoint::STYLE_AIRFIELD_GRASS); /* XXX */
        else if (strncmp(line, " #S", 3) == 0)
            tp->setStyle(TurnPoint::STYLE_GLIDER_SITE);
        else if (strncmp(line, " #M", 3) == 0)
            tp->setStyle(TurnPoint::STYLE_AIRFIELD_SOLID); /* XXX */
        else if (strncmp(line, "LW ", 3) == 0)
            tp->setStyle(TurnPoint::STYLE_OUTLANDING);
        else if (strncmp(line, "TQ ", 3) == 0)
            tp->setStyle(TurnPoint::STYLE_POWER_PLANT); /* XXX */
        else if (strncmp(line, "AL ", 3) == 0)
            tp->setStyle(TurnPoint::STYLE_INTERSECTION); /* XXX */
        else if (strncmp(line, "ZL ", 3) == 0)
            tp->setStyle(TurnPoint::STYLE_INTERSECTION); /* XXX */
        else
            tp->setStyle(TurnPoint::STYLE_UNKNOWN);

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
        
        break;
    }

    return ret;
}

const TurnPoint *CenfisTurnPointReader::read() {
    char line[1024];
    TurnPoint *ret = NULL;

    while ((fgets(line, sizeof(line), file)) != NULL) {
        TurnPoint *ret = handleLine(line);
        if (ret != NULL)
            return ret;
    }

    if (tp != NULL) {
        ret = tp;
        tp = NULL;
    }

    return ret;
}

CenfisTurnPointWriter::CenfisTurnPointWriter(FILE *_file)
    :file(_file) {
    fprintf(file, "0 created by loggertools\n");
}

static const char *formatType(TurnPoint::style_t type) {
    switch (type) {
    case TurnPoint::STYLE_AIRFIELD_GRASS:
    case TurnPoint::STYLE_AIRFIELD_SOLID:
        return " # ";
    case TurnPoint::STYLE_GLIDER_SITE:
        return " #S";
    case TurnPoint::STYLE_OUTLANDING:
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
    const char *p;

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    p = tp.getCode();
    if (p == NULL)
        p = tp.getTitle();
    if (p == NULL)
        p = "unknown";
    fprintf(file, "11 N %s\n", p);

    fprintf(file, "   T %3s",
            formatType(tp.getStyle()));

    if (tp.getTitle() != NULL)
        fprintf(file, " %s", tp.getTitle());

    fputs("\n", file);

    if (tp.getPosition().defined()) {
        char latitude[16], longitude[16];

        formatAngle(latitude, sizeof(latitude),
                    tp.getPosition().getLatitude(), "SN");
        formatAngle(longitude, sizeof(longitude),
                    tp.getPosition().getLongitude(), "EW");

        fprintf(file, "   K %s %s",
                latitude, longitude);

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
            fprintf(file, " %c %u", letter,
                    tp.getPosition().getAltitude().getValue());
        } else
            fputs(" U     0", file);

        fprintf(file, "\n");
    }

    if (tp.getFrequency() > 0)
        fprintf(file, "   F %u %03u\n",
                tp.getFrequency() / 1000000,
                (tp.getFrequency() / 1000) % 1000);

    if (tp.getDirection() != UINT_MAX) {
        fprintf(file, "   R %02u", tp.getDirection() / 10);

        if (tp.getStyle() == TurnPoint::STYLE_AIRFIELD_GRASS)
            fprintf(file, "     Grass");
        else if (tp.getStyle() == TurnPoint::STYLE_AIRFIELD_SOLID)
            fprintf(file, "     Asphalt");

        fprintf(file, "\n");
    }
}

void CenfisTurnPointWriter::flush() {
    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    fputs("0 End of File, created by loggertools\n", file);
    fclose(file);
    file = NULL;
}

TurnPointReader *CenfisTurnPointFormat::createReader(FILE *file) {
    return new CenfisTurnPointReader(file);
}

TurnPointWriter *CenfisTurnPointFormat::createWriter(FILE *file) {
    return new CenfisTurnPointWriter(file);
}
