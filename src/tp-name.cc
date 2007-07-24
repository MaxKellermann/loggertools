/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
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
 */

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"

class NameTurnPointReader : public TurnPointReader {
private:
    TurnPointReader *reader;
    std::string name;
public:
    NameTurnPointReader(TurnPointReader *_reader,
                        const std::string &_name)
        :reader(_reader), name(_name) {}
    virtual ~NameTurnPointReader();
public:
    virtual const TurnPoint *read();
};

TurnPointReader *
NameTurnPointFilter::createFilter(TurnPointReader *reader,
                                  const char *args) const {
    if (args == NULL || *args == 0)
        throw malformed_input("No name provided");

    return new NameTurnPointReader(reader, args);
}

NameTurnPointReader::~NameTurnPointReader() {
    if (reader != NULL)
        delete reader;
}

const TurnPoint *NameTurnPointReader::read() {
    const TurnPoint *tp;

    if (reader == NULL)
        return NULL;

    while ((tp = reader->read()) != NULL) {
        if (tp->getCode() == name || tp->getShortName() == name ||
            tp->getFullName() == name)
            return tp;
        else
            delete tp;
    }

    delete reader;
    reader = NULL;
    return NULL;
}
