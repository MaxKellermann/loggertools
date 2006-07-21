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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "tp.hh"

class ZanderTurnPointWriter : public TurnPointWriter {
private:
    FILE *file;
public:
    ZanderTurnPointWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

static void write_column(FILE *file, const std::string &value,
                         size_t width) {
    size_t length;

    length = value.length();
    if (length > width)
        length = width;

    fwrite(value.c_str(), length, 1, file);
    for (; length < width; ++length)
        fputc(' ', file);
}

static char *formatLatitude(char *buffer, size_t buffer_max_len,
                            const Angle &angle) {
    int value = angle.getValue();
    int a = abs(value);

    snprintf(buffer, buffer_max_len, "%02u%02u%02u%c",
             a / 60000,
             (a / 1000) % 60,
             ((a % 1000) * 60 + 500) / 1000,
             value < 0 ? 'S' : 'N');

    return buffer;
}

static char *formatLongitude(char *buffer, size_t buffer_max_len,
                             const Angle &angle) {
    int value = angle.getValue();
    int a = abs(value);

    snprintf(buffer, buffer_max_len, "%03u%02u%02u%c",
             a / 60000,
             (a / 1000) % 60,
             ((a % 1000) * 60 + 500) / 1000,
             value < 0 ? 'W' : 'E');

    return buffer;
}

ZanderTurnPointWriter::ZanderTurnPointWriter(FILE *_file)
    :file(_file) {}

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
    case TurnPoint::TYPE_INTERSECTION:
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
    char latitude[16], longitude[16];

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    if (tp.getPosition().defined()) {
        formatLatitude(latitude, sizeof(latitude),
                       tp.getPosition().getLatitude());
        formatLongitude(longitude, sizeof(longitude),
                        tp.getPosition().getLongitude());
    } else {
        latitude[0] = 0;
        longitude[0] = 0;
    }

    write_column(file, tp.getTitle(), 12);
    fputc(' ', file);
    write_column(file, latitude, 7);
    fputc(' ', file);
    write_column(file, longitude, 8);
    fputc(' ', file);
    fprintf(file, "%04ld", tp.getPosition().getAltitude().getValue());
    fputc(' ', file);
    if (tp.getFrequency() > 0)
        fprintf(file, "%3u.%03u",
                tp.getFrequency() / 1000000,
                (tp.getFrequency() / 1000) % 1000);
    else
        fputs("1      ", file);
    fputc(' ', file);
    fputc(formatType(tp), file);
    fputc(' ', file);
    write_column(file, tp.getCountry(), 2);
    fputs("\r\n", file);
}

void ZanderTurnPointWriter::flush() {
    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    fputc('\x1a', file);

    fclose(file);
    file = NULL;
}

TurnPointWriter *ZanderTurnPointFormat::createWriter(FILE *file) {
    return new ZanderTurnPointWriter(file);
}
