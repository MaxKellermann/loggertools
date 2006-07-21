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

#include <netinet/in.h>

#include "tp.hh"

#include "filser.h"

class FilserTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    unsigned count;
public:
    FilserTurnPointReader(FILE *_file);
public:
    virtual const TurnPoint *read();
};

FilserTurnPointReader::FilserTurnPointReader(FILE *_file)
    :file(_file), count(0) {
}

const TurnPoint *FilserTurnPointReader::read() {
    struct filser_turn_point data;
    size_t nmemb;
    TurnPoint *tp;
    char code[sizeof(data.code) + 1];
    size_t length;

    if (count >= 600)
        return NULL;

    do {
        nmemb = fread(&data, sizeof(data), 1, file);
        if (nmemb != 1)
            return NULL;

        count++;
    } while (data.valid != 1);

    /* create object */
    tp = new TurnPoint();

    /* extract code */
    length = sizeof(data.code);
    memcpy(code, data.code, length);
    while (length > 0 && code[length - 1] >= 0 &&
           code[length - 1] <= ' ')
        length--;
    code[length] = 0;

    if (code[0] != 0) {
        tp->setTitle(code);
        tp->setCode(code);
    }

    return tp;
}

TurnPointReader *FilserTurnPointFormat::createReader(FILE *file) {
    return new FilserTurnPointReader(file);
}
