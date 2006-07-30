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

class CenfisTurnPointWriter : public TurnPointWriter {
private:
    FILE *file;
public:
    CenfisTurnPointWriter(FILE *_file);
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush();
};

CenfisTurnPointWriter::CenfisTurnPointWriter(FILE *_file)
    :file(_file) {
    fprintf(file, "0 created by loggertools\n");
}

static const char *formatType(TurnPoint::type_t type) {
    switch (type) {
    case TurnPoint::TYPE_AIRFIELD:
        return " # ";
    case TurnPoint::TYPE_MILITARY_AIRFIELD:
        return " #M";
    case TurnPoint::TYPE_GLIDER_SITE:
        return " #S";
    case TurnPoint::TYPE_OUTLANDING:
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
    std::string p;

    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    p = tp.getCode();
    if (p.length() == 0)
        p = tp.getTitle();
    if (p.length() == 0)
        p = "unknown";
    fprintf(file, "11 N %s\n", p.c_str());

    fprintf(file, "   T %3s",
            formatType(tp.getType()));

    if (tp.getTitle().length() > 0)
        fprintf(file, " %s", tp.getTitle().c_str());

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
            fprintf(file, " %c %ld", letter,
                    tp.getPosition().getAltitude().getValue());
        } else
            fputs(" U     0", file);

        fprintf(file, "\n");
    }

    if (tp.getFrequency() > 0)
        fprintf(file, "   F %u %03u\n",
                tp.getFrequency() / 1000000,
                (tp.getFrequency() / 1000) % 1000);

    if (tp.getRunway().defined()) {
        fprintf(file, "   R %02u", tp.getRunway().getDirection() / 10);

        if (tp.getRunway().getLength() > 0)
            fprintf(file, " %04u", tp.getRunway().getLength());

        switch (tp.getRunway().getType()) {
        case Runway::TYPE_UNKNOWN:
            break;
        case Runway::TYPE_GRASS:
            fprintf(file, " GR");
            break;
        case Runway::TYPE_ASPHALT:
            fprintf(file, " AS");
            break;
        }

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

TurnPointWriter *CenfisTurnPointFormat::createWriter(FILE *file) {
    return new CenfisTurnPointWriter(file);
}