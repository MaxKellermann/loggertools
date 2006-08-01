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
#include "tp-io.hh"

class SeeYouTurnPointWriter : public TurnPointWriter {
private:
    FILE *file;
public:
    SeeYouTurnPointWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

static void write_column(FILE *file, const std::string &value) {
    if (value.length() == 0)
        return;
    putc('"', file);
    fputs(value.c_str(), file);
    putc('"', file);
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

SeeYouTurnPointWriter::SeeYouTurnPointWriter(FILE *_file)
    :file(_file) {
    fputs("Title,Code,Country,Latitude,Longitude,Elevation,Style,Direction,Length,Frequency,Description\r\n", file);
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

TurnPointWriter *SeeYouTurnPointFormat::createWriter(FILE *file) {
    return new SeeYouTurnPointWriter(file);
}
