/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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

#ifndef __LOGGERTOOLS_IO_MATCH_HH
#define __LOGGERTOOLS_IO_MATCH_HH

#include "io.hh"

/**
 * A Reader class which returns all objects matching the Match
 * operation.
 */
template<class T, class Match>
class MatchReader : public Reader<T> {
private:
    Reader<T> *reader;
    Match match;

public:
    MatchReader(Reader<T> *_reader, Match _match)
        :reader(_reader), match(_match) {}

    virtual ~MatchReader() {
        delete reader;
    }

public:
    virtual const T *read() {
        while (true) {
            const T *t = reader->read();
            if (t == NULL || match(*t))
                return t;

            /* this object didn't pass the filter: delete it */
            delete t;
        }
    }
};

#endif
