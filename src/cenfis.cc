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
    TurnPoint *handleLine(const char *line);
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

TurnPoint *CenfisTurnPointReader::handleLine(const char *line) {
    (void)line;
    return NULL;
}

const TurnPoint *CenfisTurnPointReader::read() {
    char line[1024];
    TurnPoint *ret;

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
    fprintf(file, "O created by loggertools\n");
}

void CenfisTurnPointWriter::write(const TurnPoint &tp) {
    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    fprintf(file, "11 N %s ; %s\n",
            tp.getTitle(), tp.getDescription());
}

void CenfisTurnPointWriter::flush() {
    if (file == NULL)
        throw new TurnPointWriterException("already flushed");

    fputs("O End of File, created by loggertools\n", file);
    fclose(file);
    file = NULL;
}

TurnPointReader *CenfisTurnPointFormat::createReader(FILE *file) {
    return new CenfisTurnPointReader(file);
}

TurnPointWriter *CenfisTurnPointFormat::createWriter(FILE *file) {
    return new CenfisTurnPointWriter(file);
}
