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
#include <limits.h>

#include "tp.hh"

class SeeYouTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    int is_eof;
    unsigned num_columns;
    char **columns;
public:
    SeeYouTurnPointReader(FILE *_file);
    virtual ~SeeYouTurnPointReader();
public:
    virtual const TurnPoint *read();
};

class SeeYouTurnPointWriter : public TurnPointWriter {
private:
    FILE *file;
public:
    SeeYouTurnPointWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

unsigned count_columns(const char *p) {
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

int read_column(const char **line, char *column, size_t column_max_len) {
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

void write_column(FILE *file, const char *value) {
    if (value == NULL || *value == 0)
        return;
    putc('"', file);
    fputs(value, file);
    putc('"', file);
}

SeeYouTurnPointReader::SeeYouTurnPointReader(FILE *_file)
    :file(_file), is_eof(0),
     num_columns(0), columns(NULL) {
    char line[4096], column[1024];
    const char *p;
    unsigned z;
    int ret;

    p = fgets(line, sizeof(line), file);
    if (p == NULL)
        throw TurnPointReaderException("No header");

    num_columns = count_columns(line);
    if (num_columns == 0)
        throw TurnPointReaderException("No columns in header");

    columns = (char**)malloc(num_columns * sizeof(*columns));
    if (columns == NULL)
        throw TurnPointReaderException("out of memory");

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
    free(columns);
}

static Angle *parseAngle(const char *p, const char *letters) {
    unsigned long n1, n2;
    int sign, degrees;
    char *endptr;

    if (p == NULL || *p == 0)
        return NULL;

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || *endptr != '.')
        return NULL;

    n2 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 1000 || endptr == NULL)
        return NULL;

    if (*endptr == letters[0])
        sign = -1;
    else if (*endptr == letters[1])
        sign = 1;
    else
        return NULL;

    degrees = (int)n1 / 100;
    n1 %= 100;

    if (degrees > 180 || n1 >= 60)
        return NULL;

    return new Angle(sign * (((degrees * 60) + n1) * 1000 + n2));
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

static unsigned parseFrequency(const char *p) {
    char *endptr;
    unsigned long n1, n2;

    if (p == NULL || *p == 0)
        return 0;

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || *endptr != '.' )
        return 0;

    p = endptr + 1;

    if (*p)
        n2 = strtoul(endptr + 1, &endptr, 10);

    return (n1 * 1000 + n2) * 1000;
}

const TurnPoint *SeeYouTurnPointReader::read() {
    char line[4096], column[1024];
    const char *p;
    unsigned z;
    int ret;
    TurnPoint *tp = new TurnPoint();
    Angle *latitude = NULL, *longitude = NULL;
    Altitude *altitude = NULL;
    Runway::type_t rwy_type = Runway::TYPE_UNKNOWN;
    unsigned rwy_direction = UINT_MAX, rwy_length = 0;

    if (is_eof)
        return NULL;

    p = fgets(line, sizeof(line), file);
    if (p == NULL) {
        is_eof = 1;
        return NULL;
    }

    if (strncmp(p, "-----Related", 12) == 0) {
        is_eof = 1;
        return NULL;
    }

    for (z = 0; z < num_columns; z++) {
        ret = read_column(&p, column, sizeof(column));
        if (!ret)
            column[0] = 0;

        if (columns[z] == NULL)
            continue;

        if (strcmp(columns[z], "Title") == 0)
            tp->setTitle(column);
        else if (strcmp(columns[z], "Code") == 0)
            tp->setCode(column);
        else if (strcmp(columns[z], "Country") == 0)
            tp->setCountry(column);
        else if (strcmp(columns[z], "Latitude") == 0) {
            if (latitude != NULL)
                delete latitude;
            latitude = parseAngle(column, "SN");
        } else if (strcmp(columns[z], "Longitude") == 0) {
            if (longitude != NULL)
                delete longitude;
            longitude = parseAngle(column, "WE");
        } else if (strcmp(columns[z], "Elevation") == 0) {
            if (altitude != NULL)
                delete altitude;

            if (*column == 0)
                altitude = new Altitude();
            else
                altitude = new Altitude(strtol(column, NULL, 10),
                                        Altitude::UNIT_METERS,
                                        Altitude::REF_MSL);
        } else if (strcmp(columns[z], "Style") == 0) {
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
            tp->setType(type);
        } else if (strcmp(columns[z], "Direction") == 0) {
            if (*column != 0)
                rwy_direction = (unsigned)atoi(column);
        } else if (strcmp(columns[z], "Length") == 0) {
            if (*column != 0)
                rwy_length = (unsigned)atoi(column);
        } else if (strcmp(columns[z], "Frequency") == 0)
            tp->setFrequency(parseFrequency(column));
        else if (strcmp(columns[z], "Description") == 0)
            tp->setDescription(column);
    }

    if (altitude == NULL)
        altitude = new Altitude();

    if (latitude != NULL && longitude != NULL)
        tp->setPosition(Position(*latitude,
                                 *longitude,
                                 *altitude));

    tp->setRunway(Runway(rwy_type, rwy_direction, rwy_length));

    if (latitude != NULL)
        delete latitude;
    if (longitude != NULL)
        delete longitude;
    delete altitude;

    return tp;
}

SeeYouTurnPointWriter::SeeYouTurnPointWriter(FILE *_file)
    :file(_file) {
    fputs("Title,Code,Country,Latitude,Longitude,Elevation,Style,Direction,Length,Frequency,Description\r\n", file);
}

unsigned makeSeeYouStyle(const TurnPoint &tp) {
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

void SeeYouTurnPointWriter::write(const TurnPoint &tp) {
    char latitude[16], longitude[16];

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    if (tp.getPosition().defined()) {
        formatAngle(latitude, sizeof(latitude),
                    tp.getPosition().getLatitude(), "SN");
        formatAngle(longitude, sizeof(longitude),
                    tp.getPosition().getLongitude(), "WE");
    } else {
        latitude[0] = 0;
        longitude[0] = 0;
    }

    write_column(file, tp.getTitle());
    putc(',', file);
    write_column(file, tp.getCode());
    putc(',', file);
    write_column(file, tp.getCountry());
    fprintf(file, ",%s,%s,", latitude, longitude);
    if (tp.getPosition().getAltitude().defined())
        fprintf(file, "%luM",
                tp.getPosition().getAltitude().getValue());
    putc(',', file);
    fprintf(file, "%d,",
            makeSeeYouStyle(tp));
    if (tp.getRunway().defined())
        fprintf(file, "%u", tp.getRunway().getDirection());
    putc(',', file);
    if (tp.getRunway().getLength() > 0)
        fprintf(file, "%u", tp.getRunway().getLength());
    putc(',', file);
    if (tp.getFrequency() > 0)
        fprintf(file, "%u.%03u",
                tp.getFrequency() / 1000000,
                (tp.getFrequency() / 1000) % 1000);
    putc(',', file);
    write_column(file, tp.getDescription());
    fputs("\r\n", file);
}

void SeeYouTurnPointWriter::flush() {
    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    fputs("-----Related Tasks-----\r\n", file);
    fclose(file);
    file = NULL;
}

TurnPointReader *SeeYouTurnPointFormat::createReader(FILE *file) {
    return new SeeYouTurnPointReader(file);
}

TurnPointWriter *SeeYouTurnPointFormat::createWriter(FILE *file) {
    return new SeeYouTurnPointWriter(file);
}
