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

class SeeYouTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    int is_eof;
    unsigned num_columns;
    char **columns;
public:
    SeeYouTurnPointReader(FILE *_file);
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

const TurnPoint *SeeYouTurnPointReader::read() {
    char line[4096], column[1024];
    const char *p;
    unsigned z;
    int ret;
    TurnPoint *tp = new TurnPoint();
    std::string latitude, longitude;
    Altitude *altitude = NULL;

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
        else if (strcmp(columns[z], "Latitude") == 0)
            latitude = column;
        else if (strcmp(columns[z], "Longitude") == 0)
            longitude = column;
        else if (strcmp(columns[z], "Elevation") == 0) {
            if (altitude != NULL)
                delete altitude;

            if (*column == 0)
                altitude = new Altitude();
            else
                altitude = new Altitude(strtol(column, NULL, 10),
                                        Altitude::UNIT_METERS,
                                        Altitude::REF_MSL);
        } else if (strcmp(columns[z], "Style") == 0)
            tp->setStyle((TurnPoint::style_t)atoi(column));
        else if (strcmp(columns[z], "Direction") == 0)
            tp->setDirection(column);
        else if (strcmp(columns[z], "Length") == 0)
            tp->setLength((unsigned)strtoul(column, NULL, 10));
        else if (strcmp(columns[z], "Frequency") == 0)
            tp->setFrequency(column);
        else if (strcmp(columns[z], "Description") == 0)
            tp->setDescription(column);
    }

    if (altitude == NULL)
        altitude = new Altitude();

    tp->setPosition(Position(latitude.data(),
                             longitude.data(),
                             *altitude));

    delete altitude;

    return tp;
}

SeeYouTurnPointWriter::SeeYouTurnPointWriter(FILE *_file)
    :file(_file) {
    fputs("Title,Code,Country,Latitude,Longitude,Elevation,Style,Direction,Length,Frequency,Description\r\n", file);
}

void SeeYouTurnPointWriter::write(const TurnPoint &tp) {
    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    write_column(file, tp.getTitle());
    putc(',', file);
    write_column(file, tp.getCode());
    putc(',', file);
    write_column(file, tp.getCountry());
    putc(',', file);
    write_column(file, tp.getPosition().getLatitude());
    putc(',', file);
    write_column(file, tp.getPosition().getLongitude());
    putc(',', file);
    if (tp.getPosition().getAltitude().defined())
        fprintf(file, "%luM",
                tp.getPosition().getAltitude().getValue(),
                tp.getStyle());
    putc(',', file);
    fprintf(file, "%d,",
            tp.getStyle());
    write_column(file, tp.getDirection());
    putc(',', file);
    if (tp.getLength() > 0)
        fprintf(file, "%u", tp.getLength());
    putc(',', file);
    write_column(file, tp.getFrequency());
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
